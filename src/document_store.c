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
#define PIECE_SOURCE 0
#define PIECE_ADD 1

#if EDIT_CHUNK_SIZE > MMU_PAGE_SIZE
#error "EDIT_CHUNK_SIZE must not exceed MMU_PAGE_SIZE"
#endif

unsigned char document_store_is_ascii(char c)
{
    return ((c > 0x1F) && (c < 0x7F));
}

static char document_store_piece_byte_at(const EditorState* state,
                                         const DocumentPiece* piece,
                                         uint16_t inner_offset)
{
    uint16_t idx = piece->start + inner_offset;
    if (piece->source_kind == PIECE_ADD)
        idx += state->source_size;

    if (idx >= EDIT_BUFFER_SIZE)
        return '\0';
    return (char) state->file_buffer[idx];
}

char document_store_char_at(const EditorState* state, uint16_t offset)
{
    uint16_t total = 0;

    if ((state == 0) || (offset >= state->filesize))
        return '\0';

    for (uint16_t i = 0; i < state->piece_count; i++) {
        const DocumentPiece* piece = &state->pieces[i];
        uint16_t next = total + piece->length;

        if (offset < next)
            return document_store_piece_byte_at(state, piece, offset - total);

        total = next;
    }

    return '\0';
}

static void document_store_compact_pieces(EditorState* state)
{
    uint16_t write_idx = 0;

    for (uint16_t i = 0; i < state->piece_count; i++) {
        DocumentPiece cur;
        cur.source_kind = state->pieces[i].source_kind;
        cur.start = state->pieces[i].start;
        cur.length = state->pieces[i].length;
        if (cur.length == 0)
            continue;

        if (write_idx > 0) {
            DocumentPiece* prev = &state->pieces[write_idx - 1];
            if ((prev->source_kind == cur.source_kind) && ((prev->start + prev->length) == cur.start)) {
                prev->length += cur.length;
                continue;
            }
        }

        state->pieces[write_idx] = cur;
        write_idx++;
    }

    state->piece_count = write_idx;
}

static unsigned char document_store_insert_piece_at(EditorState* state,
                                                    uint16_t index,
                                                    DocumentPiece piece)
{
    if (state->piece_count >= EDIT_PIECE_MAX)
        return 0;

    index = clamp(index, 0, state->piece_count);

    for (uint16_t i = state->piece_count; i > index; i--) {
        state->pieces[i] = state->pieces[i - 1];
    }

    state->pieces[index] = piece;
    state->piece_count++;
    return 1;
}

static void document_store_remove_piece_at(EditorState* state, uint16_t index)
{
    if (index >= state->piece_count)
        return;

    for (uint16_t i = index; i + 1 < state->piece_count; i++) {
        state->pieces[i] = state->pieces[i + 1];
    }

    if (state->piece_count > 0)
        state->piece_count--;
}

static void document_store_locate_offset(const EditorState* state,
                                         uint16_t offset,
                                         uint16_t* out_piece,
                                         uint16_t* out_inner,
                                         unsigned char* out_at_end)
{
    uint16_t total = 0;

    offset = clamp(offset, 0, state->filesize);

    for (uint16_t i = 0; i < state->piece_count; i++) {
        uint16_t next = total + state->pieces[i].length;

        if (offset < next) {
            *out_piece = i;
            *out_inner = offset - total;
            *out_at_end = 0;
            return;
        }

        total = next;
    }

    *out_piece = state->piece_count;
    *out_inner = 0;
    *out_at_end = 1;
}

