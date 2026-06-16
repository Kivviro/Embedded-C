/**
 * @file driver_manager.c
 * @brief Driver management system based on shared memory and Linux event mechanisms.
 *
 * This program implements a simple manager for worker processes ("drivers").
 * It supports creating child processes, assigning timed tasks to them,
 * monitoring their status, and performing graceful shutdown.
 *
 * Inter-process communication is implemented using POSIX shared memory,
 * eventfd, timerfd, and epoll.
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

#include <pthread.h>
#include <errno.h>
#include <limits.h>

/** @brief Maximum number of supported driver processes. */
#define MAX_DRIVERS 32

/** @brief Name of the POSIX shared memory object. */
#define SHM_NAME "/driver_shm"

/** @brief Maximum length of an input command line. */
#define MAX_LINE 256

/** @brief Minimum allowed task duration (seconds). */
#define MIN_TASK_TIME 1

/** @brief Maximum allowed task duration (seconds). */
#define MAX_TASK_TIME 3600

/**
 * @enum DriverState
 * @brief Represents the current state of a driver.
 */
typedef enum
{
    DRIVER_AVAILABLE, /**< Driver is idle and ready to accept a task. */
    DRIVER_BUSY /**< Driver is currently processing a task. */
} DriverState;

/**
 * @struct DriverSlot
 * @brief Describes a single driver entry stored in shared memory.
 */
typedef struct
{
    DriverState state; /**< Current driver state. */
    pthread_mutex_t mutex;  /**< Process-shared mutex for synchronization. */

    pid_t pid; /**< Process identifier of the driver. */
    int active;  /**< Indicates whether the slot is occupied. */

    int task_timer; /**< Task execution time in seconds. */
    time_t busy_until; /**< Timestamp when the current task finishes. */

    int event_fd; /**< Event file descriptor used for notifications. */
} DriverSlot;

/**
 * @brief Pointer to the shared memory array containing driver slots.
 */
DriverSlot *shared_ds = NULL;

/**
 * @brief File descriptor of the shared memory object.
 */
int shm_fd = -1;

/**
 * @brief Safely converts a string to a long integer.
 *
 * The function validates the input string, checks for conversion errors,
 * and ensures that the resulting value belongs to the specified range.
 *
 * @param str Input string to convert.
 * @param result Pointer to the variable that will receive the converted value.
 * @param min_val Minimum allowed value.
 * @param max_val Maximum allowed value.
 *
 * @return Returns 0 on success and -1 on failure.
 */
int safe_strtol(const char *str, long *result, long min_val, long max_val)
{
    if (!str || !result)
        return -1;
    

    char *endptr = NULL;
    errno = 0;
    
    long val = strtol(str, &endptr, 10);

    if (errno != 0)
        return -1;
    

    if (endptr == str)
        return -1;
    

    if (val < min_val || val > max_val)
        return -1;

    *result = val;
    return 0;
}

/**
 * @brief Performs a safe read operation from a file descriptor.
 *
 * Wraps the read() system call and handles common error conditions,
 * including interruption by a signal (EINTR).
 *
 * @param fd File descriptor to read from.
 * @param buf Destination buffer.
 * @param count Number of bytes to read.
 *
 * @return Returns 0 on success and -1 on error.
 */
int safe_read(int fd, void *buf, size_t count)
{
    if (fd < 0 || !buf)
        return -1;

    ssize_t result = read(fd, buf, count);
    
    if (result < 0)
    {
        if (errno == EINTR)
            return 0;
        
        perror("read");
        return -1;
    }

    return 0;
}

/**
 * @brief Performs a safe write operation to a file descriptor.
 *
 * Wraps the write() system call and verifies that all requested bytes
 * have been successfully written.
 *
 * @param fd File descriptor to write to.
 * @param buf Source buffer.
 * @param count Number of bytes to write.
 *
 * @return Returns 0 on success and -1 on error.
 */
