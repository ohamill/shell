#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include "./builtins.h"
#include "../structs.h"

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
        i = 1;
        while (cmd[i] != NULL) {
            echocmds[i] = cmd[i];
            i++;
        }
    }

    i = 0;
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
    }
    return false;
}

void set(char *cmd[]) {

}