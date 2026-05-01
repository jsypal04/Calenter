#include "array.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


union element {
    int i;
    float f;
    char* s;
};

struct array_t {
    unsigned int length;
    unsigned int capacity;
    enum type type;
    union element* array;
};

/*
 * PRIVATE FUNCTIONS
 * */

void expand_array(Array* array) {
    union element* expanded_array = 
        malloc(sizeof(union element) * 2 * array->capacity);

    for (int i = 0; i < array->length; i++) {
        expanded_array[i] = array->array[i];
    }

    free(array->array);
    array->array = expanded_array;

    array->capacity *= 2;
}

/*
 * PUBLIC FUNCTIONS
 * */

Array* new_array(unsigned int capacity, enum type type) {
    Array* array = malloc(sizeof(Array));
    array->type = type;
    array->length = 0;
    array->capacity = capacity;

    array->array = malloc(sizeof(union element) * capacity);

    return array;
}

void free_array(Array* array) {
    if (array->type == STRING) {
        for (int i = 0; i < array->length; i++) {
            free(array->array[i].s);
            array->array[i].s = NULL;
        }
    }
    free(array->array);
    array->array = NULL;
    free(array);
    array = NULL;
}

int append_int(Array* array, int val) {
    if (array->type != INT) return -1;
    if (array->length == array->capacity) expand_array(array);

    union element elem;
    elem.i = val;

    array->array[array->length] = elem;
    array->length++;

    return 0;
}

int append_float(Array* array, float val) {
    if (array->type != FLOAT) return -1;
    if (array->length == array->capacity) expand_array(array);

    union element elem;
    elem.f = val;

    array->array[array->length] = elem;
    array->length++;

    return 0;
}

int append_string(Array* array, char* val) {
    if (array->type != STRING) return -1;
    if (array->length == array->capacity) expand_array(array);

    union element elem;
    elem.s = strdup(val);

    array->array[array->length] = elem;
    array->length++;

    return 0;
}

int get_int(Array* array, int index) {
    assert(index < array->length);
    return array->array[index].i;
}

float get_float(Array* array, int index) {
    assert(index < array->length);
    return array->array[index].f;
}

char* get_string(Array* array, int index) {
    assert(index < array->length);
    return array->array[index].s;
}

int array_len(Array* array) {
    return array->length;
}

int array_cap(Array* array) {
    return array->capacity;
}

enum type array_type(Array* array) {
    return array->type;
}
