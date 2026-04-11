#ifndef DOCUMENT_STORE_H
#define DOCUMENT_STORE_H

#include <stdint.h>

#include "editor_state.h"

unsigned char document_store_is_ascii(char c);
uint16_t document_store_cursor_offset(const EditorState* state);
void document_store_set_cursor_from_offset(EditorState* state, uint16_t target_offset);
unsigned char document_store_insert(EditorState* state, uint16_t offset, char c, unsigned char insert_mode);
unsigned char document_store_backspace(EditorState* state, uint16_t cursor_offset);
unsigned char document_store_delete(EditorState* state, uint16_t cursor_offset);
uint16_t document_store_previous_line_start(EditorState* state);
uint16_t document_store_next_line_start(const EditorState* state);
int16_t document_store_load_from_path(EditorState* state, const char* path);

#endif
