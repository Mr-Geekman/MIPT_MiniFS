#ifndef FILE_HEADER
#define FILE_HEADER

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <limits.h>
#include "constants.h"
#include "superblock.h"
#include "inode.h"
#include "freemap.h"


// Запись в файл
void 
write_file(struct Inode* inode_ptr, struct SuperBlock* super_block_ptr, struct FreeMap* free_map_ptr, FILE* fs)
{
    // Будем записывать файл по размерам блока
    size_t buffer_size = BLOCK_SIZE - LINK_SIZE;
    char buffer[buffer_size];
    size_t current_block = inode_ptr->block_index;
    size_t received_bytes = fread(buffer, 1, buffer_size, stdin);
    while(received_bytes > 0)
    {
        fseek(fs, CONTROL_SIZE + current_block*BLOCK_SIZE, SEEK_SET);
        fwrite(buffer, 1, received_bytes, fs);
        fflush(fs);
        inode_ptr->size += received_bytes;
        // если мы заполнили весь блок, то имеет смысл завести новый
        if(received_bytes == sizeof(buffer))
        {
            size_t new_block_index;
            bool exist_free_block = find_free_block(free_map_ptr, &new_block_index, super_block_ptr);
            if(!exist_free_block)
            {
                fprintf(stderr, "Where is no free blocks in file system.\n");
                return;
            }
            reserve_block(free_map_ptr, new_block_index, super_block_ptr);
            fseek(fs, CONTROL_SIZE + current_block*BLOCK_SIZE + BLOCK_SIZE - LINK_SIZE, SEEK_SET);
            fwrite(&new_block_index, sizeof(uint16_t), 1, fs);
            fflush(fs);
            current_block = new_block_index;
        }
        received_bytes = fread(buffer, 1, buffer_size, stdin);
    }
}

// Вывод файла на стандартный поток вывода
void 
print_file(struct Inode* inode_ptr, FILE* fs)
{
    // Будем записывать файл по размерам блока
    size_t buffer_size = BLOCK_SIZE - LINK_SIZE;
    char buffer[buffer_size];
    size_t current_block = inode_ptr->block_index;
    size_t bytes_remain = inode_ptr->size;
    while(bytes_remain > 0)
    {
        // Прочитаем текущий блок
        fseek(fs, CONTROL_SIZE + current_block*BLOCK_SIZE, SEEK_SET);
        size_t bytes_to_read;
        if(bytes_remain >= buffer_size) 
        {
            bytes_to_read = buffer_size;
        }
        else
        {
            bytes_to_read = bytes_remain;
        }
        size_t received_bytes = fread(buffer, 1, bytes_to_read, fs);
        fwrite(buffer, 1, received_bytes, stdout);
        fflush(fs);
        bytes_remain -= received_bytes;

        // Найдем следующий блок
        fseek(fs, CONTROL_SIZE + current_block*BLOCK_SIZE + BLOCK_SIZE - LINK_SIZE, SEEK_SET);
        fread(&current_block, sizeof(uint16_t), 1, fs);
    }
}

// Очистка файла
void 
erase_file(struct Inode* inode_ptr, struct SuperBlock* super_block_ptr, struct FreeMap* free_map_ptr, FILE* fs)
{
    size_t current_block = inode_ptr->block_index;
    // Когда файл когда заполнил байтами ровно N блоков, то он уже забронировал еще один
    size_t blocks_to_free = inode_ptr->size / (BLOCK_SIZE - LINK_SIZE) + 1;
    for(size_t i = 0; i < blocks_to_free; ++i)
    {
        // Освобождаем текущий блок если это не первый блок
        if(i != 0)
        {
            free_block(free_map_ptr, current_block, super_block_ptr);
        }

        // Читаем следующий
        fseek(fs, CONTROL_SIZE + current_block*BLOCK_SIZE + BLOCK_SIZE - LINK_SIZE, SEEK_SET);
        fread(&current_block, sizeof(uint16_t), 1, fs);
    }

    // В самом конце устанавливаем размер файла равным нулю
    inode_ptr->size = 0;
}

#endif