#ifndef VARIABLES_H
#define VARIABLES_H

bool isVariable(char *str);
char *getVariableValue(char *varName, variable *root);
void getAllVariables(variable *root);
variable *createVariable(char *varName, char *varValue, bool readonly);
void addVariable(variable *v, variable *root);
variable *doesVariableAlreadyExist(char *varName, variable *root);
void freeVariables(variable *root);
bool isEnvironmentVariable(char *varName);
bool isVariableReadOnly(variable *v);
void overwriteValueOfExistingVariable(variable *v, char *newValue);

#endif