#include <stdio.h>
#include <string.h>

#include "../../src/calenter/calenter.h"
#include "../../src/common/array.h"

typedef struct test {
    char* name;
    int (*test_fn)();
} Test;

int test_split_line() {
    char line[256] = "hello world!";
    int container_width = 8;

    Array* lines = split_line(line, container_width);

    if (array_len(lines) != 2) {
        fprintf(stderr, "test_split_line failed: Expected 2 lines, got %d\n", array_len(lines));
        return 1;
    }

    for (int i = 0; i < array_len(lines); i++) {
        char* line = get_string(lines, i);
        if (strlen(line) >= container_width) {
            fprintf(
                stderr, 
                "test_split_line failed: Expected line length to be less than container width. Line length: %zu, container width: %d\n", 
                strlen(line), 
                container_width
            );
            return 1;
        }
    }

    char* line1 = get_string(lines, 0);
    if (strcmp(line1, "hello") != 0) {
        fprintf(stderr, "test_split_line failed: Expected line 1 to be 'hello', got '%s'\n", line1);
        return 1;
    }

    char* line2 = get_string(lines, 1);
    if (strcmp(line2, "world!") != 0) {
        fprintf(stderr, "test_split_line failed: Expected line 2 to be 'world!', got '%s'\n", line2);
        return 1;
    }

    free_array(lines);

    printf("test_split_line succeeded.\n");
    return 0;
}

int test_split_line_no_spaces() {
    char line[256] = "helloworld!";
    int container_width = 6;

    Array* lines = split_line(line, container_width);

    print_array(lines);

    if (array_len(lines) != 2) {
        fprintf(stderr, "test_split_line_no_spaces failed: Expected 2 lines, got %d\n", array_len(lines));
    }


    return 0;
}

void run_utils_tests() {
    test_split_line();
    test_split_line_no_spaces();
}
