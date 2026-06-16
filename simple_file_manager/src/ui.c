/**
 * @file ui.c
 * @brief User interface module.
 *
 * Responsible for:
 * - initialising ncurses
 * - drawing panels
 * - processing input
 * - updating layout
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <ncursesw/curses.h>
#include <magic.h> // для определения типа файла, чтобы не мучиться 
#include <sys/stat.h>
#include <ctype.h>

#include "ui.h"
#include "panel.h"
#include "dialog.h"


static magic_t magic_cookie = NULL;

/**
 * @brief Initialisation of ncurses and colour schemes.
 *
 * Configures terminal mode, mouse, colours,
 * and initialises libmagic for file type detection.
 */
void init_curses(void)
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(FALSE);

    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    mouseinterval(0);

    start_color();
    use_default_colors();

    init_pair(1, COLOR_WHITE, -1);
    init_pair(2, COLOR_BLACK, COLOR_WHITE);
    init_pair(3, COLOR_GREEN, -1);

    if (COLORS >= 16)
        init_pair(4, 208, -1); 
    else
        init_pair(4, COLOR_YELLOW, -1);

    init_pair(5, COLOR_MAGENTA, -1);
    init_pair(6, COLOR_BLUE, -1);
    init_pair(7, COLOR_WHITE, -1);

    magic_cookie = magic_open(MAGIC_MIME_TYPE);
    magic_load(magic_cookie, NULL);
}

/**
 * @brief Terminates ncurses.
 *
 * Frees libmagic resources and returns the terminal
 * to its normal state.
 */
void cleanup_curses(void)
{
    if (magic_cookie)
        magic_close(magic_cookie);

    endwin();
}

/**
 * @brief Determines the display colour of a file.
 *
 * @param dir The current directory of the panel.
 * @param name The file name.
 * @return The ncurses colour pair number.
 *
 * Uses stat() and libmagic to determine the type.
 */
int get_file_color(const char *dir, const char *name)
{
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s", dir, name);

    struct stat st;
    if (stat(path, &st) == 0)
    {
        if (S_ISDIR(st.st_mode))
            return 7;

        if (st.st_mode & S_IXUSR)
            return 3;
    }

    const char *mime = magic_file(magic_cookie, path);
    if (!mime)
        return 7;

    if (strstr(mime, "text"))
        return 4;

    if (strstr(mime, "zip") ||
        strstr(mime, "gzip") ||
        strstr(mime, "tar") ||
        strstr(mime, "rar") ||
        strstr(mime, "7z"))
        return 5;

    if (strstr(mime, "image") ||
        strstr(mime, "video"))
        return 6;

    if (strstr(mime, "application"))
        return 3;

    return 7;
}

/**
 * @brief Updates the sizes and positions of interface elements.
 *
 * Recalculates:
 * - panel sizes
 * - button coordinates
 * - window parameters
 *
 * @param app Pointer to the application structure.
 */
void update_layout(App *app)
{
    getmaxyx(stdscr, app->rows, app->cols);

    wresize(app->wnd, app->rows, app->cols);

    int mid = app->cols / 2;
    int bottom = app->rows - 4;

    app->left.x = 0;
    app->left.y = 0;
    app->left.w = mid;
    app->left.h = bottom;

    app->right.x = mid;
    app->right.y = 0;
    app->right.w = app->cols - mid;
    app->right.h = bottom;

    int x = 2;
    int y = app->rows - 2;

    for (int i = 0; i < app->button_count; i++)
    {
        int w = strlen(app->buttons[i].label) + 4;

        app->buttons[i].w = w;
        app->buttons[i].h = 1;
        app->buttons[i].x = x;
        app->buttons[i].y = y;

        x += w + 2;
    }
}

static void draw_button(WINDOW *wnd, Button *btn)
{
    wattron(wnd, COLOR_PAIR(2));

    for (int i = 0; i < btn->w; i++)
        mvwaddch(wnd, btn->y, btn->x + i, ' ');

    mvwprintw(wnd, btn->y, btn->x + 2, "%s", btn->label);

    wattroff(wnd, COLOR_PAIR(2));
}

/**
 * @brief Draws a single file panel.
 *
 * @param app Pointer to the application.
 * @param p Pointer to the panel.
 * @param active Panel activity flag.
 */
