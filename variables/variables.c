#include <stdbool.h>
#include "../structs.h"

bool isVariable(char *arg) {
    return *arg == '$';
}