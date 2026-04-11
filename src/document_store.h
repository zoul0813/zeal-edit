#ifndef DOCUMENT_STORE_H
#define DOCUMENT_STORE_H

#include <stdint.h>

#include "editor_state.h"

unsigned char document_store_is_ascii(char c);
uint16_t document_store_previous_line_start(EditorState* state);
uint16_t document_store_next_line_start(const EditorState* state);
int16_t document_store_load_from_path(EditorState* state, const char* path);

#endif
