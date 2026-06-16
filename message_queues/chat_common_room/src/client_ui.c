#define _XOPEN_SOURCE_EXTENDED 1
#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include "client.h"

static WINDOW *win_output = NULL;
static WINDOW *win_clients = NULL;
static WINDOW *win_input = NULL;

static int output_scroll = 0;

static client_state_t gstate;
static pthread_t reader_tid;

static void ui_init(void);
static void ui_destroy(void);
static void ui_resize_windows(void);
static void ui_draw_all(void);
static void ui_handle_input_loop(void);
static void ui_refresh_output_win(void);
static void ui_refresh_clients_win(void);
static void ui_refresh_input_win(const char *input_buf);

static volatile sig_atomic_t resize_needed = 0;
static void handle_winch(int sig) { (void)sig; resize_needed = 1; }

static void ui_init(void) 
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);
    struct sigaction sa;
    sa.sa_handler = handle_winch;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGWINCH, &sa, NULL);

    ui_resize_windows();
}

static void ui_destroy(void) 
{
    if (win_output) 
    { 
        delwin(win_output); 
        win_output = NULL; 
    }
    if (win_clients) 
    { 
        delwin(win_clients); 
        win_clients = NULL; 
    }
    if (win_input) 
    { 
        delwin(win_input); 
        win_input = NULL; 
    }
    endwin();
}

static void ui_resize_windows(void) 
{
    int h, w;
    getmaxyx(stdscr, h, w);

    int clients_w = (w > 20) ? w/4 : 20;
    int input_h = 3;
    int output_w = w - clients_w;
    int output_h = h - input_h;

    if (win_output) 
    { 
        delwin(win_output); 
        win_output = NULL; 
    }
    if (win_clients) 
    { 
        delwin(win_clients); 
        win_clients = NULL; 
    }
    if (win_input) 
    { 
        delwin(win_input); 
        win_input = NULL; 
    }

    win_output = newwin(output_h, output_w, 0, 0);
    box(win_output, 0, 0);
    mvwprintw(win_output, 0, 2, " Messages ");

    win_clients = newwin(output_h, clients_w, 0, output_w);
    box(win_clients, 0, 0);
    mvwprintw(win_clients, 0, 2, " Clients ");

    win_input = newwin(input_h, w, output_h, 0);
    box(win_input, 0, 0);
    mvwprintw(win_input, 0, 2, " Input (type /quit to exit) ");

    wrefresh(win_output);
    wrefresh(win_clients);
    wrefresh(win_input);
}

static void ui_refresh_output_win(void) 
{
    werase(win_output);
    box(win_output, 0, 0);
    mvwprintw(win_output, 0, 2, " Messages ");

    int h, w;
    getmaxyx(win_output, h, w);
    int inner_h = h - 2;

    pthread_mutex_lock(&gstate.lines_lock);

    int total = gstate.lines_count;
    int start = total - inner_h - output_scroll;
    if (start < 0) 
        start = 0;

    int line_idx = start;
    int row = 1;
    while (row <= inner_h && line_idx < total) 
    {
        mvwprintw(win_output, row, 1, "%.*s", w-2, gstate.lines[line_idx].text);
        row++; line_idx++;
    }

    pthread_mutex_unlock(&gstate.lines_lock);

    wrefresh(win_output);
}

static void ui_refresh_clients_win(void) 
{
    werase(win_clients);
    box(win_clients, 0, 0);
    mvwprintw(win_clients, 0, 2, " Clients ");

    int h, w;
    getmaxyx(win_clients, h, w);
    pthread_mutex_lock(&gstate.clients_lock);
    int count = gstate.clients_count;
    for (int i = 0; i < count && i < h-2; ++i) 
    {
        const char *qname = gstate.clients[i];
        const char *p = strrchr(qname, '_');
        if (p) p++; else p = qname;
        mvwprintw(win_clients, i+1, 1, "%s", p);
    }
    pthread_mutex_unlock(&gstate.clients_lock);

    wrefresh(win_clients);
}

static void ui_refresh_input_win(const char *input_buf) 
{
    werase(win_input);
    box(win_input, 0, 0);
    mvwprintw(win_input, 0, 2, " Input (type /quit to exit) ");
    int h, w;
    getmaxyx(win_input, h, w);
    mvwprintw(win_input, 1, 1, "%.*s", w-2, input_buf ? input_buf : "");
    wmove(win_input, 1, 1 + (int)strlen(input_buf));
    wrefresh(win_input);
}

static void ui_draw_all(void) 
{
    ui_refresh_output_win();
    ui_refresh_clients_win();
    ui_refresh_input_win("");
}

static void ui_handle_input_loop(void) 
{
    char input_buf[MAX_MSG_SIZE] = {0};
    size_t input_len = 0;

    timeout(200);

    while (!gstate.stop_requested) 
    {
        if (resize_needed) 
        {
            resize_needed = 0;
            endwin();
            refresh();
            clear();
            ui_resize_windows();
            ui_draw_all();
        }

        ui_refresh_output_win();
        ui_refresh_clients_win();
        ui_refresh_input_win(input_buf);

        int ch = getch();
        if (ch == ERR) 
        {
            continue;
        } 
        else if (ch == KEY_RESIZE) 
        {
            ui_resize_windows();
            continue;
        } 
        else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) 
        {
            if (input_len > 0) 
            {
                input_buf[--input_len] = '\0';
            }
        } 
        else if (ch == '\n' || ch == '\r') 
        {
            if (input_len > 0) 
            {
                if (strcmp(input_buf, "/quit") == 0) 
                {
                    gstate.stop_requested = 1;
                    break;
                }
                client_send_text(&gstate, input_buf);

                input_buf[0] = '\0';
                input_len = 0;
            }
        } 
        else if (ch == KEY_PPAGE) 
        {
            output_scroll += 5;
            if (output_scroll < 0) 
                output_scroll = 0;
        } 
        else if (ch == KEY_NPAGE) 
        {
            output_scroll -= 5;
            if (output_scroll < 0) 
                output_scroll = 0;
        } 
        else if (isprint(ch) && input_len + 1 < sizeof(input_buf)) 
        {
            input_buf[input_len++] = (char)ch;
            input_buf[input_len] = '\0';
        }
    }
}

int main(void) 
{
    client_state_init(&gstate);

    if (client_core_start(&gstate, &reader_tid) != 0) 
    {
        fprintf(stderr, "Failed to start client core\n");
        client_state_destroy(&gstate);
        return 1;
    }

    ui_init();
    ui_draw_all();

    ui_handle_input_loop();

    ui_destroy();
    client_state_destroy(&gstate);

    client_core_stop(&gstate, reader_tid);

    printf("Client %d exit\n", (int)gstate.pid);
    return 0;
}