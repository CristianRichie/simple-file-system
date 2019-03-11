#include <simplefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    for (i = 0; i < current_dir->dcb->num_entries; i++) 
        free(names[i]);   
    free(names);

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

    SimpleFS_closeFile(test0_fh);

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

    char* input = calloc(1025, sizeof(char));
    memset(input, 0x61, 1024);
    ret = SimpleFS_write(test1_fh, input, 1024);
    if (ret != 1024) {
        printf("Something went wrong while writing on file.\n");
        exit(EXIT_FAILURE); 
    }
    printf("Position in file after write: %d\n", test1_fh->pos_in_file);

    ret = SimpleFS_seek(test1_fh, 0);
    if (ret != 0) {
        printf("HERE %d\n", ret);
        printf("Something went wrong while seeking.\n");
        exit(EXIT_FAILURE); 
    } 
    printf("Position in file after seek: %d\n", test1_fh->pos_in_file);

    printf("Reading from file\n");
    char* output = calloc(1025, sizeof(char));
    ret = SimpleFS_read(test1_fh, output, 1024);
    if (ret != 1024 && strcmp(input, output) != 0) {
        printf("Something went wrong while reading from file.\n");
        exit(EXIT_FAILURE); 
    }
 
    ret = SimpleFS_seek(test1_fh, 0);
    if (ret != 0) {
        printf("Something went wrong while seeking.\n");
        exit(EXIT_FAILURE); 
    } 

    ret = SimpleFS_seek(test1_fh, 600);
    if (ret != 600) {
        printf("Something went wrong while seeking.\n");
        exit(EXIT_FAILURE); 
    } 
    printf("Position in file after seek: %d\n", test1_fh->pos_in_file);

    ret = SimpleFS_seek(test1_fh, 1800);
    if (ret != 1024) {
        printf("Something went wrong while seeking.\n");
        exit(EXIT_FAILURE); 
    } 

    printf("Reading from file\n");
    int to_read = test1_fh->fcb->fcb.size_in_bytes - test1_fh->pos_in_file;
    ret = SimpleFS_read(test1_fh, output, to_read);
    if (ret != to_read) {
        printf("HERE %d\n", ret);
        printf("Something went wrong while reading from file.\n");
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

    SimpleFS_closeFile(test1_fh);

    ret = SimpleFS_remove(current_dir, "testdir");
    if (ret != -1) {
        printf("Something went wrong while removing dir.\n");
        exit(EXIT_FAILURE); 
    }

    ret = SimpleFS_remove(current_dir, "test0.txt");
    if (ret == -1) {
        printf("Something went wrong while removing file.\n");
        exit(EXIT_FAILURE); 
    }
    printf("Removed test0.txt\n");

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

    free(input);
    free(output);
    SimpleFS_closeDir(current_dir);
    printf("TEST COMPLETED. SUCCESS.\n");
    return 0; 
}
