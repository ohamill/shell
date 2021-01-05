typedef struct variable {
    char *name;
    char *value;
    struct variable *next;
} variable;