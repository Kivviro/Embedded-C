/*
    MESSAGE:
    
*/
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#include <stdint.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>

#define MAX_DRIVERS 64 // make to dynamic
#define SHM_NAME "/driver_shm"

typedef struct
{
    pid_t pid;
    int active;

    int task_timer;
    time_t busy_until;

    int event_fd;
} DriverSlot;

DriverSlot *shared_ds = NULL;
int shm_fd = -1;

int find_slot(pid_t pid);
void driver_loop(int efd);
void init_shm();


int find_slot(pid_t pid)
{
    for (int i = 0; i < MAX_DRIVERS; i++)
    {
        if (shared_ds[i].active && shared_ds[i].pid == pid)
            return i;
    }
    return -1;
}

void driver_loop(int efd)
{
    pid_t pid = getpid();

    int idx = find_slot(pid);
    
    if (idx == -1)
        exit(EXIT_FAILURE);
    
    int epfd = epoll_create1(0);

    if (epfd == -1)
    {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    int tfd = timerfd_create(CLOCK_MONOTONIC, 0);

    if (tfd == -1)
    {
        perror("timerfd_create");
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev;

    ev.events = EPOLLIN;
    ev.data.fd = efd;

    epoll_ctl(epfd, EPOLL_CTL_ADD, efd, &ev);

    ev.events = EPOLLIN;
    ev.data.fd = tfd;

    epoll_ctl(epfd, EPOLL_CTL_ADD, tfd, &ev);

    while(1)
    {
       struct epoll_event events[2];

       int n = epoll_wait(epfd, events, 2, -1);

       for (int i = 0; i < n; i++)
       {
            if (events[i].data.fd == efd)
            {
                uint64_t value;

                read(efd, &value, sizeof(value));

                int task_time = shared_ds[idx].task_timer;

                shared_ds[idx].busy_until = time(NULL) + task_time;

                struct itimerspec spec;

                memset(&spec, 0, sizeof(spec));

                spec.it_value.tv_sec = task_time;

                timerfd_settime(tfd, 0, &spec, NULL);

                printf("[Driver %d] Started task (%d sec)\n", pid, task_time);
            }
            else if (events[i].data.fd == tfd)
            {
                uint64_t expirations;

                read(tfd, &expirations, sizeof(expirations));

                shared_ds[idx].busy_until = 0;

                printf("[Driver %d] Task completed!\n", pid);
            }
       }
    }
}

void init_shm()
{
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);

    if (shm_fd < 0)
    {
        perror("shm_open");
        exit(1);
    }

    ftruncate(shm_fd, sizeof(DriverSlot) * MAX_DRIVERS);

    shared_ds = mmap(NULL, sizeof(DriverSlot) * MAX_DRIVERS, PROT_READ | PROT_WRITE, 
    MAP_SHARED, shm_fd, 0);

    if (shared_ds == MAP_FAILED)
    {
        perror("mmap");
        exit(1);
    }

    for (int i = 0; i < MAX_DRIVERS; i++)
    {
        shared_ds[i].active = 0;
        shared_ds[i].pid = 0;
        shared_ds[i].busy_until = 0;
        shared_ds[i].task_timer = 0;
    }
}

// commands func
void create_driver()
{
    int efd = eventfd(0, EFD_CLOEXEC);

    if (efd == -1)
    {
        perror("eventfd");
        return;
    }

    pid_t pid = fork();

    if(pid < 0)
    {
        perror("fork");
        return;
    }

    if (pid == 0)
    {
        driver_loop(efd);
        exit(0);
    }

    for (int i = 0; i < MAX_DRIVERS; i++)
    {
        if(!shared_ds[i].active)
        {
            shared_ds[i].pid = pid;
            shared_ds[i].active = 1;
            shared_ds[i].busy_until = 0;
            shared_ds[i].task_timer = 0;
            shared_ds[i].event_fd = efd;

            printf("Driver created with PID=%d\n", pid);
            return;
        }
    }
    
    printf("No free slots\n"); // create dynamic memory later
}


// for send_task and get_status add less code funs
void send_task(pid_t pid, int t)
{
    int idx = find_slot(pid);

    if (idx == -1)
    {
        printf("Driver not found\n");
        return;
    }

    time_t now = time(NULL);

    if (shared_ds[idx].busy_until > now)
    {
        int left = (int)(shared_ds[idx].busy_until - now);
        printf("Busy %d\n", left);
        return;
    }

    shared_ds[idx].task_timer = t;
    //shared_ds[idx].busy_until = now + t;
    uint64_t value = 1;
    write(shared_ds[idx].event_fd, &value, sizeof(value));

    printf("Task sent to PID=%d for %d sec.\n", pid, t);
}

void get_status(pid_t pid)
{
    int idx = find_slot(pid);

    if (idx == -1)
    {
        printf("Driver not found\n");
        return;
    }

    time_t now = time(NULL);

    if (shared_ds[idx].busy_until > now)
    {
        int left = (int)(shared_ds[idx].busy_until - now);
        printf("Busy %d\n", left);
    }
    else
    {
        printf("Available\n");
    }
}

void get_drivers()
{
    time_t now = time(NULL);

    for (int i = 0; i < MAX_DRIVERS; i++)
    {
        if (shared_ds[i].active)
        {
            if (shared_ds[i].busy_until > now)
            {
                printf("pid=%d BUSY %ld sec left\n", 
                    shared_ds[i].pid, shared_ds[i].busy_until - now);
            }
            else
            {
                printf("pid=%d AVAILABLE\n", shared_ds[i].pid);
            }
        }
    }
}

void terminate_all()
{
    printf("Terminating drivers...\n");

    for (int i = 0; i < MAX_DRIVERS; i++)
    {
        if (shared_ds[i].active)
        {
            close(shared_ds[i].event_fd);
            kill(shared_ds[i].pid, SIGTERM);
        }
    }
    
    for (int i = 0; i < MAX_DRIVERS; i++)
    {
        if (shared_ds[i].active)
        {
            waitpid(shared_ds[i].pid, NULL, 0);
        }
    }

    munmap(shared_ds, sizeof(DriverSlot) * MAX_DRIVERS);
    close(shm_fd);
    shm_unlink(SHM_NAME);

    printf("Cleanup all done");

    exit(0);
}

void process_command(char *line)
{
    char *cmd = strtok(line, " ");

    if (!cmd)
        return;

    if (strcmp(cmd, "create_driver") == 0 || strcmp(cmd, "crt") == 0)
    { 
        create_driver();
    }
    else if (strcmp(cmd, "send_task") == 0 || strcmp(cmd, "sndk") == 0)
    {
        char *pid_s = strtok(NULL, " ");
        char *t_s = strtok(NULL, " ");

        if (!pid_s || !t_s)
        {
            printf("Usage: sent_task (or sndk) <pid> <time>\n");
        }

        send_task((pid_t)atoi(pid_s), atoi(t_s));
    }
    else if (strcmp(cmd, "get_status") == 0 || strcmp(cmd, "gs") == 0)
    {
        char *pid_s = strtok(NULL, " ");

        if (!pid_s)
        {
            printf("Usage: get_status (or gs) <pid>\n");
            return;
        }

        get_status((pid_t)atoi(pid_s));
    }
    else if (strcmp(cmd, "get_drivers") == 0 || strcmp(cmd, "gd") == 0)
    {
        get_drivers();
    }
    else if (strcmp(cmd, "terminate_all") == 0 || strcmp(cmd, "term") == 0)
    {
        terminate_all();
    }
    else
    {
        printf("Unknown command\n");
    }
}

int main()
{
    init_shm();

    char line[256];

    printf("Commands:\n");
    printf("create_driver (crt) | send_task (sndk) | get_status (gs) | get_drivers (gd) | terminate_all (term)\n");

    while (1)
    {
        printf("> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin))
            break;

        line[strcspn(line, "\n")] = 0;
        
        process_command(line);
    }

    terminate_all();
    return 0;
}