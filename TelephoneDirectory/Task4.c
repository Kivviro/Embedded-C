#include <stdio.h>

#define CUSTOMER_ARRAY_LENGTH 100

// Start "functions for work with strings" area --
void ReadString(char *buf, int size)
{
    int c;
    int i = 0;

    while ((c = getchar()) == '\n');

    if (c == EOF)
    {
        buf[0] = '\0';
        return;
    }

    buf[i++] = (char)c;

    while (i < size - 1 && (c = getchar()) != '\n' && c != EOF)
        buf[i++] = (char)c;


    buf[i] = '\0';

    if (c != '\n' && c != EOF)
        while ((c = getchar()) != '\n' && c != EOF);
}

int IsDigitsOnly(const char *s)
{
    int i = 0;

    while (s[i] != '\0')
    {
        if (s[i] < '0' || s[i] > '9')
            return 0;
        i++;
    }

    return 1;
}

int SubstringFounder(const char *str, const char *substr)
{
    if (!substr[0])
        return 1;

    for (int i = 0; str[i] != '\0'; i++)
    {
        int j = 0;
        while (substr[j] != '\0' && str[i + j] != '\0' && str[i + j] == substr[j])
            j++;

        if (substr[j] == '\0')
            return 1;
    }
    return 0;
}
// -- end "functions for work with strings" area

struct customer
{
    char name[20];
    char secondName[20];
    char tel[11];
};

void PrintCustomer(int index, struct customer *cptr)
{
    if (cptr->name[0] != '\0')
        printf("%d. %s %s %s\n", index + 1, cptr->name, cptr->secondName, cptr->tel);
}

void AddCustomer(struct customer _customers[]) // Ok
{
    for (int i = 0; i < CUSTOMER_ARRAY_LENGTH; i++)
    {
        if (_customers[i].name[0] == '\0')
        {
            printf("Введите имя: ");
            ReadString(_customers[i].name, sizeof(_customers[i].name));

            printf("Введите фамилию: ");
            ReadString(_customers[i].secondName, sizeof(_customers[i].secondName));

            do
            {
                printf("Введите телефон: ");
                ReadString(_customers[i].tel, sizeof(_customers[i].tel));
                if (!IsDigitsOnly(_customers[i].tel)) printf("Номер должен содержать только цифры.\n");
            }
            while (!IsDigitsOnly(_customers[i].tel));

            printf("Запись добавлена в позицию %d.\n", i + 1);
            return;
        }
    }

    printf("Справочник заполнен.\n");
}

void DeleteCustomer(struct customer _customers[]) // Ok
{
    int n;
    printf("Укажите позицию в списке, которую хотите очистить: ");
    char line[16];
    ReadString(line, sizeof(line));
    if (sscanf(line, "%d", &n) != 1)
    {
        printf("Некорректный ввод.\n");
        return;
    }

    if (n < 1 || n > CUSTOMER_ARRAY_LENGTH)
    {
        printf("Неверная позиция.\n");
        return;
    }

    int i = n - 1;
    _customers[i].name[0] = '\0';
    _customers[i].secondName[0] = '\0';
    _customers[i].tel[0] = '\0';

    printf("Запись %d удалена.\n", n);
}

void SearchCustomers(struct customer _customers[]) // OK
{
    char query[20];

    printf("Введите текст для поиска: ");
    ReadString(query, sizeof(query));

    printf("\nРезультаты поиска:\n");

    int found = 0;
    for (int i = 0; i < CUSTOMER_ARRAY_LENGTH; i++)
    {
        if (_customers[i].name[0] == '\0')
            continue;

        if (SubstringFounder(_customers[i].name, query) || SubstringFounder(_customers[i].secondName, query) || SubstringFounder(_customers[i].tel, query))
        {
            PrintCustomer(i, &_customers[i]);
            found = 1;
        }
    }

    if (!found)
        printf("Совпадений не найдено.\n");
}

void OutputAllCustomers(struct customer _customers[])
{
    printf("\nСписок записей:\n");
    for (int i = 0; i < CUSTOMER_ARRAY_LENGTH; i++)
        PrintCustomer(i, &_customers[i]);
}

int main()
{
    int choice;
    struct customer customers[CUSTOMER_ARRAY_LENGTH] = {0};

    while (1)
    {
        printf("\nМеню:\n");
        printf("1. Добавить абонента\n");
        printf("2. Удалить абонента\n");
        printf("3. Поиск абонента по ключевым символам\n");
        printf("4. Вывод всех записей\n");
        printf("5. Выход\n");
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
                AddCustomer(customers);
                break;
            case 2:
                DeleteCustomer(customers);
                break;
            case 3:
                SearchCustomers(customers);
                break;
            case 4:
                OutputAllCustomers(customers);
                break;
            case 5:
                return 0;

            default:
                printf("Недоступный выбор.\n");
        }
    }
}