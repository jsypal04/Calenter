#include <algorithm>
#include <cctype>

#include "../../include/tests.h"
#include "oauth.hpp"

const auto valid_non_alpha_chars = {'-', '.', '_', '~'};

int test_pkce_generates_valid_key() {
    Pkce key;

    auto begin = valid_non_alpha_chars.begin();
    auto end = valid_non_alpha_chars.end();

    for (char c : key.key()) {
        if (std::isalnum(c) || std::find(begin, end, c) != end) {
            continue;
        }
        return 1;
    }

    std::cout << "pkce = " << key.key() << std::endl;

    return 0;
}

extern "C" {
    struct tests make_oauth_tests() {
        std::string pkce_generates_valid_key_lbl = "test_pkce_generates_valid_key";
        std::string oauth_tests_lbl = "oauth_tests";

        struct tests tests = make_tests(10, oauth_tests_lbl.c_str());
        add_test(
            &tests, 
            pkce_generates_valid_key_lbl.c_str(), 
            test_pkce_generates_valid_key
        );
        return tests;
    }
}



