#include "array.h"
#include "types.h"

#include "../include/tests.h"

int comp_ints(union element e) {
    return e.i;
}

int comp_byxxx(union element e) {
    return e.b.type;
}

int test_sort_reversed_ints() {
    Array* array = new_array(10, INT);
    for (int i = 9; i >= 0; i--) {
        append_int(array, i);
    }

    print_array(array);
    printf("\n");
    sort(array, comp_ints);
    print_array(array);

    for (int i = 0; i < array_len(array) - 1; i++) {
        int ith = get_int(array, i);
        int i_plus_1th = get_int(array, i + 1);
        if (ith > i_plus_1th) return FAILED;
    }

    return PASSED;
}

int test_sort_BYxxx() {
    Array* array = new_array(5, BYXXX_RULE);

    BYxxx_Rule byday = {0};
    BYxxx_Rule byweek = {0};
    BYxxx_Rule byyear = {0};
    BYxxx_Rule bymonth = {0};
    BYxxx_Rule byminute = {0};
    
    byday.type = BYDAY;
    byday.values = new_array(3, INT);
    byweek.type = BYWEEKNO;
    byweek.values = new_array(3, INT);
    byyear.type = BYYEARDAY;
    byyear.values = new_array(3, INT);
    bymonth.type = BYMONTH;
    bymonth.values = new_array(3, INT);
    byminute.type = BYMINUTE;
    byminute.values = new_array(3, INT);

    for (int i = 0; i < 3; i++) {
        append_int(byday.values, i + 1);
    }
    for (int i = 0; i < 3; i++) {
        append_int(byweek.values, i + 1);
    }
    for (int i = 0; i < 3; i++) {
        append_int(byyear.values, i + 1);
    }
    for (int i = 0; i < 3; i++) {
        append_int(bymonth.values, i + 1);
    }
    for (int i = 0; i < 3; i++) {
        append_int(byminute.values, i + 1);
    }

    append_BYxxx_Rule(array, byday);
    append_BYxxx_Rule(array, byweek);
    append_BYxxx_Rule(array, byyear);
    append_BYxxx_Rule(array, bymonth);
    append_BYxxx_Rule(array, byminute);

    print_array(array);
    printf("\n");
    sort(array, comp_byxxx);
    print_array(array);

    for (int i = 0; i < array_len(array) - 1; i++) {
        BYxxx_Rule ith = get_BYxxx_Rule(array, i);
        BYxxx_Rule i_plus_1th = get_BYxxx_Rule(array, i + 1);
        if (ith.type > i_plus_1th.type) return FAILED;
    }

    return PASSED;
}

struct tests make_array_tests() {
    struct tests tests = make_tests(10, "array_tests");
    add_test(&tests, "test_sort_reversed_ints", test_sort_reversed_ints);
    add_test(&tests, "test_sort_BYxxx", test_sort_BYxxx);

    return tests;
}
