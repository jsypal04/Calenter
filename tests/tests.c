#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "tests.h"

#define TEST_RUN_LOG "logs/test_run.log"

struct tests make_array_tests();
struct tests make_oauth_tests();
struct tests make_calenter_utils_tests();


struct tests make_tests(size_t cap, const char* label) {
    struct tests tests = {0};
    struct test* tests_arr = malloc(sizeof(struct test) * cap);
    bzero(tests_arr, sizeof(struct test) * cap);
    strncpy(tests.label, label, 256);
    tests.len = 0;
    tests.cap = cap;
    tests.tests = tests_arr;
    return tests;
}

void destroy_tests(struct tests* tests) {
    free(tests->tests);
    tests->tests = NULL;
}

int add_test(struct tests* tests, const char* label, int (*run)()) {
    struct test test = {0};
    strncpy(test.label, label, 256);
    test.run = run;
    
    if (tests->len >= tests->cap) 
        return 1; 

    tests->tests[tests->len] = test;
    tests->len++;
    
    return 0;
}

int redirect_stdout() {
    int original_fd = dup(STDOUT_FILENO);
    if (freopen(TEST_RUN_LOG, "a", stdout) == NULL) {
        perror("Failed to redirect stdout\n");
        _exit(EXIT_FAILURE);
    }

    return original_fd;
}

void restore_stdout(int original_fd) {
    fflush(stdout);
    dup2(original_fd, STDOUT_FILENO);
    close(original_fd);
}


void run(struct tests* tests, char* test_label) {
    char test_or_tests[64] = "\0";
    if (tests->len == 1) {
        strncpy(test_or_tests, "test", 64);
    } else {
        strncpy(test_or_tests, "tests", 64);
    }
    printf("Running %s (%zu %s): ", tests->label, tests->len, test_or_tests);
    for (int i = 0; i < tests->len; i++) {
        struct test test = tests->tests[i];

        if (test_label != NULL && strcmp(test_label, test.label) != 0)
            continue;

        int fd = redirect_stdout();
        printf("BEGIN_TEST:%s\n", test.label);
        test.result = test.run();
        printf("\nEND_TEST\n");
        restore_stdout(fd);

        if (test.result != 0) {
            printf("\033[31mX\033[0m");
            continue;
        }

        printf("\e[32m.\e[0m");
    }
    printf("\n");
}

int main() {
    // Clear the test run log
    FILE* fd = fopen("logs/test_run.log", "w");
    fclose(fd);

    struct tests array_tests = make_array_tests();
    struct tests oauth_tests = make_oauth_tests();
    struct tests calenter_utils_tests = make_calenter_utils_tests();

    printf("==================\n");
    printf("Running Test Suite\n");
    printf("==================\n\n");

    run(&array_tests, NULL);
    run(&calenter_utils_tests, NULL);
    run(&oauth_tests, NULL);

    destroy_tests(&array_tests);
    destroy_tests(&calenter_utils_tests);
    destroy_tests(&oauth_tests);
    
    return 0;
}
