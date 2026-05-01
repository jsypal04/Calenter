#include <stdio.h>
#include "array.h"


int main() {
    Array* array = new_array(2, STRING);
    
    printf("capacity = %d\n", array_cap(array));
    printf("length = %d\n", array_len(array));

    for (int i = 0; i < 5; i++) {
        append_string(array, "hello");
    }

    for (int i = 0; i < 5; i++) {
        char* v = get_string(array, i);
        printf("'%s' ", v);
    }

    printf("\n");

    printf("capacity = %d\n", array_cap(array));
    printf("length = %d\n", array_len(array));
}
