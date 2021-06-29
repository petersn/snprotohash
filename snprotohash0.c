#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "snprotohash0.h"

#define READ_SIZE (1024 * 1024)

uint8_t* buffer;
snprotohash0_t sh;

void process(const char* name, int print_spaces, int fd) {
    snprotohash0_t sh;
    snprotohash0_init(&sh, NULL);
    while (1) {
        ssize_t amount = read(fd, buffer, READ_SIZE);
        if (amount < 0) {
            perror("Read failed");
            exit(1);
        }
        if (amount == 0)
            break;
        snprotohash0_update(&sh, amount, buffer);
    }
    uint8_t hash_result[32];
    snprotohash0_end(&sh, hash_result);
    for (int i = 0; i < 32; i++)
        printf("%02x", hash_result[i]);
    if (print_spaces)
        printf("  ");
    printf("%s", name);
    printf("\n");
}

int main(int argc, char** argv) {
    buffer = malloc(READ_SIZE);

    if (argc == 1) {
        process("", 0, STDIN_FILENO);
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        int fd = open(argv[i], O_RDONLY);
        if (fd < 0) {
            perror("Open failed");
            return 1;
        }
        process(argv[i], 1, fd);
        close(fd);
    }

    return 0;
}
