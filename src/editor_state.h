#ifndef EDITOR_STATE_H
#define EDITOR_STATE_H

#include <stdint.h>

#include <core.h>
#include <windows.h>

#define EDIT_BUFFER_SIZE (32 * KILOBYTE)
#define EDIT_CHUNK_SIZE 1024

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
    uint16_t filesize;
    unsigned char scratch[EDIT_CHUNK_SIZE];
    unsigned char file_buffer[EDIT_BUFFER_SIZE];
    uint16_t buffer_position;
    uint16_t buffer_top;

    window_t win_document;
    window_t win_dialog;
} EditorState;

void editor_state_init(EditorState* state, const char* path);

#endif
