#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../structs.h"

bool isVariable(char *str) {
    return *str == '$';
}

variable *createVariable(char *varName, char *varValue, bool readonly) {
    variable *v;
    v = malloc(sizeof(variable));
    v->name = varName;
    v->value = varValue;
    v->readonly = readonly;
    v->next = NULL;
    return v;
}

char *getVariableValue(char *varName, variable *root) {
    char *environmentVariable;
    char *emptyString = "";
    variable *tmp;
    tmp = malloc(sizeof(variable));
    tmp = root->next;
    varName += 1; // Move varName pointer past leading $

    while (tmp != NULL) {
        if (strcmp(tmp->name, varName) == 0) {
            return tmp->value;
        }
        tmp = tmp->next;
    }
    environmentVariable = getenv(varName);
    if (environmentVariable != NULL) {
        return environmentVariable;
    }
    return emptyString; // If variable is not a valid local or environment variable, return empty string
}

void getAllVariables(variable *root) {
    variable *tmp;
    tmp = malloc(sizeof(variable));
    tmp = root->next;

    while (tmp != NULL) {
        printf("%s = %s", tmp->name, tmp->value);
        if (tmp->readonly) {
            printf(" (readonly)\n");
        } else {
            printf("\n");
        }
        tmp = tmp->next;
    }
}

void addVariable(variable *v, variable *root) {
    if (root->next == NULL) {
        root->next = v;
    } else {
        v->next = root->next;
        root->next = v;
    }
}

variable *doesVariableAlreadyExist(char *varName, variable *root) {
    variable *tmp;
    tmp = malloc(sizeof(variable));
    tmp = root->next;

    while (tmp != NULL) {
        if (strcmp(varName, tmp->name) == 0) {
            return tmp;
        }
        tmp = tmp->next;
    }
    return tmp;
}

void freeVariables(variable *root) {
    variable *tmp, *trav;
    tmp = malloc(sizeof(variable));
    trav = malloc(sizeof(variable));

    if (root->next == NULL) {
        free(root);
        return;
    }

    tmp = root;
    trav = root->next;
    while (tmp != NULL) {
        printf("Freeing %s\n", tmp->name);
        free(tmp);
        tmp = trav;
        trav = trav->next;
    }
}

bool isEnvironmentVariable(char *varName) {
    char *var;
    if ((var = getenv(varName)) == NULL) {
        return false;
    } else {
        return true;
    }
}

bool isVariableReadOnly(variable *v) {
    return v->readonly;
}

void overwriteValueOfExistingVariable(variable *v, char *newValue) {
    v->value = newValue;
}