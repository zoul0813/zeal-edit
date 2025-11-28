#include <zos_keyboard.h>
#include <zos_sys.h>
#include <zos_video.h>
#include <zos_vfs.h>
#include <zvb_hardware.h>
#include <common.h>
#include <windows.h>
#include <keyboard.h>
#include <conio.h>


#include "edit.h"

#define BUFFER_SIZE 32 * KILOBYTE

#define CHUNK_SIZE 1024
#define DOCUMENT_X 0
#define DOCUMENT_Y 1
#define DOCUMENT_LINES SCREEN_COL80_HEIGHT - DOCUMENT_Y - 2

// static unsigned char width = 0, height = 0;
unsigned char cursor_x, cursor_y;
unsigned char dialog_open, meta_open;

const char* filename;
uint16_t filesize;
unsigned char file_buffer[BUFFER_SIZE];
uint16_t buffer_position;
uint16_t buffer_top;

static window_t winDocument = {
    .x = DOCUMENT_X,
    .y = DOCUMENT_Y,
    .w = SCREEN_COL80_WIDTH,
    .h = SCREEN_COL80_HEIGHT - DOCUMENT_Y,
    .fg    = TEXT_COLOR_WHITE,
    .bg    = TEXT_COLOR_DARK_BLUE,
    .flags = WIN_BORDER,
    .title = "filename",
};

static window_t winDialog = {
    .x     = 15,
    .y     = 10,
    .w     = 50,
    .h     = 8,
    .bg    = TEXT_COLOR_LIGHT_GRAY,
    .fg    = TEXT_COLOR_BLACK,
    .flags = WIN_SHADOW,
};

unsigned char is_ascii(char c)
{
    return ((c > 0x1F) && (c < 0x7F));
}

uint16_t previous_line_start(void)
{
    if (buffer_top < 2)
        return 0;
    // buffer_top - 1 (skip the \n)
    buffer_top -= 2;
    uint16_t i  = 0;
    for (i = buffer_top; i > 0; i--) {
        if (file_buffer[i] == '\n')
            break;
    }
    if(i == 0) return 0;
    return i + 1; // start of the previous line
}

uint16_t next_line_start(void)
{
    uint16_t i = 0;
    for (i = buffer_top; i < filesize; i++) {
        if (file_buffer[i] == '\n')
            break;
    }
    return i + 1; // start of the next line
}

void draw_topmenu(void)
{
    /* draw the main menu */
    uint8_t c = COLOR(TEXT_COLOR_BLACK, TEXT_COLOR_LIGHT_GRAY);
    text_menu(0, 0, c, "  Esc=Quit Home/End,PgUp/PgDn,Arrows=Move");
}

void draw_bottomenu(void)
{
    /* DRAW THE BOTTOM MENU */

    char text[80] = { 0 };
    // sprintf(text, " Filesize: %05d  Position: %05d  Top: %05d    X: %02d Y: %02d", filesize, buffer_position,
    //         buffer_top, cursor_x, cursor_y);
    uint8_t c = COLOR(TEXT_COLOR_CYAN, TEXT_COLOR_BLACK);
    text_menu(0, SCREEN_COL80_HEIGHT - 1, c, text);
}

void draw_document(void)
{
    // window_clrscr(&winDocument);
    window_gotoxy(&winDocument, 0, 0);
    uint16_t i          = buffer_top;
    buffer_position     = buffer_top;
    unsigned char cur   = cursor(0);
    unsigned char lines = 0;
    for (i = buffer_position; i < filesize; i++) {
        char c = file_buffer[i];
        if (c == '\n') {
            window_clreol(&winDocument);
            lines++;
        } else {
            lines += window_putc(&winDocument, c);
        }
        if (lines >= DOCUMENT_LINES)
            break;
        // if(window_wherey(&winDocument) > DOCUMENT_LINES) break;
    }
    cursor(cur);
    buffer_position = clamp(i, 0, filesize);
}

