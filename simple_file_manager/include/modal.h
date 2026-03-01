#ifndef MODAL_H
#define MODAL_H

#include "app.h"

int run_confirm_dialog(App *app, const char *title, const char *line1,
    const char *line2, const char *ok_label, const char *cancel_label);

int run_input_dialog(App *app, const char *title, const char *prompt,
    wchar_t *buffer, size_t buffer_len);

#endif