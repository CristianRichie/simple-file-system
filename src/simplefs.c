#include <simplefs.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static int SimpleFS_exists(DirectoryHandle* d, const char* filename) {
    
    int max_entries_fdb = (BLOCK_SIZE - sizeof(BlockHeader) -
                           sizeof(FileControlBlock) - sizeof(int)) / sizeof(int);
    int max_entries_db = (BLOCK_SIZE - sizeof(BlockHeader)) / sizeof(int);

    FirstDirectoryBlock* fdb = d->dcb;
    int entries = fdb->num_entries;
    int idx, ret;

    for (idx = 0; idx < max_entries_fdb; idx++) {
        if (idx >= entries)
            return 0;
        
        FirstFileBlock ffb;
        int block_num = fdb->file_blocks[idx];
        ret = DiskDriver_readBlock(d->sfs->disk, &ffb, block_num);
        if (ret == -1)
            return 0;
        
        if (strncmp(ffb.fcb.name, filename, 128) == 0)
            return ffb.header.block_in_disk;
    }

    entries -= idx;

    if (entries == 0)
        return 0;

    DirectoryBlock db;
    ret = DiskDriver_readBlock(d->sfs->disk, &db, fdb->header.next_block);
    if (ret == -1) {
        printf("HERE\n");
        if (DEBUG) printf("[SFS - exists] Cannot read from disk.\n");
        return 0;
    }
    for (idx = 0; idx < max_entries_db; idx++) {
        if (idx >= entries)
            return 0;

        FirstFileBlock ffb;
        int block_num = db.file_blocks[idx];
        ret = DiskDriver_readBlock(d->sfs->disk, &ffb, block_num);
        if (ret == -1)
            return 0;
        
        if (strncmp(ffb.fcb.name, filename, 128) == 0)
            return ffb.header.block_in_disk;
    }
    entries -= idx;
    while (db.header.next_block != -1) {
        ret = DiskDriver_readBlock(d->sfs->disk, &db, db.header.next_block);
        if (ret == -1) {
            if (DEBUG) printf("[SFS - exists] Cannot read from disk.\n");
            return 0;
        }
        for (idx = 0; idx < max_entries_db; idx++) {
            if (idx >= entries)
                return 0;

            FirstFileBlock ffb;
            int block_num = db.file_blocks[idx];
            ret = DiskDriver_readBlock(d->sfs->disk, &ffb, block_num);
            if (ret == -1)
                return 0;

            if (strncmp(ffb.fcb.name, filename, 128) == 0)
                return ffb.header.block_in_disk;
        }
        entries -= idx;
    }
    return 0;
}


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
    first_directory_block.header.block_in_disk = 0;

    first_directory_block.fcb.directory_block = -1;
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

