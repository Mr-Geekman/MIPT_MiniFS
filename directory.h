#ifndef DIRECTORY
#define DIRECTORY

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


// Запись в директории
struct DirectoryRecord
{
    // номер inode файла
    uint16_t inode;
    // имя файла
    char filename[FILENAME_SIZE + 1];
};

// Нахождение файла/директории в данной директории
bool 
find_in_dir(struct Inode* dir_inode_ptr, char filename[FILENAME_SIZE + 1], bool find_directory, 
    size_t* found_inode_index_ptr, struct Inode* inodes, FILE* fs)
{
    size_t records_per_block = (BLOCK_SIZE - LINK_SIZE) / sizeof(struct DirectoryRecord);
    // последний занятый блок
    size_t last_block_index = 0;
    if(dir_inode_ptr->size > 0)
    {
        last_block_index = (dir_inode_ptr->size-1) / records_per_block;
    }
    size_t in_block_size = dir_inode_ptr->size % records_per_block;
    size_t current_block_index = dir_inode_ptr->block_index;
    for(size_t i = 0; i <= last_block_index; ++i)
    {
        // Чтение следующего блока
        if(i != 0) 
        {
            uint16_t next_block_index;
            fseek(fs, CONTROL_SIZE, SEEK_SET);
            fseek(fs, current_block_index*BLOCK_SIZE + BLOCK_SIZE - LINK_SIZE, SEEK_CUR);
            fread(&next_block_index, sizeof(uint16_t), 1, fs);
            current_block_index = (size_t) next_block_index;
        }
        // Переход к следующему блоку
        fseek(fs, CONTROL_SIZE, SEEK_SET);
        fseek(fs, current_block_index*BLOCK_SIZE, SEEK_CUR);
        for(size_t j = 0; j < in_block_size; ++j)
        {
            struct DirectoryRecord current_record;
            fread(&current_record, sizeof(struct DirectoryRecord), 1, fs);
            if((find_directory == inodes[current_record.inode].is_directory) && strcmp(current_record.filename, filename) == 0)
            {
                *found_inode_index_ptr = current_record.inode;
                return true;
            }
        }
    }
    return false;
}


// Вывод содержимого директории
void 
print_directory(struct Inode* inode_ptr, struct Inode* inodes, FILE* fs)
{
    size_t records_per_block = (BLOCK_SIZE - LINK_SIZE) / sizeof(struct DirectoryRecord);
    // последний занятый блок
    size_t last_block_index = 0;
    if(inode_ptr->size > 0)
    {
        last_block_index = (inode_ptr->size-1) / records_per_block;
    }
    // индекс нового элемента в своем блоке
    size_t last_block_size = inode_ptr->size % records_per_block;
    size_t current_block_index = inode_ptr->block_index;
    size_t current_block_size;
    for(size_t i = 0; i <= last_block_index; ++i)
    {
        // Чтение следующего блока
        if(i != 0) 
        {
            uint16_t next_block_index;
            fseek(fs, CONTROL_SIZE, SEEK_SET);
            fseek(fs, current_block_index*BLOCK_SIZE + BLOCK_SIZE - LINK_SIZE, SEEK_CUR);
            fread(&next_block_index, sizeof(uint16_t), 1, fs);
            current_block_index = (size_t) next_block_index;
        }
        // Переход к следующему блоку
        fseek(fs, CONTROL_SIZE, SEEK_SET);
        fseek(fs, current_block_index*BLOCK_SIZE, SEEK_CUR);
        
        current_block_size = (i == last_block_index) ? last_block_size : records_per_block;
        for(size_t j = 0; j < current_block_size; ++j)
        {
            struct DirectoryRecord current_record;
            fread(&current_record, sizeof(struct DirectoryRecord), 1, fs);
            if(inodes[current_record.inode].is_directory)
            {
                printf("D\t%u\t%u\t%s\n", current_record.inode, inodes[current_record.inode].size, current_record.filename);
            }
            else
            {
                printf("F\t%u\t%u\t%s\n", current_record.inode, inodes[current_record.inode].size, current_record.filename);
            }
            
        }
    }
}


// Добавление новой записи в директорию
// Возвращет false если не хватило места для записи (закончились блоки и нужно было расширение)
bool 
add_directory_record(struct Inode* inode_ptr, struct DirectoryRecord* new_record_ptr, struct SuperBlock* super_block_ptr, 
    struct FreeMap* free_map_ptr, FILE* fs) 
{
    size_t records_per_block = (BLOCK_SIZE - LINK_SIZE) / sizeof(struct DirectoryRecord);
    // последний занятый блок
    size_t last_block_index = 0;
    if(inode_ptr->size > 0)
    {
        last_block_index = (inode_ptr->size-1) / records_per_block;
    }
    // индекс нового элемента в своем блоке
    size_t in_block_index = inode_ptr->size % records_per_block;
    // получим адрес последнего блока (по цепочке)
    size_t current_block_index = inode_ptr->block_index;
    for(size_t i = 0; i < last_block_index; ++i)
    {
        // перемещаемся к нужному блоку и читаем ссылку на следующий блок
        uint16_t next_block_index;
        fseek(fs, CONTROL_SIZE, SEEK_SET);
        fseek(fs, current_block_index*BLOCK_SIZE + BLOCK_SIZE - LINK_SIZE, SEEK_CUR);
        fread(&next_block_index, sizeof(uint16_t), 1, fs);
        current_block_index = (size_t) next_block_index;
    }

