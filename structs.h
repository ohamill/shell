#ifndef STRUCTS_H
#define STRUCTS_H

typedef struct variable {
    char *name;
    char *value;
    bool readonly;
    struct variable *next;
} variable;

#endif