int SimpleFS_createFile(DirectoryHandle* d, const char* filename) {

    if (SimpleFS_exists(d, filename)) {
        if (DEBUG) printf("[SFS - createFile] File already exists.\n");
        return -1;
    }

    int max_entries_fdb = (BLOCK_SIZE - sizeof(BlockHeader) -
                           sizeof(FileControlBlock) - sizeof(int)) / sizeof(int);
    int max_entries_db = (BLOCK_SIZE - sizeof(BlockHeader)) / sizeof(int);

    FirstDirectoryBlock* fdb = d->dcb;

    int ret;
    int free_block = DiskDriver_getFreeBlock(d->sfs->disk, 0);
    if (free_block == -1) {
        if (DEBUG) printf("[SFS - createFile] No free block.\n");
        return -1;
    }

    FirstFileBlock ffb = {0};
    ffb.header.previous_block = -1;
    ffb.header.next_block = -1;
    ffb.header.block_in_file = 0;
    ffb.header.block_in_disk = free_block;

    ffb.fcb.directory_block = fdb->header.block_in_disk;
    ffb.fcb.size_in_bytes = BLOCK_SIZE;
    ffb.fcb.size_in_blocks = 1;
    ffb.fcb.is_dir = 0;
    strncpy(ffb.fcb.name, filename, 128);

    ret = DiskDriver_writeBlock(d->sfs->disk, &ffb, free_block);
    if (ret == -1) {            
        if (DEBUG) printf("[SFS - createFile] Cannot write on disk.\n");
        return -1;
    }

    if (fdb->num_entries < max_entries_fdb) {
        fdb->file_blocks[fdb->num_entries] = free_block;
        fdb->num_entries += 1;
        
        ret = DiskDriver_writeBlock(d->sfs->disk, fdb, fdb->header.block_in_disk);
        if (ret == -1) {
            if (DEBUG) printf("[SFS - createFile] Cannot write on disk.\n");
            return -1; 
        }
    }
    else {
        int entries = fdb->num_entries - max_entries_fdb;
        if (entries == 0 || entries % max_entries_db == 0) {
            int block_free_block = DiskDriver_getFreeBlock(d->sfs->disk, 0);
            if (block_free_block == -1) {
                if (DEBUG) printf("[SFS - createFile] No free block.\n");
                return -1;
            }

            DirectoryBlock new_block = {0};
            new_block.header.next_block = -1;
            new_block.header.block_in_disk = block_free_block;

            if (entries == 0) {
                new_block.header.block_in_file = fdb->header.block_in_file + 1;
                new_block.header.previous_block = fdb->header.block_in_disk;
                new_block.file_blocks[0] = free_block;
                fdb->header.next_block = block_free_block;
            }
            else {
                DirectoryBlock last_block;
                ret = DiskDriver_readBlock(d->sfs->disk, &last_block, fdb->header.next_block);
                if (ret == -1) {
                    if (DEBUG) printf("[SFS - createFile] Cannot read from disk.\n");
                    return -1;
                }
                while (last_block.header.next_block != -1) {
                    ret = DiskDriver_readBlock(d->sfs->disk, &last_block, last_block.header.next_block);
                    if (ret == -1) {
                        if (DEBUG) printf("[SFS - createFile] Cannot read from disk.\n");
                        return -1;
                    }
                }

                new_block.header.block_in_file = last_block.header.block_in_file + 1;
                new_block.header.previous_block = last_block.header.block_in_disk;
                new_block.file_blocks[0] = free_block;
                last_block.header.next_block = block_free_block;

                ret = DiskDriver_writeBlock(d->sfs->disk, &last_block, last_block.header.block_in_disk);
                if (ret == -1) {
                    if (DEBUG) printf("[SFS - createFile] Cannot write on disk.\n");
                    return -1; 
                }
            }
            ret = DiskDriver_writeBlock(d->sfs->disk, &new_block, new_block.header.block_in_disk);
            if (ret == -1) {
                if (DEBUG) printf("[SFS - createFile] Cannot write on disk.\n");
                return -1; 
            }
        }
        else {
            DirectoryBlock db;
            ret = DiskDriver_readBlock(d->sfs->disk, &db, fdb->header.next_block);
            if (ret == -1) {
                if (DEBUG) printf("[SFS - createFile] Cannot read from disk.\n");
                return -1;
            }
            while (db.header.next_block != -1) {
                ret = DiskDriver_readBlock(d->sfs->disk, &db, db.header.next_block);
                if (ret == -1) {
                    if (DEBUG) printf("[SFS - createFile] Cannot read from disk.\n");
                    return -1;
                }
            }

            db.file_blocks[entries % max_entries_db] = free_block;
            ret = DiskDriver_writeBlock(d->sfs->disk, &db, db.header.block_in_disk);
            if (ret == -1) {
                if (DEBUG) printf("[SFS - createFile] Cannot write on disk.\n");
                return -1; 
            }
        }
        fdb->num_entries += 1;
        fdb->fcb.size_in_bytes += BLOCK_SIZE;
        fdb->fcb.size_in_blocks += 1;

        ret = DiskDriver_writeBlock(d->sfs->disk, fdb, fdb->header.block_in_disk);
        if (ret == -1) {
            if (DEBUG) printf("[SFS - createFile] Cannot write on disk.\n");
            return -1; 
        }
    }
    return 0;
}

