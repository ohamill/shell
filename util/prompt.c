#include <pwd.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    BUFSZ = 512
};

char *getUsername(struct passwd *pw) {
    pw = getpwuid(getuid());
    return pw->pw_name;
}

char *getPrompt(char *pwname) {
    char buf[BUFSZ];
    char *bufptr, *prompt;
    prompt = malloc(128);

    getcwd(buf, BUFSZ);
    for (int i = strlen(buf); i >= 0; i--) {
        if (buf[i] == '/') {
            bufptr = &buf[i+1];
            break;
        }
    }
    if (strcmp(bufptr, pwname) == 0) {
        sprintf(prompt, "%s/~: ", pwname);
    } else {
        sprintf(prompt, "%s/...%s: ", pwname, bufptr);
    }
    return prompt;
}