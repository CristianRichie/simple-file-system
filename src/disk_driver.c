#include <disk_driver.h>
#include <common.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>     


void DiskDriver_init(DiskDriver* disk, const char* filename, int num_blocks) {
    int ret;
    int exists = access(filename, F_OK) != -1;
    int fd = open(filename, O_CREAT | O_RDWR, 0600);
    CHECK_ERROR(fd == -1, "[DD - init] open failed.\n");


    int bitmap_size = num_blocks >> 3;
    if (num_blocks & 0x7) bitmap_size ++;

    int zone_size = sizeof(DiskHeader) + bitmap_size + BLOCK_SIZE * num_blocks;
    
    if (!exists) {
        ret = posix_fallocate(fd, 0, zone_size);
        CHECK_ERROR(ret != 0, "[DD - init] fallocate failed.\n"); 
    }
    
    void * zone = mmap(0, zone_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    CHECK_ERROR(!zone, "[DD - init] mmap failed.\n");

    disk->header = (DiskHeader*)zone;
    disk->bitmap_data = (char*)zone + sizeof(DiskHeader);
    disk->fd = fd;

    if (exists) {
        int difference = num_blocks - disk->header->num_blocks;
        if (difference > 0) { 
            ret = posix_fallocate(disk->fd, zone_size - difference * BLOCK_SIZE, zone_size);
            CHECK_ERROR(ret != 0, "[DD - init] fallocate failed.\n"); 

            disk->header->num_blocks = num_blocks;
            disk->header->bitmap_blocks = num_blocks;
            disk->header->bitmap_entries = bitmap_size;
            disk->header->free_blocks += difference;

            bzero(disk->bitmap_data + difference, bitmap_size);
        }
    } 
    else {
        disk->header->num_blocks = num_blocks;
        disk->header->bitmap_blocks = num_blocks;
        disk->header->bitmap_entries = bitmap_size;
        disk->header->free_blocks = num_blocks;
        disk->header->first_free_block = 0;

        bzero(disk->bitmap_data, bitmap_size);
    }
}

int DiskDriver_readBlock(DiskDriver* disk, void* dest, int block_num) {
    if (block_num >= disk->header->num_blocks || block_num < 0)
        return -1;

    BitMap bmap = {
        .num_bits = disk->header->bitmap_blocks,
        .entries = disk->bitmap_data
    };
    if (BitMap_get(&bmap, block_num, 0) == block_num)
        return -1;
    
    void * blocks_start = disk->bitmap_data + disk->header->bitmap_entries;
    memcpy(dest, blocks_start + block_num * BLOCK_SIZE, BLOCK_SIZE);
    return 0;
}

int DiskDriver_writeBlock(DiskDriver* disk, void* src, int block_num) {
    if (block_num >= disk->header->num_blocks || block_num < 0)
        return -1;
    
    if (block_num == disk->header->first_free_block) 
        disk->header->first_free_block = DiskDriver_getFreeBlock(disk, block_num + 1);

    BitMap bmap = {
        .num_bits = disk->header->bitmap_blocks,
        .entries = disk->bitmap_data
    };
    if (BitMap_get(&bmap, block_num, 0) == block_num)
        disk->header->free_blocks --;

    if (BitMap_set(&bmap, block_num, 1) == -1)
        return -1;

    void * blocks_start = disk->bitmap_data + disk->header->bitmap_entries;
    memcpy(blocks_start + block_num * BLOCK_SIZE, src, BLOCK_SIZE);
    return 0;
}

int DiskDriver_freeBlock(DiskDriver* disk, int block_num) {
    if (block_num >= disk->header->num_blocks || block_num < 0)
        return -1;

    BitMap bmap = {
        .num_bits = disk->header->bitmap_blocks,
        .entries = disk->bitmap_data
    };
    if (BitMap_set(&bmap, block_num, 0) == -1)
        return -1;

    disk->header->free_blocks ++;
    disk->header->first_free_block = block_num < disk->header->first_free_block ? 
                                        block_num : disk->header->first_free_block;
    return 0;
}

int DiskDriver_getFreeBlock(DiskDriver* disk, int start) {
    if (start >= disk->header->num_blocks)
        return -1;
    
    BitMap bmap = {
        .num_bits = disk->header->bitmap_blocks,
        .entries = disk->bitmap_data
    };
    return BitMap_get(&bmap, start, 0);
}

int DiskDriver_flush(DiskDriver* disk) {
    int ret;
    int zone_size = sizeof(DiskHeader) + disk->header->bitmap_entries +  
                        disk->header->num_blocks * BLOCK_SIZE;
    ret = msync(disk->header, zone_size, MS_ASYNC);
    if (ret == -1)
        return -1;
    return 0;
}
