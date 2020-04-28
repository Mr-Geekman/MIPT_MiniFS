#ifndef API_FS
#define API_FS

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdbool.h>
#include <limits.h>
#include "constants.h"
#include "superblock.h"
#include "inode.h"
#include "freemap.h"
#include "path.h"
#include "directory.h"
#include "file.h"


// Реализация операции создания файла/директории
void 
create_item(char operation_type, char* path, struct Inode* inodes, struct SuperBlock* super_block_ptr, 
    struct FreeMap* free_map_ptr, FILE* fs)
{
    bool process_directory = false;
    if(operation_type == 'd')
    {
        process_directory = true;
    }
    if(operation_type != 'f' && operation_type != 'd')
    {
        fprintf(stderr, "Wrong parameter operation type parameter.\n");
        return;
    }
    size_t parent_dir;
    size_t file;
    char filename[FILENAME_SIZE + 1];
    enum finding_result res;
    res = find_file(path, process_directory, &parent_dir, &file, filename, inodes, fs);

    // Нельзя создавать файлы с именами (точка, две точки)
    if(strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0)
    {
        fprintf(stderr, "You can not create files . and ..");
        return;
    }

    // Если все прошло корректно
    if(res == FOUND_DIR) 
    {
        size_t new_inode_index;
        size_t new_block_index;
        bool exist_free_inode = find_free_inode(inodes, &new_inode_index, super_block_ptr);
        bool exist_free_block = find_free_block(free_map_ptr, &new_block_index, super_block_ptr);
        if(!exist_free_block || !exist_free_inode)
        {
            fprintf(stderr, "Where is no free blocks/inodes in file system.\n");
        }
        else
        {
            // Создаем inode узла, резервируем блок
            struct Inode* new_inode_ptr = inodes + new_inode_index;
            reserve_inode(new_inode_ptr, process_directory, new_block_index, super_block_ptr);
            reserve_block(free_map_ptr, new_block_index, super_block_ptr);
            struct Inode* parent_dir_inode_ptr = inodes + parent_dir;

            // Добавляем в родительскую директорию запись о новом узле
            struct DirectoryRecord new_record;
            new_record.inode = new_inode_index;
            strcpy(new_record.filename, filename);
            add_directory_record(parent_dir_inode_ptr, &new_record, super_block_ptr, free_map_ptr, fs);
            
            // Если это директория, то ее надо инициализировать
            if(process_directory)
            {
                struct DirectoryRecord current_directory = {new_inode_index, "."};
                struct DirectoryRecord parent_directory = {parent_dir, ".."};
                add_directory_record(new_inode_ptr, &current_directory, super_block_ptr, free_map_ptr, fs);
                add_directory_record(new_inode_ptr, &parent_directory, super_block_ptr, free_map_ptr, fs);
            }
        }
    }
    // Если нашли все, значит создавать уже нечего
    else if(res == FOUND_BOTH)
    {
        fprintf(stderr, "File/directory already exists.\n");
    }
    // Если не нашли путь до директории, то ошибка
    else
    {
        fprintf(stderr, "Error occured or directory not found.\n");
    }
}

// Реализация операции удаления файла/директории
void 
delete_item(char operation_type, char* path, struct Inode* inodes, struct SuperBlock* super_block_ptr, 
    struct FreeMap* free_map_ptr, FILE* fs)
{
    bool process_directory = false;
    if(operation_type == 'd')
    {
        process_directory = true;
    }
    if(operation_type != 'f' && operation_type != 'd')
    {
        fprintf(stderr, "Wrong parameter operation type parameter.\n");
        return;
    }
    size_t parent_dir;
    size_t file;
    char filename[FILENAME_SIZE + 1];
    enum finding_result res;
    res = find_file(path, process_directory, &parent_dir, &file, filename, inodes, fs);
    // Нельзя удалять файлы, отвечающие за родительскую директорию и текущую директорию
    if(strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0)
    {
        fprintf(stderr, "You can not create files . and ..");
        return;
    }

