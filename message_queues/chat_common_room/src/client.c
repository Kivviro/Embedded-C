// client.c
// Компиляция: gcc -o client client.c -lrt -pthread
#define _POSIX_C_SOURCE 200809L
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>

#define SERVER_QUEUE_NAME   "/server_queue"
#define MAX_MSG_SIZE        1024
#define MAX_MESSAGES        10
#define CLIENT_NAME_LEN     64

static volatile sig_atomic_t stop_requested = 0;

static void sigint_handler(int signo) 
{
    (void)signo;
    stop_requested = 1;
}

typedef struct 
{
    mqd_t client_mqd;
} reader_arg_t;

static void *reader_thread(void *arg) 
{
    reader_arg_t *ra = (reader_arg_t *)arg;
    char buf[MAX_MSG_SIZE + CLIENT_NAME_LEN + 8];
    unsigned int prio;
    while (!stop_requested) 
    {
        ssize_t r = mq_receive(ra->client_mqd, buf, MAX_MSG_SIZE + CLIENT_NAME_LEN + 8, &prio);
        if (r >= 0) 
        {
            buf[r] = '\0';
            printf("[IN] %s\n", buf);
        } 
        else 
        {
            if (errno == EINTR) continue;
            // При EAGAIN — можно подождать. Здесь блокируемся, поэтому другие ошибки печатаем и завершаем цикл.
            perror("client mq_receive");
            break;
        }
    }
    return NULL;
}

int main(void) 
{
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    pid_t pid = getpid();
    char client_qname[CLIENT_NAME_LEN];
    snprintf(client_qname, sizeof(client_qname), "/client_%d", pid);

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    // Создаём собственную очередь для получения сообщений от сервера
    mqd_t client_mqd = mq_open(client_qname, O_CREAT | O_RDONLY, 0666, &attr);
    if (client_mqd == (mqd_t)-1) 
    {
        perror("mq_open(client)");
        exit(EXIT_FAILURE);
    }

    // Открываем серверную очередь для записи
    mqd_t server_mqd = mq_open(SERVER_QUEUE_NAME, O_WRONLY);
    if (server_mqd == (mqd_t)-1) 
    {
        perror("mq_open(server)");
        mq_close(client_mqd);
        mq_unlink(client_qname);
        exit(EXIT_FAILURE);
    }

    // Запускаем поток чтения входящих сообщений
    pthread_t tid;
    reader_arg_t ra = { .client_mqd = client_mqd };
    if (pthread_create(&tid, NULL, reader_thread, &ra) != 0) 
    {
        perror("pthread_create");
        mq_close(client_mqd);
        mq_unlink(client_qname);
        mq_close(server_mqd);
        exit(EXIT_FAILURE);
    }

    // Отправляем JOIN
    char join_buf[128];
    snprintf(join_buf, sizeof(join_buf), "JOIN:%d", (int)pid);
    if (mq_send(server_mqd, join_buf, strlen(join_buf) + 1, 0) == -1) 
    {
        perror("mq_send JOIN");
    }

    // Основной цикл: читаем stdin и отправляем сообщения
    char line[MAX_MSG_SIZE];
    while (!stop_requested && fgets(line, sizeof(line), stdin) != NULL) 
    {
        // Убираем перевод строки
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') line[len-1] = '\0';

        if (strcmp(line, "/quit") == 0) 
        {
            stop_requested = 1;
            break;
        }

        // Формируем MSG:<pid>:<text>
        char out[MAX_MSG_SIZE + 64];
        snprintf(out, sizeof(out), "MSG:%d:%s", (int)pid, line);
        if (mq_send(server_mqd, out, strlen(out) + 1, 0) == -1) 
        {
            perror("mq_send MSG");
        }
    }

    // Отправляем LEAVE
    char leave_buf[128];
    snprintf(leave_buf, sizeof(leave_buf), "LEAVE:%d", (int)pid);
    if (mq_send(server_mqd, leave_buf, strlen(leave_buf) + 1, 0) == -1) 
    {
        // возможно сервер уже завершился
    }

    // Завершение: просим поток чтения закончить
    stop_requested = 1;
    pthread_kill(tid, SIGTERM); // попытка разблокировать mq_receive (может не помочь)
    pthread_join(tid, NULL);

    mq_close(client_mqd);
    mq_unlink(client_qname);
    mq_close(server_mqd);

    printf("Client %d exiting\n", (int)pid);
    return 0;
}