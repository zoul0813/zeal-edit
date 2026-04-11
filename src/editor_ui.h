#ifndef EDITOR_UI_H
#define EDITOR_UI_H

#include "editor_state.h"

void editor_ui_draw_topmenu(void);
void editor_ui_draw_bottomenu(const EditorState* state);
void editor_ui_draw_document(EditorState* state);
void editor_ui_draw_dialog(EditorState* state);
void editor_ui_redraw(EditorState* state);
void editor_ui_refresh(EditorState* state);

#endif
