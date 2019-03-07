#include <simplefs.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
   
    SimpleFS fs;
    DiskDriver disk;

    DiskDriver_init(&disk, "disk_sfs_test.img", 256);
    DirectoryHandle* root_dir = SimpleFS_init(&fs, &disk);

    printf("Root directory: %s\n", root_dir->dcb->fcb.name);

    FileHandle* test0_fh = SimpleFS_createFile(root_dir, "test0.txt");
    if (test0_fh == NULL) {
        printf("Something went wrong while creating file.\n");
        exit(EXIT_FAILURE);
    }
    printf("Created file: %s\n", test0_fh->fcb->fcb.name);

    FileHandle* test1_fh = SimpleFS_createFile(root_dir, "test1.txt");
    if (test1_fh == NULL) {
        printf("Something went wrong while creating file.\n");
        exit(EXIT_FAILURE);
    }
    printf("Created file: %s\n", test1_fh->fcb->fcb.name);

    FileHandle* null = SimpleFS_createFile(root_dir, "test0.txt");
    if (null != NULL) {
        printf("Created the same file twice.\n");
        exit(EXIT_FAILURE);
    }

    char** names = calloc(root_dir->dcb->num_entries, sizeof(char*));    
    int ret = SimpleFS_readDir(names, root_dir);
    if (ret == -1) {
        printf("Something went wrong while listing dir files.\n");
        exit(EXIT_FAILURE); 
    }

    printf("***** LIST FILES *****\n");
    int i;
    for (i = 0; i < root_dir->dcb->num_entries; i++) 
        printf("File/Dir: %s\n", names[i]);   
    printf("**********************\n");

    for (i = 0; i < root_dir->dcb->num_entries; i++) 
        free(names[i]);   
    free(names);

    SimpleFS_closeDir(root_dir);
    SimpleFS_closeFile(test0_fh);
    SimpleFS_closeFile(test1_fh);
    printf("TEST COMPLETED. SUCCESS.\n");

    return 0; 
}
