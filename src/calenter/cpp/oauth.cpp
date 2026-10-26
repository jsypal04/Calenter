#include <chrono>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <future>
#include <ios>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <iostream>
#include <thread>
#include <vector>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/url.hpp>
#include <boost/json.hpp>

#include "oauth.hpp"
#include "array.h"
#include "http.hpp"

#define HTTPS_PORT "443"

namespace beast = boost::beast;
namespace net = boost::asio;
namespace http = beast::http;
namespace json = boost::json;
using tcp = net::ip::tcp;



void destroy_oauth2_tokens(struct oauth2_tokens* tokens) {
    if (tokens->access_token != NULL)
        free(tokens->access_token);
    if (tokens->refresh_token != NULL)
        free(tokens->refresh_token);
    if (tokens->token_type != NULL)
        free(tokens->token_type);
    if (tokens->scope != NULL)
        free_array(tokens->scope); 

    memset(tokens, 0, sizeof(struct oauth2_tokens));
}

Pkce::Pkce() {
    key_ = gen_random_key(64);
    hash_method_ = "plain";
}

std::string Pkce::key() {
    return key_;
}

std::string Pkce::hash_method() {
    return hash_method_;
}

std::string Pkce::gen_random_key(unsigned int length) {
    std::ifstream urand("/dev/urandom", std::ios::binary);
    if (!urand.is_open()) {
        throw std::runtime_error("Failed to open /dev/urandom.");
    }

    std::string bytes(length * 6 / 8, '\0');
    if (!urand.read(bytes.data(), static_cast<std::streamsize>(bytes.size()))) {
        throw std::runtime_error("Failed to read from /dev/urandom.");
    }

    return to_ascii(bytes);
}

std::string Pkce::to_ascii(std::string binary) {
    std::vector<char> encoding_map = {
        'A', 'B', 'C', 'D',
        'E', 'F', 'G', 'H',
        'I', 'J', 'K', 'L',
        'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T',
        'U', 'V', 'W', 'X',
        'Y', 'Z', 'a', 'b',
        'c', 'd', 'e', 'f',
        'g', 'h', 'i', 'j',
        'k', 'l', 'm', 'n',
        'o', 'p', 'q', 'r',
        's', 't', 'u', 'v',
        'w', 'x', 'y', 'z',
        '0', '1', '2', '3',
        '4', '5', '6', '7',
        '8', '9', '-', '.',
        '_', '~'
    };

    int counter = 0;
    unsigned int triple = 0;
    unsigned int bottom_six_mask = 0x0000003F;

    std::string encoded_binary;

    for (unsigned char byte : binary) {
        triple <<= 8;
        triple += byte;
        counter++;
        if (counter < 3) continue;

        unsigned char fourth_sextet = triple & bottom_six_mask;
        triple >>= 6;

        unsigned char third_sextet = triple & bottom_six_mask;
        triple >>= 6;

        unsigned char second_sextet = triple & bottom_six_mask;
        triple >>= 6;

        unsigned char first_sextet = triple & bottom_six_mask;

        encoded_binary.push_back(encoding_map[first_sextet]);
        encoded_binary.push_back(encoding_map[second_sextet]);
        encoded_binary.push_back(encoding_map[third_sextet]);
        encoded_binary.push_back(encoding_map[fourth_sextet]);

        counter = 0;
        triple = 0;
    }

    // TODO: handle if the number of bytes is not divisible by six. Not really
    // urgent because I generate 48 bytes rn.

    return encoded_binary;
}

CredentialStore::CredentialStore() {
    path_ = "/home/jsypal/.config/calenter/credentials.json";
}

oauth2_tokens CredentialStore::get() {
    if (!std::filesystem::exists(path_))
        throw std::runtime_error("Credentials file does not exist");

    std::ifstream credentials_stream(path_);

    if (!credentials_stream.is_open())
        throw std::runtime_error("Failed to open credentials file");

    std::string file_contents(
        (std::istreambuf_iterator<char>(credentials_stream)), 
        std::istreambuf_iterator<char>()
    );

    json::value parsed_credentials = json::parse(file_contents);
    return parse_oauth2_resp(parsed_credentials);
}

