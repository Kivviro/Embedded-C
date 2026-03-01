#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define QUERY_FOR_SEARCH_INPUT_ARRAY 20

// structure for implementation via a doubly linked list (pointers to the next and previous elements)
struct customer
{
    char name[20];
    char secondName[20];
    char tel[11];

    struct customer *prev;
    struct customer *next;
};

/* Prototypes */
void PrintCustomer(int index, struct customer *cptr);
void AddCustomer(struct customer **head);
void DeleteCustomer(struct customer **head);
void SearchCustomers(struct customer *head);
void OutputAllCustomers(struct customer *head);

int IsDigitsOnly(const char *s);
void ReadString(char *buf, size_t size);

int main(void)
{
    int choice;
    struct customer *head = NULL;

    while (1)
    {
        printf("\nМеню:\n");
        printf("1. Добавить абонента\n");
        printf("2. Удалить абонента\n");
        printf("3. Поиск абонента по ключевым символам\n");
        printf("4. Вывод всех записей\n");
        printf("0. Выход\n");
        printf("Выбор: ");

        char line[16];
        ReadString(line, sizeof(line));

        if (sscanf(line, "%d", &choice) != 1)
        {
            printf("Некорректный ввод.\n");
            continue;
        }

        switch (choice)
        {
            case 1:
                AddCustomer(&head);
                break;
            case 2:
                DeleteCustomer(&head);
                break;
            case 3:
                SearchCustomers(head);
                break;
            case 4:
                OutputAllCustomers(head);
                break;
            case 0: // Exit the program with memory cleanup
                while (head)
                {
                    struct customer *tmp = head;
                    head = head->next;
                    free(tmp);
                }
                return 0;
            default:
                printf("Недоступный выбор.\n");
        }
    }
}

void PrintCustomer(int index, struct customer *cptr)
{
    if (cptr->name[0] != '\0')
        printf("%d. %s %s %s\n", index + 1, cptr->name, cptr->secondName, cptr->tel);
}

// Adding customer to the dictionary using a doubly linked list
void AddCustomer(struct customer **head)
{
    struct customer *newNode = malloc(sizeof(struct customer));
    if (newNode == NULL)
    {
        fprintf(stderr, "malloc failed!\n");
        return;
    }

    printf("Введите имя: ");
    ReadString(newNode->name, sizeof(newNode->name));

    printf("Введите фамилию: ");
    ReadString(newNode->secondName, sizeof(newNode->secondName));

    do
    {
        printf("Введите телефон: ");
        ReadString(newNode->tel, sizeof(newNode->tel));

        if (!IsDigitsOnly(newNode->tel))
            printf("Номер должен содержать только цифры.\n");

    }
    while (!IsDigitsOnly(newNode->tel));

    newNode->next = NULL;
    newNode->prev = NULL;

    if (*head == NULL)
    {
        *head = newNode;
        return;
    }

    struct customer *currentNode = *head;
    while (currentNode->next)
        currentNode = currentNode->next;

    currentNode->next = newNode;
    newNode->prev = currentNode;
}

// Delete customer ... but now we have a doubly linked list :D
void DeleteCustomer(struct customer **head)
{
    int n;
    char line[16];

    printf("Укажите позицию в списке, которую хотите очистить: ");
    ReadString(line, sizeof(line));

    if (sscanf(line, "%d", &n) != 1 || n < 1)
    {
        printf("Некорректный ввод.\n");
        return;
    }

    struct customer *current = *head;
    for (int i = 1; current && i < n; i++)
        current = current->next;

    if (!current)
    {
        printf("Неверная позиция.\n");
        return;
    }

    if (current->prev)
        current->prev->next = current->next;
    else
        *head = current->next;

    if (current->next)
        current->next->prev = current->prev;

    free(current);
    printf("Запись %d удалена.\n", n);
}

// Search customers... but now we have a doubly linked list :D
void SearchCustomers(struct customer *head)
{
    char query[QUERY_FOR_SEARCH_INPUT_ARRAY];
    int found = 0;

    printf("Введите текст для поиска: ");
    ReadString(query, sizeof(query));

    int index = 1;

    while (head)
    {
        if (strstr(head->name, query) || strstr(head->secondName, query)
            || strstr(head->tel, query))
        {
            printf("%d. %s %s %s\n", index, head->name, head->secondName, head->tel);
            found = 1;
        }
        head = head->next;
        index++;
    }

    if (!found)
        printf("Совпадений не найдено.\n");
}

// List output
void OutputAllCustomers(struct customer *head)
{
    int index = 1;
    while (head)
    {
        printf("%d. %s %s %s\n", index++, head->name, head->secondName, head->tel);
        head = head->next;
    }
}

/* For strings */
void ReadString(char *buf, size_t size)
{
    if (fgets(buf, size, stdin) == NULL)
    {
        buf[0] = '\0';
        return;
    }

    buf[strcspn(buf, "\n")] = '\0';
}

int IsDigitsOnly(const char *s)
{
    if (*s == '\0')
        return 0;

    while (*s)
    {
        if (!isdigit((unsigned char)*s))
            return 0;
        s++;
    }
    return 1;
}