#ifndef VALIDATION_H
#define VALIDATION_H

#include <dirent.h>

bool validateSetCommandFormat(char *cmd[]);
bool isCommandFormattedCorrectly(char *cmd[]);
bool isReadOnlyValueValid(char *readOnlyValue);
bool doesUserWantToSeeAllVariables(char *isNull);
bool setReadOnlyValue(char *cmd[]);
bool isBuiltinCommand(char *cmd);
void printBuiltinCommandMessage(char *cmd);
void printCompleteFilePath(char *filePath, char *fileName);
bool fileHasBeenFound(struct dirent *d, char *fileToFind);

#endif