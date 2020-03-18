#ifndef FREE_MAP
#define FREE_MAP

#include "superblock.h"
#include "constants.h"

// Битовая карта свободных блоков памяти
struct FreeMap
{
    // 1 если блок зарезервирован, 0 иначе.
    char reserved[NUM_BLOCKS / CHAR_BIT];
};

// Проверка на то, что блок свободен
bool 
is_free_block(struct FreeMap* free_map_ptr, size_t num_block)
{
    size_t num_byte = num_block / CHAR_BIT;
    size_t num_bit = num_block % CHAR_BIT;
    return ((*free_map_ptr).reserved[num_byte] >> num_bit & 1) == 0;
}

// Зарезервировать блок
void 
reserve_block(struct FreeMap* free_map_ptr, size_t num_block, struct SuperBlock* super_block_ptr)
{
    size_t num_byte = num_block / CHAR_BIT;
    size_t num_bit = num_block % CHAR_BIT;
    free_map_ptr->reserved[num_byte] |= (1 << num_bit);
    super_block_ptr->free_blocks -= 1;
}

// Освободить блок
void 
free_block(struct FreeMap* free_map_ptr, size_t num_block, struct SuperBlock* super_block_ptr)
{
    size_t num_byte = num_block / CHAR_BIT;
    size_t num_bit = num_block % CHAR_BIT;
    free_map_ptr->reserved[num_byte] |= (~(1 << num_bit));
    super_block_ptr->free_blocks += 1;
}

// Поиск свободного блока
// Выдает успех или неуспех, в случае успеха по указателю лежит номер
bool 
find_free_block(struct FreeMap* free_map_ptr, size_t* block_index_ptr, struct SuperBlock* super_block_ptr) 
{
    if(super_block_ptr->free_inodes == 0) 
    {
        return false;
    }
    for(size_t i = 0; i < NUM_BLOCKS; ++i) 
    {
        if(is_free_block(free_map_ptr, i))
        {
            *block_index_ptr = i;
            return true;
        }
    }
    return false;
}

#endif