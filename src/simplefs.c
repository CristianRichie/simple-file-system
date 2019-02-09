#include "simplefs.h"


DirectoryHandle* SimpleFS_init(SimpleFS* fs, DiskDriver* disk) {

    fs->disk = disk;

    FirstDirectoryBlock * first_dir_block = malloc(BLOCK_SIZE);
    int ret = DiskDriver_readBlock(disk, first_dir_block, 0);
    if (ret == -1)
        return NULL;

    DirectoryHandle * dir_handle = malloc(sizeof(DirectoryHandle));
    dir_hanlde->sfs = fs;
    dir_handle->dcb = first_dir_block;
    dir_handle->directory = NULL;
    dir_handle->current_block = first_dir_block->header;
    dir_handle->pos_in_dir = 0;
    dir_handle->pos_in_block = 0;

    return dir_handle;
}

void SimpleFS_format(SimpleFS* fs);

FileHandle* SimpleFS_createFile(DirectoryHandle* d, const char* filename);

int SimpleFS_readDir(char** names, DirectoryHandle* d);

FileHandle* SimpleFS_openFile(DirectoryHandle* d, const char* filename);

int SimpleFS_close(FileHandle* f);

int SimpleFS_write(FileHandle* f, void* data, int size);

int SimpleFS_read(FileHandle* f, void* data, int size);

int SimpleFS_seek(FileHandle* f, int pos);

int SimpleFS_changeDir(DirectoryHandle* d, char* dirname);

int SimpleFS_mkDir(DirectoryHandle* d, char* dirname);

int SimpleFS_remove(SimpleFS* fs, char* filename);
