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

static EditorState g_editor_state;

static void reset_startup_screen_state(void)
{
    (void) ioctl(DEV_STDOUT, CMD_RESET_SCREEN, NULL);
    clrscr();
    gotoxy(0, 0);
}

static unsigned char edit_loop(EditorState* state)
{
    if (state->meta_open)
        editor_ui_draw_bottomenu(state);
    if (state->dialog_open)
        editor_ui_draw_dialog(state);

    gotoxy(state->cursor_x + (DOCUMENT_X + 1), state->cursor_y + (DOCUMENT_Y + 1));

    kb_keys_t key = getkey();
    switch (key) {
        case KB_ESC:
            return 1;
        case KB_F1:
            state->dialog_open ^= 1;
            if (!state->dialog_open)
                editor_ui_redraw(state);
            break;
        case KB_INSERT:
            state->meta_open ^= 1;
            editor_ui_redraw(state);
            break;
        case KB_HOME:
            state->buffer_top = 0;
            state->cursor_y = 0;
            state->cursor_x = 0;
            editor_ui_refresh(state);
            break;
        case KB_END: {
            state->buffer_top = state->filesize;
            for (unsigned char i = 0; i < (SCREEN_COL80_HEIGHT - DOCUMENT_Y - 2); i++) {
                state->buffer_top = document_store_previous_line_start(state);
            }
            state->buffer_top = clamp(state->buffer_top, 0, state->filesize);
            state->cursor_x = 0;
            state->cursor_y = (SCREEN_COL80_HEIGHT - 1 - (DOCUMENT_Y + 2));
            editor_ui_refresh(state);
        } break;
        case KB_PG_UP: {
            for (unsigned char i = 0; i < (SCREEN_COL80_HEIGHT - DOCUMENT_Y - 2); i++) {
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
            if (state->cursor_y < (SCREEN_COL80_HEIGHT - 1 - (DOCUMENT_Y + 2))) {
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
            if (state->cursor_x < (SCREEN_COL80_WIDTH - 1 - (DOCUMENT_X + 2)))
                state->cursor_x++;
            break;
    }

    return 0;
}

void edit(const char* path)
{
    EditorState* state = &g_editor_state;

    kb_mode_non_block_raw();
    reset_startup_screen_state();
    editor_state_init(state, path);

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
