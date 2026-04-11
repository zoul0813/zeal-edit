#include <zos_errors.h>
#include <zos_sys.h>
#include <zos_vfs.h>
#include <zos_video.h>
#include <core.h>

#include "document_store.h"

#define MMU_PAGE_SIZE 0x4000
#define MMU_WINDOW_ADDR 0x8000
#define MMU_WINDOW ((unsigned char*) MMU_WINDOW_ADDR)
#define DOCUMENT_X 0
#define DOCUMENT_Y 1
#define DOCUMENT_COLUMNS (SCREEN_COL80_WIDTH - (DOCUMENT_X + 2))
#define DOCUMENT_LINES (SCREEN_COL80_HEIGHT - DOCUMENT_Y - 3)
#define PIECE_SOURCE 0
#define PIECE_ADD 1
#define EDIT_ADD_MAX_BYTES ((uint32_t) EDIT_ADD_PAGE_MAX * (uint32_t) MMU_PAGE_SIZE)
#define SOURCE_IO_CHUNK_SIZE 64

const __sfr __banked __at(0xF2) mmu_page2_ro;
__sfr __at(0xF2) mmu_page2;

#if EDIT_CHUNK_SIZE > MMU_PAGE_SIZE
#error "EDIT_CHUNK_SIZE must not exceed MMU_PAGE_SIZE"
#endif

static uint32_t clamp_u32(uint32_t value, uint32_t min_value, uint32_t max_value)
{
    if (value < min_value)
        return min_value;
    if (value > max_value)
        return max_value;
    return value;
}

static unsigned char document_store_map_page(uint8_t page_index)
{
    mmu_page2 = page_index;
    return 1;
}

static unsigned char document_store_map_page2_guarded(uint8_t page_index, uint8_t* out_prev_page2)
{
    *out_prev_page2 = mmu_page2_ro;
    if (!document_store_map_page(page_index))
        return 0;
    return 1;
}

static void document_store_restore_page2(uint8_t page_index)
{
    mmu_page2 = page_index;
}

static unsigned char document_store_ensure_add_page(EditorState* state, uint8_t page_slot)
{
    while (state->add_page_count <= page_slot) {
        uint8_t page_index = 0;
        if (state->add_page_count >= EDIT_ADD_PAGE_MAX)
            return 0;
        if (palloc(&page_index) != ERR_SUCCESS)
            return 0;
        state->add_pages[state->add_page_count] = page_index;
        state->add_page_count++;
    }

    return 1;
}

static unsigned char document_store_write_add_byte(EditorState* state, uint32_t add_offset, unsigned char value)
{
    uint8_t page_slot = (uint8_t) (add_offset / MMU_PAGE_SIZE);
    uint16_t page_inner = (uint16_t) (add_offset & (MMU_PAGE_SIZE - 1));
    uint8_t prev_page2 = 0;

    if (!document_store_ensure_add_page(state, page_slot))
        return 0;
    if (!document_store_map_page2_guarded(state->add_pages[page_slot], &prev_page2))
        return 0;

    MMU_WINDOW[page_inner] = value;
    document_store_restore_page2(prev_page2);
    return 1;
}

static char document_store_read_add_byte(EditorState* state, uint32_t add_offset)
{
    uint8_t page_slot = 0;
    uint16_t page_inner = 0;
    uint8_t prev_page2 = 0;

    if (add_offset >= state->add_size)
        return '\0';

    page_slot = (uint8_t) (add_offset / MMU_PAGE_SIZE);
    page_inner = (uint16_t) (add_offset & (MMU_PAGE_SIZE - 1));

    if (page_slot >= state->add_page_count)
        return '\0';
    if (!document_store_map_page2_guarded(state->add_pages[page_slot], &prev_page2))
        return '\0';

    char c = (char) MMU_WINDOW[page_inner];
    document_store_restore_page2(prev_page2);
    return c;
}

unsigned char document_store_is_ascii(char c)
{
    return ((c > 0x1F) && (c < 0x7F));
}

void document_store_close_source(EditorState* state)
{
    uint8_t i = 0;

    if (state == 0)
        return;

    if (state->source_dev >= 0)
        (void) close(state->source_dev);

    if (state->source_cache_page_allocated) {
        (void) pfree(state->source_cache_page);
        state->source_cache_page_allocated = 0;
        state->source_cache_page = 0;
    }

    for (i = 0; i < state->add_page_count; i++) {
        (void) pfree(state->add_pages[i]);
        state->add_pages[i] = 0;
    }
    state->add_page_count = 0;
    state->add_size = 0;

    state->source_dev = -1;
    state->source_cache_valid = 0;
    state->source_cache_base = 0;
    state->source_cache_size = 0;
}

