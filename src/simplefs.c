#include "simplefs.h"

#include <stdlib.h>
#include <string.h>

DirectoryHandle* SimpleFS_init(SimpleFS* fs, DiskDriver* disk) {

    fs->disk = disk;

    FirstDirectoryBlock * first_dir_block = malloc(BLOCK_SIZE);
    if (!first_dir_block)
        return NULL;
    int ret = DiskDriver_readBlock(disk, first_dir_block, 0);
    if (ret == -1) {
        free(first_dir_block);
        return NULL;
    }

    DirectoryHandle * dir_handle = malloc(sizeof(DirectoryHandle));
    if (!dir_handle) 
        return NULL;
    dir_hanlde->sfs = fs;
    dir_handle->dcb = first_dir_block;
    dir_handle->directory = NULL;
    dir_handle->current_block = first_dir_block->header;
    dir_handle->pos_in_dir = 0;
    dir_handle->pos_in_block = 0;

    return dir_handle;
}

void SimpleFS_format(SimpleFS* fs) {
    int bmap_size = fs->disk->header->bitmap_entries;
    bzero(fs->disk->bitmap_data, bmap_size);
    fs->disk->header->free_blocks = fs->disk->header->num_blocks;
    fs->disk->header->first_free_block = 0;

    FirstDirectoryBlock * fdb = malloc(sizeof(FirstDirectoryBlock));
    if (!fdb)
        return;
    fdb->header.previous_block = -1;
    fdb->header.next_block = -1;
    fdb->header.block_in_file = 0;
    fdb->fcb.directory_block = -1;
    fdb->fcb.block_in_disk = 0;
    strcpy(fdb->fcb.name, "/");
    fdb->fcb.size_in_bytes = BLOCK_SIZE;
    fdb->fcb.size_in_blocks = 1;
    fdb->fcb.is_dir = 1;
    fdb->num_entries = 0;

    int ret = DiskDriver_writeBlock(fs->disk, fdb, 0);
    if (ret == -1) {
        free(fdb);
        return;
    }

    fs->current_directory_block = fdb;
}

FileHandle* SimpleFS_createFile(DirectoryHandle* d, const char* filename) {
    
}

int SimpleFS_readDir(char** names, DirectoryHandle* d);

FileHandle* SimpleFS_openFile(DirectoryHandle* d, const char* filename);

int SimpleFS_close(FileHandle* f);

int SimpleFS_write(FileHandle* f, void* data, int size);

int SimpleFS_read(FileHandle* f, void* data, int size);

int SimpleFS_seek(FileHandle* f, int pos);

int SimpleFS_changeDir(DirectoryHandle* d, char* dirname);

int SimpleFS_mkDir(DirectoryHandle* d, char* dirname);

int SimpleFS_remove(SimpleFS* fs, char* filename);
