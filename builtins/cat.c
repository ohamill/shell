#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

enum {
    BUFSZ = 512
};

void readFile(int);

int main(int argc, char *argv[]) {
    int i, fd;

    if (argc == 1) {
        readFile(0);
    } else {
        for (i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-") == 0) {
                readFile(0);
            } else {
                fd = open(argv[i], O_RDONLY);
                readFile(fd);
                close(fd);
            }
        }
    }
}

void readFile(int fd) {
    int n;
    char buf[BUFSZ];
    
    while ((n = read(fd, buf, BUFSZ)) > 0) {
        write(1, buf, n);
    }
    if (n < 0) {
        perror("read");
    }
    printf("\n");
}