void draw_dialog(void)
{
    /* draw the dialog */
    unsigned char len  = 49; // length of "Find What: ..."
    unsigned char left = (SCREEN_COL80_WIDTH - len) >> 1;
    unsigned char top  = 8;

    window(&winDialog);
    window_banner(&winDialog, 0, 0, 1, "Find");

    window_puts(&winDialog, "\n");
    window_puts(&winDialog,
                "  Find What: [this is the end_\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9]\n");
    window_puts(&winDialog, "\n");
    window_puts(&winDialog, "  [ ] Match Whole Word Only\n");
    window_puts(&winDialog, "  [ ] Match Case\n");

    window_banner(&winDialog, 0, winDialog.h - 1, 1, "[OK]  [Cancel]  [Help]");
}

void redraw(void)
{
    draw_topmenu();
    window(&winDocument);
    draw_document();
}

void refresh(void)
{
    draw_document();
}

unsigned char edit_loop(void)
{
    if (meta_open)
        draw_bottomenu();
    if (dialog_open)
        draw_dialog();
    gotoxy(cursor_x + (DOCUMENT_X + 1), cursor_y + (DOCUMENT_Y + 1));

    kb_keys_t key = getkey();
    switch (key) {
        case KB_ESC:
            return 1;
        case KB_F1:
            dialog_open ^= 1;
            if (!dialog_open)
                redraw();
            break;
        case KB_INSERT:
            meta_open ^= 1;
            redraw();
            break;
        case KB_HOME: // document top
            buffer_top = 0;
            cursor_y = 0;
            cursor_x = 0;
            refresh();
            break;
        case KB_END: { // document bottom
            buffer_top = filesize;
            for (unsigned char i = 0; i < DOCUMENT_LINES; i++) {
                buffer_top = previous_line_start();
            }
            buffer_top = clamp(buffer_top, 0, filesize);
            cursor_x = 0;
            cursor_y = (SCREEN_COL80_HEIGHT - 1 - (DOCUMENT_Y + 2));
            refresh();
        } break;
        case KB_PG_UP: {
            for (unsigned char i = 0; i < DOCUMENT_LINES; i++) {
                buffer_top = previous_line_start();
            }
            buffer_top = clamp(buffer_top, 0, filesize);
            refresh();
        } break;
        case KB_PG_DOWN:
            buffer_top = clamp(buffer_position + 1, 0, filesize);
            refresh();
            break;
        case KB_UP_ARROW:
            if (cursor_y >= 1)
                cursor_y--;
            else if (buffer_top > 0) {
                buffer_top = previous_line_start();
                refresh();
            }
            break;
        case KB_DOWN_ARROW:
            if (cursor_y < (SCREEN_COL80_HEIGHT - 1 - (DOCUMENT_Y + 2)))
                cursor_y++;
            else if (buffer_position < filesize) {
                buffer_top = clamp(next_line_start(), 0, filesize);
                refresh();
            }
            break;
        case KB_LEFT_ARROW:
            if (cursor_x >= 1)
                cursor_x--;
            break;
        case KB_RIGHT_ARROW:
            if (cursor_x < (SCREEN_COL80_WIDTH - 1 - (DOCUMENT_X + 2)))
                cursor_x++;
            break;
            // default:
            //   if(is_ascii(c)) {
            //     cputc(c);
            //     cursor_x++;
            //   }
    }

    return 0;
}

void edit(const char* path)
{
    kb_mode_non_block_raw();
    filename          = path;
    winDocument.title = path;

    zos_dev_t dev = open(path, O_RDONLY);
    if (dev < 0) {
        put_s("Failed to open file ");
        put_u8(-dev);
        put_c(CH_NEWLINE);
        exit(-dev);
    }

    uint16_t size = CHUNK_SIZE;
    filesize      = 0;
    do {
        zos_err_t err = read(dev, &file_buffer[filesize], &size);
        if (err != ERR_SUCCESS) {
            put_s("Failed to read file ");
            put_u8(err);
            put_c(CH_NEWLINE);
            exit(err);
        }
        filesize += size;
    } while (size == CHUNK_SIZE);
    close(dev);

    setcursortype(177);
    cursor(1);

    redraw();

    cursor_x = 0;
    cursor_y = 0;

    while (1) {
        unsigned char action = edit_loop();
        if (action) {
            // TODO: implement more actions?
            break;
        }
    }
    kb_mode_default();
    return;
}

//