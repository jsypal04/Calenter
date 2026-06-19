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

#include "../../common/types.h"


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

/* Appends a BYxxx_Rule to an array of that same type.
 * Note: This function takes ownership of the Array* in
 * the BYxxx_Rule. It may be safely freed after calling this
 * function
 */
int append_BYxxx_Rule(Array* array, BYxxx_Rule rule);

int append_UIObject(Array* array, UIObject* layout_obj);

int get_int(Array* array, int index);

float get_float(Array* array, int index);

/*
 * This is a little dangerous because right now it returns the
 * pointer to the string that is in the array. No duplication.
 * */
char* get_string(Array* array, int index);

/*
 * This is a little dangerous because right now it returns the
 * pointer to the Array member of the BYxxx_Rule that. No duplication.
 * */
BYxxx_Rule get_BYxxx_Rule(Array* array, int index);

UIObject* get_UIObject(Array* array, int index);

// TODO: Implement the pop methods for the rest of the types

UIObject* pop_UIObject(Array* array, int index);

int array_len(Array* array);

int array_cap(Array* array);

enum type array_type(Array* array);

void sort(Array* array, int (*comparator)(union element));

void print_array(Array* array);

#endif