void CredentialStore::store(struct oauth2_tokens credentials) {
    std::stringstream scopes;

    int num_scopes = array_len(credentials.scope);
    for (int i = 0; i < num_scopes; i++) {
        scopes << std::string(get_string(credentials.scope, i));
        if (i < num_scopes - 1) {
            scopes << " ";
        }
    }

    json::value jv = {
        {"access_token", credentials.access_token},
        {"expires_at", credentials.expires_at},
        {"token_type", credentials.token_type},
        {"scope", scopes.str()},
        {"refresh_token", credentials.refresh_token}
    };

    std::string credentials_json = json::serialize(jv);
    store(credentials_json);
}

void CredentialStore::store(std::string tokens) {
    std::ofstream credentials_stream(path_);

    if (!credentials_stream.is_open())
        throw std::runtime_error("Failed to open credentials file");

    credentials_stream << tokens;
}

struct oauth2_tokens CredentialStore::parse_oauth2_resp(json::value body) {
    struct oauth2_tokens tokens = {0};

    auto now = OAuth::now();

    try {
        tokens.expires_at = body.at("expires_at").as_int64();
    } catch (std::exception& e) {
        tokens.expires_at = body.at("expires_in").as_int64() + now;
    }

    tokens.access_token = strdup(body.at("access_token").as_string().c_str());
    tokens.token_type = strdup(body.at("token_type").as_string().c_str());

    try {
        tokens.refresh_token = strdup(body.at("refresh_token").as_string().c_str());
    } catch (std::exception& e) {
        // This should happend on refresh so ok to ignore i think.
        tokens.refresh_token = NULL;
    }

    auto scopes = body.at("scope").as_string();
    int start = 0;
    int num_scopes = 0;
    // TODO: Failes to parse the scopes when there is only one scope.
    for (auto ch : scopes) {
        if (ch != ' ') continue;
        num_scopes++;
    }
    num_scopes++;

    tokens.scope = new_array(num_scopes, STRING);

    size_t i;
    for (i = 0; i < scopes.size(); i++) {
        if (scopes[i] != ' ') continue;
        std::string scope = scopes.subview(start, i - start);
        char* tmp = strdup(scope.c_str());
        append_string(tokens.scope, tmp);
        free(tmp);
        tmp = NULL;
        start = i + 1;
    }

    std::string scope = scopes.subview(start, i - start);
    char* tmp = strdup(scope.c_str());
    append_string(tokens.scope, tmp);
    free(tmp);
    tmp = NULL;

    return tokens;
}

OAuth::OAuth() : http_client_(), cred_store_(), ioc_(), listener_(ioc_) {
    boost::urls::url_view oauth2_base_url = "https://accounts.google.com/o/oauth2/v2/auth";
    oauth2_server_url_ = boost::urls::url(oauth2_base_url);

    boost::urls::url_view oauth2_base_token_url = "https://oauth2.googleapis.com/token";
    oauth2_token_ex_url_ = boost::urls::url(oauth2_base_token_url);

    // These macros are defined at compile time.
    client_id_     = CLIENT_ID;
    client_secret_ = CLIENT_SECRET;

    scope_ = "https://www.googleapis.com/auth/calendar";

    redirect_addr_ = "127.0.0.1";

    tokens_future_ = tokens_promise_.get_future();
}

OAuth::OAuth(
    HttpClient& client, 
    CredentialStore& cred_store
) : http_client_(client), cred_store_(cred_store), ioc_(), listener_(ioc_) {

    boost::urls::url_view oauth2_base_url = "https://accounts.google.com/o/oauth2/v2/auth";
    oauth2_server_url_ = boost::urls::url(oauth2_base_url);

    boost::urls::url_view oauth2_base_token_url = "https://oauth2.googleapis.com/token";
    oauth2_token_ex_url_ = boost::urls::url(oauth2_base_token_url);

    // These macros are defined at compile time.
    client_id_     = CLIENT_ID;
    client_secret_ = CLIENT_SECRET;

    scope_ = "https://www.googleapis.com/auth/calendar";

    redirect_addr_ = "127.0.0.1";

    tokens_future_ = tokens_promise_.get_future();
}