int SimpleFS_readDir(char** names, DirectoryHandle* d) {
 
    int max_entries_fdb = (BLOCK_SIZE - sizeof(BlockHeader) -
                           sizeof(FileControlBlock) - sizeof(int)) / sizeof(int);
    int max_entries_db = (BLOCK_SIZE - sizeof(BlockHeader)) / sizeof(int);

    FirstDirectoryBlock* fdb = d->dcb;
    int entries = fdb->num_entries;
    int idx, ret;

    for (idx = 0; idx < max_entries_fdb; idx++) {
        if (idx >= entries)
            return 0;
        
        FirstFileBlock ffb;
        int block_num = fdb->file_blocks[idx];
        ret = DiskDriver_readBlock(d->sfs->disk, &ffb, block_num);
        if (ret == -1)
            return -1;

        names[idx] = strndup(ffb.fcb.name, 128);
    }

    entries -= idx;
    DirectoryBlock db;
    ret = DiskDriver_readBlock(d->sfs->disk, &db, fdb->header.next_block);
    if (ret == -1) {
        if (DEBUG) printf("[SFS - createFile] Cannot read from disk.\n");
        return 0;
    }
    for (idx = 0; idx < max_entries_db; idx++) {
        if (idx >= entries)
            return 0;

        FirstFileBlock ffb;
        int block_num = db.file_blocks[idx];
        ret = DiskDriver_readBlock(d->sfs->disk, &ffb, block_num);
        if (ret == -1)
            return -1;
        
        int names_idx = max_entries_fdb + idx + max_entries_db * (db.header.block_in_file - 1); 
        names[names_idx] = strndup(ffb.fcb.name, 128); 
    }
    entries -= idx;
    while (db.header.next_block != -1) {
        ret = DiskDriver_readBlock(d->sfs->disk, &db, db.header.next_block);
        if (ret == -1) {
            if (DEBUG) printf("[SFS - createFile] Cannot read from disk.\n");
            return 0;
        }
        for (idx = 0; idx < max_entries_db; idx++) {
            if (idx >= entries)
                return 0;

            FirstFileBlock ffb;
            int block_num = db.file_blocks[idx];
            ret = DiskDriver_readBlock(d->sfs->disk, &ffb, block_num);
            if (ret == -1)
                return -1;

            int names_idx = max_entries_fdb + idx + max_entries_db * (db.header.block_in_file - 1); 
            names[names_idx] = strndup(ffb.fcb.name, 128); 
        }
        entries -= idx;
    }
    return 0;
}

int SimpleFS_closeDir(DirectoryHandle* d) {
    if (d->directory != NULL)
        free(d->directory);
    free(d->dcb);
    free(d);
    return 0;
}

FileHandle* SimpleFS_openFile(DirectoryHandle* d, const char* filename) {

    int block_num = SimpleFS_exists(d, filename);
    if (block_num == 0) {
        if (DEBUG) printf("[SFS - openFile] File doesn't exists.\n");
        return NULL;
    }

    FirstFileBlock* ffb = calloc(1, sizeof(FirstFileBlock));
    int ret = DiskDriver_readBlock(d->sfs->disk, ffb, block_num);
    if (ret == -1) {
        if (DEBUG) printf("[SFS - openFile] Cannot read from disk.\n");
        free(ffb);
        return NULL;
    }

    if (ffb->fcb.is_dir == 1) {
        if (DEBUG) printf("[SFS - openFile] Cannot open a directory.\n");
        free(ffb);
        return NULL;
    }

    FileHandle* new_fh = calloc(1, sizeof(FileHandle));
    new_fh->sfs = d->sfs;
    new_fh->fcb = ffb;
    new_fh->directory = d->dcb;
    new_fh->current_block = &ffb->header;
    new_fh->pos_in_file = 0;

    return new_fh;
}

int SimpleFS_closeFile(FileHandle* f) {
    free(f->fcb);
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
    return -1;
}

