typedef struct variable {
    char *name;
    char *value;
    bool readonly;
    struct variable *next;
} variable;