    // Если нашли файл для удаления
    if(res == FOUND_BOTH) 
    {
        // Если пытаемся удалить корневую директорию, то не делаем этого
        if(file == 0)
        {
            fprintf(stderr, "You are trying to delete root directory. It is not allowed.\n");
            return;
        }

        // Если это директория и внутри нее есть файлы, то не удаляем
        if(process_directory && inodes[file].size > 2)
        {
            fprintf(stderr, "You are trying to delete directory with files. Delete files first.\n");
            return;
        }

        struct Inode* inode_to_delete_ptr = inodes + file;
        free_block(free_map_ptr, inode_to_delete_ptr->block_index, super_block_ptr);
        if(!process_directory)
        {
            // Если удаляем файл, то надо освободить и все блоки, которые он задействует внутри себя
            erase_file(inodes + file, super_block_ptr, free_map_ptr, fs);
        }
        free_inode(inode_to_delete_ptr, super_block_ptr);
        
        // Удаляем файл из родительской директории
        remove_directory_record(inodes + parent_dir, file, super_block_ptr, free_map_ptr, fs);
    }
    // Если нашли все, значит создавать уже нечего
    else if(res == FOUND_DIR)
    {
        fprintf(stderr, "File/directory does not exist.\n");
    }
    // Если не нашли путь до директории, то ошибка
    else
    {
        fprintf(stderr, "Error occured or directory not found.\n");
    }
}

// Реализация операции записи
void 
write_item(char* path, struct Inode* inodes, struct SuperBlock* super_block_ptr, 
    struct FreeMap* free_map_ptr, FILE* fs)
{
    size_t parent_dir;
    size_t file;
    char filename[FILENAME_SIZE + 1];
    enum finding_result res;
    res = find_file(path, false, &parent_dir, &file, filename, inodes, fs);
    // Если все прошло корректно
    if(res == FOUND_BOTH) 
    {
        // В таком случай файл сначала надо очистить от того, что есть внутри него
        erase_file(inodes + file, super_block_ptr, free_map_ptr, fs);
        write_file(inodes + file, super_block_ptr, free_map_ptr, fs);
    }
    // Если что-то не было найдено
    else
    {
        fprintf(stderr, "Error occured or file not found.\n");
    }
}

// Реализация операции чтения
void 
read_item(char operation_type, char* path, struct Inode* inodes, FILE* fs)
{
    bool process_directory = false;
    if(operation_type == 'd')
    {
        process_directory = true;
    }
    if(operation_type != 'f' && operation_type != 'd')
    {
        fprintf(stderr, "Wrong parameter operation type parameter.\n");
        return;
    }
    size_t parent_dir;
    size_t file;
    char filename[FILENAME_SIZE + 1];
    enum finding_result res;
    res = find_file(path, process_directory, &parent_dir, &file, filename, inodes, fs);
    // Если все прошло корректно
    if(res == FOUND_BOTH) 
    {
        // Рассмотрим случаи файла и директории
        if(process_directory)
        {
            print_directory(inodes + file, inodes, fs);
        }
        else
        {
            print_file(inodes + file, fs);
        }
    }
    // Если что-то не было найдено
    else
    {
        fprintf(stderr, "Error occured or file/directory not found.\n");
    }
}


// Инициализация файловой системы
void 
init_minifs() 
{
    // Создание файла под ФС
    FILE* fs = fopen("fs.fs", "w+");
    if (fs == NULL) 
    { 
        fprintf(stderr, "\nError opend file\n"); 
        exit(1); 
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

    // Создание адресов для удобной работы с таблицами
    struct SuperBlock* super_block_ptr = (struct SuperBlock*) start_addr;
    struct Inode* inodes = (struct Inode*) (start_addr + sizeof(struct SuperBlock));
    struct FreeMap* free_map_ptr = (struct FreeMap*) (start_addr + sizeof(struct SuperBlock) + NUM_INODES * sizeof(struct Inode));

    // Создание суперблока и запись его в ФС
    struct SuperBlock super_block = {NUM_BLOCKS, NUM_BLOCKS, NUM_INODES, NUM_INODES, BLOCK_SIZE, sizeof(struct Inode)};
    *super_block_ptr = super_block;

    // Инициализация таблицы файловых дескрипторов
    struct Inode empty_inode = {0, true, false, 0};
    for(int i = 0; i < NUM_INODES; ++i) 
    {
        inodes[i] = empty_inode;
    }

    // Инициализация битовой карты
    struct FreeMap free_map = {0};
    *free_map_ptr = free_map;

    // Создание корневой директории
    struct Inode* root_ptr = inodes + 0;
    // не проверяем свободные блоки и берем первый, так как ФС пуста
    reserve_inode(root_ptr, true, 0, super_block_ptr);
    reserve_block(free_map_ptr, 0, super_block_ptr);
    // Инициализируем директорию со ссылкой на саму директорию (.) и на родительскую (..)
    struct DirectoryRecord current_directory = {0, "."};
    struct DirectoryRecord parent_directory = {0, ".."};
    add_directory_record(root_ptr, &current_directory, super_block_ptr, free_map_ptr, fs);
    add_directory_record(root_ptr, &parent_directory, super_block_ptr, free_map_ptr, fs);

    // Очистка
    munmap(start_addr, CONTROL_SIZE);
    fclose(fs);
}

#endif