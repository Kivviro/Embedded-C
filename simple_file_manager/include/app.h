/**
 * @file app.h
 * @brief Main structures and application interface.
 *
 * Contains definitions of the Panel, Button, and App structures
 * and a prototype of the application launch function.
 */
#ifndef APP_H
#define APP_H

#include <ncursesw/curses.h>
#include <dirent.h>
#include <linux/limits.h>
#include <wchar.h>

typedef struct App App; 

/**
 * @struct Panel
 * @brief Represents a single file panel.
 *
 * Contains the current directory, list of items,
 * display settings, and selection state.
 */
typedef struct Panel
{
    char cwd[PATH_MAX];

    struct dirent **entries;
    int count;

    int selected;
    int scroll;

    int x, y;
    int w, h;

} Panel;

/**
 * @struct Button
 * @brief Describes the button on the bottom control panel.
 *
 * Contains coordinates, dimensions, label text,
 * hotkey, and pointer to the action function.
 */
typedef struct Button
{
    int x, y;
    int w, h;
    const char *label;
    int hotkey;
    void (*action)(struct App *app);
} Button;

/**
 * @struct App
 * @brief Central application structure.
 *
 * Contains:
 * - main ncurses window
 * - screen dimensions
 * - button array
 * - two panels
 * - active panel index
 */
typedef struct App
{
    WINDOW *wnd;
    int rows, cols;

    Button buttons[6];
    int button_count;

    Panel left;
    Panel right;

    int active;

} App;

int run_app(void);

#endif