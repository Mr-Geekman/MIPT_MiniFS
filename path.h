#ifndef PATH
#define PATH

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdbool.h>
#include <limits.h>
#include <libgen.h>
#include "constants.h"
#include "directory.h"
#include "inode.h"


// Нормализация пути, убирание всего лишнего
void 
normalize_path(char path[PATH_MAX], char new_path[PATH_MAX])
{
    FILE* fp;
    char command[PATH_MAX + 100];
    sprintf(command, "/usr/bin/realpath -m %s", path);
    fp = popen(command, "r");
    if(fp == NULL)
    {
        new_path[0] = '\0';
    }

    while (fgets(new_path, PATH_MAX, fp) != NULL) {
        continue;
    }

    if(strlen(new_path) > 0 && new_path[strlen(new_path) - 1] == '\n') {
        new_path[strlen(new_path) - 1] = '\0';
    }

    pclose(fp);
    return;
}

enum finding_result 
{
    NOT_FOUND, // ничего не найдено
    FOUND_DIR, // найдена родительская директория
    FOUND_BOTH, // найдена и конечная директория и сам файл
};

// Нахождение пути к файлу
enum finding_result 
find_file(char* path, bool is_directory, size_t* parent_dir_index_ptr, size_t* file_index_ptr, char* filename, struct Inode* inodes, FILE* fs)
{
    // Проверка, что путь абсолютный
    if(path[0] != '/')
    {
        fprintf(stderr, "Path should starts with /.\n");
        return NOT_FOUND;
    }

    // Проверка, что путь не слишком длинный
    if(strlen(path) >= PATH_MAX)
    {
        fprintf(stderr, "Path is too long.\n");
        return NOT_FOUND;
    }

    // Нормализация пути
    char new_path[PATH_MAX + 1];
    normalize_path(path, new_path);

    if(strlen(new_path) == 0)
    {
        fprintf(stderr, "Wrong path.");
        return NOT_FOUND;
    }

    // Проверим, что мы находимся в корне
    if(strcmp(new_path, "/") == 0) 
    {
        // Если требовалось найти директорию, то все хорошо
        if(is_directory)
        {
            *parent_dir_index_ptr = 0;
            *file_index_ptr = 0;
            filename = "/";
            return FOUND_BOTH;
        }
        // Иначе мы не нашли, т.к. такого быть не может
        return NOT_FOUND;
    }

    // Иначе надо определить inode для родительской папке искомого файла и для него самого
    char dirc[PATH_MAX + 1];
    char basec[PATH_MAX + 1];
    char* bname;
    char* dname;
    strcpy(dirc, new_path);
    strcpy(basec, new_path);
    dname = dirname(dirc);
    bname = basename(basec);
    if(strlen(bname) > FILENAME_SIZE)
    {
        fprintf(stderr, "Filename is too long.\n");
        return NOT_FOUND;
    }
    strcpy(filename, bname);

    // Спустимся по пути в dname и найдем inode родительской директории

    // Начинаем с корня
    size_t current_inode = 0;
    size_t start_name_index = 1;
    size_t end_name_index = 1;
    size_t dir_path_len = strlen(dname);
    // +1 для \0
    char current_filename[FILENAME_SIZE + 1];
    for(size_t i = 1; i <= dir_path_len; ++i)
    {
        // Если мы дошли до слэша или до конца текста, то переходим в поддиректорию
        if(dname[i] == '/' || i == dir_path_len)
        {
            // Этот индекс не включаем
            end_name_index = i;
            // Если мы обрабатываем корень (мы сразу оказываемся в конце строки)
            if(end_name_index == start_name_index)
            {
                continue;
            }
            // Если мы обнаружили слишком длинное имя при спуске, то такого точно быть не может
            if(end_name_index - start_name_index > FILENAME_SIZE)
            {
                fprintf(stderr, "Wrong path.\n");
                return NOT_FOUND;
            }
            strncpy(current_filename, dname + start_name_index, end_name_index - start_name_index);
            current_filename[end_name_index - start_name_index] = '\0';
            start_name_index = i + 1;
            // Оысуществляем поиск полученной директории внутри текущей
            bool res_found;
            res_found = find_in_dir(inodes + current_inode, current_filename, true, &current_inode, inodes, fs);
            if(!res_found)
            {
                fprintf(stderr, "Wrong path.\n");
                return false;
            }
        }
    }
    *parent_dir_index_ptr = current_inode;
    // Проверим есть ли файл в найденной родительской директории
    bool res_found;
    res_found = find_in_dir(inodes + current_inode, bname, is_directory, file_index_ptr, inodes, fs);
    if(res_found)
    {
        return FOUND_BOTH;
    }
    else
    {
        return FOUND_DIR;
    }
}

#endif