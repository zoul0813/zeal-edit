#ifndef DOCUMENT_STORE_H
#define DOCUMENT_STORE_H

#include <stdint.h>

#include "editor_state.h"

#define DOCUMENT_STORE_ERR_FILE_TOO_LARGE ((int16_t) -30000)

unsigned char document_store_is_ascii(char c);
char document_store_char_at(EditorState* state, uint32_t offset);
uint32_t document_store_cursor_offset(EditorState* state);
void document_store_set_cursor_from_offset(EditorState* state, uint32_t target_offset);
unsigned char document_store_insert(EditorState* state, uint32_t offset, char c, unsigned char insert_mode);
unsigned char document_store_backspace(EditorState* state, uint32_t cursor_offset);
unsigned char document_store_delete(EditorState* state, uint32_t cursor_offset);
uint32_t document_store_previous_line_start(EditorState* state);
uint32_t document_store_next_line_start(EditorState* state);
int16_t document_store_validate_path_size(EditorState* state, const char* path);
int16_t document_store_load_from_path(EditorState* state, const char* path);
int16_t document_store_save_to_path(EditorState* state, const char* path);
void document_store_close_source(EditorState* state);

#endif
