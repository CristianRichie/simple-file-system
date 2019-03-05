#include <simplefs.h>

#include <stdlib.h>
#include <string.h>

DirectoryHandle* SimpleFS_init(SimpleFS* fs, DiskDriver* disk) {
    return NULL;
}

void SimpleFS_format(SimpleFS* fs) {
    return;
}

FileHandle* SimpleFS_createFile(DirectoryHandle* d, const char* filename) {
    return NULL;
}

int SimpleFS_readDir(char** names, DirectoryHandle* d) {
    return -1;
}

FileHandle* SimpleFS_openFile(DirectoryHandle* d, const char* filename) {
    return NULL;
}

int SimpleFS_close(FileHandle* f) {
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
