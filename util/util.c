#include <sys/wait.h>
#include <stdio.h>
#include <readline/history.h>
#include <stdbool.h>
#include "prompt.h"

enum {
    BUFSZ = 512
};

void checkForCompletedBackgroundProcesses() {
    int pid, wstatus;

    if ((pid = waitpid(-1, &wstatus, WNOHANG)) > 0) {
        printf("Process completed: %d\n", pid);
    }
}

char *getUserInput(char *pwname) {
    char *bufptr, *usrinput, *prompt;

    prompt = getPrompt(pwname);
    usrinput = readline(prompt);
    return usrinput;
}

bool isUserInputNull(char *userInput) {
    return !userInput;
}

bool isUserInputBlank(char *userInput) {
    return *userInput == '\n';
}