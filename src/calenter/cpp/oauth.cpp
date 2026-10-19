#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ios>
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

#define HTTPS_PORT "443"

namespace beast = boost::beast;
namespace net = boost::asio;
namespace http = beast::http;
namespace ssl = net::ssl;
namespace json = boost::json;
using tcp = net::ip::tcp;

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

OAuth::OAuth(std::string credentials_path) : ioc_(), listener_(ioc_) {
    credentials_path_ = credentials_path;

    boost::urls::url_view oauth2_base_url = "https://accounts.google.com/o/oauth2/v2/auth";
    oauth2_server_url_ = boost::urls::url(oauth2_base_url);

    boost::urls::url_view oauth2_base_token_url = "https://oauth2.googleapis.com/token";
    oauth2_token_ex_url_ = boost::urls::url(oauth2_base_token_url);

    client_id_ = "978982380444-r4etnkc05or6rgusmu1cdhv35s5ru840.apps.googleusercontent.com";

    scope_ = "https://www.googleapis.com/auth/calendar.events";

    redirect_addr_ = "127.0.0.1";

    tokens_future_ = tokens_promise_.get_future();
}

OAuth::OAuth(
    std::string credentials_path,
    std::string oauth2_base_url,
    std::string client_id,
    std::string scope
) : ioc_(), listener_(ioc_) {
    credentials_path_ = credentials_path;

    oauth2_server_url_ = boost::urls::url(oauth2_base_url);
    client_id_ = client_id;
    scope_ = scope;

    redirect_uri_ = "127.0.0.1";

    tokens_future_ = tokens_promise_.get_future();
}

void OAuth::store_credentials(struct oauth2_resp credentials) {
    std::stringstream scopes;

    // TODO: Fix the signedness warnings
    int num_scopes = credentials.scope.size();
    for (int i = 0; i < num_scopes; i++) {
        scopes << credentials.scope[i];
        if (i < num_scopes - 1) {
            scopes << " ";
        }
    }

    json::value jv = {
        {"access_token", credentials.access_token},
        {"expires_in", credentials.expires_in},
        {"token_type", credentials.token_type},
        {"scope", scopes.str()},
        {"refresh_token", credentials.refresh_token}
    };

    std::string credentials_json = json::serialize(jv);
    store_credentials(credentials_json);
}

void OAuth::store_credentials(std::string credentials_json) {
    std::ofstream credentials_stream(credentials_path_);

    if (!credentials_stream.is_open())
        throw std::runtime_error("Failed to open credentials file");

    credentials_stream << credentials_json;
}

struct oauth2_resp OAuth::get_credentials() {
    if (!std::filesystem::exists(credentials_path_))
        throw std::runtime_error("Credentials file does not exist");

    std::ifstream credentials_stream(credentials_path_);

    if (!credentials_stream.is_open())
        throw std::runtime_error("Failed to open credentials file");

    std::string file_contents(
        (std::istreambuf_iterator<char>(credentials_stream)), 
        std::istreambuf_iterator<char>()
    );

    json::value parsed_credentials = json::parse(file_contents);
    return parse_oauth2_resp(parsed_credentials);
}

struct oauth2_resp OAuth::authenticate() {
    start_loopback_server();
    make_query_params();

    std::stringstream command;
    command << "xdg-open \"" << oauth2_server_url_.buffer() << "\"";

    std::system(command.str().c_str());

    loopback_thread_.join();

    struct oauth2_resp tokens = tokens_future_.get();

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
    try {
        tcp::socket socket(ioc_);

        listener_.accept(socket);

        handle_loopback_connection(std::move(socket));

        http::request<http::string_body> req{
            http::verb::post,
            oauth2_token_ex_url_.encoded_path(),
            11
        };

        req.set(http::field::host, oauth2_token_ex_url_.host());
        req.set(http::field::content_type, "application/x-www-form-urlencoded");

        req.body() = make_token_ex_query_params();

        std::string content_length = std::to_string(req.body().size());

        req.set(http::field::content_length, content_length);

        std::cout << req << std::endl << std::endl;

        std::string res_body_raw = send_token_ex_request(req);
        store_credentials(res_body_raw);
        json::value res_body = json::parse(res_body_raw);

        struct oauth2_resp tokens = parse_oauth2_resp(res_body);
        tokens_promise_.set_value(tokens);
    } catch (std::exception& e) {
        struct oauth2_resp tokens = {0};
        tokens_promise_.set_value(tokens);
        oauth2_error_occured_ = true;
    }
}

struct oauth2_resp OAuth::parse_oauth2_resp(json::value body) {
    struct oauth2_resp tokens = {0};

    tokens.access_token = body.at("access_token").as_string();
    tokens.expires_in = body.at("expires_in").as_int64();
    tokens.token_type = body.at("token_type").as_string();
    tokens.refresh_token = body.at("refresh_token").as_string();

    auto scopes = body.at("scope").as_string();
    int start = 0;
    int num_scopes = scopes.size();
    for (int i = 0; i < num_scopes; i++) {
        if (scopes[i] != ' ') continue;
        std::string scope = scopes.subview(start, i - start);
        tokens.scope.push_back(scope);
        start = i + 1;
    }

    return tokens;
}

void OAuth::handle_loopback_connection(tcp::socket conn) {
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

// TODO: Wrap this in a try-catch block
std::string OAuth::send_token_ex_request(http::request<http::string_body> req) {
    auto host = oauth2_token_ex_url_.host();

    net::io_context ioc;
    
    ssl::context ctx{ssl::context::tls_client};
    ctx.set_default_verify_paths();

    tcp::resolver resolver(ioc);
    ssl::stream<tcp::socket> stream{ioc, ctx};

    auto const results = resolver.resolve(host, HTTPS_PORT);

    net::connect(stream.next_layer(), results);

    stream.set_verify_mode(ssl::verify_peer);
    stream.set_verify_callback(
        ssl::host_name_verification(host)
    );

    if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
        throw std::runtime_error("Failed to set SNI");
    }

    stream.handshake(ssl::stream_base::client);

    http::write(stream, req);

    beast::flat_buffer buffer;
    http::response<http::string_body> res;

    http::read(stream, buffer, res);

    std::cout << res << std::endl;

    return res.body();
}


// int main() {
//     OAuth oauth_client("/home/jsypal/.config/calenter/credentials.json");
//
//     oauth_client.authenticate();
// }
