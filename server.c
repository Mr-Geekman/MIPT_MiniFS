#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include "constants.h"
#include "api_fs.h"
#include "api_server.h"


int conn_fd = -1;
int listen_fd = -1;


void
shutdown_server()
{
    if(conn_fd != -1) {
        shutdown(conn_fd, SHUT_RDWR);
        close(conn_fd);
    }
    if(listen_fd != -1) {
        shutdown(listen_fd, SHUT_RDWR);
        close(listen_fd);
    }
    exit(0);
}


void
prepare_signals()
{
    signal(SIGPIPE, SIG_IGN);

    // prepare structures
    struct sigaction int_action;
    memset(&int_action, 0, sizeof(struct sigaction));
    int_action.sa_handler = shutdown_server;
    int_action.sa_flags = SA_RESTART;


    struct sigaction term_action;
    memset(&term_action, 0, sizeof(struct sigaction));
    term_action.sa_handler = shutdown_server;
    term_action.sa_flags = SA_RESTART;

    // set sigaction
    sigaction(SIGINT, &int_action, NULL);
    sigaction(SIGTERM, &term_action, NULL);
}


int
main() 
{
    // Обработка сигналов
    prepare_signals();

    // Демонизация
    daemon(0, 0);
    chdir("/run/minifs/");

    // Инициализация файловой системы
    FILE* fs = fopen("fs.fs", "r+");
    fseek(fs, 0, SEEK_END);
    size_t fs_file_size = ftell(fs);
    fseek(fs, 0, SEEK_SET);
    size_t fs_size = CONTROL_SIZE + NUM_BLOCKS * BLOCK_SIZE;
    if (fs == NULL || fs_file_size != fs_size) 
    { 
        // Файла нет, а значит надо самим инициализировать файловую систему
        init_minifs();
        fs = fopen("fs.fs", "r+");
    }

    ftruncate(fileno(fs), fs_size);

    // Создание отображения файла в память
    char* start_addr = mmap(0, CONTROL_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fileno(fs), 0);
    if (start_addr == MAP_FAILED) 
    {
        fprintf(stderr, "MMAP failed!\n");
        fclose(fs);
        exit(1);
    }
    struct SuperBlock* super_block_ptr = (struct SuperBlock*) start_addr;
    struct Inode* inodes = (struct Inode*) (start_addr + sizeof(struct SuperBlock));
    struct FreeMap* free_map_ptr = (struct FreeMap*) (start_addr + sizeof(struct SuperBlock) + NUM_INODES * sizeof(struct Inode));

    // Инициализация сетевой части
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) 
    {
        fprintf(stderr, "Error during creating socket.\n");
        exit(1);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) 
    {
        fprintf(stderr, "Bind error.\n");
        exit(1);
    }

    // Будем слушать лишь одного клиента
    listen(listen_fd, 1);

    // Цикл обработки
    while (true) 
    {
        // Выполим соединение

        conn_fd = accept(listen_fd, NULL, NULL);
        if(conn_fd < 0)
        {
            sleep(1);
            continue;
        }

        // Заведем файл для вывода программы/ошибок
        // После завершения обработки пользователя, ему будет отправлено содержимое файла
        freopen("minifs_output.log", "wb", stdout);
        freopen("minifs_output.log", "wb", stderr);

        // Прочитаем тип операции и выполним
        TypeOperation operation = NONE;
        recv(conn_fd, &operation, sizeof(TypeOperation), MSG_WAITALL);

        switch (operation) 
        {
            case INIT:
                server_init_minifs();
                break;
            case CREATE:
                server_create(conn_fd, inodes, super_block_ptr, free_map_ptr, fs);
                break;
            case DELETE:
                server_delete(conn_fd, inodes, super_block_ptr, free_map_ptr, fs);
                break;
            case READ:
                server_read(conn_fd, inodes, fs);
                break;
            case WRITE:
                server_write(conn_fd, inodes, super_block_ptr, free_map_ptr, fs);
                break;
            case NONE:
                fprintf(stderr, "Comand not found.\n");
                continue;
        }

        // Прочитаем содержимое нашего файла logs/output.log и отправим его пользователю
        fflush(stdout);
        fflush(stderr);
        FILE* output_file = fopen("minifs_output.log", "rb");
        fseek(output_file, 0, SEEK_END);
        size_t output_size = ftell(output_file);
        fseek(output_file, 0, SEEK_SET);
        send(conn_fd, &output_size, sizeof(size_t), MSG_CONFIRM);


        if (output_size > 0)
        {
            char* buffer = malloc(output_size);
            fread(buffer, 1, output_size, output_file);
            send(conn_fd, buffer, output_size, MSG_CONFIRM);
            free(buffer);
        }
        fclose(output_file);

        // Закроем соединение и немного подождем (иначе будем зря грузить процессор).
        close(conn_fd);
        conn_fd = -1;
        sleep(1);
    }

    // Очистка
    munmap(start_addr, CONTROL_SIZE);
    fclose(fs);
    shutdown_server();
    return 0;
}