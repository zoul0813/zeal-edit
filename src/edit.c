#include <zos_errors.h>
#include <zos_keyboard.h>
#include <zos_sys.h>
#include <zos_vfs.h>
#include <core.h>
#include <keyboard.h>
#include <conio.h>

#include "document_store.h"
#include "editor_state.h"
#include "editor_ui.h"
#include "edit.h"

#define DOCUMENT_X 0
#define DOCUMENT_Y 1
#define DOCUMENT_MAX_X (SCREEN_COL80_WIDTH - 1 - (DOCUMENT_X + 2))
#define DOCUMENT_LINES (SCREEN_COL80_HEIGHT - DOCUMENT_Y - 3)
#define DOCUMENT_MAX_Y (DOCUMENT_LINES - 1)

static const char k_status_modified[] = "Modified";
static const char k_status_saved[] = "Saved";
static const char k_status_save_needs_name[] = "Save needs filename (Task 6: Open/Save-As)";
static const char k_status_save_open_failed[] = "Save failed: open/truncate";
static const char k_status_save_write_failed[] = "Save failed: write/close";

static EditorState g_editor_state;

static void reset_startup_screen_state(void)
{
    (void) ioctl(DEV_STDOUT, CMD_RESET_SCREEN, NULL);
    clrscr();
    gotoxy(0, 0);
}

static unsigned char is_printable(unsigned char c)
{
    return ((c >= 0x20) && (c <= 0x7E));
}

static void apply_insert(EditorState* state, char c)
{
    uint16_t offset = document_store_cursor_offset(state);
    if (!document_store_insert(state, offset, c, state->insert_mode))
        return;

    state->dirty = 1;
    state->status_text = k_status_modified;
    document_store_set_cursor_from_offset(state, offset + 1);
    editor_ui_refresh(state);
}

static void apply_backspace(EditorState* state)
{
    uint16_t offset = document_store_cursor_offset(state);
    if (!document_store_backspace(state, offset))
        return;

    state->dirty = 1;
    state->status_text = k_status_modified;
    document_store_set_cursor_from_offset(state, offset - 1);
    editor_ui_refresh(state);
}

static void apply_delete(EditorState* state)
{
    uint16_t offset = document_store_cursor_offset(state);
    if (!document_store_delete(state, offset))
        return;

    state->dirty = 1;
    state->status_text = k_status_modified;
    document_store_set_cursor_from_offset(state, offset);
    editor_ui_refresh(state);
}

static void apply_save(EditorState* state)
{
    if (!state->has_named_path) {
        state->status_text = k_status_save_needs_name;
        editor_ui_draw_bottomenu(state);
        return;
    }

    int16_t save_result = document_store_save_to_path(state, state->filename);
    if (save_result == 0) {
        state->dirty = 0;
        state->status_text = k_status_saved;
    } else if (save_result < 0) {
        state->status_text = k_status_save_open_failed;
    } else {
        state->status_text = k_status_save_write_failed;
    }

    editor_ui_draw_bottomenu(state);
}

static unsigned char edit_loop(EditorState* state)
{
    if (state->dialog_open)
        editor_ui_draw_dialog(state);

    gotoxy(state->cursor_x + (DOCUMENT_X + 1), state->cursor_y + (DOCUMENT_Y + 1));

    kb_keys_t key = getkey();
    if (key == 0)
        return 0;

    switch (key) {
        /* Session and UI controls */
        case KB_ESC:
            return 1;
        case KB_F1:
            state->dialog_open ^= 1;
            if (!state->dialog_open)
                editor_ui_redraw(state);
            break;
        case KB_F2:
            apply_save(state);
            break;

        /* Editing mode and mutation keys */
        case KB_INSERT:
            state->insert_mode ^= 1;
            editor_ui_draw_bottomenu(state);
            break;
        case KB_KEY_BACKSPACE:
            apply_backspace(state);
            break;
        case KB_DELETE:
            apply_delete(state);
            break;
        case KB_KEY_ENTER:
            apply_insert(state, '\n');
            break;

        /* Navigation keys */
        case KB_HOME:
            state->buffer_top = 0;
            state->cursor_y = 0;
            state->cursor_x = 0;
            editor_ui_refresh(state);
            break;
        case KB_END: {
            state->buffer_top = state->filesize;
            for (unsigned char i = 0; i < DOCUMENT_LINES; i++) {
                state->buffer_top = document_store_previous_line_start(state);
            }
            state->buffer_top = clamp(state->buffer_top, 0, state->filesize);
            state->cursor_x = 0;
            state->cursor_y = DOCUMENT_MAX_Y;
            editor_ui_refresh(state);
        } break;
        case KB_PG_UP: {
            for (unsigned char i = 0; i < DOCUMENT_LINES; i++) {
                state->buffer_top = document_store_previous_line_start(state);
            }
            state->buffer_top = clamp(state->buffer_top, 0, state->filesize);
            editor_ui_refresh(state);
        } break;
        case KB_PG_DOWN:
            state->buffer_top = clamp(state->buffer_position + 1, 0, state->filesize);
            editor_ui_refresh(state);
            break;
        case KB_UP_ARROW:
            if (state->cursor_y >= 1) {
                state->cursor_y--;
            } else if (state->buffer_top > 0) {
                state->buffer_top = document_store_previous_line_start(state);
                editor_ui_refresh(state);
            }
            break;
        case KB_DOWN_ARROW:
            if (state->cursor_y < DOCUMENT_MAX_Y) {
                state->cursor_y++;
            } else if (state->buffer_position < state->filesize) {
                state->buffer_top = clamp(document_store_next_line_start(state), 0, state->filesize);
                editor_ui_refresh(state);
            }
            break;
        case KB_LEFT_ARROW:
            if (state->cursor_x >= 1)
                state->cursor_x--;
            break;
        case KB_RIGHT_ARROW:
            if (state->cursor_x < DOCUMENT_MAX_X)
                state->cursor_x++;
            break;

        /* Printable/translated character input */
        default: {
            unsigned char ch = getch(key);
            if (is_printable(ch))
                apply_insert(state, (char) ch);
        } break;
    }

    return 0;
}

void edit(const char* path)
{
    EditorState* state = &g_editor_state;

    kb_mode_non_block_raw();
    reset_startup_screen_state();
    editor_state_init(state, path);

    if ((path != 0) && (path[0] != '\0')) {
        int16_t load_result = document_store_load_from_path(state, path);
        if (load_result != 0) {
#ifdef BUILD_VERIFY
            if (load_result == -ERR_INVALID_PARAMETER) {
                put_s("Scratch buffer crosses MMU page boundary\n");
                exit(ERR_INVALID_PARAMETER);
            }
#endif
            if (load_result < 0) {
                put_s("Failed to open file ");
                put_u8((uint8_t) -load_result);
                put_c(CH_NEWLINE);
                exit((uint8_t) -load_result);
            }

            put_s("Failed to read file ");
            put_u8((uint8_t) load_result);
            put_c(CH_NEWLINE);
            exit((uint8_t) load_result);
        }
    }

    state->status_text = "Ready";

    setcursortype(177);
    cursor(1);

    editor_ui_redraw(state);

    while (1) {
        unsigned char action = edit_loop(state);
        if (action)
            break;
    }

    kb_mode_default();
}
