#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

bool isFirstCharQuote(const char *str) {
    return *str == '"';
}

char *packageQuotedArg(char *token) {
    char *quotedArg, *tmp;
    quotedArg = malloc(512);

    // Copy contents of token into quotedArg
    token += 1; // Move token pointer past first character, which is a double quote
    strlcat(quotedArg, token, 512);
    strlcat(quotedArg, " \0", 512);
    // Get rest of quoted argument
    tmp = strtok(NULL, "\"");
    // Concatenate quoted argument together into one string
    strlcat(quotedArg, tmp, 512);
    return quotedArg;
}