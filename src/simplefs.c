#include <simplefs.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


DirectoryHandle* SimpleFS_init(SimpleFS* fs, DiskDriver* disk) {

    fs->disk = disk;

    FirstDirectoryBlock* first_directory_block = calloc(1, sizeof(FirstDirectoryBlock));
    int ret = DiskDriver_readBlock(disk, first_directory_block, 0);
    if (ret == -1) {
        if (DEBUG) printf("[SFS - init] Formatting disk.\n");
        SimpleFS_format(fs);
        int ret = DiskDriver_readBlock(disk, first_directory_block, 0);
        if (ret == -1)
            return NULL;
    }

    DirectoryHandle* dh = malloc(sizeof(DirectoryHandle));
    dh->sfs = fs;
    dh->dcb = first_directory_block;
    dh->directory = NULL;
    dh->current_block = &first_directory_block->header;
    dh->pos_in_dir = 0;
    dh->pos_in_block = 0;

    return dh;
}

void SimpleFS_format(SimpleFS* fs) {
    
    fs->disk->header->free_blocks = fs->disk->header->num_blocks;
    fs->disk->header->first_free_block = 0;
    bzero(fs->disk->bitmap_data, fs->disk->header->bitmap_entries);
    
    FirstDirectoryBlock first_directory_block = {0};

    first_directory_block.header.previous_block = -1;
    first_directory_block.header.next_block = -1;
    first_directory_block.header.block_in_file = 0;

    first_directory_block.fcb.directory_block = -1;
    first_directory_block.fcb.block_in_disk = 0;
    first_directory_block.fcb.size_in_bytes = BLOCK_SIZE; 
    first_directory_block.fcb.size_in_blocks = 1;
    first_directory_block.fcb.is_dir = 1;
    strncpy(first_directory_block.fcb.name, "/", 128);

    first_directory_block.num_entries = 0;
    
    int ret = DiskDriver_writeBlock(fs->disk, &first_directory_block, 0);
    if (ret == -1) {
        if (DEBUG) printf("[SFS - init] Unable to format disk.\n");
        return;
    }
}

FileHandle* SimpleFS_createFile(DirectoryHandle* d, const char* filename) {
    return NULL;
}

int SimpleFS_readDir(char** names, DirectoryHandle* d) {
    return -1;
}

int SimpleFS_closeDir(DirectoryHandle* d) {
    return -1;
}

FileHandle* SimpleFS_openFile(DirectoryHandle* d, const char* filename) {
    return NULL;
}

int SimpleFS_closeFile(FileHandle* f) {
    return -1;
}

int SimpleFS_write(FileHandle* f, void* data, int size) {
    return -1;
}

int SimpleFS_read(FileHandle* f, void* data, int size) {
    return -1;
}

int SimpleFS_seek(FileHandle* f, int pos) {
    return -1;
}

int SimpleFS_changeDir(DirectoryHandle* d, char* dirname) {
    return -1;
}

int SimpleFS_mkDir(DirectoryHandle* d, char* dirname) {
    return -1;
}

int SimpleFS_remove(SimpleFS* fs, char* filename) {
    return -1;
}
