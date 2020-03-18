#ifndef SUPER_BLOCK
#define SUPER_BLOCK


struct SuperBlock 
{
    size_t num_blocks;
    size_t free_blocks;
    size_t num_inodes;
    size_t free_inodes;
    size_t block_size;
    size_t inode_size; 
};

#endif