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
#include <limits.h>
#include "constants.h"
#include "superblock.h"
#include "inode.h"
#include "freemap.h"
#include "path.h"
#include "directory.h"
#include "file.h"
#include "api.h"


int 
main(int argc, char* argv[]) 
{
    if(argc < 2) {
        fprintf(stderr, "You haven't given a comand to execute.\n");
        exit(0);
    }
    if(strcmp(argv[1], "init") == 0) {
        init_minifs();
        exit(0);
    }

    FILE* fs = fopen("fs.fs", "r+");
    if (fs == NULL) 
    { 
        fprintf(stderr, "Error opend file\n"); 
        exit(1); 
    }
    size_t fs_size = CONTROL_SIZE + NUM_BLOCKS * BLOCK_SIZE;
    ftruncate(fileno(fs), fs_size);

    // Создание отображения файла в память
    char* start_addr = mmap(0, CONTROL_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fileno(fs), 0);
    if (start_addr == MAP_FAILED) {
        fprintf(stderr, "MMAP failed!\n");
        fclose(fs);
        exit(0);
    }
    struct SuperBlock* super_block_ptr = (struct SuperBlock*) start_addr;
    struct Inode* inodes = (struct Inode*) (start_addr + sizeof(struct SuperBlock));
    struct FreeMap* free_map_ptr = (struct FreeMap*) (start_addr + sizeof(struct SuperBlock) + NUM_INODES * sizeof(struct Inode));
    if(strcmp(argv[1], "read") == 0) {
        if(argc < 4) {
            fprintf(stderr, "You haven't given path for reading or type of operation.\n");
            exit(0);
        }
        read_item(argv[2], argv[3], inodes, fs);
    }

    else if(strcmp(argv[1], "create") == 0) 
    {
        if(argc < 4) {
            fprintf(stderr, "You haven't given path for creating or type of operation.\n");
            exit(0);
        }
        create_item(argv[2], argv[3], inodes, super_block_ptr, free_map_ptr, fs);
    }

    else if(strcmp(argv[1], "delete") == 0) 
    {
        if(argc < 4) {
            fprintf(stderr, "You haven't given path for deleting or type of operation.\n");
            exit(0);
        }
        delete_item(argv[2], argv[3], inodes, super_block_ptr, free_map_ptr, fs);
    }

    else if(strcmp(argv[1], "write") == 0) 
    {
        if(argc < 3) {
            fprintf(stderr, "You haven't given path for writing.\n");
            exit(0);
        }
        write_item(argv[2], inodes, super_block_ptr, free_map_ptr, fs);
    }

    else 
    {
        fprintf(stderr, "Comand not found.\n");
    }

    // Очистка
    munmap(start_addr, CONTROL_SIZE);
    fclose(fs);
};