int SimpleFS_changeDir(DirectoryHandle* d, char* dirname) {
    
    if (strcmp(d->dcb->fcb.name, dirname) == 0)
        return 0;

    if (strcmp(dirname, ".") == 0) 
        return 0;

    if (strcmp(dirname, "..") == 0) {
        if (strcmp(d->dcb->fcb.name, "/") == 0)
            return 0;

        free(d->dcb);
        d->dcb = d->directory;
        d->directory = NULL;
        d->current_block = &d->dcb->header;
        d->pos_in_dir = 0;
        d->pos_in_block = 0;
        
        int parent_block = d->dcb->fcb.directory_block;
        if (parent_block != -1) {
            FirstDirectoryBlock* fdb = calloc(1, sizeof(FirstDirectoryBlock));
            int ret = DiskDriver_readBlock(d->sfs->disk, fdb, parent_block);
            if (ret == -1) {
                if (DEBUG) printf("[SFS - changeDir] Cannot read from disk.\n");
                free(fdb);
                return -1;
            }

            d->directory = fdb;
        }

        return 0;
    }

    if (strcmp(dirname, "/") == 0) {
        free(d->dcb);    
        free(d->directory);   

        d->directory = NULL;

        FirstDirectoryBlock* fdb = calloc(1, sizeof(FirstDirectoryBlock));
        int ret = DiskDriver_readBlock(d->sfs->disk, fdb, 0);
        if (ret == -1) {
            if (DEBUG) printf("[SFS - changeDir] Cannot read from disk.\n");
            free(fdb);
            return -1;
        }

        d->dcb = fdb;
        d->current_block = &fdb->header;
        d->pos_in_dir = 0;
        d->pos_in_block = 0;

        return 0;
    }

    int dir_block;
    if ((dir_block = SimpleFS_exists(d, dirname))) {
        if (d->directory != NULL)
            free(d->directory);
        d->directory = d->dcb;

        FirstDirectoryBlock* fdb = calloc(1, sizeof(FirstDirectoryBlock));
        int ret = DiskDriver_readBlock(d->sfs->disk, fdb, dir_block);
        if (ret == -1) {
            if (DEBUG) printf("[SFS - changeDir] Cannot read from disk.\n");
            free(fdb);
            return -1;
        }

        d->dcb = fdb;
        d->current_block = &fdb->header;
        d->pos_in_dir = 0;
        d->pos_in_block = 0;

        return 0;
    }

    if (DEBUG) printf("[SFS - changeDir] Directory doesn't exists.\n");
    return -1;
}

