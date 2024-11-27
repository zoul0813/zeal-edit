/** Fake Edit.com Interface */
#ifndef EDIT_H
#define EDIT_H

void draw_topmenu(void);
void draw_bottomenu(void);
void draw_document(void);
void draw_dialog(void);
void redraw(void);
void edit(const char* path);

#endif