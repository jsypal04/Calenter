#include <stdio.h>
#include <string.h>

#include "calenter.h"
#include "array.h"
#include "../include/tests.h"


int test_split_line() {
    char line[256] = "hello world!";
    int container_width = 8;

    Array* lines = split_line(line, container_width);

    if (array_len(lines) != 2) {
        printf("test_split_line failed: Expected 2 lines, got %d\n", array_len(lines));
        return FAILED;
    }

    for (int i = 0; i < array_len(lines); i++) {
        char* line = get_string(lines, i);
        if (strlen(line) >= container_width) {
            printf(
                "test_split_line failed: Expected line length to be less than container width. Line length: %zu, container width: %d\n", 
                strlen(line), 
                container_width
            );
            return FAILED;
        }
    }

    char* line1 = get_string(lines, 0);
    if (strcmp(line1, "hello") != 0) {
        printf("test_split_line failed: Expected line 1 to be 'hello', got '%s'\n", line1);
        return FAILED;
    }

    char* line2 = get_string(lines, 1);
    if (strcmp(line2, "world!") != 0) {
        printf("test_split_line failed: Expected line 2 to be 'world!', got '%s'\n", line2);
        return FAILED;
    }

    free_array(lines);

    printf("test_split_line succeeded.\n");
    return PASSED;
}

int test_split_line_no_spaces() {
    char line[256] = "helloworld!";
    int container_width = 6;

    Array* lines = split_line(line, container_width);

    print_array(lines);

    if (array_len(lines) != 2) {
        printf("test_split_line_no_spaces failed: Expected 2 lines, got %d\n", array_len(lines));
        return FAILED;
    }


    return PASSED;
}


struct tests make_calenter_utils_tests() {
    struct tests tests = make_tests(10, "calenter_utils_tests");
    add_test(&tests, "test_split_line_no_spaces", test_split_line_no_spaces);
    add_test(&tests, "test_split_line", test_split_line);
    return tests;
}
