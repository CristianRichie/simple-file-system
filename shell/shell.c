#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <simplefs.h>

#define MAX_COMMAND_LENGTH 256
#define MAX_ARGUMENTS_NUM  2
#define MAX_INPUT_SIZE     2048

// GLOBAL VARIABLES
DiskDriver disk;
SimpleFS fs;
DirectoryHandle* current_dir = NULL;
char command[MAX_COMMAND_LENGTH];

// COMMANDS
/*
 * Format the disk.
 */
void format(int argc, char* argv[MAX_ARGUMENTS_NUM + 1]) {

    if (argc != 1) {
        printf("Usage: format\n");
        return;
    }
    SimpleFS_format(&fs);
    SimpleFS_closeDir(current_dir);
    current_dir = SimpleFS_init(&fs, &disk);
}

/*
 * Creates an empty directory inside the current one.
 */
void mkdir(int argc, char* argv[MAX_ARGUMENTS_NUM + 1]) {

    if (argc != 2) {
        printf("Usage: mkdir <dirname>\n");
        return;
    }

    int ret = SimpleFS_mkDir(current_dir, argv[1]);
    if (ret == -1) 
        fprintf(stderr, "An error occurred in creating new directory.\n");
}

/*
 * Enables to write inside an existing file.
 */
void write(int argc, char* argv[MAX_ARGUMENTS_NUM + 1]) {

    if (argc != 2) {
        printf("Usage: write <filename>\n");
        return;
    }
    FileHandle* fh = SimpleFS_openFile(current_dir, argv[1]);
    if (fh == NULL) {
        fprintf(stderr, "An error occurred in opening file.\n");
        return;
    }

    int starting_point, ret;
    printf("Enter starting point: ");
    scanf("%d", &starting_point); 
    getchar();

    ret = SimpleFS_seek(fh, starting_point);
    if (ret == -1) {
        fprintf(stderr, "An error occurred in seeking in file.\n");
        SimpleFS_closeFile(fh);
        return;
    }

    printf("Enter text:\n");
    char* str = calloc(MAX_INPUT_SIZE, sizeof(char));
    fgets(str, MAX_INPUT_SIZE, stdin);
    str[strlen(str) - 1] = 0x00;
    ret = SimpleFS_write(fh, str, strlen(str));
    if (ret == -1) {
        fprintf(stderr, "An error occurred in writing in file.\n");
        free(str);
        SimpleFS_closeFile(fh);
        return;
    }

    free(str);
    SimpleFS_closeFile(fh);
}

/*
 * Prints the content of the given file, it must exists.
 */
void cat(int argc, char* argv[MAX_ARGUMENTS_NUM + 1]) {

    if (argc != 2) {
        printf("Usage: cat <filename>\n");
        return;
    }
    FileHandle* fh = SimpleFS_openFile(current_dir, argv[1]);
    if (fh == NULL) {
        fprintf(stderr, "An error occurred in opening file.\n");
        return;
    }

    int file_size = fh->fcb->fcb.size_in_bytes;
    char* str = calloc(file_size + 1, sizeof(char));
    int ret = SimpleFS_read(fh, str, file_size);
    if (ret == -1) {
        fprintf(stderr, "An error occurred in reading from file.\n");
        free(str);
        SimpleFS_closeFile(fh);
        return;
    }

    printf("%s\n", str);

    free(str);
    SimpleFS_closeFile(fh);
}

/*
 * Creates an exmpty file inside the current directory..
 */
void touch(int argc, char* argv[MAX_ARGUMENTS_NUM + 1]) {

    if (argc != 2) {
        printf("Usage: touch <filename>\n");
        return;
    }

    int ret = SimpleFS_createFile(current_dir, argv[1]);
    if (ret == -1) 
        fprintf(stderr, "An error occurred in creating new file.\n");
}

/*
 * Set the current directory to the given one if it exists.
 */
