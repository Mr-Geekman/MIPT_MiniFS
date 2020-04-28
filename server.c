#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "constants.h"
#include "api_fs.h"
#include "api_server.h"


int
main() 
{
    // Инициализация файловой системы
    FILE* fs = fopen("fs.fs", "r+");
    if (fs == NULL) 
    { 
        // Файла нет, а значит надо самим инициализировать файловую систему
        init_minifs();
        fs = fopen("fs.fs", "r+");
    }
    size_t fs_size = CONTROL_SIZE + NUM_BLOCKS * BLOCK_SIZE;
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
    // ДОБАВИТЬ ДЕМОНИЗАЦИЮ ПОСЛЕ ОТЛАДКИ!!!!!!!!
    // daemon(0, 0);

    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) 
    {
        fprintf(stderr, "Error during creating socket.\n");
        exit(1);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) < 0) 
    {
        fprintf(stderr, "Bind error.\n");
        exit(1);
    }

    // Будем слушать лишь одного клиента
    listen(listener, 1);

    // Цикл обработки
    while (true) 
    {
        // Выполим соединение
        int conn_fd = accept(listener, NULL, NULL);

        // Заведем файл для вывода программы/ошибок
        // После завершения обработки пользователя, ему будет отправлено содержимое файла
        freopen("logs/output.log", "wb+", stdout);
        freopen("logs/output.log", "wb+", stderr);

        // Прочитаем тип операции и выполним
        TypeOperation operation = INIT;
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
            default:
                fprintf(stderr, "Comand not found.\n");
                break;
        }

        // Прочитаем содержимое нашего файла logs/output.log и отправим его пользователю
        FILE* output_file = fopen("logs/output.log", "rb");
        fseek(output_file, 0, SEEK_END);
        size_t output_size = ftell(output_file);
        fseek(output_file, 0, SEEK_SET);

        char* buffer = malloc(output_size + 1);
        fread(buffer, 1, output_size, output_file);
        fclose(output_file);

        send(conn_fd, &output_size, sizeof(size_t), MSG_WAITALL);
        send(conn_fd, buffer, output_size, MSG_WAITALL);

        // Закроем соединение и немного подождем (иначе будем зря грузить процессор).
        close(conn_fd);
        sleep(0.1);
    }

    // Очистка
    munmap(start_addr, CONTROL_SIZE);
    fclose(fs);
    return 0;
}