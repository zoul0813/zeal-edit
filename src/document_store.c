#include <zos_errors.h>
#include <zos_sys.h>
#include <zos_vfs.h>
#include <zos_video.h>
#include <core.h>

#include "document_store.h"

#define MMU_PAGE_SIZE 0x4000
#define DOCUMENT_X 0
#define DOCUMENT_Y 1
#define DOCUMENT_COLUMNS (SCREEN_COL80_WIDTH - (DOCUMENT_X + 2))
#define DOCUMENT_LINES (SCREEN_COL80_HEIGHT - DOCUMENT_Y - 3)

#if EDIT_CHUNK_SIZE > MMU_PAGE_SIZE
#error "EDIT_CHUNK_SIZE must not exceed MMU_PAGE_SIZE"
#endif

unsigned char document_store_is_ascii(char c)
{
    return ((c > 0x1F) && (c < 0x7F));
}

static uint16_t document_store_line_start_at(const EditorState* state, uint16_t offset)
{
    uint16_t i = clamp(offset, 0, state->filesize);
    while (i > 0) {
        if (state->file_buffer[i - 1] == '\n')
            break;
        i--;
    }
    return i;
}

static unsigned char document_store_locate_offset_in_view(const EditorState* state,
                                                          uint16_t target_offset,
                                                          unsigned char* out_x,
                                                          unsigned char* out_y)
{
    uint16_t i = state->buffer_top;
    unsigned char x = 0;
    unsigned char y = 0;

    target_offset = clamp(target_offset, 0, state->filesize);

    while (1) {
        if (i == target_offset) {
            *out_x = x;
            *out_y = y;
            return 1;
        }

        if (i >= state->filesize)
            break;
        if (y >= DOCUMENT_LINES)
            break;

        if (state->file_buffer[i] == '\n') {
            y++;
            x = 0;
        } else {
            x++;
            if (x >= DOCUMENT_COLUMNS) {
                x = 0;
                y++;
            }
        }

        i++;
    }

    return 0;
}

uint16_t document_store_cursor_offset(const EditorState* state)
{
    uint16_t i = state->buffer_top;
    unsigned char x = 0;
    unsigned char y = 0;

    while (1) {
        if ((y == state->cursor_y) && (x >= state->cursor_x))
            return i;

        if (i >= state->filesize)
            break;

        if (state->file_buffer[i] == '\n') {
            if (y == state->cursor_y)
                return i;
            y++;
            x = 0;
        } else {
            x++;
            if (x >= DOCUMENT_COLUMNS) {
                y++;
                x = 0;
            }
        }

        i++;
    }

    return state->filesize;
}

void document_store_set_cursor_from_offset(EditorState* state, uint16_t target_offset)
{
    unsigned char x = 0;
    unsigned char y = 0;
    target_offset = clamp(target_offset, 0, state->filesize);

    if (target_offset < state->buffer_top)
        state->buffer_top = document_store_line_start_at(state, target_offset);

    while (1) {
        if (document_store_locate_offset_in_view(state, target_offset, &x, &y)) {
            state->cursor_x = x;
            state->cursor_y = y;
            return;
        }

        if (state->buffer_top >= target_offset) {
            state->cursor_x = 0;
            state->cursor_y = 0;
            return;
        }

        state->buffer_top = clamp(document_store_next_line_start(state), 0, state->filesize);
    }
}

unsigned char document_store_insert(EditorState* state, uint16_t offset, char c, unsigned char insert_mode)
{
    offset = clamp(offset, 0, state->filesize);

    if (!insert_mode && (offset < state->filesize)) {
        state->file_buffer[offset] = c;
        return 1;
    }

    if (state->filesize >= EDIT_BUFFER_SIZE)
        return 0;

    uint16_t i = 0;
    for (i = state->filesize; i > offset; i--) {
        state->file_buffer[i] = state->file_buffer[i - 1];
    }

    state->file_buffer[offset] = c;
    state->filesize++;
    return 1;
}

unsigned char document_store_delete(EditorState* state, uint16_t cursor_offset)
{
    if ((state->filesize == 0) || (cursor_offset >= state->filesize))
        return 0;

    uint16_t i = 0;
    for (i = cursor_offset; i < (state->filesize - 1); i++) {
        state->file_buffer[i] = state->file_buffer[i + 1];
    }

    state->filesize--;
    return 1;
}

unsigned char document_store_backspace(EditorState* state, uint16_t cursor_offset)
{
    if (cursor_offset == 0)
        return 0;
    return document_store_delete(state, cursor_offset - 1);
}

uint16_t document_store_previous_line_start(EditorState* state)
{
    if (state->buffer_top < 2)
        return 0;
    state->buffer_top -= 2;

    uint16_t i = 0;
    for (i = state->buffer_top; i > 0; i--) {
        if (state->file_buffer[i] == '\n')
            break;
    }
    if (i == 0)
        return 0;
    return i + 1;
}

uint16_t document_store_next_line_start(const EditorState* state)
{
    uint16_t i = 0;
    for (i = state->buffer_top; i < state->filesize; i++) {
        if (state->file_buffer[i] == '\n')
            break;
    }
    return i + 1;
}

int16_t document_store_load_from_path(EditorState* state, const char* path)
{
    zos_dev_t dev = open(path, O_RDONLY);
    if (dev < 0)
        return dev;

#ifdef BUILD_VERIFY
    /* read() rejects buffers that cross an MMU page boundary */
    uint16_t scratch_addr = (uint16_t) state->scratch;
    if (((scratch_addr & (MMU_PAGE_SIZE - 1)) + EDIT_CHUNK_SIZE) > MMU_PAGE_SIZE) {
        close(dev);
        return -ERR_INVALID_PARAMETER;
    }
#endif

    state->filesize = 0;
    uint16_t read_size = 0;
    do {
        uint16_t remaining = EDIT_BUFFER_SIZE - state->filesize;
        if (remaining == 0) {
            read_size = 0;
            break;
        }
        read_size = EDIT_CHUNK_SIZE;
        if (read_size > remaining)
            read_size = remaining;

        zos_err_t err = read(dev, state->scratch, &read_size);
        if (err != ERR_SUCCESS) {
            close(dev);
            return err;
        }
        if (read_size == 0)
            break;

        mem_cpy(&state->file_buffer[state->filesize], state->scratch, read_size);
        state->filesize += read_size;
    } while (read_size == EDIT_CHUNK_SIZE);

    close(dev);
    return 0;
}
