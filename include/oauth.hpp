#pragma once

#include <future>
#include <string>
#include <thread>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/url.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "http.hpp"
#include "types.h"

namespace net = boost::asio;
namespace json = boost::json;
using tcp = net::ip::tcp;

struct oauth2_tokens {
    char*   access_token;
    long    expires_at; // Seconds since unix epoch
    char*   token_type;
    Array*  scope;
    char*   refresh_token;
};


void destroy_oauth2_tokens(struct oauth2_tokens* tokens);


class Pkce {
    private:
        std::string key_;
        std::string hash_method_;

        /**
         * Encodes the given string using an encoding similar to base64
         */
        static std::string to_ascii(std::string bytes);

        /**
         * Generates a cryptographically random key of the specified
         * length.
         */
        static std::string gen_random_key(unsigned int length);

    public:
        Pkce();

        std::string key();

        std::string hash_method();
};


class CredentialStore {
    private:
        std::string path_;

    public:
        CredentialStore();
        CredentialStore(std::string path);

        struct oauth2_tokens get();

        void store(struct oauth2_tokens tokens);
        void store(std::string tokens);

        static struct oauth2_tokens parse_oauth2_resp(json::value json);
};

class OAuth {
    private:
        HttpClient http_client_;
        CredentialStore cred_store_;

        std::string credentials_path_;

        boost::urls::url oauth2_server_url_;
        boost::urls::url oauth2_token_ex_url_;

        std::string client_id_;
        std::string client_secret_;
        std::string scope_;
        std::string redirect_uri_;
        std::string response_type_ = "code";
        Pkce code_challenge_;

        std::string code_;

        std::string redirect_addr_;
        net::io_context ioc_;
        tcp::acceptor listener_;
        std::thread loopback_thread_;

        std::promise<struct oauth2_tokens> tokens_promise_;
        std::future<struct oauth2_tokens> tokens_future_;
        bool oauth2_error_occured_ = false;

        /**
         * Appends the url encoded query params sent to the
         * OAuth server to the oauth2_server_url_.
         * */
        void make_query_params();

        std::string make_token_ex_query_params();

        /**
         * Starts an HTTP listening on 127.0.0.1 and an OS-determined port.
         * The server automatically stops when the auth flow ends.
         * */
        void start_loopback_server();

        /**
         * HTTP loopback server routine. Runs in a separated thread.
         * */
        void loopback_listener_thread();

        /**
         * Handles each redirect from the oauth server.
         * Returns the connection TCP socket when finished.
         * */
        tcp::socket handle_loopback_connection(tcp::socket conn);

        std::string success_html();
        std::string failure_html();

    public:
        OAuth();
        OAuth(HttpClient& client, CredentialStore& cred_store);

        struct oauth2_tokens get_credentials();

        struct oauth2_tokens authenticate();

        struct oauth2_tokens refresh(std::string refresh_token);

        std::string make_token_refresh_query_params(std::string refresh_token);

        static long now();
};

/**
 * OAuth Exception types
 * */
class ExpiredException : public std::exception {
    private:
        std::string message_;

    public:
        explicit ExpiredException(std::string msg) : message_(std::move(msg)) {}

        const char* what() const noexcept override {
            return message_.c_str();
        }
};
