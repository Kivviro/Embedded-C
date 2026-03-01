#ifndef UI_H
#define UI_H

#include "app.h"

void init_curses(void);
void cleanup_curses(void);
void update_layout(App *app);
void draw_ui(App *app);
int handle_input(App *app);

#endif