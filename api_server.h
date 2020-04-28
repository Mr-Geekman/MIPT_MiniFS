#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "constants.h"
#include "api_fs.h"

// Инициализация файловой системы
void 
server_init_minifs() 
{
    init_minifs();
}

// Создание файла/директории
void 
server_create(int conn_fd, struct Inode* inodes, struct SuperBlock* super_block_ptr, struct FreeMap* free_map_ptr, FILE* fs)
{
    // Принимаем от клиента все необходимые данные
    char node_type;
    size_t path_size;
    recv(conn_fd, &node_type, sizeof(char), MSG_WAITALL);
    recv(conn_fd, &path_size, sizeof(size_t), MSG_WAITALL);
    char* path = malloc(path_size);
    recv(conn_fd, path, path_size, MSG_WAITALL);

    // Выполняем операцию
    create_item(node_type, path, inodes, super_block_ptr, free_map_ptr, fs);
}

// Удаление файла/директории
void 
server_delete(int conn_fd, struct Inode* inodes, struct SuperBlock* super_block_ptr, struct FreeMap* free_map_ptr, FILE* fs)
{
    // Принимаем от клиента все необходимые данные
    char node_type;
    size_t path_size;
    recv(conn_fd, &node_type, sizeof(char), MSG_WAITALL);
    recv(conn_fd, &path_size, sizeof(size_t), MSG_WAITALL);
    char* path = malloc(path_size);
    recv(conn_fd, path, path_size, MSG_WAITALL);

    // Выполняем операцию
    delete_item(node_type, path, inodes, super_block_ptr, free_map_ptr, fs);
}

// Операция чтения
void 
server_read(int conn_fd, struct Inode* inodes, FILE* fs)
{
    // Принимаем от клиента все необходимые данные
    char node_type;
    size_t path_size;
    recv(conn_fd, &node_type, sizeof(char), MSG_WAITALL);
    recv(conn_fd, &path_size, sizeof(size_t), MSG_WAITALL);
    char* path = malloc(path_size);
    recv(conn_fd, path, path_size, MSG_WAITALL);

    // Выполняем операцию
    read_item(node_type, path, inodes, fs);
}

// Операция записи
void 
server_write(int conn_fd, struct Inode* inodes, struct SuperBlock* super_block_ptr, struct FreeMap* free_map_ptr, FILE* fs)
{
    // Принимаем от клиента все необходимые данные
    char node_type;
    size_t path_size;
    recv(conn_fd, &node_type, sizeof(char), MSG_WAITALL);
    recv(conn_fd, &path_size, sizeof(size_t), MSG_WAITALL);
    char* path = malloc(path_size);
    recv(conn_fd, path, path_size, MSG_WAITALL);

    // Теперь надо как-то принять файл, который отправляется блоками. Подменим стандартный вход на conn_fd
    dup2(conn_fd, 0);

    // Выполняем операцию
    write_item(path, inodes, super_block_ptr, free_map_ptr, fs);
}
