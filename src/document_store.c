#include <zos_errors.h>
#include <zos_sys.h>
#include <zos_vfs.h>
#include <core.h>

#include "document_store.h"

#define MMU_PAGE_SIZE 0x4000

#if EDIT_CHUNK_SIZE > MMU_PAGE_SIZE
#error "EDIT_CHUNK_SIZE must not exceed MMU_PAGE_SIZE"
#endif

unsigned char document_store_is_ascii(char c)
{
    return ((c > 0x1F) && (c < 0x7F));
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
