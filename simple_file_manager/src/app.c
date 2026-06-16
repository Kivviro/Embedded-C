/**
 * @file app.c
 * @brief Implementation of initialisation and the main application loop.
 *
 * Responsible for:
 * - creating the App structure
 * - configuring the interface
 * - registering buttons
 * - starting the main event processing loop
 * - correct termination
 */
#include <unistd.h>
#include "app.h"
#include "ui.h"
#include "panel.h"
#include "dialog.h"

/**
 * @brief Starts the application and the main file manager loop.
 *
 * Sequence:
 * - initialise ncurses
 * - create main window
 * - get current directories for panels
 * - configure control buttons
 * - load directory contents
 * - start input processing and rendering loop
 * - free resources on exit
 *
 * @return 0 if programme exits correctly.
 */
int run_app(void)
{
    App app;

    init_curses();

    getmaxyx(stdscr, app.rows, app.cols);
    app.wnd = newwin(app.rows, app.cols, 0, 0);

    getcwd(app.left.cwd,sizeof(app.left.cwd));
    getcwd(app.right.cwd,sizeof(app.right.cwd));

    app.left.entries = NULL;
    app.right.entries = NULL;
    app.active = 0;

    app.button_count = 6;

    app.buttons[0] = (Button){0};
    app.buttons[1] = (Button){0};
    app.buttons[2] = (Button){0};
    app.buttons[3] = (Button){0};

    app.buttons[0].label = "MOVE (m)";
    app.buttons[0].hotkey = 'm';
    app.buttons[0].action = move_dialog;

    app.buttons[1].label = "OPEN FILE (e)";
    app.buttons[1].hotkey = 'n';
    app.buttons[1].action = open_file_dialog;

    app.buttons[4].label = "DEL DIR (d)";
    app.buttons[4].hotkey = 'd';
    app.buttons[4].action = delete_directory_dialog;

    app.buttons[2].label = "NEW DIR (c)";
    app.buttons[2].hotkey = 'c';
    app.buttons[2].action = create_directory_dialog;

    app.buttons[3].label = "COPY (f)";
    app.buttons[3].hotkey = 'f';
    app.buttons[3].action = copy_dialog;

    app.buttons[5].label = "EXIT (q)";
    app.buttons[5].hotkey = 'q';
    app.buttons[5].action = NULL;

    update_layout(&app);

    load_directory(&app.left);
    load_directory(&app.right);

    int running = 1;
    while(running)
    {
        running = handle_input(&app);
        draw_ui(&app);
        napms(16);
    }

    free_panel(&app.left);
    free_panel(&app.right);
    delwin(app.wnd);
    cleanup_curses();
    return 0;
}