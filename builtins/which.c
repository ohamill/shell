#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

int searchDir(char *, char *);

int main(int argc, char *argv[]) {
    char *path, *token;
    int wstatus;

    path = getenv("PATH");

    token = strtok(path, ":");
    if (searchDir(token, argv[1]) == 1) {
        printf("%s\n", token);
        exit(0);
    }
    while ((token = strtok(NULL, ":")) != NULL) {
        if (searchDir(token, argv[1]) == 1) {
            printf("%s\n", token);
            exit(0);
        }
    }
    printf("File not found\n");   
}

int searchDir(char *fileToSearch, char *fileToFind) {
    DIR *dir;
    struct dirent *d;

    dir = opendir(fileToSearch);
    while ((d = readdir(dir)) != NULL) {
        if (strcmp(d->d_name, fileToFind) == 0) {
            closedir(dir);
            return 1;
        }
    }
    closedir(dir);
    return 0;
}