int SimpleFS_mkDir(DirectoryHandle* d, char* dirname) {

    if (SimpleFS_exists(d, dirname)) {
        if (DEBUG) printf("[SFS - mkDir] Directory already exists.\n");
        return -1;
    }

    int max_entries_fdb = (BLOCK_SIZE - sizeof(BlockHeader) -
                           sizeof(FileControlBlock) - sizeof(int)) / sizeof(int);
    int max_entries_db = (BLOCK_SIZE - sizeof(BlockHeader)) / sizeof(int);

    FirstDirectoryBlock* fdb = d->dcb;

    int ret;
    int free_block = DiskDriver_getFreeBlock(d->sfs->disk, 0);
    if (free_block == -1) {
        if (DEBUG) printf("[SFS - mkDir] No free block.\n");
        return -1;
    }

    FirstDirectoryBlock new_fdb = {0};
    new_fdb.header.previous_block = -1;
    new_fdb.header.next_block = -1;
    new_fdb.header.block_in_file = 0;
    new_fdb.header.block_in_disk = free_block;

    new_fdb.fcb.directory_block = fdb->header.block_in_disk;
    new_fdb.fcb.size_in_bytes = BLOCK_SIZE;
    new_fdb.fcb.size_in_blocks = 1;
    new_fdb.fcb.is_dir = 1;
    strncpy(new_fdb.fcb.name, dirname, 128);

    new_fdb.num_entries = 0;

    ret = DiskDriver_writeBlock(d->sfs->disk, &new_fdb, free_block);
    if (ret == -1) {            
        if (DEBUG) printf("[SFS - mkDir] Cannot write on disk.\n");
        return -1;
    }

    if (fdb->num_entries < max_entries_fdb) {
        fdb->file_blocks[fdb->num_entries] = free_block;
        fdb->num_entries += 1;
        
        ret = DiskDriver_writeBlock(d->sfs->disk, fdb, fdb->header.block_in_disk);
        if (ret == -1) {
            if (DEBUG) printf("[SFS - mkDir] Cannot write on disk.\n");
            return -1; 
        }
    }
    else {
        int entries = fdb->num_entries - max_entries_fdb;
        if (entries == 0 || entries % max_entries_db == 0) {
            int block_free_block = DiskDriver_getFreeBlock(d->sfs->disk, 0);
            if (block_free_block == -1) {
                if (DEBUG) printf("[SFS - mkDir] No free block.\n");
                return -1;
            }

            DirectoryBlock new_block = {0};
            new_block.header.next_block = -1;
            new_block.header.block_in_disk = block_free_block;

            if (entries == 0) {
                new_block.header.block_in_file = fdb->header.block_in_file + 1;
                new_block.header.previous_block = fdb->header.block_in_disk;
                new_block.file_blocks[0] = free_block;
                fdb->header.next_block = block_free_block;
            }
            else {
                DirectoryBlock last_block;
                ret = DiskDriver_readBlock(d->sfs->disk, &last_block, fdb->header.next_block);
                if (ret == -1) {
                    if (DEBUG) printf("[SFS - mkDir] Cannot read from disk.\n");
                    return -1;
                }
                while (last_block.header.next_block != -1) {
                    ret = DiskDriver_readBlock(d->sfs->disk, &last_block, last_block.header.next_block);
                    if (ret == -1) {
                        if (DEBUG) printf("[SFS - mkDir] Cannot read from disk.\n");
                        return -1;
                    }
                }

                new_block.header.block_in_file = last_block.header.block_in_file + 1;
                new_block.header.previous_block = last_block.header.block_in_disk;
                new_block.file_blocks[0] = free_block;
                last_block.header.next_block = block_free_block;

                ret = DiskDriver_writeBlock(d->sfs->disk, &last_block, last_block.header.block_in_disk);
                if (ret == -1) {
                    if (DEBUG) printf("[SFS - mkDir] Cannot write on disk.\n");
                    return -1; 
                }
            }
            ret = DiskDriver_writeBlock(d->sfs->disk, &new_block, new_block.header.block_in_disk);
            if (ret == -1) {
                if (DEBUG) printf("[SFS - mkDir] Cannot write on disk.\n");
                return -1; 
            }
        }
        else {
            DirectoryBlock db;
            ret = DiskDriver_readBlock(d->sfs->disk, &db, fdb->header.next_block);
            if (ret == -1) {
                if (DEBUG) printf("[SFS - mkDir] Cannot read from disk.\n");
                return -1;
            }
            while (db.header.next_block != -1) {
                ret = DiskDriver_readBlock(d->sfs->disk, &db, db.header.next_block);
                if (ret == -1) {
                    if (DEBUG) printf("[SFS - mkDir] Cannot read from disk.\n");
                    return -1;
                }
            }

            db.file_blocks[entries % max_entries_db] = free_block;
            ret = DiskDriver_writeBlock(d->sfs->disk, &db, db.header.block_in_disk);
            if (ret == -1) {
                if (DEBUG) printf("[SFS - mkDir] Cannot write on disk.\n");
                return -1; 
            }
        }
        fdb->num_entries += 1;
        fdb->fcb.size_in_bytes += BLOCK_SIZE;
        fdb->fcb.size_in_blocks += 1;

        ret = DiskDriver_writeBlock(d->sfs->disk, fdb, fdb->header.block_in_disk);
        if (ret == -1) {
            if (DEBUG) printf("[SFS - mkDir] Cannot write on disk.\n");
            return -1; 
        }
    }
    return 0;
}

int SimpleFS_remove(DirectoryHandle* d, char* filename) {
    return -1;
}
