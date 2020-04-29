#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "constants.h"

// Инициализация файловой системы
void 
client_init_minifs(int conn_fd) 
{
    TypeOperation type = INIT;

    // Отправляем серверу всю необходимую информацию для выполнения команды
    send(conn_fd, &type, sizeof(TypeOperation), MSG_CONFIRM);

    // Могла произойти ошибка, поэтому надо проверить, что серверу еще есть, что нам сказать
    size_t content_length = 0;
    recv(conn_fd, &content_length, sizeof(size_t), MSG_WAITALL);
    if(content_length > 0)
    {
        char* content = malloc(content_length);
        recv(conn_fd, content, content_length, MSG_WAITALL);

        printf("%.*s", (int)content_length, content);
        free(content);
    }
}

// Создание файла/директории
void 
client_create_item(char node_type, char* path, int conn_fd)
{
    TypeOperation type = CREATE;
    size_t path_size = strlen(path) + 1;

    // Отправляем серверу всю необходимую информацию для выполнения команды
    send(conn_fd, &type, sizeof(TypeOperation), MSG_CONFIRM);
    send(conn_fd, &node_type, sizeof(char), MSG_CONFIRM);
    send(conn_fd, &path_size, sizeof(size_t), MSG_CONFIRM);
    send(conn_fd, path, path_size, MSG_CONFIRM);

    // Могла произойти ошибка, поэтому надо проверить, что серверу еще есть, что нам сказать
    size_t content_length = 0;
    recv(conn_fd, &content_length, sizeof(size_t), MSG_WAITALL);
    if(content_length > 0)
    {
        char* content = malloc(content_length);
        recv(conn_fd, content, content_length, MSG_WAITALL);

        printf("%.*s", (int)content_length, content);
        free(content);
    }
}

// Удаление файла/директории
void 
client_delete_item(char node_type, char* path, int conn_fd)
{
    TypeOperation type = DELETE;
    size_t path_size = strlen(path) + 1;

    // Отправляем серверу всю необходимую информацию для выполнения команды
    send(conn_fd, &type, sizeof(TypeOperation), MSG_CONFIRM);
    send(conn_fd, &node_type, sizeof(char), MSG_CONFIRM);
    send(conn_fd, &path_size, sizeof(size_t), MSG_CONFIRM);
    send(conn_fd, path, path_size, MSG_CONFIRM);

    // Могла произойти ошибка, поэтому надо проверить, что серверу еще есть, что нам сказать
    size_t content_length = 0;
    recv(conn_fd, &content_length, sizeof(size_t), MSG_WAITALL);
    if(content_length > 0)
    {
        char* content = malloc(content_length);
        recv(conn_fd, content, content_length, MSG_WAITALL);

        printf("%.*s", (int)content_length, content);
        free(content);
    }
}

// Операция чтения
void 
client_read_item(char node_type, char* path, int conn_fd)
{
    TypeOperation type = READ;
    size_t path_size = strlen(path) + 1;

    // Отправляем серверу всю необходимую информацию для выполнения команды
    send(conn_fd, &type, sizeof(TypeOperation), MSG_CONFIRM);
    send(conn_fd, &node_type, sizeof(char), MSG_CONFIRM);
    send(conn_fd, &path_size, sizeof(size_t), MSG_CONFIRM);
    send(conn_fd, path, path_size, MSG_CONFIRM);

    // Читаем ответ сервера
    size_t content_length = 0;
    recv(conn_fd, &content_length, sizeof(size_t), MSG_WAITALL);
    char* content = malloc(content_length);
    recv(conn_fd, content, content_length, MSG_WAITALL);

    fwrite(content, content_length, 1, stdout);
    free(content);
}

// Операция записи
void 
client_write_item(char* path, int conn_fd)
{
    TypeOperation type = WRITE;
    size_t path_size = strlen(path) + 1;


    // Отправляем серверу всю необходимую информацию для выполнения команды
    send(conn_fd, &type, sizeof(TypeOperation), MSG_CONFIRM);
    send(conn_fd, &path_size, sizeof(size_t), MSG_CONFIRM);
    send(conn_fd, path, path_size, MSG_CONFIRM);

    // Теперь надо отправлять информацию в ФС блоками, пока на stdin что-то остается
    // Как только буффер станет пустым, сервер получит, что размер буффера равен нулю и начнет обработку
    char buffer[BLOCK_SIZE];
    size_t fs_size = CONTROL_SIZE + NUM_BLOCKS * BLOCK_SIZE;
    size_t total_sent = 0;
    size_t buffer_size = fread(buffer, 1, BLOCK_SIZE, stdin);
    send(conn_fd, &buffer_size, sizeof(size_t), MSG_CONFIRM);
    while (buffer_size > 0)
    {
        send(conn_fd, buffer, buffer_size, MSG_CONFIRM);
        total_sent += buffer_size;
        buffer_size = fread(buffer, 1, BLOCK_SIZE, stdin);

        // Если вдруг мы отправляем что-то слишком огромное для нашей файловой системы, то прекратим это
        if (total_sent >= fs_size)
        {
            buffer_size = 0;
        }
        send(conn_fd, &buffer_size, sizeof(size_t), MSG_CONFIRM);
    }

    // Могла произойти ошибка, поэтому надо проверить, что серверу еще есть, что нам сказать
    size_t content_length = 0;
    recv(conn_fd, &content_length, sizeof(size_t), MSG_WAITALL);
    if(content_length > 0)
    {
        char* content = malloc(content_length);
        recv(conn_fd, content, content_length, MSG_WAITALL);

        printf("%.*s", (int)content_length, content);
        free(content);
    }
}
