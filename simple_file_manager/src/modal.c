/**
 * @file modal.c
 * @brief Modal window module (confirm and input).
 *
 * Implements confirmation and text input dialogs.
 */
#define _GNU_SOURCE
#include <string.h>
#include <ncursesw/curses.h>
#include "modal.h"
#include "ui.h"

/**
 * @brief Displays a confirmation dialogue box.
 *
 * @param app Pointer to the application.
 * @param title Window title.
 * @param line1 First line of text.
 * @param line2 Second line of text (can be NULL).
 * @param ok_label Confirmation button text.
 * @param cancel_label Cancel button text.
 * @return 1 if confirmed, 0 if cancelled.
 */
int run_confirm_dialog(App *app, const char *title, const char *line1,
    const char *line2, const char *ok_label, const char *cancel_label)
{
    const int w = 60;
    const int h = 10;

    int focus = 0;
    WINDOW *win = NULL;

    int old_delay = is_nodelay(stdscr);
    nodelay(stdscr, FALSE);

    mmask_t old_mask;
    mousemask(BUTTON1_RELEASED, &old_mask);

    while (1)
    {
        getmaxyx(stdscr, app->rows, app->cols);

        int startx = (app->cols - w) / 2;
        int starty = (app->rows - h) / 2;

        if (startx < 0) 
            startx = 0;
        if (starty < 0) 
            starty = 0;

        if (win)
            delwin(win);

        win = newwin(h, w, starty, startx);
        keypad(win, TRUE);

        while (1)
        {
            werase(win);
            box(win, 0, 0);

            int title_len = strlen(title);
            int title_x = (w - title_len) / 2;
            if (title_x < 1) title_x = 1;
            mvwprintw(win, 1, title_x, "%s", title);

            mvwprintw(win, 3, 2, "%s", line1);
            if (line2)
                mvwprintw(win, 4, 2, "%s", line2);

            int ok_len = strlen(ok_label) + 4;
            int cancel_len = strlen(cancel_label) + 4;
            int spacing = 6;

            int total = ok_len + cancel_len + spacing;
            int start_buttons = (w - total) / 2;

            int ok_x = start_buttons;
            int cancel_x = ok_x + ok_len + spacing;
            int buttons_y = h - 3;

            if (focus == 0)
                wattron(win, COLOR_PAIR(2));
            mvwprintw(win, buttons_y, ok_x, "[ %s ]", ok_label);
            if (focus == 0)
                wattroff(win, COLOR_PAIR(2));

            if (focus == 1)
                wattron(win, COLOR_PAIR(2));
            mvwprintw(win, buttons_y, cancel_x, "[ %s ]", cancel_label);
            if (focus == 1)
                wattroff(win, COLOR_PAIR(2));

            wrefresh(win);

            wint_t ch;
            int r = wget_wch(win, &ch);

            if (r == KEY_CODE_YES && ch == KEY_RESIZE)
            {
                int rows, cols;
                getmaxyx(stdscr, rows, cols);
                resize_term(rows, cols);

                touchwin(stdscr);
                wrefresh(stdscr);
                break;
            }

            if (ch == '\t')
            {
                focus = !focus;
                continue;
            }

            if (r == KEY_CODE_YES && ch == KEY_MOUSE)
            {
                MEVENT e;
                if (getmouse(&e) == OK)
                {
                    if (e.y == starty + buttons_y)
                    {
                        if (e.x >= startx + ok_x &&
                            e.x <  startx + ok_x + ok_len)
                            focus = 0;

                        if (e.x >= startx + cancel_x &&
                            e.x <  startx + cancel_x + cancel_len)
                            focus = 1;

                        ch = 10;
                    }
                }
            }

            if (ch == 27)
            {
                werase(win);
                wrefresh(win);
                delwin(win);

                touchwin(app->wnd);
                wrefresh(app->wnd);

                mousemask(old_mask, NULL);
                keypad(stdscr, TRUE);
                nodelay(stdscr, old_delay);
                return 0;
            }

            if (ch == 10)
            {
                int result = (focus == 0);

                delwin(win);
                mousemask(old_mask, NULL);
                nodelay(stdscr, old_delay);

                touchwin(app->wnd);
                wrefresh(app->wnd);

                return result;
            }
        }
    }
}

/**
 * @brief Displays a text input dialogue box.
 *
 * @param app Pointer to the application.
 * @param title Window title.
 * @param prompt Prompt text.
 * @param buffer Wide character buffer.
 * @param buffer_len Buffer size.
 * @return 1 if confirmed, 0 if cancelled.
 */
