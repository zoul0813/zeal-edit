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
static const char k_status_opened[] = "Opened";
static const char k_status_open_cancelled[] = "Open cancelled";
static const char k_status_open_empty[] = "Open cancelled: empty path";
static const char k_status_open_failed[] = "Open failed";
static const char k_status_dirty_prompt_cancelled[] = "Open cancelled: unsaved changes kept";

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

static void set_status_and_draw(EditorState* state, const char* status_text)
{
    state->status_text = status_text;
    editor_ui_draw_bottomenu(state);
}

static void draw_prompt_banner(const char* prompt, const char* value)
{
    char line[80] = { 0 };
    uint8_t out = 0;
    uint8_t i = 0;

    while ((prompt[i] != '\0') && (out < 79)) {
        line[out] = prompt[i];
        out++;
        i++;
    }

    i = 0;
    while ((value[i] != '\0') && (out < 79)) {
        line[out] = value[i];
        out++;
        i++;
    }

    text_banner(0, SCREEN_COL80_HEIGHT - 1, g_editor_menu_color, 0, line);
}

enum {
    OPEN_PROMPT_CANCEL = 0,
    OPEN_PROMPT_OK = 1,
    OPEN_PROMPT_EMPTY = 2
};

static unsigned char prompt_open_path(char* out_path, uint8_t out_path_len)
{
    uint8_t len = 0;

    if ((out_path == 0) || (out_path_len == 0))
        return 0;

    out_path[0] = '\0';
    draw_prompt_banner("Open path (Enter=OK, Esc=Cancel): ", out_path);
    while (1) {
        kb_keys_t key = getkey();
        if (key == 0)
            continue;

        if (key == KB_ESC)
            return OPEN_PROMPT_CANCEL;

        if (key == KB_KEY_ENTER)
            return (len > 0) ? OPEN_PROMPT_OK : OPEN_PROMPT_EMPTY;

        if (key == KB_KEY_BACKSPACE) {
            if (len > 0) {
                len--;
                out_path[len] = '\0';
                draw_prompt_banner("Open path (Enter=OK, Esc=Cancel): ", out_path);
            }
            continue;
        }

        unsigned char ch = getch(key);
        if (!is_printable(ch))
            continue;
        if (len >= (out_path_len - 1))
            continue;

        out_path[len] = (char) ch;
        len++;
        out_path[len] = '\0';
        draw_prompt_banner("Open path (Enter=OK, Esc=Cancel): ", out_path);
    }
}

enum {
    DIRTY_ACTION_CANCEL = 0,
    DIRTY_ACTION_SAVE = 1,
    DIRTY_ACTION_DISCARD = 2
};

static unsigned char prompt_dirty_action(EditorState* state)
{
    (void) state;
    text_banner(0,
                SCREEN_COL80_HEIGHT - 1,
                g_editor_menu_color,
                0,
                "Unsaved changes. S=Save D=Discard C/Esc=Cancel");
    while (1) {
        kb_keys_t key = getkey();
        if (key == 0)
            continue;

        if (key == KB_ESC)
            return DIRTY_ACTION_CANCEL;

        unsigned char ch = getch(key);
        if ((ch == 's') || (ch == 'S'))
            return DIRTY_ACTION_SAVE;
        if ((ch == 'd') || (ch == 'D'))
            return DIRTY_ACTION_DISCARD;
        if ((ch == 'c') || (ch == 'C'))
            return DIRTY_ACTION_CANCEL;
    }
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

static void apply_open(EditorState* state)
{
    if (state->dirty) {
        unsigned char action = prompt_dirty_action(state);
        if (action == DIRTY_ACTION_CANCEL) {
            set_status_and_draw(state, k_status_dirty_prompt_cancelled);
            return;
        }

        if (action == DIRTY_ACTION_SAVE) {
            apply_save(state);
            if (state->dirty)
                return;
        }
    }

    unsigned char open_prompt_result = prompt_open_path(state->prompt_storage, EDIT_PATH_MAX);
    if (open_prompt_result != OPEN_PROMPT_OK) {
        if (open_prompt_result == OPEN_PROMPT_EMPTY)
            set_status_and_draw(state, k_status_open_empty);
        else
            set_status_and_draw(state, k_status_open_cancelled);
        return;
    }

    zos_dev_t preflight_dev = open(state->prompt_storage, O_RDONLY);
    if (preflight_dev < 0) {
        set_status_and_draw(state, k_status_open_failed);
        return;
    }
    (void) close(preflight_dev);

    if (document_store_load_from_path(state, state->prompt_storage) != 0) {
        set_status_and_draw(state, k_status_open_failed);
        return;
    }

    editor_state_set_named_path(state, state->prompt_storage);
    editor_state_reset_document_view(state);
    state->dirty = 0;
    state->status_text = k_status_opened;
    editor_ui_redraw(state);
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
        case KB_F3:
            apply_open(state);
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