struct oauth2_tokens OAuth::get_credentials() {
    struct oauth2_tokens tokens = cred_store_.get();
    auto now = OAuth::now();

    if (now < tokens.expires_at)
        return tokens;

    try {
        struct oauth2_tokens new_tokens = refresh(tokens.refresh_token);
        destroy_oauth2_tokens(&tokens);
        return new_tokens;
    } catch (std::exception& e) {
        throw ExpiredException("OAuth tokens have expired.");
    }
}

struct oauth2_tokens OAuth::refresh(std::string refresh_token) {
    auto body = make_token_refresh_query_params(refresh_token);
    http_client_.post(oauth2_token_ex_url_);
    http_client_.header(
        http::field::content_type, 
        "application/x-www-form-urlencoded"
    );
    http_client_.body(body);
    auto res = http_client_.send();
    auto res_body = json::parse(res.body());
    struct oauth2_tokens tokens = CredentialStore::parse_oauth2_resp(res_body);

    // This should be null because the google oauth call should not
    // return a refresh token when we refresh the access token. Checking
    // just in case :)
    if (tokens.refresh_token == NULL)
        tokens.refresh_token = strdup(refresh_token.c_str());

    return tokens;
}

struct oauth2_tokens OAuth::authenticate() {
    start_loopback_server();
    make_query_params();

    std::stringstream command;
    command << "xdg-open \"" << oauth2_server_url_.buffer() << "\"";
    command << " > /dev/null 2> /dev/null";
    std::system(command.str().c_str());

    struct oauth2_tokens tokens = {0};
    auto start = std::chrono::system_clock::now();