static uint16_t document_store_line_start_at(const EditorState* state, uint16_t offset)
{
    uint16_t i = clamp(offset, 0, state->filesize);
    while (i > 0) {
        if (document_store_char_at(state, i - 1) == '\n')
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

        if (document_store_char_at(state, i) == '\n') {
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

        if (document_store_char_at(state, i) == '\n') {
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
        uint16_t overwrite_piece = 0;
        uint16_t overwrite_inner = 0;
        unsigned char overwrite_at_end = 0;
        document_store_locate_offset(state,
                                     offset,
                                     &overwrite_piece,
                                     &overwrite_inner,
                                     &overwrite_at_end);

        if (!overwrite_at_end && (state->pieces[overwrite_piece].source_kind == PIECE_ADD)) {
            uint16_t idx = state->source_size + state->pieces[overwrite_piece].start + overwrite_inner;
            if (idx >= EDIT_BUFFER_SIZE)
                return 0;
            state->file_buffer[idx] = (unsigned char) c;
            return 1;
        }

        if ((state->source_size + state->add_size) >= EDIT_BUFFER_SIZE)
            return 0;
        if (state->piece_count >= (EDIT_PIECE_MAX - 2))
            return 0;

        if (!document_store_delete(state, offset))
            return 0;
        return document_store_insert(state, offset, c, 1);
    }

    if ((state->source_size + state->add_size) >= EDIT_BUFFER_SIZE)
        return 0;

    if (state->piece_count >= (EDIT_PIECE_MAX - 2))
        return 0;

    state->file_buffer[state->source_size + state->add_size] = (unsigned char) c;

    DocumentPiece add_piece;
    add_piece.source_kind = PIECE_ADD;
    add_piece.start = state->add_size;
    add_piece.length = 1;

    state->add_size++;

    if (state->piece_count == 0) {
        if (!document_store_insert_piece_at(state, 0, add_piece))
            return 0;
        state->filesize++;
        return 1;
    }

    uint16_t piece_index = 0;
    uint16_t inner_offset = 0;
    unsigned char at_end = 0;
    document_store_locate_offset(state, offset, &piece_index, &inner_offset, &at_end);

    if (at_end) {
        if (!document_store_insert_piece_at(state, state->piece_count, add_piece))
            return 0;
        document_store_compact_pieces(state);
        state->filesize++;
        return 1;
    }

    DocumentPiece hit_piece;
    hit_piece.source_kind = state->pieces[piece_index].source_kind;
    hit_piece.start = state->pieces[piece_index].start;
    hit_piece.length = state->pieces[piece_index].length;

    if (inner_offset == 0) {
        if (!document_store_insert_piece_at(state, piece_index, add_piece))
            return 0;
    } else if (inner_offset >= hit_piece.length) {
        if (!document_store_insert_piece_at(state, piece_index + 1, add_piece))
            return 0;
    } else {
        DocumentPiece right_piece;
        right_piece.source_kind = hit_piece.source_kind;
        right_piece.start = hit_piece.start;
        right_piece.length = hit_piece.length;
        right_piece.start += inner_offset;
        right_piece.length -= inner_offset;

        state->pieces[piece_index].length = inner_offset;

        if (!document_store_insert_piece_at(state, piece_index + 1, add_piece))
            return 0;
        if (!document_store_insert_piece_at(state, piece_index + 2, right_piece))
            return 0;
    }

    document_store_compact_pieces(state);
    state->filesize++;
    return 1;
}

unsigned char document_store_delete(EditorState* state, uint16_t cursor_offset)
{
    if ((state->filesize == 0) || (cursor_offset >= state->filesize))
        return 0;

    uint16_t piece_index = 0;
    uint16_t inner_offset = 0;
    unsigned char at_end = 0;
    document_store_locate_offset(state, cursor_offset, &piece_index, &inner_offset, &at_end);

    if (at_end || (piece_index >= state->piece_count))
        return 0;

    DocumentPiece* piece = &state->pieces[piece_index];

    if (piece->length <= 1) {
        document_store_remove_piece_at(state, piece_index);
    } else if (inner_offset == 0) {
        piece->start++;
        piece->length--;
    } else if (inner_offset == (piece->length - 1)) {
        piece->length--;
    } else {
        if (state->piece_count >= (EDIT_PIECE_MAX - 1))
            return 0;

        DocumentPiece right_piece;
        right_piece.source_kind = piece->source_kind;
        right_piece.start = piece->start;
        right_piece.length = piece->length;
        right_piece.start += inner_offset + 1;
        right_piece.length -= (inner_offset + 1);

        piece->length = inner_offset;

        if (!document_store_insert_piece_at(state, piece_index + 1, right_piece))
            return 0;
    }

    document_store_compact_pieces(state);

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
        if (document_store_char_at(state, i) == '\n')
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
        if (document_store_char_at(state, i) == '\n')
            break;
    }
    return i + 1;
}

int16_t document_store_validate_path_size(EditorState* state, const char* path)
{
    if ((state == 0) || (path == 0) || (path[0] == '\0'))
        return -ERR_INVALID_PARAMETER;

    zos_dev_t dev = open(path, O_RDONLY);
    if (dev < 0)
        return dev;

    uint16_t total_read = 0;
    while (1) {
        uint16_t read_size = EDIT_CHUNK_SIZE;
        zos_err_t err = read(dev, state->scratch, &read_size);
        if (err != ERR_SUCCESS) {
            close(dev);
            return err;
        }
        if (read_size == 0)
            break;

        if ((EDIT_BUFFER_SIZE - total_read) < read_size) {
            close(dev);
            return DOCUMENT_STORE_ERR_FILE_TOO_LARGE;
        }

        total_read += read_size;
    }

    close(dev);
    return 0;
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
    state->source_size = 0;
    state->add_size = 0;
    state->piece_count = 0;

    uint16_t read_size = 0;
    do {
        uint16_t remaining = EDIT_BUFFER_SIZE - state->filesize;
        if (remaining == 0) {
            read_size = 1;
            zos_err_t extra_err = read(dev, state->scratch, &read_size);
            close(dev);
            if (extra_err != ERR_SUCCESS)
                return extra_err;
            if (read_size != 0)
                return DOCUMENT_STORE_ERR_FILE_TOO_LARGE;
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

    state->source_size = state->filesize;
    if (state->source_size > 0) {
        state->piece_count = 1;
        state->pieces[0].source_kind = PIECE_SOURCE;
        state->pieces[0].start = 0;
        state->pieces[0].length = state->source_size;
    }

    return 0;
}

int16_t document_store_save_to_path(EditorState* state, const char* path)
{
    if ((path == 0) || (path[0] == '\0'))
        return -ERR_INVALID_PARAMETER;

    zos_dev_t dev = open(path, O_WRONLY | O_CREAT | O_TRUNC);
    if (dev < 0)
        return dev;

#ifdef BUILD_VERIFY
    /* write() rejects buffers that cross an MMU page boundary */
    uint16_t scratch_addr = (uint16_t) state->scratch;
    if (((scratch_addr & (MMU_PAGE_SIZE - 1)) + EDIT_CHUNK_SIZE) > MMU_PAGE_SIZE) {
        close(dev);
        return -ERR_INVALID_PARAMETER;
    }
#endif

    uint16_t offset = 0;
    while (offset < state->filesize) {
        uint16_t chunk_size = state->filesize - offset;
        if (chunk_size > EDIT_CHUNK_SIZE)
            chunk_size = EDIT_CHUNK_SIZE;

        for (uint16_t i = 0; i < chunk_size; i++) {
            state->scratch[i] = (unsigned char) document_store_char_at(state, offset + i);
        }

        uint16_t written = chunk_size;
        zos_err_t err = write(dev, state->scratch, &written);
        if (err != ERR_SUCCESS) {
            close(dev);
            return err;
        }

        if (written == 0) {
            close(dev);
            return ERR_FAILURE;
        }

        offset += written;
    }

    zos_err_t close_err = close(dev);
    if (close_err != ERR_SUCCESS)
        return close_err;

    return 0;
}