int run_input_dialog(App *app, const char *title, const char *prompt,
    wchar_t *buffer, size_t buffer_len)
{
    const int w = 70;
    const int h = 12;

    int focus = 0;
    int len = wcslen(buffer);

    WINDOW *win = NULL;

    int old_delay = is_nodelay(stdscr);
    nodelay(stdscr, FALSE);
    curs_set(TRUE);

    mmask_t old_mask;
    mousemask(BUTTON1_RELEASED, &old_mask);

    while (1)
    {
        getmaxyx(stdscr, app->rows, app->cols);

        int startx = (app->cols - w) / 2;
        int starty = (app->rows - h) / 2;

        if (win)
            delwin(win);

        win = newwin(h, w, starty, startx);
        keypad(win, TRUE);

        while (1)
        {
            werase(win);
            box(win, 0, 0);

            int title_len = strlen(title);
            int title_x = (w - title_len) / 2;
            if (title_x < 1) title_x = 1;
            mvwprintw(win, 1, title_x, "%s", title);

            mvwprintw(win, 3, 2, "%s", prompt);

            mvwprintw(win, 5, 2, "> ");
            mvwaddwstr(win, 5, 4, buffer);

            const char *ok_text = "OK";
            const char *cancel_text = "Cancel";

            int ok_len = strlen(ok_text) + 4;
            int cancel_len = strlen(cancel_text) + 4;
            int spacing = 18;

            int total = ok_len + cancel_len + spacing;
            int start_buttons = (w - total) / 2;

            int ok_x = start_buttons;
            int cancel_x = ok_x + ok_len + spacing;
            int buttons_y = h - 3;

            if (focus == 1)
                wattron(win, COLOR_PAIR(2));
            mvwprintw(win, buttons_y, ok_x, "[ %s ]", ok_text);
            if (focus == 1)
                wattroff(win, COLOR_PAIR(2));

            if (focus == 2)
                wattron(win, COLOR_PAIR(2));
            mvwprintw(win, buttons_y, cancel_x, "[ %s ]", cancel_text);
            if (focus == 2)
                wattroff(win, COLOR_PAIR(2));

            if (focus == 0)
                wmove(win, 5, 4 + len);

            wrefresh(win);

            wint_t ch;
            int r = wget_wch(win, &ch);

            if (r == KEY_CODE_YES && ch == KEY_RESIZE)
            {
                int rows, cols;
                getmaxyx(stdscr, rows, cols);

                resize_term(rows, cols);

                app->rows = rows;
                app->cols = cols;

                wresize(app->wnd, rows, cols);

                update_layout(app);

                draw_ui(app);

                break;
            }

            if (ch == '\t')
            {
                focus = (focus + 1) % 3;
                continue;
            }

            if (r == KEY_CODE_YES && ch == KEY_MOUSE)
            {
                MEVENT e;
                if (getmouse(&e) == OK)
                {
                    if (e.y == starty + buttons_y)
                    {
                        if (e.x >= startx + ok_x &&
                            e.x <  startx + ok_x + ok_len)
                            focus = 1;

                        if (e.x >= startx + cancel_x &&
                            e.x <  startx + cancel_x + cancel_len)
                            focus = 2;

                        ch = 10;
                    }
                }
            }

            if (ch == 27)
            {
                werase(win);
                wrefresh(win);
                delwin(win);

                touchwin(app->wnd);
                wrefresh(app->wnd);

                mousemask(old_mask, NULL);
                keypad(stdscr, TRUE);
                curs_set(FALSE);
                nodelay(stdscr, old_delay);
                return 0;
            }

            if (ch == 10)
            {
                if (focus == 0)
                {
                    if (len > 0)
                    {
                        delwin(win);
                        mousemask(old_mask, NULL);
                        curs_set(FALSE);
                        nodelay(stdscr, old_delay);

                        touchwin(app->wnd);
                        wrefresh(app->wnd);
                        return 1;
                    }
                    continue;
                }
                
                if (focus == 1)
                {
                    if (len > 0)
                    {
                        delwin(win);
                        mousemask(old_mask, NULL);
                        curs_set(FALSE);
                        nodelay(stdscr, old_delay);

                        touchwin(app->wnd);
                        wrefresh(app->wnd);
                        return 1;
                    }
                }

                if (focus == 2)
                {
                    delwin(win);
                    mousemask(old_mask, NULL);
                    curs_set(FALSE);
                    nodelay(stdscr, old_delay);

                    touchwin(app->wnd);
                    wrefresh(app->wnd);
                    return 0;
                }
            }

            if (focus == 0)
            {
                if (r == KEY_CODE_YES && ch == KEY_BACKSPACE)
                {
                    if (len > 0)
                        buffer[--len] = L'\0';
                }
                else if (r == OK && len < buffer_len - 1)
                {
                    buffer[len++] = ch;
                    buffer[len] = L'\0';
                }
            }
        }
    }
}