void cd(int argc, char* argv[MAX_ARGUMENTS_NUM + 1]) {

    if (argc != 2) {
        printf("Usage: cd <dirname>\n");
        return;
    }

    int ret = SimpleFS_changeDir(current_dir, argv[1]);
    if (ret == -1) 
        fprintf(stderr, "An error occurred in changing directory.\n");
}

/*
 * List the current directory content.
 */
void ls(int argc, char* argv[MAX_ARGUMENTS_NUM + 1]) {

    if (argc != 1) {
        printf("Usage: ls\n");
        return;
    }

    int i;
    char** names = calloc(current_dir->dcb->num_entries, sizeof(char*));
    int ret = SimpleFS_readDir(names, current_dir);
    if (ret == -1) {
        fprintf(stderr, "An error occurred while listing files and dirs.\n");
        for (i = 0; i < current_dir->dcb->num_entries; i++) 
            free(names[i]);   
        free(names);
        return;
    }

    for (i = 0; i < current_dir->dcb->num_entries; i++) {
        
        FileHandle* something = SimpleFS_openFile(current_dir, names[i]);
        if (something == NULL) printf("dir: ");
        else { 
            printf("file: ");
            SimpleFS_closeFile(something);
        }
        
        printf("%s\n", names[i]);
    }

    for (i = 0; i < current_dir->dcb->num_entries; i++) 
        free(names[i]);   
    free(names);
}

/*
 * Prints the directory tree starting from the current one.
 */
void tree_aux(DirectoryHandle* d, FileHandle* f, int lvl, int is_dir) {
    
    int i;
    for (i = 0; i < lvl * 2 - 2; i++) printf(" ");
    if (lvl > 0) printf("|-");
    if (is_dir == 0) {
        printf("%s\n", f->fcb->fcb.name);
        return;
    }
    else {
        printf("%s\n", d->dcb->fcb.name);

        char** names = calloc(d->dcb->num_entries, sizeof(char*));
        int ret = SimpleFS_readDir(names, d);
        if (ret == -1) {
            fprintf(stderr, "An error occurred while listing files and dirs.\n");
            return;
        }

        int i;
        for (i = 0; i < d->dcb->num_entries; i++) {
         
            FileHandle* fh = SimpleFS_openFile(d, names[i]);
            if (fh == NULL) {
                SimpleFS_changeDir(d, names[i]);
                tree_aux(d, NULL, lvl + 1, 1);
                SimpleFS_changeDir(d, "..");
            }
            else { 
                tree_aux(NULL, fh, lvl + 1, 0);
                SimpleFS_closeFile(fh);
            }
        }

        for (i = 0; i < d->dcb->num_entries; i++) 
            free(names[i]);   
        free(names);

    }
}

void tree(int argc, char* argv[MAX_ARGUMENTS_NUM + 1]) {

    if (argc != 1) {
        printf("Usage: tree\n");
        return;
    }
    DirectoryHandle* tmp = current_dir;
    tree_aux(tmp, NULL, 0, 1);
}

/*
 * Removes a file or an empty dir from the file system.
 */
void rm(int argc, char* argv[MAX_ARGUMENTS_NUM + 1]) {

    if (argc != 2) {
        printf("Usage: rm <filename>\n");
        return;
    }

    int ret = SimpleFS_remove(current_dir, argv[1]);
    if (ret == -1) 
        fprintf(stderr, "An error occurred in removing.\n");
}

/*
 * Removes a dir even if it is not empty.
 */
void rmf_aux(DirectoryHandle* p, char* name) {
    
    int ret = SimpleFS_remove(p, name);
    if (ret == -1) {
        ret = SimpleFS_changeDir(p, name);
        if (ret == -1) {
            fprintf(stderr, "An error occurred while changing dir.\n");
            return; 
        }

        int i;
        char** names = calloc(p->dcb->num_entries, sizeof(char*));
        int ret = SimpleFS_readDir(names, p);
        if (ret == -1) {
            fprintf(stderr, "An error occurred while listing files and dirs.\n");
            for (i = 0; i < p->dcb->num_entries; i++) 
                free(names[i]);   
            free(names);
            return;
        }

        int entries = p->dcb->num_entries;
        for (i = 0; i < entries; i++) { 
            rmf_aux(p, names[i]);
        }

        SimpleFS_changeDir(p, "..");

        for (i = 0; i < entries; i++) 
            free(names[i]);   
        free(names);

        ret = SimpleFS_remove(p, name);
        if (ret == -1)      
            fprintf(stderr, "An error occurred in removing.\n");
    }
}

