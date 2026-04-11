#include <zos_video.h>
#include <core.h>
#include <windows.h>
#include <conio.h>

#include "document_store.h"
#include "editor_ui.h"

#define DOCUMENT_Y 1
#define DOCUMENT_LINES (SCREEN_COL80_HEIGHT - DOCUMENT_Y - 3)

uint8_t g_editor_menu_color = COLOR(TEXT_COLOR_BLACK, TEXT_COLOR_LIGHT_GRAY);

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
    text_menu(0, 0, g_editor_menu_color, "  F2 Save  F3 Open  Esc Quit");
}

void editor_ui_draw_bottomenu(const EditorState* state)
{
    char text[80] = { 0 };
    unsigned char pos = 0;

    status_append(text, &pos, state->dirty ? " * " : " - ");
    status_append(text, &pos, state->insert_mode ? "[ INSERT ]" : "[ OVER ]");
    status_append(text, &pos, " | ");
    status_append(text, &pos, state->status_text);

    text_banner(0, SCREEN_COL80_HEIGHT - 1, g_editor_menu_color, 0, text);
}

void editor_ui_draw_document(EditorState* state)
{
    window_gotoxy(&state->win_document, 0, 0);
    uint16_t i = state->buffer_top;
    state->buffer_position = state->buffer_top;
    unsigned char cur = cursor(0);
    unsigned char lines = 0;

    for (i = state->buffer_position; i < state->filesize; i++) {
        char c = document_store_char_at(state, i);
        if (c == '\n') {
            window_clreol(&state->win_document);
            lines++;
        } else {
            lines += window_putc(&state->win_document, c);
        }
        if (lines >= DOCUMENT_LINES)
            break;
    }

    if (lines < DOCUMENT_LINES) {
        window_clreol(&state->win_document);
        for (unsigned char line = lines + 1; line < DOCUMENT_LINES; line++) {
            window_gotoxy(&state->win_document, 0, line);
            window_clreol(&state->win_document);
        }
    }

    cursor(cur);
    state->buffer_position = clamp(i, 0, state->filesize);
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