int safe_write(int fd, const void *buf, size_t count)
{
    if (fd < 0 || !buf)
        return -1;

    ssize_t result = write(fd, buf, count);
    
    if (result < 0)
    {
        if (errno == EINTR)
            return 0;
        
        perror("write");
        return -1;
    }

    if ((size_t)result != count)
        return -1;

    return 0;
}

/**
 * @brief Finds a driver slot by process identifier.
 *
 * Searches the shared memory table for an active driver with the
 * specified PID.
 *
 * @param pid Driver process identifier.
 *
 * @return Index of the corresponding slot, or -1 if not found.
 */
int find_slot(pid_t pid)
{
    if (pid <= 0)
        return -1;

    for (int i = 0; i < MAX_DRIVERS; i++)
    {
        if (shared_ds[i].active && shared_ds[i].pid == pid)
            return i;
    }

    return -1;
}

/**
 * @brief Main event-processing loop of a driver process.
 *
 * The driver waits for incoming task notifications through eventfd
 * using epoll. Once a task is received, a timerfd object is configured
 * to track task completion time. When the timer expires, the driver
 * becomes available again.
 *
 * @param efd Event file descriptor used to receive task notifications.
 */
void driver_loop(int efd)
{
    pid_t pid = getpid();
    int idx = find_slot(pid);
    
    if (idx == -1)
    {
        exit(EXIT_FAILURE);
    }
    
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
        close(epfd);
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev;

    ev.events = EPOLLIN;
    ev.data.fd = efd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, efd, &ev) == -1)
    {
        perror("epoll_ctl (efd)");
        close(tfd);
        close(epfd);
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = tfd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, tfd, &ev) == -1)
    {
        perror("epoll_ctl (tfd)");
        close(tfd);
        close(epfd);
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        struct epoll_event events[2];
        int n = epoll_wait(epfd, events, 2, -1);

        if (n == -1)
        {
            if (errno == EINTR)
                continue;
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; i++)
        {
            if (events[i].data.fd == efd)
            {
                uint64_t value;

                if (safe_read(efd, &value, sizeof(value)) < 0)
                    continue;

                pthread_mutex_lock(&shared_ds[idx].mutex);

                if (shared_ds[idx].busy_until > time(NULL))
                {
                    pthread_mutex_unlock(&shared_ds[idx].mutex);
                    continue;
                }

                if (shared_ds[idx].task_timer <= 0)
                {
                    pthread_mutex_unlock(&shared_ds[idx].mutex);
                    continue;
                }

                shared_ds[idx].state = DRIVER_BUSY;
                int task_time = shared_ds[idx].task_timer;
                shared_ds[idx].busy_until = time(NULL) + task_time;

                pthread_mutex_unlock(&shared_ds[idx].mutex);

                struct itimerspec spec;
                memset(&spec, 0, sizeof(spec));
                spec.it_value.tv_sec = task_time;

                if (timerfd_settime(tfd, 0, &spec, NULL) == -1)
                {
                    perror("timerfd_settime");
                    continue;
                }
            }
            else if (events[i].data.fd == tfd)
            {
                uint64_t expirations;

                if (safe_read(tfd, &expirations, sizeof(expirations)) < 0)
                    continue;

                pthread_mutex_lock(&shared_ds[idx].mutex);
                shared_ds[idx].state = DRIVER_AVAILABLE;
                shared_ds[idx].busy_until = 0;
                pthread_mutex_unlock(&shared_ds[idx].mutex);
                
                printf("[Driver %d] Task completed!\n", pid);
            }
        }
    }

    close(tfd);
    close(epfd);
    exit(EXIT_SUCCESS);
}

/**
 * @brief Initializes the shared memory region.
 *
 * Creates and maps the shared memory object, initializes all driver
 * slots, and creates process-shared mutexes for synchronization.
 */
