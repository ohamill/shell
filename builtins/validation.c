#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include "validation.h"
#include "builtins.h"

const char *builtincmds[] = {"echo", "cd", "set", "which"};
const int builtincount = 4;

bool validateSetCommandFormat(char *cmd[]) {
    char *variableName = cmd[1];
    char *readOnlyValue = cmd[4];

    if (!isCommandFormattedCorrectly(cmd)) {
        fprintf(stderr, "Invalid format! Usage: set varName = varValue [readonly]\n");
        return false;
    }
    if (!isReadOnlyValueValid(readOnlyValue)) {
        fprintf(stderr, "Unrecognized command: %s\n", readOnlyValue);
        return false;
    }
    if (isEnvironmentVariable(variableName)) {
        fprintf(stderr, "Unable to overwrite environment variable\n");
        return false;
    }
    return true;
}

bool isCommandFormattedCorrectly(char *cmd[]) {
    char *equalsSign = cmd[2];
    return strcmp(equalsSign, "=") == 0 && (cmd[4] == NULL || cmd[5] == NULL);
}

bool isReadOnlyValueValid(char *readOnlyValue) {
    return readOnlyValue == NULL || strcmp(readOnlyValue, "readonly") == 0;
}

bool doesUserWantToSeeAllVariables(char *isNull) {
    return isNull == NULL;
}

bool setReadOnlyValue(char *cmd[]) {
    char *readOnlyValue = cmd[4];

    if (readOnlyValue != NULL && strcmp(readOnlyValue, "readonly") == 0) {
        return true;
    } else {
        return false;
    }
}

bool isBuiltinCommand(char *cmd) {
    for (int j = 0; j < builtincount; j++) {
        if (strcmp(cmd, builtincmds[j]) == 0) {
            return true;
        }
    }
    return false;
}

void printBuiltinCommandMessage(char *cmd) {
    printf("%s: shell built-in command\n", cmd);
}

void printCompleteFilePath(char *filePath, char *fileName) {
    printf("%s/%s\n", filePath, fileName);
}

bool fileHasBeenFound(struct dirent *d, char *fileToFind) {
    return strcmp(d->d_name, fileToFind) == 0;
}