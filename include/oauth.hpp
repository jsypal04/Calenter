#pragma once

#include <boost/beast/http/string_body_fwd.hpp>
#include <future>
#include <string>
#include <thread>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/url.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace http = boost::beast::http;
namespace net = boost::asio;
namespace json = boost::json;
using tcp = net::ip::tcp;

struct oauth2_resp {
    std::string              access_token;
    int                      expires_in;
    std::string              token_type;
    std::vector<std::string> scope;
    std::string              refresh_token;
};

class Pkce {
    private:
        std::string key_;
        std::string hash_method_;

        /**
         * Encodes the given string using base64 encoding.
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

class OAuth {
    private:
        std::string credentials_path_;

        boost::urls::url oauth2_server_url_;
        boost::urls::url oauth2_token_ex_url_;

        std::string client_id_;
        // TODO:: DO NOT COMMIT THIS!!!!!!
        std::string client_secret_ = "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";
        std::string scope_;
        std::string redirect_uri_;
        std::string response_type_ = "code";
        Pkce code_challenge_;

        std::string code_;

        std::string redirect_addr_;
        net::io_context ioc_;
        tcp::acceptor listener_;
        std::thread loopback_thread_;

        std::promise<struct oauth2_resp> tokens_promise_;
        std::future<struct oauth2_resp> tokens_future_;
        bool oauth2_error_occured_ = false;

        /**
         * Appends the url encoded query params sent to the
         * OAuth server to the oauth2_server_url_.
         * */
        void make_query_params();

        std::string make_token_ex_query_params();

        std::string send_token_ex_request(http::request<http::string_body> req);

        struct oauth2_resp parse_oauth2_resp(json::value body);

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
         * Returns true when the auth flow finishes, otherwise returns false.
         * */
        void handle_loopback_connection(tcp::socket conn);

        void store_credentials(struct oauth2_resp credentials);

        void store_credentials(std::string credentials_json);

    public:
        OAuth(std::string credentials_path);

        OAuth(
            std::string credentials_path,
            std::string oauth2_base_url,
            std::string client_id,
            std::string scope
        );

        struct oauth2_resp get_credentials();

        struct oauth2_resp authenticate();
};
