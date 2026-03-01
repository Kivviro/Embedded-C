/**
 * @file dialog.c
 * @brief Module for operations on files and directories.
 *
 * Implements:
 * - directory creation
 * - deletion
 * - copying
 * - moving
 * - opening files
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <wchar.h>
#include <ncursesw/curses.h>
#include <magic.h>
#include <sys/stat.h> 
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>

#include "dialog.h"
#include "panel.h"
#include "ui.h"
#include "modal.h"

/**
 * @enum Operation
 * @brief File operation type.
 */
typedef enum
{
    OP_COPY, /**< Copy */
    OP_DELETE, /**< Delete */
    OP_MOVE /**< Move */
} Operation;

void create_directory_dialog(App *app);
void delete_directory_dialog(App *app);
void copy_dialog(App *app);
void open_file_dialog(App *app);
void move_dialog(App *app);

static int process_tree(const char *src, const char *dst, Operation op);
static int copy_file(const char *src, const char *dst);
static char *build_path_alloc(const char *dir, const char *name);

/**
 * @brief Directory creation dialogue.
 * @param app Pointer to the application.
 */
void create_directory_dialog(App *app)
{
    Panel *p = (app->active == 0) ? &app->left : &app->right;

    wchar_t name[PATH_MAX] = {0};

    if (!run_input_dialog(app, "Make directory",
        "Enter the directory name:", name, PATH_MAX))
        return;

    char utf8[PATH_MAX] = {0};
    wcstombs(utf8, name, sizeof(utf8));

    char *fullpath = build_path_alloc(p->cwd, utf8);
    if (!fullpath)
        return;

    mkdir(fullpath, 0755);

    free(fullpath);

    load_directory(p);
}

/**
 * @brief Copies a file block by block.
 *
 * Uses open(), read(), write().
 *
 * @param src Source file.
 * @param dst Destination file.
 * @return 0 on success, -1 on error.
 */
static int copy_file(const char *src, const char *dst)
{
    int in = open(src, O_RDONLY);
    if (in < 0)
        return -1;

    struct stat st;
    fstat(in, &st);

    int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode);
    if (out < 0)
    {
        close(in);
        return -1;
    }

    char buf[8192];
    ssize_t r;

    while ((r = read(in, buf, sizeof(buf))) > 0)
        write(out, buf, r);

    close(in);
    close(out);
    return 0;
}

/**
 * @brief File or directory copy dialogue.
 * @param app Pointer to the application.
 */
void copy_dialog(App *app)
{
    Panel *src = (app->active == 0) ? &app->left : &app->right;
    Panel *dst = (app->active == 0) ? &app->right : &app->left;

    if (src->count <= 0)
        return;

    struct dirent *e = src->entries[src->selected];

    if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
        return;

    wchar_t input[PATH_MAX];
    mbstowcs(input, dst->cwd, PATH_MAX);

    if (!run_input_dialog(app, "Copy", "Enter the destination path:", 
        input, PATH_MAX))
        return;

    char dst_dir[PATH_MAX] = {0};
    wcstombs(dst_dir, input, sizeof(dst_dir));

    char *src_path = build_path_alloc(src->cwd, e->d_name);
    char *dst_path = build_path_alloc(dst_dir, e->d_name);

    if (!src_path || !dst_path)
    {
        free(src_path);
        free(dst_path);
        return;
    }

    process_tree(src_path, dst_path, OP_COPY);

    free(src_path);
    free(dst_path);

    load_directory(dst);
}

/**
 * @brief Directory deletion dialogue.
 * @param app Pointer to the application.
 */
void delete_directory_dialog(App *app)
{
    Panel *p = (app->active == 0) ? &app->left : &app->right;

    if (p->count <= 0)
        return;

    struct dirent *e = p->entries[p->selected];

    if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
        return;

    char *path = build_path_alloc(p->cwd, e->d_name);
    if (!path)
        return;

    struct stat st;
    if (stat(path, &st) != 0)
    {
        free(path);
        return;
    }

    const char *title;
    const char *line1;

    if (S_ISDIR(st.st_mode))
    {
        title = "Delete directory";
        line1 = e->d_name;
    }
    else
    {
        title = "Delete file";
        line1 = e->d_name;
    }

    if (!run_confirm_dialog(app, title, line1, NULL, "Delete", "Cancel"))
    {
        free(path);
        return;
    }

    process_tree(path, NULL, OP_DELETE);

    free(path);

    load_directory(p);
}

/**
 * @brief Recursive processing of the file tree.
 *
 * Performs copying, deletion, or moving
 * depending on the value of op.
 *
 * @param src Source path.
 * @param dst Destination path (can be NULL for deletion).
 * @param op Type of operation.
 * @return 0 on success, -1 on error.
 */