void init_shm()
{
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd < 0)
    {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, sizeof(DriverSlot) * MAX_DRIVERS) == -1)
    {
        perror("ftruncate");
        close(shm_fd);
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    shared_ds = mmap(NULL, sizeof(DriverSlot) * MAX_DRIVERS, 
                     PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (shared_ds == MAP_FAILED)
    {
        perror("mmap");
        close(shm_fd);
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

    for (int i = 0; i < MAX_DRIVERS; i++)
    {
        if (pthread_mutex_init(&shared_ds[i].mutex, &attr) != 0)
        {
            fprintf(stderr, "Error: failed to initialize mutex %d\n", i);
            pthread_mutexattr_destroy(&attr);
            munmap(shared_ds, sizeof(DriverSlot) * MAX_DRIVERS);
            close(shm_fd);
            shm_unlink(SHM_NAME);
            exit(EXIT_FAILURE);
        }

        shared_ds[i].active = 0;
        shared_ds[i].pid = 0;
        shared_ds[i].busy_until = 0;
        shared_ds[i].task_timer = 0;
        shared_ds[i].event_fd = -1;
        shared_ds[i].state = DRIVER_AVAILABLE;
    }

    pthread_mutexattr_destroy(&attr);
}

/**
 * @brief Creates a new driver process.
 *
 * Allocates a free driver slot, creates an eventfd object,
 * forks a child process, and initializes the corresponding
 * shared memory entry.
 */
void create_driver()
{
    int free_slot = -1;
    for (int i = 0; i < MAX_DRIVERS; i++)
    {
        if (!shared_ds[i].active)
        {
            free_slot = i;
            break;
        }
    }

    if (free_slot == -1)
    {
        fprintf(stderr, "Error: no free driver slots available\n");
        return;
    }

    int efd = eventfd(0, EFD_CLOEXEC);
    if (efd == -1)
    {
        perror("eventfd");
        return;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork");
        close(efd);
        return;
    }

    if (pid == 0)
    {
        driver_loop(efd);
        exit(EXIT_SUCCESS);
    }

    shared_ds[free_slot].pid = pid;
    shared_ds[free_slot].active = 1;
    shared_ds[free_slot].busy_until = 0;
    shared_ds[free_slot].task_timer = 0;
    shared_ds[free_slot].event_fd = efd;
    shared_ds[free_slot].state = DRIVER_AVAILABLE;

    printf("Driver created with PID=%d\n", pid);
}

/**
 * @brief Assigns a task to a driver.
 *
 * Stores the task duration in shared memory and notifies the
 * target driver through its eventfd descriptor.
 *
 * @param pid Process identifier of the target driver.
 * @param task_time Task duration in seconds.
 */
void send_task(pid_t pid, int task_time)
{
    int idx = find_slot(pid);
    if (idx == -1)
    {
        printf("Driver not found\n");
        return;
    }

    if (task_time < MIN_TASK_TIME || task_time > MAX_TASK_TIME)
        return;

    if (shared_ds[idx].event_fd < 0)
        return;

    if (shared_ds[idx].state == DRIVER_BUSY)
    {
        printf("Driver is busy!\n");
        return;
    }

    pthread_mutex_lock(&shared_ds[idx].mutex);
    shared_ds[idx].task_timer = task_time;
    pthread_mutex_unlock(&shared_ds[idx].mutex);

    uint64_t value = 1;
    if (safe_write(shared_ds[idx].event_fd, &value, sizeof(value)) < 0)
        return;

    printf("Task sent to PID=%d for %d sec\n", pid, task_time);
}

/**
 * @brief Displays the current status of a driver.
 *
 * If the driver is busy, the remaining execution time of the
 * current task is also displayed.
 *
 * @param pid Process identifier of the driver.
 */
void get_status(pid_t pid)
{
    int idx = find_slot(pid);
    if (idx == -1)
    {
        printf("Driver not found\n");
        return;
    }

    time_t now = time(NULL);

    pthread_mutex_lock(&shared_ds[idx].mutex);
    
    if (shared_ds[idx].state == DRIVER_BUSY && shared_ds[idx].busy_until > now)
    {
        time_t remaining = shared_ds[idx].busy_until - now;
        printf("Status [PID=%d]: BUSY (%ld sec remaining)\n", pid, remaining);
    }
    else
    {
        printf("Status [PID=%d]: AVAILABLE\n", pid);
    }
    
    pthread_mutex_unlock(&shared_ds[idx].mutex);
}

/**
 * @brief Displays information about all active drivers.
 *
 * Prints the PID and current state of every active driver.
 * If a driver is busy, the remaining execution time is shown.
 */
void get_drivers()
{
    time_t now = time(NULL);
    int found_any = 0;
    
    for (int i = 0; i < MAX_DRIVERS; i++)
    {
        if (shared_ds[i].active)
        {
            found_any = 1;
            pthread_mutex_lock(&shared_ds[i].mutex);
            
            if (shared_ds[i].busy_until > now)
            {
                time_t remaining = shared_ds[i].busy_until - now;
                printf("PID=%d | Status: BUSY | Time left: %ld sec\n", 
                       shared_ds[i].pid, remaining);
            }
            else
            {
                printf("PID=%d | Status: AVAILABLE\n", shared_ds[i].pid);
            }
            
            pthread_mutex_unlock(&shared_ds[i].mutex);
        }
    }

    if (!found_any)
        printf("No active drivers\n");
}

/**
 * @brief Terminates all driver processes and releases allocated resources.
 *
 * Closes file descriptors, terminates child processes, waits for their
 * completion, destroys synchronization primitives, unmaps shared memory,
 * and removes the shared memory object.
 */
void terminate_all()
{
    printf("\nTerminating all drivers...\n");

    for (int i = 0; i < MAX_DRIVERS; i++)
    {
        if (shared_ds[i].active)
        {
            if (shared_ds[i].event_fd >= 0)
            {
                close(shared_ds[i].event_fd);
                shared_ds[i].event_fd = -1;
            }

            if (shared_ds[i].pid > 0)
            {
                if (kill(shared_ds[i].pid, SIGTERM) == -1)
                {
                    if (errno != ESRCH)
                        perror("kill");
                }
            }
        }
    }
    
    for (int i = 0; i < MAX_DRIVERS; i++)
    {
        if (shared_ds[i].active && shared_ds[i].pid > 0)
        {
            int status;
            pid_t result = waitpid(shared_ds[i].pid, &status, 0);
            
            if (result == -1 && errno != ECHILD)
            {
                perror("waitpid");
            }
            
            shared_ds[i].active = 0;
        }
    }

    if (shared_ds != NULL && shared_ds != MAP_FAILED)
    {
        for (int i = 0; i < MAX_DRIVERS; i++)
        {
            pthread_mutex_destroy(&shared_ds[i].mutex);
        }
        
        if (munmap(shared_ds, sizeof(DriverSlot) * MAX_DRIVERS) == -1)
        {
            perror("munmap");
        }
        shared_ds = NULL;
    }

    if (shm_fd >= 0)
    {
        close(shm_fd);
        shm_fd = -1;
    }

    if (shm_unlink(SHM_NAME) == -1)
        if (errno != ENOENT)
            perror("shm_unlink");
    

    printf("Cleanup completed\n");
    exit(EXIT_SUCCESS);
}

/**
 * @brief Parses and processes a user command.
 *
 * Recognizes supported console commands and dispatches them
 * to the appropriate handler functions.
 *
 * Supported commands:
 * - create_driver (crt)
 * - send_task (sndk)
 * - get_status (gs)
 * - get_drivers (gd)
 * - help (h or ?)
 * - exit (ex or q)
 *
 * @param line Null-terminated string containing the user input.
 */
void process_command(char *line)
{
    if (!line)
        return;

    char line_copy[MAX_LINE];
    strncpy(line_copy, line, MAX_LINE - 1);
    line_copy[MAX_LINE - 1] = '\0';

    char *cmd = strtok(line_copy, " \t");

    if (!cmd || strlen(cmd) == 0)
        return;

    if (strcmp(cmd, "create_driver") == 0 || strcmp(cmd, "crt") == 0)
    {
        create_driver();
    }
    else if (strcmp(cmd, "send_task") == 0 || strcmp(cmd, "sndk") == 0)
    {
        char *pid_str = strtok(NULL, " \t");
        char *time_str = strtok(NULL, " \t");

        if (!pid_str)
        {
            printf("Usage: send_task (or sndk) <pid> <time_seconds>\n");
            return;
        }

        if (!time_str)
        {
            printf("Usage: send_task (or sndk) <pid> <time_seconds>\n");
            return;
        }

        long pid_val;
        if (safe_strtol(pid_str, &pid_val, 1, INT_MAX) < 0)
        {
            printf("Usage: send_task (or sndk) <pid> <time_seconds>\n");
            return;
        }

        long time_val;
        if (safe_strtol(time_str, &time_val, MIN_TASK_TIME, MAX_TASK_TIME) < 0)
        {
            printf("Usage: send_task (or sndk) <pid> <time_seconds>\n");
            return;
        }

        send_task((pid_t)pid_val, (int)time_val);
    }
    else if (strcmp(cmd, "get_status") == 0 || strcmp(cmd, "gs") == 0)
    {
        char *pid_str = strtok(NULL, " \t");

        if (!pid_str)
        {
            printf("Usage: get_status (or gs) <pid>\n");
            return;
        }

        long pid_val;
        if (safe_strtol(pid_str, &pid_val, 1, INT_MAX) < 0)
        {
            printf("Usage: get_status (or gs) <pid>\n");
            return;
        }

        get_status((pid_t)pid_val);
    }
    else if (strcmp(cmd, "get_drivers") == 0 || strcmp(cmd, "gd") == 0)
    {
        get_drivers();
    }
    else if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "ex") == 0 || strcmp(cmd, "quit") == 0 || strcmp(cmd, "q") == 0)
    {
        terminate_all();
    }
    else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "h") == 0 || strcmp(cmd, "?") == 0)
    {
        printf("create_driver (crt)           - Create a new driver\n");
        printf("send_task (sndk) <pid> <sec>  - Send task to driver (1-3600 sec)\n");
        printf("get_status (gs) <pid>         - Get driver status\n");
        printf("get_drivers (gd)              - List all active drivers\n");
        printf("exit(ex or q)                 - Terminate all drivers and exit\n");
        printf("help (h, ?)                   - Show this help message\n");
    }
    else
    {
        printf("Type 'help' for available commands\n");
    }
}

