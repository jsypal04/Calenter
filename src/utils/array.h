/*
 * array.h
 *
 * This file contains the the interface for the Array type.
 * The Array type is an array of ints, floats, or strings (char*).
 * That is, you can have an array of all ints, all floats, or all
 * strings (no mixing). If you try to add the wrong type to an array it
 * will return an error code.
 *
 * The actual Array type and the associated functions are defined in array.c
 * */

#ifndef ARRAY_H
#define ARRAY_H


enum type {
    INT,
    FLOAT,
    STRING,
};

typedef struct array_t Array;

/*
 * Creates a new array of the given type with the given capacity
 * */
Array* new_array(unsigned int capacity, enum type);

void free_array(Array* array);

int append_int(Array* array, int val);

int append_float(Array* array, float val);

/*
 * Duplicates the string and appends it to the array.
 *
 * NOTE: This function takes ownership of the string. You may safely 
 * free it after it is appended to the array. The string elements of the
 * array will be free by the free_array function.
 * */
int append_string(Array* array, char* val);

int get_int(Array* array, int index);

float get_float(Array* array, int index);

/*
 * This is a little dangerous because right now it returns the
 * pointer to the string that is in the array. No duplication.
 * */
char* get_string(Array* array, int index);

int array_len(Array* array);

int array_cap(Array* array);

enum type array_type(Array* array);

#endif
