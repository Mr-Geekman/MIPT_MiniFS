#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include "constants.h"
#include "superblock.h"
#include "inode.h"
#include "freemap.h"
#include "api_client.h"


int 
main(int argc, char* argv[]) 
{
    // Установка соединения с сервером
    struct sockaddr_in addr;
    int conn_fd = socket(AF_INET, SOCK_STREAM, 0);

    // Если прозошла ошибка при получении сокета.
    if (conn_fd < 0) 
    {
        fprintf(stderr, "Error during creating socket.\n");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (connect(conn_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) 
    {
        fprintf(stderr, "Connection error.\n");
        close(conn_fd);
        exit(1);
    }

    // Обработка аргументов командной строки
    if(argc < 2) 
    {
        fprintf(stderr, "You haven't given a comand to execute.\n");
        close(conn_fd);
        exit(1);
    }
    if(strcmp(argv[1], "init") == 0) 
    {
        client_init_minifs(conn_fd);
    }

    if(strcmp(argv[1], "read") == 0) 
    {
        if(argc < 4) 
        {
            fprintf(stderr, "You haven't given path for reading or type of operation.\n");
            close(conn_fd);
            exit(1);
        }
        client_read_item(argv[2][0], argv[3], conn_fd);
    }

    else if(strcmp(argv[1], "create") == 0) 
    {
        if(argc < 4) 
        {
            fprintf(stderr, "You haven't given path for creating or type of operation.\n");
            close(conn_fd);
            exit(1);
        }
        client_create_item(argv[2][0], argv[3], conn_fd);
    }

    else if(strcmp(argv[1], "delete") == 0) 
    {
        if(argc < 4) 
        {
            fprintf(stderr, "You haven't given path for deleting or type of operation.\n");
            close(conn_fd);
            exit(1);
        }
        client_delete_item(argv[2][0], argv[3], conn_fd);
    }

    else if(strcmp(argv[1], "write") == 0) 
    {
        if(argc < 3) 
        {
            fprintf(stderr, "You haven't given path for writing.\n");
            close(conn_fd);
            exit(0);
        }
        client_write_item(argv[2], conn_fd);
    }

    else 
    {
        fprintf(stderr, "Command not found.\n");
    }

    // Закрываем соединение
    close(conn_fd);
};