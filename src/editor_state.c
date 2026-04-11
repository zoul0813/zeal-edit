#include <zos_video.h>

#include "editor_state.h"

#define DOCUMENT_X 0
#define DOCUMENT_Y 1
static const char k_unnamed_title[] = "<unnamed>";
static const char k_status_ready[] = "Ready";

static void copy_path(char* dst, const char* src)
{
    uint8_t i = 0;

    if ((dst == 0) || (src == 0))
        return;

    while ((src[i] != '\0') && (i < (EDIT_PATH_MAX - 1))) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

void editor_state_set_named_path(EditorState* state, const char* path)
{
    if ((state == 0) || (path == 0) || (path[0] == '\0'))
        return;

    copy_path(state->filename_storage, path);
    state->filename = state->filename_storage;
    state->has_named_path = 1;
    state->win_document.title = state->filename;
}

void editor_state_reset_document_view(EditorState* state)
{
    if (state == 0)
        return;

    state->cursor_x = 0;
    state->cursor_y = 0;
    state->buffer_position = 0;
    state->buffer_top = 0;
}

void editor_state_init(EditorState* state, const char* path)
{
    editor_state_reset_document_view(state);
    state->dialog_open = 0;
    state->meta_open = 0;
    state->insert_mode = 1;
    state->dirty = 0;
    state->has_named_path = 0;

    state->filename_storage[0] = '\0';
    state->prompt_storage[0] = '\0';
    state->filename = k_unnamed_title;
    state->status_text = k_status_ready;
    state->filesize = 0;
    state->source_size = 0;
    state->add_size = 0;
    state->piece_count = 0;
    state->source_cache_base = 0;
    state->source_cache_size = 0;
    state->source_cache_valid = 0;
    state->source_dev = -1;

    state->win_document.x = DOCUMENT_X;
    state->win_document.y = DOCUMENT_Y;
    state->win_document.w = SCREEN_COL80_WIDTH;
    state->win_document.h = SCREEN_COL80_HEIGHT - DOCUMENT_Y - 1;
    state->win_document.fg = TEXT_COLOR_WHITE;
    state->win_document.bg = TEXT_COLOR_DARK_BLUE;
    state->win_document.flags = WIN_BORDER;
    state->win_document.title = state->filename;

    if ((path != 0) && (path[0] != '\0'))
        editor_state_set_named_path(state, path);

    state->win_dialog.x = 15;
    state->win_dialog.y = 10;
    state->win_dialog.w = 50;
    state->win_dialog.h = 8;
    state->win_dialog.bg = TEXT_COLOR_LIGHT_GRAY;
    state->win_dialog.fg = TEXT_COLOR_BLACK;
    state->win_dialog.flags = WIN_SHADOW;
    state->win_dialog.title = 0;
}
