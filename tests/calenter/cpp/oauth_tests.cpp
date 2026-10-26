#include <algorithm>
#include <cctype>
#include <boost/beast/http.hpp>

#include "../../include/tests.h"
#include "oauth.hpp"
#include "array.h"

namespace http = boost::beast::http;
namespace json = boost::json;

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
        return FAILED;
    }

    std::cout << "pkce = " << key.key() << std::endl;

    return PASSED;
}

int test_parse_oauth2_resp_one_scope() {
    json::value oauth2_resp = {
        {"access_token", "this_is_a_fake_token_0"},
        {"expires_in", 3599},
        {"refresh_token", "this_is_a_fake_token_1"},
        {"scope", "https://www.googleapis.com/auth/calendar.events"},
        {"token_type", "Bearer"}
    }; 

    CredentialStore store;
    struct oauth2_tokens tokens = store.parse_oauth2_resp(oauth2_resp);

    if (tokens.access_token != std::string("this_is_a_fake_token_0")) {
        std::cout << "Wrong access_token: " << tokens.access_token << std::endl;
        return FAILED;
    }
    if (tokens.refresh_token != std::string("this_is_a_fake_token_1")) {
        std::cout << "Wrong refresh_token: " << tokens.refresh_token << std::endl;
        return FAILED;
    }
    if (array_len(tokens.scope) != 1) {
        std::cout << "Expected 1 scope, found " << array_len(tokens.scope);
        std::cout << std::endl;
        
        print_array(tokens.scope);

        return FAILED;
    }
    if (tokens.token_type != std::string("Bearer")) {
        std::cout << "Wrong token_type: " << tokens.token_type << std::endl;
        return FAILED;
    }

    return PASSED;
}

// NOTE: This test is a work in progress.
int test_authenticate_should_pass() {
    MockHttpClient http_client;
    MockCredentialStore cred_store;
    OAuth oc(http_client, cred_store);
    oc.authenticate();
    return PASSED;
}

extern "C" {
    struct tests make_oauth_tests() {
        std::string pkce_generates_valid_key_lbl = "test_pkce_generates_valid_key";
        std::string parse_oauth2_resp_one_scope_lbl = "test_parse_oauth2_resp_one_scope";
        std::string oauth_tests_lbl = "oauth_tests";

        struct tests tests = make_tests(10, oauth_tests_lbl.c_str());
        add_test(
            &tests, 
            pkce_generates_valid_key_lbl.c_str(), 
            test_pkce_generates_valid_key
        );
        add_test(
            &tests, 
            parse_oauth2_resp_one_scope_lbl.c_str(), 
            test_parse_oauth2_resp_one_scope
        );
        return tests;
    }
}



