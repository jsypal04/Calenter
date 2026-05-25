#include "array.h"
#include "types.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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

/* Duplicates the given array. Allocates additional memory */
Array* array_dup(Array* array) {
    Array* new_arr = new_array(array->capacity, array->type);

    for (int i = 0; i < array->length; i++) {
        switch (array->type) {
            case INT: 
                append_int(new_arr, get_int(array, i));
                break;
            case FLOAT:
                append_float(new_arr, get_float(array, i));
                break;
            case STRING:
                append_string(new_arr, get_string(array, i));
                break;
            case BYXXX_RULE:
                break;
        }
    }

    return new_arr;
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
    } else if (array->type == BYXXX_RULE) {
        for (int i = 0; i < array->length; i++) {
            free_array(array->array[i].b.values);
            array->array[i].b.values = NULL;
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

int append_BYxxx_Rule(Array* array, BYxxx_Rule rule) {
    assert(rule.values->type != BYXXX_RULE);

    if (array->type != BYXXX_RULE) return -1;
    if (array->length == array->capacity) expand_array(array);

    BYxxx_Rule owned_rule = {0};
    owned_rule.type = rule.type;
    owned_rule.values = array_dup(rule.values);

    union element elem;
    elem.b = owned_rule;

    array->array[array->length] = elem;
    array->length++;

    return 0;
}

int get_int(Array* array, int index) {
    assert(index < array->length);
    assert(array->type == INT);
    return array->array[index].i;
}

float get_float(Array* array, int index) {
    assert(index < array->length);
    assert(array->type == FLOAT);
    return array->array[index].f;
}

char* get_string(Array* array, int index) {
    assert(index < array->length);
    assert(array->type == STRING);
    return array->array[index].s;
}

BYxxx_Rule get_BYxxx_Rule(Array* array, int index) {
    assert(index < array->length);
    assert(array->type == BYXXX_RULE);
    return array->array[index].b;
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

void sort(Array* array, int (*comparator)(union element)) {
    // Index of the first unsorted item
    int unsorted_index = 1;

    while (unsorted_index < array->length) {
        int current_index = unsorted_index;
        union element current_elem = array->array[current_index];

        int index = current_index - 1;
        union element elem = array->array[index];
        while (index >= 0 && comparator(current_elem) < comparator(elem)) {
            // perform swap
            array->array[index] = current_elem;
            array->array[current_index] = elem;

            current_index = index;
            index--;
            elem = array->array[index];
        }
        unsorted_index++;
    }
}

void print_array(Array* array) {
    for (int i = 0; i < array->length; i++) {
        switch (array->type) {
            case INT:
                printf("%d ", array->array[i].i);
                break;
            case FLOAT:
                printf("%f ", array->array[i].f);
                break;
            case STRING:
                printf("\"%s\" ", array->array[i].s);
                break;
            case BYXXX_RULE:
                switch (array->array[i].b.type) {
                    case BYDAY:
                        printf("BYDAY ");
                        break;
                    case BYHOUR:
                        printf("BYHOUR ");
                        break;
                    case BYMINUTE:
                        printf("BYMINUTE ");
                        break;
                    case BYMONTH:
                        printf("BYMONTH ");
                        break;
                    case BYMONTHDAY:
                        printf("BYMONTHDAY ");
                        break;
                    case BYSECOND:
                        printf("BYSECOND ");
                        break;
                    case BYSETPOS:
                        printf("BYSETPOS ");
                        break;
                    case BYWEEKNO:
                        printf("BYWEEKNO ");
                        break;
                    case BYYEARDAY:
                        printf("BYYEARDAY ");
                        break;
                }
        }
    }
    printf("\n");
}
