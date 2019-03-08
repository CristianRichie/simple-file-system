#include <simplefs.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
   
    SimpleFS fs;
    DiskDriver disk;
    int ret;

    DiskDriver_init(&disk, "disk_sfs_test.img", 256);
    DirectoryHandle* current_dir = SimpleFS_init(&fs, &disk);

    printf("Current directory: %s\n", current_dir->dcb->fcb.name);

    ret = SimpleFS_createFile(current_dir, "test0.txt");
    if (ret == -1) {
        printf("Something went wrong while creating file.\n");
        exit(EXIT_FAILURE);
    }

    ret = SimpleFS_createFile(current_dir, "test1.txt");
    if (ret == -1) {
        printf("Something went wrong while creating file.\n");
        exit(EXIT_FAILURE);
    }

    ret = SimpleFS_createFile(current_dir, "test0.txt");
    if (ret != -1) {
        printf("Created the same file twice.\n");
        exit(EXIT_FAILURE);
    }

    char** names = calloc(current_dir->dcb->num_entries, sizeof(char*));    
    ret = SimpleFS_readDir(names, current_dir);
    if (ret == -1) {
        printf("Something went wrong while listing dir files.\n");
        exit(EXIT_FAILURE); 
    }

    printf("***** LIST FILES *****\n");
    int i;
    for (i = 0; i < current_dir->dcb->num_entries; i++) 
        printf("File/Dir: %s\n", names[i]);   
    printf("**********************\n");

    FileHandle* test0_fh = SimpleFS_openFile(current_dir, "test0.txt"); 
    if (test0_fh == NULL) {
        printf("Something went wrong while opening file.\n");
        exit(EXIT_FAILURE); 
    }
    printf("Opened file: %s\n", test0_fh->fcb->fcb.name);

    FileHandle* test1_fh = SimpleFS_openFile(current_dir, "test1.txt"); 
    if (test1_fh == NULL) {
        printf("Something went wrong while opening file.\n");
        exit(EXIT_FAILURE); 
    }
    printf("Opened file: %s\n", test1_fh->fcb->fcb.name);

    for (i = 0; i < current_dir->dcb->num_entries; i++) 
        free(names[i]);   
    free(names);

    SimpleFS_closeFile(test0_fh);
    SimpleFS_closeFile(test1_fh);

    ret = SimpleFS_mkDir(current_dir, "testdir");
    if (ret == -1) {
        printf("Something went wrong while creating directory.\n");
        exit(EXIT_FAILURE); 
    }

    ret = SimpleFS_changeDir(current_dir, "testdir");
    if (ret == -1) {
        printf("Something went wrong while changing directory.\n");
        exit(EXIT_FAILURE); 
    }
    printf("Current directory: %s\n", current_dir->dcb->fcb.name);

    ret = SimpleFS_createFile(current_dir, "test0.txt");
    if (ret == -1) {
        printf("Something went wrong while creating file.\n");
        exit(EXIT_FAILURE);
    }

    names = calloc(current_dir->dcb->num_entries, sizeof(char*));    
    ret = SimpleFS_readDir(names, current_dir);
    if (ret == -1) {
        printf("Something went wrong while listing dir files.\n");
        exit(EXIT_FAILURE); 
    }

    printf("***** LIST FILES *****\n");
    for (i = 0; i < current_dir->dcb->num_entries; i++) 
        printf("File/Dir: %s\n", names[i]);   
    printf("**********************\n");

    for (i = 0; i < current_dir->dcb->num_entries; i++) 
        free(names[i]);   
    free(names);

    ret = SimpleFS_changeDir(current_dir, "..");
    if (ret == -1) {
        printf("Something went wrong while changing directory.\n");
        exit(EXIT_FAILURE); 
    }
    printf("Current directory: %s\n", current_dir->dcb->fcb.name);

    SimpleFS_closeDir(current_dir);
    printf("TEST COMPLETED. SUCCESS.\n");
    return 0; 
}
