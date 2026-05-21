#include "../src/utils/array.h"

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

    sort(array, comp_ints);

    print_array(array);

    return 0;
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
    sort(array, comp_byxxx);
    print_array(array);

    return 0;
}

int main() {

    test_sort_BYxxx();
    // test_sort_reversed_ints();
}
