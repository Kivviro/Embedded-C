#ifndef PANEL_H
#define PANEL_H

#include "app.h"

void free_panel(Panel *p);
int load_directory(Panel *p);
void enter_directory(Panel *p);
void move_selection(Panel *p, int dir);

#endif