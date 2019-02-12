#include <disk_driver.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char * argv[]) {

    int ret;

    DiskDriver disk_driver;
    DiskDriver_init(&disk_driver, "test_disk_driver.img", 10);
    printf("DiskDriver created.\n");
     
    int free_block = DiskDriver_getFreeBlock(&disk_driver, 0);
    printf("First free block: %d\n", free_block);

    char buffer_out[BLOCK_SIZE];
    strncpy(buffer_out, "RANDOMDATA-RANDOMDATA", sizeof(buffer_out));
    ret = DiskDriver_writeBlock(&disk_driver, (void*)buffer_out, free_block);
    if (ret == -1) {
        printf("Write on %d failed.\n", free_block);
        exit(EXIT_FAILURE);
    }
    printf("Write on %d succedeed\n", free_block);

    int next_free_block = DiskDriver_getFreeBlock(&disk_driver, 0);
    if (next_free_block == free_block) {
        printf("Written block considered free.\n");
        exit(EXIT_FAILURE);
    }
    
    ret = DiskDriver_flush(&disk_driver);
    if (ret == -1) {
        printf("Flushing failed.\n");
        exit(EXIT_FAILURE);
    }
    printf("Disk flush succedeed.\n");

    char buffer_in[BLOCK_SIZE];
    ret = DiskDriver_readBlock(&disk_driver, (void*)buffer_in, free_block);
    if (ret == -1) {
        printf("Unable to read from %d.\n", free_block);
        exit(EXIT_FAILURE);
    }
    printf("Read from %d succedeed\n", free_block);
    
    ret = strcmp(buffer_in, buffer_out);
    if (ret != 0) {
        printf("Read wrong data.\n");
        exit(EXIT_FAILURE);
    }

    ret = DiskDriver_readBlock(&disk_driver, (void*)buffer_in, next_free_block);
    if (ret != -1) {
        printf("Reading from free block.\n");
        exit(EXIT_FAILURE);
    }

    ret = DiskDriver_freeBlock(&disk_driver, free_block);
    if (ret == -1) {
        printf("Failed to free block.\n");
        exit(EXIT_FAILURE);
    }
    printf("Free block succedeed.\n");

    ret = DiskDriver_getFreeBlock(&disk_driver, 0);
    if (ret != free_block) {
        printf("Something went wrong in freeing.\n");
        exit(EXIT_FAILURE);
    }

    printf("TEST COMPLETED. SUCCESS.\n");
    return 0;
}

