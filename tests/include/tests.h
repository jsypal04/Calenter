#ifndef TESTS_H
#define TESTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>


struct test {
    char label[256];
    int (*run)();
};

struct tests {
    char label[256];
    size_t cap;
    size_t len;
    struct test* tests;
};

struct tests make_tests(size_t cap, const char* label);

void destroy_tests(struct tests* tests);

int add_test(struct tests* tests, const char* label, int (*run)());

void run(struct tests* tests, char* test_label);

#ifdef __cplusplus
}
#endif

#endif
