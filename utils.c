#include "utils.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "shared.h"

void *safe_malloc(int size) {
    void *res = malloc(size);
    if (res == NULL) {
        printf("Malloc failed, exiting the shell...\n");
        exit(12); // ENOMEM
    }
    return res;
}

char *concat_array(int count, char *words[]) {
    int length = 0;
    for (int i = 0; i < count; i++) {
        length += strlen(words[i]);
    }

    length += count + 1; // count - 1 spaces, newline and null byte
    char *result = safe_malloc(length);
    
    for (int i = 0; i < count; i++) {
        strcat(result, words[i]);
        strcat(result, " ");
    }
    strcat(result, "\n");
    return result;
}