/**
 * @brief Reads a command line from standard input.
 *
 * Uses fgets() to safely read a line, handles input errors,
 * and removes the trailing newline character if present.
 *
 * @param buffer Destination buffer.
 * @param size Size of the destination buffer in bytes.
 *
 * @return Returns 0 on success and -1 on error or end-of-file.
 */
int read_command_line(char *buffer, size_t size)
{
    if (!buffer || size == 0)
        return -1;

    if (fgets(buffer, size, stdin) == NULL)
    {
        if (feof(stdin))
            return -1;
        
        if (ferror(stdin))
        {
            perror("fgets");
            clearerr(stdin);
            return -1;
        }
    }

    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n')
    {
        buffer[len - 1] = '\0';
    }

    return 0;
}

/**
 * @brief Program entry point.
 *
 * Initializes shared memory resources, enters the interactive
 * command-processing loop, and performs cleanup before exiting.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line argument strings.
 *
 * @return Returns EXIT_SUCCESS on normal program termination.
 */
int main(int argc, char *argv[])
{
    init_shm();

    char line[MAX_LINE];

    printf("Type 'help' for available commands\n");

    while (1)
    {
        printf("\n> ");
        fflush(stdout);

        if (read_command_line(line, sizeof(line)) < 0)
        {
            printf("\n");
            break;
        }

        size_t i = 0;
        int is_empty = 1;
        while (line[i] != '\0')
        {
            if (line[i] != ' ' && line[i] != '\t')
            {
                is_empty = 0;
                break;
            }
            i++;
        }

        if (is_empty)
            continue;

        process_command(line);
    }

    terminate_all();
    return EXIT_SUCCESS;
}