    // проверяем, что нужно брать еще один блок
    if(last_block_index != 0 && in_block_index == 0) 
    {
        size_t new_block_index;
        bool exist_free_block = find_free_block(free_map_ptr, &new_block_index, super_block_ptr);
        // если свободных блоков нет, то не можем завести новый блок под запись
        if(!exist_free_block)
        {
            return false;
        }
        reserve_block(free_map_ptr, new_block_index, super_block_ptr);
        // делаем в текущем блоке ссылку на следующий
        uint16_t new_block_index_limited = (uint16_t) new_block_index;
        fseek(fs, CONTROL_SIZE, SEEK_SET);
        fseek(fs, current_block_index*BLOCK_SIZE + BLOCK_SIZE - LINK_SIZE, SEEK_CUR);
        fwrite(&new_block_index_limited, sizeof(uint16_t), 1, fs);
        fflush(fs);
        // переходим к слеющему блоку
        current_block_index = new_block_index;
    }

    // находим нужную позицию в текущем блоке и выполняем запись
    fseek(fs, CONTROL_SIZE, SEEK_SET);
    fseek(fs, current_block_index*BLOCK_SIZE + sizeof(struct DirectoryRecord)*in_block_index, SEEK_CUR);
    fwrite(new_record_ptr, sizeof(struct DirectoryRecord), 1, fs);
    fflush(fs);
    // обновляем данные о числе записей в директории
    inode_ptr->size += 1;
    return true;
}


// Удаление записи из директории
void 
remove_directory_record(struct Inode* dir_inode_ptr, size_t inode_to_delete, struct SuperBlock* super_block_ptr, 
    struct FreeMap* free_map_ptr, FILE* fs) 
{
    /*
    Дело в том, что записи в директории хранятся линейно. При удалении одной записи все остальные 
    придется двигать.
    Для начала найдем позицию блока, который хотим удалить. Если он не последний, то переместим на его место последний.
    Если при этом освободился блок памяти, то освобождаем его (это по идее просто).
    */
    size_t records_per_block = (BLOCK_SIZE - LINK_SIZE) / sizeof(struct DirectoryRecord);
    // последний занятый блок
    size_t last_block_index = 0;
    if(dir_inode_ptr->size > 0)
    {
        last_block_index = (dir_inode_ptr->size-1) / records_per_block;
    }
    size_t last_block_size = dir_inode_ptr->size % records_per_block;
    size_t current_block_index = dir_inode_ptr->block_index;
    size_t record_to_delete_offset;
    size_t current_block_size;
    bool found_to_delete = false;
    for(size_t i = 0; i <= last_block_index; ++i)
    {
        // Чтение следующего блока
        if(i != 0) 
        {
            uint16_t next_block_index;
            fseek(fs, CONTROL_SIZE, SEEK_SET);
            fseek(fs, current_block_index*BLOCK_SIZE + BLOCK_SIZE - LINK_SIZE, SEEK_CUR);
            fread(&next_block_index, sizeof(uint16_t), 1, fs);
            current_block_index = (size_t) next_block_index;
        }
        // Переход к следующему блоку
        fseek(fs, CONTROL_SIZE, SEEK_SET);
        fseek(fs, current_block_index*BLOCK_SIZE, SEEK_CUR);
        // Если мы уже нашли блок для удаления, то просто доходим до конца.
        if(found_to_delete)
        {
            continue;
        }
        current_block_size = (i == last_block_index) ? last_block_size : records_per_block;
        for(size_t j = 0; j < current_block_size; ++j)
        {
            struct DirectoryRecord current_record;
            fread(&current_record, sizeof(struct DirectoryRecord), 1, fs);
            // Если мы нашли то, что хотим удалить, то запоминаем и продолжаем идти до последнего блока.
            if(current_record.inode == inode_to_delete)
            {
                record_to_delete_offset = current_block_index*BLOCK_SIZE + j*sizeof(struct DirectoryRecord);
                continue;
            }
        }
    }
    // Сместимся к последнему блоку в директории
    fseek(fs, CONTROL_SIZE, SEEK_SET);
    fseek(fs, current_block_index*BLOCK_SIZE + (last_block_size-1)*sizeof(struct DirectoryRecord), SEEK_CUR);
    // В принципе удаляемый блок может быть последним, 
    // но чтобы не плодить код мы не будем выделять это в отдельный случай
    struct DirectoryRecord last_record;
    fread(&last_record, sizeof(struct DirectoryRecord), 1, fs);
    fseek(fs, CONTROL_SIZE, SEEK_SET);
    fseek(fs, record_to_delete_offset, SEEK_CUR);
    fwrite(&last_record, sizeof(struct DirectoryRecord), 1, fs);
    fflush(fs);

    // Теперь, когда мы переместили последний блок на место удаляемого, можно освободить место
    dir_inode_ptr->size -= 1;
    // Могло так оказаться, что теперь нам не нужен еще один блок, тогда освободим его
    if(dir_inode_ptr->size % records_per_block == 0)
    {
        free_block(free_map_ptr, current_block_index, super_block_ptr);
    }
}

#endif