static unsigned char document_store_source_cache_load(EditorState* state, uint32_t source_offset)
{
    unsigned char io_chunk[SOURCE_IO_CHUNK_SIZE];
    zos_dev_t source_dev = 0;
    uint8_t cache_page = 0;

    if ((state == 0) || (state->source_dev < 0))
        return 0;

    if (!state->source_cache_page_allocated) {
        if (palloc(&state->source_cache_page) != ERR_SUCCESS)
            return 0;
        state->source_cache_page_allocated = 1;
    }
    source_dev = state->source_dev;
    cache_page = state->source_cache_page;

    uint32_t page_base = source_offset & ~((uint32_t) (MMU_PAGE_SIZE - 1));
    int32_t seek_to = (int32_t) page_base;
    zos_err_t seek_err = seek(source_dev, &seek_to, SEEK_SET);
    if (seek_err != ERR_SUCCESS)
        return 0;

    uint16_t filled = 0;
    while (filled < MMU_PAGE_SIZE) {
        uint16_t read_size = SOURCE_IO_CHUNK_SIZE;
        uint16_t remaining = MMU_PAGE_SIZE - filled;
        uint8_t prev_page2 = 0;

        if (read_size > remaining)
            read_size = remaining;

        zos_err_t err = read(source_dev, io_chunk, &read_size);
        if (err != ERR_SUCCESS)
            return 0;
        if (read_size == 0)
            break;

        if (!document_store_map_page2_guarded(cache_page, &prev_page2))
            return 0;
        for (uint16_t i = 0; i < read_size; i++) {
            MMU_WINDOW[filled + i] = io_chunk[i];
        }
        document_store_restore_page2(prev_page2);

        filled += read_size;
        if (read_size < SOURCE_IO_CHUNK_SIZE)
            break;
    }

    state->source_cache_base = page_base;
    state->source_cache_size = filled;
    state->source_cache_valid = 1;

    return (filled > 0);
}

static char document_store_source_byte_at(EditorState* state, uint32_t source_offset)
{
    if ((state == 0) || (source_offset >= state->source_size))
        return '\0';

    if (!state->source_cache_valid || (source_offset < state->source_cache_base)
        || (source_offset >= (state->source_cache_base + state->source_cache_size))) {
        if (!document_store_source_cache_load(state, source_offset))
            return '\0';
    }

    uint32_t cache_base = state->source_cache_base;
    uint16_t cache_size = state->source_cache_size;
    uint32_t inner = source_offset - cache_base;
    uint8_t cache_page = state->source_cache_page;
    uint8_t prev_page2 = 0;

    if (inner >= cache_size)
        return '\0';
    if (!document_store_map_page2_guarded(cache_page, &prev_page2))
        return '\0';

    char c = (char) MMU_WINDOW[inner];
    document_store_restore_page2(prev_page2);
    return c;
}

static char document_store_piece_byte_at(EditorState* state,
                                         const DocumentPiece* piece,
                                         uint32_t inner_offset)
{
    uint32_t idx = piece->start + inner_offset;

    if (piece->source_kind == PIECE_SOURCE)
        return document_store_source_byte_at(state, idx);

    if (idx >= state->add_size)
        return '\0';

    return document_store_read_add_byte(state, idx);
}

