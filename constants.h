#ifndef CONSTANTS
#define CONSTANTS

#include <limits.h>
#include "superblock.h"

// Глобальные константы
enum 
{
    BLOCK_SIZE = 4096, // размер блока
    NUM_BLOCKS = 65536, // количество блоков
    NUM_INODES = 65536, // количество индексных узлов
    LINK_SIZE = 2, // размер, отведенный под ссылку при косвенной адресации
    INODE_SIZE = 8, // размер Inode (если его определить не здесь, то будет цикл)
    CONTROL_SIZE = sizeof(struct SuperBlock) + NUM_INODES * INODE_SIZE + (NUM_BLOCKS / CHAR_BIT), // размер контрольной части (без файлов)
    FILENAME_SIZE = 40, // максимальный размер имени файла
    SERVER_PORT = 9063, // порт сервера
};

// Типы операций для клиента и сервера
typedef enum 
{
    INIT,
    CREATE,
    DELETE,
    READ,
    WRITE,
    NONE
} TypeOperation;

#endif