#ifndef EDITOR_STATE_H
#define EDITOR_STATE_H

#include <stdint.h>

#include <core.h>
#include <windows.h>

#define EDIT_BUFFER_SIZE (16 * KILOBYTE)
#define EDIT_CHUNK_SIZE 1024
#define EDIT_PATH_MAX 128
#define EDIT_PIECE_MAX 768

typedef struct DocumentPiece {
    uint8_t source_kind;
    uint16_t start;
    uint16_t length;
} DocumentPiece;

typedef struct EditorState {
    unsigned char cursor_x;
    unsigned char cursor_y;
    unsigned char dialog_open;
    unsigned char meta_open;
    unsigned char insert_mode;
    unsigned char dirty;
    unsigned char has_named_path;

    const char* filename;
    const char* status_text;
    char filename_storage[EDIT_PATH_MAX];
    char prompt_storage[EDIT_PATH_MAX];
    uint16_t filesize;
    uint16_t source_size;
    uint16_t add_size;
    uint16_t piece_count;
    DocumentPiece pieces[EDIT_PIECE_MAX];
    unsigned char scratch[EDIT_CHUNK_SIZE];
    unsigned char file_buffer[EDIT_BUFFER_SIZE];
    uint16_t buffer_position;
    uint16_t buffer_top;

    window_t win_document;
    window_t win_dialog;
} EditorState;

void editor_state_init(EditorState* state, const char* path);
void editor_state_set_named_path(EditorState* state, const char* path);
void editor_state_reset_document_view(EditorState* state);

#endif
