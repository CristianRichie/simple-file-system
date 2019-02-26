#include <simplefs.h>

#include <stdlib.h>
#include <string.h>

voi SimpleFS_init(SimpleFS* fs, DiskDriver* disk) {

    fs->disk = disk;

    FirstDirectoryBlock first_dir_block;
    int ret = DiskDriver_readBlock(disk, &first_dir_block, 0);
    if (ret == -1) 
        return NULL;

    fs->current_directory_block = &first_dir_block;

    /*
    DirectoryHandle * dir_handle = malloc(sizeof(DirectoryHandle));
    if (!dir_handle) 
        return NULL;
    dir_handle->sfs = fs;
    dir_handle->dcb = &first_dir_block;
    dir_handle->directory = NULL;
    dir_handle->current_block = &first_dir_block.header;
    dir_handle->pos_in_dir = 0;
    dir_handle->pos_in_block = 0;

    return dir_handle;
    */
}

void SimpleFS_format(SimpleFS* fs) {
    int bmap_size = fs->disk->header->bitmap_entries;
    bzero(fs->disk->bitmap_data, bmap_size);
    fs->disk->header->free_blocks = fs->disk->header->num_blocks;
    fs->disk->header->first_free_block = 0;

    FirstDirectoryBlock fdb;
    fdb.header.previous_block = -1;
    fdb.header.next_block = -1;
    fdb.header.block_in_file = 0;
    fdb.fcb.directory_block = -1;
    fdb.fcb.block_in_disk = 0;
    strcpy(fdb.fcb.name, "/");
    fdb.fcb.size_in_bytes = BLOCK_SIZE;
    fdb.fcb.size_in_blocks = 1;
    fdb.fcb.is_dir = 1;
    fdb.num_entries = 0;

    int ret = DiskDriver_writeBlock(fs->disk, &fdb, 0);
    if (ret == -1) 
        return;

    fs->current_directory_block = &fdb;
}

FileHandle* SimpleFS_createFile(SimpleFS* fs, const char* filename) {
    return NULL;
}

int SimpleFS_readDir(char** names, DirectoryHandle* d) {
    return -1;
}

FileHandle* SimpleFS_openFile(SimpleFS* fs, const char* filename) {
    return NULL;
}

int SimpleFS_close(FileHandle* f) {
    if (!f) 
        return -1;
    free(f);
    return 0;
}

int SimpleFS_write(FileHandle* f, void* data, int size) {
    return -1;
}

int SimpleFS_read(FileHandle* f, void* data, int size) {
    return -1;
}

int SimpleFS_seek(FileHandle* f, int pos) {
    if (!f) 
        return -1;

    int ret;

    int data_size = BLOCK_SIZE - sizeof(BlockHeader);
    int target_block = pos % data_size;
    if (target_block == f->current_block->block_in_file) {
        f->pos_in_file = pos;
        return pos;
    }
    else if (target_block < f->current_block->block_in_file) {
        BlockHeader future_header;
        do {
            int prev_block = f->current_block->previous_block;
            FirstFileBlock block;
            ret = DiskDriver_readBlock(f->sfs->disk, &block, prev_block);
            if (ret == -1)
                return -1;
            future_header = block.header;
            
        } while(future_header.block_in_file != target_block);
        f->pos_in_file = pos;
        f->current_block = &future_header;
        return pos;
    }
    else {
        BlockHeader future_header;
        do {
            int next_block = f->current_block->next_block;
            if (next_block == -1)
                return -1;
            FirstFileBlock block;
            ret = DiskDriver_readBlock(f->sfs->disk, &block, next_block);
            if (ret == -1)
                return -1;
            future_header = block.header;
        } while(future_header.block_in_file != target_block);
        f->pos_in_file = pos;
        f->current_block = &future_header;
        return pos;
    }
}

int SimpleFS_changeDir(SimpleFS* fs, char* dirname) {
    return -1;
}

int SimpleFS_mkDir(SimpleFS* fs, char* dirname) {
    if (!fs) 
        return -1;

    int free_block = DiskDriver_getFreeBlock(fs->disk, fs->current_directory_block->fcb.block_in_disk);
    if (free_block == -1)
        return -1;

    FirstDirectoryBlock fdb;
    fdb.header.previous_block = -1;
    fdb.header.next_block = -1;
    fdb.header.block_in_file = 0;
    fdb.fcb.directory_block = fs->current_directory_block->fcb.block_in_disk;
    fdb.fcb.block_in_disk = free_block;
    strcpy(fdb.fcb.name, dirname);
    fdb.fcb.size_in_bytes = BLOCK_SIZE;
    fdb.fcb.size_in_blocks = 1;
    fdb.fcb.is_dir = 1;
    fdb.num_entries = 0;

    return DiskDriver_writeBlock(fs->disk, &fdb, free_block);
}

int SimpleFS_remove(SimpleFS* fs, char* filename) {
    return -1;
}
