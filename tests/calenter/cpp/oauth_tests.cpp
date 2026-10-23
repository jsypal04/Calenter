#include <algorithm>
#include <cctype>
#include <boost/beast/http.hpp>

#include "../../include/tests.h"
#include "oauth.h"
#include "oauth.hpp"

namespace http = boost::beast::http;

class MockHttpClient : public HttpClient {
    http::response<http::string_body> send() {
        // TODO: check the structure of the request before returning

        http::response<http::string_body> res;

        return res;
    }
};

class MockCredentialStore : public CredentialStore {
    void store(struct oauth2_tokens tokens) {
        return;
    }

    struct oauth2_tokens get() {
        struct oauth2_tokens tokens;
        
        // TODO: Add fake data

        return tokens;
    }
};

const auto valid_non_alpha_chars = {'-', '.', '_', '~'};

// TBH this doesn't really test much since the pkce is random.
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

// NOTE: This test is a work in progress.
int test_authenticate_should_pass() {
    MockHttpClient http_client;
    MockCredentialStore cred_store;
    OAuth oc(http_client, cred_store);
    oc.authenticate();
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



