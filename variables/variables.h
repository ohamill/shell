bool isVariable(char *str);
char *getVariableValue(char *varName, variable *root);
void getAllVariables(variable *root);
variable *createVariable(char *varName, char *varValue, bool readonly);
void addVariable(variable *v, variable *root);
variable *doesVariableAlreadyExist(char *varName, variable *root);
void freeVariables(variable *root);
bool isEnvironmentVariable(char *varName);