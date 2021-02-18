#ifndef BUILTINS_H
#define BUILTINS_H

#include "../structs.h"
#include "../variables/variables.h"

void echo(char *[]);
bool echoDashOptions(char *[]);
void set(char *[], variable *);
void which(char *cmd[]);
int searchDir(char *fileToSearch, char *fileToFind);

#endif