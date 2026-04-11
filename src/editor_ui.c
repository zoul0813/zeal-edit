#include <zos_video.h>
#include <core.h>
#include <windows.h>
#include <conio.h>

#include "editor_ui.h"

#define DOCUMENT_Y 1
#define DOCUMENT_LINES (SCREEN_COL80_HEIGHT - DOCUMENT_Y - 2)

static void status_append(char* dst, unsigned char* offset, const char* src)
{
    if (src == 0)
        return;
    while ((*src != '\0') && (*offset < 79)) {
        dst[*offset] = *src;
        (*offset)++;
        src++;
    }
}

void editor_ui_draw_topmenu(void)
{
    uint8_t c = COLOR(TEXT_COLOR_BLACK, TEXT_COLOR_LIGHT_GRAY);
    text_menu(0, 0, c, "  Esc=Quit Home/End,PgUp/PgDn,Arrows=Move");
}

void editor_ui_draw_bottomenu(const EditorState* state)
{
    char text[80] = { 0 };
    unsigned char pos = 0;

    status_append(text, &pos, " ");
    status_append(text, &pos, state->filename);
    status_append(text, &pos, " ");
    status_append(text, &pos, state->dirty ? "*" : "-");
    status_append(text, &pos, " ");
    status_append(text, &pos, state->insert_mode ? "INS" : "OVR");

    uint8_t c = COLOR(TEXT_COLOR_CYAN, TEXT_COLOR_BLACK);
    text_menu(0, SCREEN_COL80_HEIGHT - 1, c, text);
}

void editor_ui_draw_document(EditorState* state)
{
    window_gotoxy(&state->win_document, 0, 0);
    uint16_t i = state->buffer_top;
    state->buffer_position = state->buffer_top;
    unsigned char cur = cursor(0);
    unsigned char lines = 0;

    for (i = state->buffer_position; i < state->filesize; i++) {
        char c = state->file_buffer[i];
        if (c == '\n') {
            window_clreol(&state->win_document);
            lines++;
        } else {
            lines += window_putc(&state->win_document, c);
        }
        if (lines >= DOCUMENT_LINES)
            break;
    }

    cursor(cur);
    state->buffer_position = clamp(i, 0, state->filesize);
}

void editor_ui_draw_dialog(EditorState* state)
{
    window(&state->win_dialog);
    window_banner(&state->win_dialog, 0, 0, 1, "Find");

    window_puts(&state->win_dialog, "\n");
    window_puts(&state->win_dialog,
                "  Find What: [this is the end_\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9]\n");
    window_puts(&state->win_dialog, "\n");
    window_puts(&state->win_dialog, "  [ ] Match Whole Word Only\n");
    window_puts(&state->win_dialog, "  [ ] Match Case\n");

    window_banner(&state->win_dialog, 0, state->win_dialog.h - 1, 1, "[OK]  [Cancel]  [Help]");
}

void editor_ui_redraw(EditorState* state)
{
    editor_ui_draw_topmenu();
    window(&state->win_document);
    editor_ui_draw_document(state);
    editor_ui_draw_bottomenu(state);
}

void editor_ui_refresh(EditorState* state)
{
    editor_ui_draw_document(state);
    editor_ui_draw_bottomenu(state);
}
