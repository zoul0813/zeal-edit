#include <zos_video.h>

#include "editor_state.h"

#define DOCUMENT_X 0
#define DOCUMENT_Y 1

void editor_state_init(EditorState* state, const char* path)
{
    state->cursor_x = 0;
    state->cursor_y = 0;
    state->dialog_open = 0;
    state->meta_open = 0;

    state->filename = path;
    state->filesize = 0;
    state->buffer_position = 0;
    state->buffer_top = 0;

    state->win_document.x = DOCUMENT_X;
    state->win_document.y = DOCUMENT_Y;
    state->win_document.w = SCREEN_COL80_WIDTH;
    state->win_document.h = SCREEN_COL80_HEIGHT - DOCUMENT_Y;
    state->win_document.fg = TEXT_COLOR_WHITE;
    state->win_document.bg = TEXT_COLOR_DARK_BLUE;
    state->win_document.flags = WIN_BORDER;
    state->win_document.title = path;

    state->win_dialog.x = 15;
    state->win_dialog.y = 10;
    state->win_dialog.w = 50;
    state->win_dialog.h = 8;
    state->win_dialog.bg = TEXT_COLOR_LIGHT_GRAY;
    state->win_dialog.fg = TEXT_COLOR_BLACK;
    state->win_dialog.flags = WIN_SHADOW;
    state->win_dialog.title = 0;
}