static int process_tree(const char *src, const char *dst, Operation op)
{
    struct stat st;

    if (stat(src, &st) != 0)
        return -1;

    if (op == OP_MOVE)
    {
        if (!dst)
            return -1;

        if (strcmp(src, dst) == 0)
            return 0;

        if (rename(src, dst) == 0)
            return 0;


        if (S_ISDIR(st.st_mode))
        {
            if (mkdir(dst, 0755) != 0)
                return -1;

            DIR *dir = opendir(src);
            if (!dir)
                return -1;

            struct dirent *e;

            while ((e = readdir(dir)) != NULL)
            {
                if (!strcmp(e->d_name, ".") ||
                    !strcmp(e->d_name, ".."))
                    continue;

                char src_path[PATH_MAX];
                char dst_path[PATH_MAX];

                snprintf(src_path, sizeof(src_path), "%s/%s", 
                src, e->d_name);


                snprintf(dst_path, sizeof(dst_path), "%s/%s", 
                dst, e->d_name);

                process_tree(src_path, dst_path, OP_MOVE);
            }

            closedir(dir);

            rmdir(src);
            return 0;
        }
        else
        {
            if (copy_file(src, dst) == 0)
            {
                unlink(src);
                return 0;
            }

            return -1;
        }
    }

    if (op == OP_COPY)
    {
        if (S_ISDIR(st.st_mode))
        {
            if (mkdir(dst, 0755) != 0)
                return -1;

            DIR *dir = opendir(src);
            if (!dir)
                return -1;

            struct dirent *e;

            while ((e = readdir(dir)) != NULL)
            {
                if (!strcmp(e->d_name, ".") ||
                    !strcmp(e->d_name, ".."))
                    continue;

                char src_path[PATH_MAX];
                char dst_path[PATH_MAX];

                snprintf(src_path, sizeof(src_path), "%s/%s", 
                src, e->d_name);

                snprintf(dst_path, sizeof(dst_path), "%s/%s", 
                dst, e->d_name);

                process_tree(src_path, dst_path, OP_COPY);
            }

            closedir(dir);
            return 0;
        }
        else
        {
            return copy_file(src, dst);
        }
    }

    if (op == OP_DELETE)
    {
        if (S_ISDIR(st.st_mode))
        {
            DIR *dir = opendir(src);
            if (!dir)
                return -1;

            struct dirent *e;

            while ((e = readdir(dir)) != NULL)
            {
                if (!strcmp(e->d_name, ".") ||
                    !strcmp(e->d_name, ".."))
                    continue;

                char path[PATH_MAX];

                snprintf(path, sizeof(path), "%s/%s", src, e->d_name);

                process_tree(path, NULL, OP_DELETE);
            }

            closedir(dir);
            rmdir(src);
        }
        else
        {
            unlink(src);
        }

        return 0;
    }

    return -1;
}

/**
 * @brief Opens a file in an external editor.
 *
 * Terminates ncurses, launches the editor,
 * then restores the interface.
 *
 * @param app Pointer to the application.
 */
void open_file_dialog(App *app)
{
    Panel *p = (app->active == 0) ? &app->left : &app->right;

    if (p->count <= 0)
        return;

    struct dirent *e = p->entries[p->selected];

    if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
        return;

    char *path = build_path_alloc(p->cwd, e->d_name);
    if (!path)
        return;

    struct stat st;
    if (stat(path, &st) != 0 || S_ISDIR(st.st_mode))
    {   
        free(path);
        return;
    }

    endwin();

    char command[PATH_MAX + 10];
    snprintf(command, sizeof(command), "nano \"%s\"", path);

    system(command);

    refresh();
    clear();

    free(path);
}

/**
 * @brief Moves a file or directory
 *
 * Moves a file or directory using mkdir, 
 * opendir, closedir, rmdir
 *
 * @param app Pointer to the application.
 */
void move_dialog(App *app)
{
    Panel *src = (app->active == 0) ? &app->left : &app->right;
    Panel *dst = (app->active == 0) ? &app->right : &app->left;

    if (src->count <= 0)
        return;

    struct dirent *e = src->entries[src->selected];

    if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
        return;

    char *src_path = build_path_alloc(src->cwd, e->d_name);
    if (!src_path)
        return;

    wchar_t input[PATH_MAX];
    mbstowcs(input, dst->cwd, PATH_MAX);

    if (!run_input_dialog(app, "Move", "Enter the destination path:",
    input, PATH_MAX))
    {
        free(src_path);
        return;
    }

    char new_dir[PATH_MAX] = {0};
    wcstombs(new_dir, input, sizeof(new_dir));

    char *dst_path = build_path_alloc(new_dir, e->d_name);
    if (!dst_path)
    {
        free(src_path);
        return;
    }

    process_tree(src_path, dst_path, OP_MOVE);

    free(src_path);
    free(dst_path);

    load_directory(src);
    load_directory(dst);
}

/**
 * @brief Creates a path string with dynamic memory allocation.
 *
 * @param dir Base directory.
 * @param name File/directory name.
 * @return Pointer to the new string or NULL.
 */
static char *build_path_alloc(const char *dir, const char *name)
{
    if (!dir || !name)
        return NULL;

    size_t len_dir = strlen(dir);
    size_t len_name = strlen(name);

    size_t total = len_dir + 1 + len_name + 1;

    char *path = malloc(total);
    if (!path)
        return NULL;

    snprintf(path, total, "%s/%s", dir, name);

    return path;
}