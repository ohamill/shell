#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <dirent.h>
#include "./builtins.h"
#include "validation.h"

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
    char *varName, *varValue;
    bool readonly;

    if (doesUserWantToSeeAllVariables(cmd[1])) {
        getAllVariables(root);
        return;
    }

    varName = malloc(strlen(cmd[1] + 1));
    varValue = malloc(strlen(cmd[3] + 1));
    strcpy(varName, cmd[1]);
    strcpy(varValue, cmd[3]);

    if (!validateSetCommandFormat(cmd)) {
        return;
    }
    
    v = doesVariableAlreadyExist(varName, root);
    if (v == NULL) {
        readonly = setReadOnlyValue(cmd);
        v = createVariable(varName, varValue, readonly);
        addVariable(v, root);
    } else {
        free(varName);
        if (isVariableReadOnly(v)) {
            fprintf(stderr, "Cannot overwrite the value of a read-only variable!\n");
            return;
        }
        overwriteValueOfExistingVariable(v, varValue);
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
        
        if (isBuiltinCommand(cmd[i])) {
            printBuiltinCommandMessage(cmd[i]);
            return;
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
        if (fileHasBeenFound(d, fileToFind)) {
            printCompleteFilePath(fileToSearch, fileToFind);
            closedir(dir);
            return 1;
        }
    }
    closedir(dir);
    return 0;
}