#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include "./builtins.h"
#include <stdlib.h>
#include <dirent.h>

const char *builtincmds[] = {"echo", "cd", "set", "which"};
const int builtincount = 4;

void echo(char *cmd[]) {
    char *echocmds[50];
    int i, echoindex, j, n;
    char delim;

    if (echoDashOptions(cmd)) {
        i = 1;
        echoindex = 0;
        while (cmd[i] != NULL) {
            if (strcmp(cmd[i], "-d") == 0) {
                delim = *cmd[i+1];
                i++;
            } else {
                echocmds[echoindex] = cmd[i];
                echoindex++;
            }
            i++;
        }
        echocmds[echoindex] = NULL;
    } else {
        i = 0;
        while (cmd[i] != NULL) {
            echocmds[i] = cmd[i];
            i++;
        }
        echocmds[i] = NULL;
    }

    i = 1;
    while (echocmds[i] != NULL) {
        for (j = 0, n = strlen(echocmds[i]); j < n; j++) {
            if (echocmds[i][j] == delim) {
                echocmds[i][j] = '\n';
            }
        }
        printf("%s ", echocmds[i]);
        i++;
    }
    printf("\n");
}

// echoDashOptions searches the input string array for the existence of any dash options. 
// If any dash options are present, echoDashOptions returns true, else returns false
bool echoDashOptions(char *cmd[]) {
    int i = 1;
    while (cmd[i] != NULL) {
        if (strcmp(cmd[i], "-d") == 0) {
            return true;
        }
        i++;
    }
    return false;
}

void set(char *cmd[], variable *root) {
    variable *v;
    char *name, *value;
    bool readonly;

    // If user types only the keyword "set", show all variables and their current values
    if (cmd[1] == NULL) {
        getAllVariables(root);
        return;
    }

    name = malloc(strlen(cmd[1] + 1));
    value = malloc(strlen(cmd[3] + 1));
    strcpy(name, cmd[1]);
    strcpy(value, cmd[3]);

    // Validate command format
    if (strcmp(cmd[2], "=") != 0 || (cmd[4] != NULL && cmd[5] != NULL)) {
        printf("Invalid format! Usage: set varName = varValue [readonly]\n");
        return;
    }
    if (cmd[4] != NULL && strcmp(cmd[4], "readonly") != 0) {
        printf("Unrecognized command: %s\n", cmd[4]);
        return;
    }
    // Check if variable is already an environment variable
    if (isEnvironmentVariable(cmd[1])) {
        printf("Unable to overwrite environment variable\n");
        return;
    }
    
    v = doesVariableAlreadyExist(cmd[1], root);
    if (v == NULL) {
        if (cmd[4] != NULL && strcmp(cmd[4], "readonly") == 0) {
            readonly = true;
        } else {
            readonly = false;
        }
        v = createVariable(name, value, readonly);
        addVariable(v, root);
    } else {
        free(name);
        if (v->readonly) {
            printf("Cannot overwrite the value of a read-only variable!\n");
            return;
        }
        v->value = value; // Overwrite variable's existing value
    }
}

void which(char *cmd[]) {
    char *path, *pathstart, *token;
    bool fileFound;
    int i = 1;

    path = getenv("PATH");
    pathstart = malloc(strlen(path) + 1);
    strcpy(pathstart, path);

    while (cmd[i] != NULL) {
        fileFound = false;
        // Check if command is a built-in command
        for (int j = 0; j < builtincount; j++) {
            if (strcmp(cmd[i], builtincmds[j]) == 0) {
                printf("%s: shell built-in command\n", cmd[i]);
                return;
            }
        }

        token = strtok(pathstart, ":");
        if (searchDir(token, cmd[i])) {
            strcpy(pathstart, path);
            continue;
        }
        while ((token = strtok(NULL, ":")) != NULL) {
            if (searchDir(token, cmd[i])) {
                fileFound = true;
                break;
            }
        }
        if (!fileFound) {
            printf("File not found\n");
        }
        i++;
        strcpy(pathstart, path);
    }
    free(pathstart);
}

int searchDir(char *fileToSearch, char *fileToFind) {
    DIR *dir;
    struct dirent *d;

    dir = opendir(fileToSearch);
    while ((d = readdir(dir)) != NULL) {
        if (strcmp(d->d_name, fileToFind) == 0) {
            printf("%s/%s\n", fileToSearch, fileToFind);
            closedir(dir);
            return 1;
        }
    }
    closedir(dir);
    return 0;
}