void draw_panel(App *app, Panel *p, int active)
{
    int x0 = p->x;
    int y0 = p->y;
    int w  = p->w;
    int h  = p->h;

    // рамка
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
        {
            if (y == 0 && x == 0) 
                mvwaddch(app->wnd,y0+y,x0+x,ACS_ULCORNER);
            else if (y == 0 && x == w-1) 
                mvwaddch(app->wnd,y0+y,x0+x,ACS_URCORNER);
            else if (y == h-1 && x == 0) 
                mvwaddch(app->wnd,y0+y,x0+x,ACS_LLCORNER);
            else if (y == h-1 && x == w-1) 
                mvwaddch(app->wnd,y0+y,x0+x,ACS_LRCORNER);
            else if (y == 0 || y == h-1) 
                mvwaddch(app->wnd,y0+y,x0+x,ACS_HLINE);
            else if (x == 0 || x == w-1) 
                mvwaddch(app->wnd,y0+y,x0+x,ACS_VLINE);
        }

    mvwprintw(app->wnd,y0,x0+2,"%.*s",w-4,p->cwd);

    int visible = h-2;
    for (int i = 0; i < visible; i++)
    {
        int idx = p->scroll + i;
        if (idx >= p->count)
            break;

        int is_selected = (idx == p->selected && active);

        if (is_selected)
        {
            wattron(app->wnd, COLOR_PAIR(2));
        }
        else
        {
            int color = get_file_color(p->cwd, p->entries[idx]->d_name);
            wattron(app->wnd, COLOR_PAIR(color));
        }

        mvwprintw(app->wnd, y0 + 1 + i, x0 + 2, "%-*.*s", w - 4, w - 4, p->entries[idx]->d_name);

        if (is_selected)
            wattroff(app->wnd, COLOR_PAIR(2));
        else
            wattroff(app->wnd, COLOR_PAIR(get_file_color(p->cwd, p->entries[idx]->d_name)));
    }
}

/**
 * @brief Processes user input.
 *
 * Supports:
 * - keyboard
 * - mouse
 * - window resize :')
 * - hotkeys
 *
 * @param app Pointer to the application.
 * @return 0 if the programme needs to be terminated, otherwise 1.
 */
int handle_input(App *app)
{
    static int last_click_idx = -1;
    static clock_t last_click_time = 0;

    int ch = getch();
    Panel *p = (app->active == 0) ? &app->left : &app->right;

    if (ch == ERR)
        return 1;

    if (ch == KEY_RESIZE)
    {
        int new_rows, new_cols;
        getmaxyx(stdscr, new_rows, new_cols);

        resize_term(new_rows, new_cols); 
        wresize(app->wnd, new_rows, new_cols);

        app->rows = new_rows;
        app->cols = new_cols;

        update_layout(app);

        wclear(stdscr);
        wclear(app->wnd);

        return 1;
    }

    if (ch == 'q' || ch == 'Q')
        return 0;

    for (int i = 0; i < app->button_count; i++)
    {
        if (ch == app->buttons[i].hotkey || 
            ch == toupper(app->buttons[i].hotkey))
        {
            if (app->buttons[i].action)
                app->buttons[i].action(app);
            else
                return 0; // EXIT

            update_layout(app);
            return 1;
        }
    }

    if (ch == KEY_UP)
        move_selection(p, -1);

    if (ch == KEY_DOWN)
        move_selection(p, 1);

    if (ch == '\t')
        app->active = !app->active;

    if (ch == 10)
        enter_directory(p);

    if (ch == 'e' || ch == 'E')
    {
        open_file_dialog(app);
    }

    if (ch == KEY_MOUSE)
    {
        MEVENT e;
        if (getmouse(&e) != OK)
            return 1;

        for (int i = 0; i < app->button_count; i++)
        {
            Button *b = &app->buttons[i];

            if (e.y == b->y &&
                e.x >= b->x &&
                e.x < b->x + b->w)
            {
                if (b->action)
                    b->action(app);
                else
                    return 0;

                return 1;
            }
        }

        // панели
        if (e.x < app->cols / 2)
            app->active = 0;
        else
            app->active = 1;

        p = (app->active == 0) ? &app->left : &app->right;

        if (e.bstate & (BUTTON4_PRESSED | BUTTON4_RELEASED))
            move_selection(p, -1);

        if (e.bstate & (BUTTON5_PRESSED | BUTTON5_RELEASED))
            move_selection(p, 1);

        if (e.bstate & BUTTON1_RELEASED)
        {
            int ry = e.y - (p->y + 1);
            int idx = p->scroll + ry;

            if (idx >= 0 && idx < p->count)
            {
                p->selected = idx;

                clock_t now = clock();

                if (idx == last_click_idx &&
                    (double)(now - last_click_time) / CLOCKS_PER_SEC < 0.5)
                {
                    enter_directory(p);
                    last_click_idx = -1;
                }
                else
                {
                    last_click_idx = idx;
                    last_click_time = now;
                }
            }
        }
    }

    return 1;
}

/**
 * @brief Draws the entire interface.
 *
 * Redraws panels and buttons,
 * then refreshes the window.
 *
 * @param app Pointer to the application.
 */
void draw_ui(App *app)
{
    werase(app->wnd);
    wbkgd(app->wnd,COLOR_PAIR(1));

    draw_panel(app,&app->left, app->active == 0);
    draw_panel(app,&app->right,app->active == 1);

    for (int i = 0; i < app->button_count; i++)
        draw_button(app->wnd, &app->buttons[i]);

    wrefresh(app->wnd);
}