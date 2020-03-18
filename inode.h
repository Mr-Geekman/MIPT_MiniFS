#ifndef INODE
#define INODE

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdbool.h>
#include <limits.h>
#include "superblock.h"

// Индексный узел
// Можно было съкономить 2 байта, но пришлось бы отключать выравнивание
struct Inode 
{
    uint16_t block_index; // индекс блока в ФС
    bool is_free; // занят или нет
    bool is_directory; // директория или нет
    uint32_t size; // размер если это файл
};

// Зарезервировать inode при создании
void 
reserve_inode(struct Inode* inode_ptr, bool is_directory, size_t block_index, struct SuperBlock* super_block_ptr) {
    inode_ptr -> block_index = block_index;
    inode_ptr->is_free = false;
    inode_ptr->is_directory = is_directory;
    inode_ptr-> size = 0;
    super_block_ptr->free_inodes -= 1;
}

//  inode при создании
void 
free_inode(struct Inode* inode_ptr, struct SuperBlock* super_block_ptr) {
    inode_ptr->is_free = true;
    super_block_ptr->free_inodes += 1;
}

// Поиск свободного индексного узла
// Выдает успех или неуспех, в случае успеха по указателю лежит номер
bool 
find_free_inode(struct Inode* inodes_ptr, size_t* inode_index_ptr, struct SuperBlock* super_block_ptr) 
{
    if(super_block_ptr->free_inodes == 0) {
        return false;
    }
    for(size_t i = 0; i < NUM_INODES; ++i) 
    {
        if((inodes_ptr + i)->is_free)
        {
            *inode_index_ptr = i;
            return true;
        }
    }
    return false;
}

#endif