void rmf(int argc, char* argv[MAX_ARGUMENTS_NUM + 1]) {

    if (argc != 2) {
        printf("Usage: rmf <dirname>\n");
        return;
    }
    
    if (strcmp(argv[1], "/") == 0) {
        printf("Maybe another day\n");
        return;
    }

    DirectoryHandle* parent = current_dir;
    rmf_aux(parent, argv[1]);
}

void help(int argc, char* argv[MAX_ARGUMENTS_NUM + 1]) {
    
    if (argc != 1) {
        printf("Usage: help\n");
        return;
    }
    printf("Available commands:\n");
    printf("format: formats the disk.\n");
    printf("mkdir: create a new directory in the current one.\n");
    printf("write: enables to write into an existing file.\n");
    printf("cat: prints out the content of an existing file.\n");
    printf("touch: create a new empty file in the current directory.\n");
    printf("cd: change the current directory.\n");
    printf("ls: list all the files in the current directory.\n");
    printf("tree: print the tree starting from the current directory.\n");
    printf("rm: remove a file or an empty directory.\n");
    printf("rmf: remove a file or a not empty directory.\n");
    printf("help: command inception.\n");
    printf("exit: exit the shell.\n");
}


void do_command_loop(void) {

    do {
        char* argv[MAX_ARGUMENTS_NUM + 1] = {NULL};

        printf("%s> ", current_dir->dcb->fcb.name);
        fgets(command, MAX_COMMAND_LENGTH, stdin);
        
        argv[0] = strtok(command, " ");
        argv[1] = strtok(NULL, "\n");
        int argc = (argv[1] == NULL) ? 1 : 2;
        if (argv[1] == NULL) 
            argv[0][strlen(argv[0]) - 1] = 0x00;

        if (strcmp(argv[0], "format") == 0) {
            format(argc, argv);
        }
        else if (strcmp(argv[0], "mkdir") == 0) {
            mkdir(argc, argv); 
        }
        else if (strcmp(argv[0], "write") == 0) {
            write(argc, argv); 
        }
        else if (strcmp(argv[0], "cat") == 0) {
            cat(argc, argv); 
        }
        else if (strcmp(argv[0], "touch") == 0) {
            touch(argc, argv); 
        }
        else if (strcmp(argv[0], "cd") == 0) {
            cd(argc, argv); 
        }
        else if (strcmp(argv[0], "ls") == 0) {
            ls(argc, argv); 
        }
        else if (strcmp(argv[0], "tree") == 0) {
            tree(argc, argv); 
        }
        else if (strcmp(argv[0], "rm") == 0) {
            rm(argc, argv); 
        }
        else if (strcmp(argv[0], "rmf") == 0) {
            rmf(argc, argv); 
        }
        else if (strcmp(argv[0], "help") == 0) {
            help(argc, argv); 
        }
        else if (strcmp(argv[0], "exit") == 0) {
            printf("That's all folks!\n");
            SimpleFS_closeDir(current_dir);
            exit(EXIT_SUCCESS);
        }
        else {
            printf("Invalid command\n");
        }
    } while(1);
}


int main(int argc, char * argv[]) {
 
    DiskDriver_init(&disk, "disk.img", 2048);
    
    current_dir = SimpleFS_init(&fs, &disk);
    if (current_dir == NULL) {
        fprintf(stderr, "[ERROR] Cannot initialize file system.\n");
        exit(EXIT_FAILURE);
    }

    printf("Welcome, run 'help' to see available commands!\n");
    do_command_loop();
    return 0;
}