    while (true) {
        auto now = std::chrono::system_clock::now();
        auto status = tokens_future_.wait_for(std::chrono::seconds(0));
        if (status == std::future_status::ready) {
            tokens = tokens_future_.get();
            break;
        } else if (now - start >= std::chrono::seconds(300)) {
            // 5 minute timeout
            throw std::runtime_error("Oauth redirect server timed out.");  
        } else {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    if (oauth2_error_occured_) {
        throw std::runtime_error("Failed to authenticate with oauth.");
    }

    return tokens;
}

void OAuth::start_loopback_server() {
    auto address = net::ip::make_address(redirect_addr_);
    tcp::endpoint listener_endpoint(address, 0);


    listener_.open(listener_endpoint.protocol());
    listener_.bind(listener_endpoint);
    listener_.listen();
    listener_.set_option(tcp::acceptor::reuse_address(true));

    auto port = listener_.local_endpoint().port();

    std::stringstream redirect_uri_stream;
    redirect_uri_stream << "http://" << redirect_addr_ << ":" << port;
    redirect_uri_ = redirect_uri_stream.str();

    std::cout << redirect_uri_ << std::endl;

    loopback_thread_ = std::thread(&OAuth::loopback_listener_thread, this);
}

void OAuth::loopback_listener_thread() {
    tcp::socket socket(ioc_);
    listener_.accept(socket);

    try {
        socket = handle_loopback_connection(std::move(socket));

        http_client_.post(oauth2_token_ex_url_);
        http_client_.header(
            http::field::content_type, 
            "application/x-www-form-urlencoded"
        );
        http_client_.body(make_token_ex_query_params());
        auto res = http_client_.send();

        std::cout << res << std::endl;

        if (!HttpClient::is_success(res)) {
            throw std::runtime_error(
                "Token exchange did not return successfully."
            );
        }
        
        json::value res_body = json::parse(res.body());

        struct oauth2_tokens tokens = CredentialStore::parse_oauth2_resp(res_body);
        cred_store_.store(tokens);

        http::response<http::string_body> server_res(
            http::status::ok,
            11
        );
        server_res.set(
            http::field::content_type,
            "text/html; charset=utf-8"
        );
        server_res.body() = success_html();
        server_res.prepare_payload();
        http::write(socket, server_res);
        
        // Do set this at the end because we use the future to determine
        // if the thread is still running.
        tokens_promise_.set_value(tokens);
    } catch (std::exception& e) {
        http::response<http::string_body> server_res(
            http::status::unauthorized,
            11
        );
        server_res.set(
            http::field::content_type, 
            "text/html; charset=utf-8"
        );
        server_res.body() = failure_html();
        server_res.prepare_payload();
        http::write(socket, server_res);

        oauth2_error_occured_ = true;
        struct oauth2_tokens tokens = {0};
        tokens_promise_.set_value(tokens);
    }
}

tcp::socket OAuth::handle_loopback_connection(tcp::socket conn) {
    beast::flat_buffer buffer;
    http::request<http::string_body> req;

    beast::error_code ec;
    http::read(conn, buffer, req, ec);

    std::cout << req << std::endl;

    auto target = req.target();
    
    std::stringstream uri;
    uri << redirect_uri_ << target;

    auto parsed_target = boost::urls::parse_uri(uri.str());

    if (!parsed_target)
        throw std::runtime_error("failed to parse uri");

    auto it = parsed_target->params().find("code");

    if (it == parsed_target->params().end())
        throw std::runtime_error("no 'code' url parameter");

    code_ = std::string((*it)->value);
    return conn;
}

void OAuth::make_query_params() {
    oauth2_server_url_.params().set("client_id", client_id_);
    oauth2_server_url_.params().set("redirect_uri", redirect_uri_);
    oauth2_server_url_.params().set("response_type", response_type_);
    oauth2_server_url_.params().set("scope", scope_);
    oauth2_server_url_.params().set("code_challenge", code_challenge_.key());
    oauth2_server_url_.params().set(
        "code_challenge_method", code_challenge_.hash_method()
    );

    std::cout << oauth2_server_url_ << std::endl;
}

std::string OAuth::make_token_refresh_query_params(std::string refresh_token) {
    std::stringstream param_stream;

    param_stream << "client_id=" << client_id_ << "&";
    param_stream << "client_secret=" << client_secret_ << "&";
    param_stream << "grant_type=refresh_token&";
    param_stream << "refresh_token=" << refresh_token << "&";

    return param_stream.str();
}

std::string OAuth::make_token_ex_query_params() {
    std::stringstream param_stream;

    param_stream << "client_id=" << client_id_ << "&";
    param_stream << "client_secret=" << client_secret_ << "&";
    param_stream << "code=" << code_ << "&";
    param_stream << "code_verifier=" << code_challenge_.key() << "&";
    param_stream << "grant_type=authorization_code&";
    param_stream << "redirect_uri=" << redirect_uri_;

    return param_stream.str();
}

long OAuth::now() {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(now).count();
}

std::string OAuth::success_html() {
    return R"(
        <!DOCTYPE html>
        <html>
            <head>
                <style>
                    body {
                        display: flex;
                        align-items: center;
                        justify-content: center;
                        font-family: sans-serif;
                        background-color: black;
                        color: white;
                    }
                    div {
                        border: 2px solid green;
                        border-radius: 5px;
                        padding: 20px;
                    }
                </style>
            </head>
            <body>
                <div>
                    <h2>Authentication Successful.</h2>
                    <p>You may close this window.</p>
                </div>
            </body>
        </html>
    )";
}

std::string OAuth::failure_html() {
    return R"(
        <!DOCTYPE html>
        <html>
            <head>
                <style>
                    body {
                        display: flex;
                        align-items: center;
                        justify-content: center;
                        font-family: sans-serif;
                        background-color: black;
                        color: white;
                    }
                    div {
                        border: 2px solid red;
                        border-radius: 5px;
                        padding: 20px;
                    }
                </style>
            </head>
            <body>
                <div>
                    <h2>Authentication Failed.</h2>
                    <p>&#128577;</p>
                </div>
            </body>
        </html>
    )";
}