char document_store_char_at(EditorState* state, uint32_t offset)
{
    uint32_t total = 0;

    if ((state == 0) || (offset >= state->filesize))
        return '\0';

    for (uint16_t i = 0; i < state->piece_count; i++) {
        const DocumentPiece* piece = &state->pieces[i];
        uint32_t next = total + piece->length;

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
                                         uint32_t offset,
                                         uint16_t* out_piece,
                                         uint32_t* out_inner,
                                         unsigned char* out_at_end)
{
    uint32_t total = 0;

    offset = clamp_u32(offset, 0, state->filesize);

    for (uint16_t i = 0; i < state->piece_count; i++) {
        uint32_t next = total + state->pieces[i].length;

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

static uint32_t document_store_line_start_at(EditorState* state, uint32_t offset)
{
    uint32_t i = clamp_u32(offset, 0, state->filesize);
    while (i > 0) {
        if (document_store_char_at(state, i - 1) == '\n')
            break;
        i--;
    }
    return i;
}

uint32_t document_store_cursor_offset(EditorState* state)
{
    uint32_t i = state->buffer_top;
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

void document_store_set_cursor_from_offset(EditorState* state, uint32_t target_offset)
{
    uint32_t i = 0;
    unsigned char x = 0;
    unsigned char y = 0;

    target_offset = clamp_u32(target_offset, 0, state->filesize);

    if (target_offset < state->buffer_top)
        state->buffer_top = document_store_line_start_at(state, target_offset);

    i = state->buffer_top;
    while ((i < target_offset) && (y < DOCUMENT_LINES)) {
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

    if (y >= DOCUMENT_LINES) {
        state->buffer_top = document_store_line_start_at(state, target_offset);
        i = state->buffer_top;
        x = 0;
        y = 0;
        while ((i < target_offset) && (y < DOCUMENT_LINES)) {
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
    }

    if (y >= DOCUMENT_LINES) {
        state->cursor_x = 0;
        state->cursor_y = DOCUMENT_LINES - 1;
        return;
    }

    state->cursor_x = x;
    state->cursor_y = y;
}

unsigned char document_store_insert(EditorState* state, uint32_t offset, char c, unsigned char insert_mode)
{
    offset = clamp_u32(offset, 0, state->filesize);

    if (!insert_mode && (offset < state->filesize)) {
        uint16_t overwrite_piece = 0;
        uint32_t overwrite_inner = 0;
        unsigned char overwrite_at_end = 0;
        document_store_locate_offset(state,
                                     offset,
                                     &overwrite_piece,
                                     &overwrite_inner,
                                     &overwrite_at_end);

        if (!overwrite_at_end && (state->pieces[overwrite_piece].source_kind == PIECE_ADD)) {
            uint32_t idx = state->pieces[overwrite_piece].start + overwrite_inner;
            return document_store_write_add_byte(state, idx, (unsigned char) c);
        }

        if (state->add_size >= EDIT_ADD_MAX_BYTES)
            return 0;
        if (state->piece_count >= (EDIT_PIECE_MAX - 2))
            return 0;

        if (!document_store_delete(state, offset))
            return 0;
        return document_store_insert(state, offset, c, 1);
    }

    if (state->add_size >= EDIT_ADD_MAX_BYTES)
        return 0;

    if (state->piece_count >= (EDIT_PIECE_MAX - 2))
        return 0;

    if (!document_store_write_add_byte(state, state->add_size, (unsigned char) c))
        return 0;

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
    uint32_t inner_offset = 0;
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

unsigned char document_store_delete(EditorState* state, uint32_t cursor_offset)
{
    if ((state->filesize == 0) || (cursor_offset >= state->filesize))
        return 0;

    uint16_t piece_index = 0;
    uint32_t inner_offset = 0;
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

unsigned char document_store_backspace(EditorState* state, uint32_t cursor_offset)
{
    if (cursor_offset == 0)
        return 0;
    return document_store_delete(state, cursor_offset - 1);
}

uint32_t document_store_previous_line_start(EditorState* state)
{
    if (state->buffer_top < 2)
        return 0;
    state->buffer_top -= 2;

    uint32_t i = 0;
    for (i = state->buffer_top; i > 0; i--) {
        if (document_store_char_at(state, i) == '\n')
            break;
    }
    if (i == 0)
        return 0;
    return i + 1;
}

uint32_t document_store_next_line_start(EditorState* state)
{
    uint32_t i = 0;
    for (i = state->buffer_top; i < state->filesize; i++) {
        if (document_store_char_at(state, i) == '\n')
            break;
    }
    return i + 1;
}

int16_t document_store_validate_path_size(EditorState* state, const char* path)
{
    (void) state;

    if ((path == 0) || (path[0] == '\0'))
        return -ERR_INVALID_PARAMETER;

    zos_stat_t st;
    zos_err_t err = stat(path, &st);
    if (err != ERR_SUCCESS)
        return err;

    return 0;
}

int16_t document_store_load_from_path(EditorState* state, const char* path)
{
    if ((state == 0) || (path == 0) || (path[0] == '\0'))
        return -ERR_INVALID_PARAMETER;

    zos_dev_t dev = open(path, O_RDONLY);
    if (dev < 0)
        return dev;

    zos_stat_t st;
    zos_err_t stat_err = dstat(dev, &st);
    if (stat_err != ERR_SUCCESS) {
        (void) close(dev);
        return stat_err;
    }

    document_store_close_source(state);

    state->filesize = st.s_size;
    state->source_size = st.s_size;
    state->add_size = 0;
    state->piece_count = 0;
    state->source_cache_base = 0;
    state->source_cache_size = 0;
    state->source_cache_valid = 0;
    state->source_cache_page = 0;
    state->source_cache_page_allocated = 0;
    state->add_page_count = 0;
    state->source_dev = dev;

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
    if ((state == 0) || (path == 0) || (path[0] == '\0'))
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

    uint32_t offset = 0;
    while (offset < state->filesize) {
        uint32_t remaining = state->filesize - offset;
        uint16_t chunk_size = (remaining > EDIT_CHUNK_SIZE) ? EDIT_CHUNK_SIZE : (uint16_t) remaining;

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
