/**
 * @file panel.c
 * @brief Module for working with file panels.
 *
 * Responsible for:
 * - loading the directory
 * - navigation
 * - freeing memory
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <dirent.h>

#include "panel.h"

/**
 * @brief Frees the resources of the panel.
 *
 * Frees the dirent array,
 * obtained via scandir().
 *
 * @param p Pointer to the panel.
 */
void free_panel(Panel *p)
{
    if (!p->entries) 
        return;

    for (int i = 0; i < p->count; i++)
        free(p->entries[i]);

    free(p->entries);
    p->entries = NULL;
}

/**
 * @brief Loads the contents of the current directory of the panel.
 *
 * Uses scandir() to obtain a list of files.
 *
 * @param p Pointer to the panel.
 * @return 0 on success, -1 on error.
 */
int load_directory(Panel *p)
{
    free_panel(p);

    p->count = scandir(p->cwd, &p->entries, NULL, alphasort);
    if (p->count < 0)
        return -1;

    p->selected = 0;
    p->scroll = 0;

    for (int i = 0; i < p->count; i++)
        if (strcmp(p->entries[i]->d_name, "..") == 0)
            p->selected = i;

    return 0;
}

/**
 * @brief Navigate to the selected directory.
 *
 * Updates the panel's cwd and reloads the content.
 *
 * @param p Pointer to the panel.
 */
void enter_directory(Panel *p)
{
    struct dirent *e = p->entries[p->selected];
    if (e->d_type != DT_DIR) 
        return;

    char new_path[PATH_MAX];

    if (strcmp(e->d_name, "..") == 0)
    {
        if (snprintf(new_path, sizeof(new_path),
             "%s/..", p->cwd) >= sizeof(new_path))
            return;
    }
    else
    {
        if (snprintf(new_path, sizeof(new_path),
             "%s/%s", p->cwd, e->d_name) >= sizeof(new_path))
            return;
    }

    if (!realpath(new_path, p->cwd))
        return;
    load_directory(p);
}

/**
 * @brief Moves the selection in the panel.
 *
 * Updates selected and scroll based on
 * the visible area.
 *
 * @param p Pointer to the panel.
 * @param dir Direction (-1 up, 1 down).
 */
void move_selection(Panel *p, int dir)
{
    if (p->count <= 0)
            return;

    if (p->h <= 2)
    {
        p->selected = 0;
        p->scroll = 0;
        return;
    }

    int visible = p->h - 2;

    p->selected += dir;

    if (p->selected < 0)
        p->selected = 0;

    if (p->selected >= p->count)
        p->selected = p->count - 1;

    if (p->selected < p->scroll)
        p->scroll = p->selected;

    if (p->selected >= p->scroll + visible)
        p->scroll = p->selected - visible + 1;

    if (p->scroll < 0)
        p->scroll = 0;

    if (p->scroll > p->count - visible)
        p->scroll = p->count - visible;

    if (p->scroll < 0)
        p->scroll = 0;
}