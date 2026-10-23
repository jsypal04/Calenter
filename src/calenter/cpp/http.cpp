#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/http/string_body_fwd.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/url.hpp>
#include <boost/json.hpp>

#include "http.hpp"

namespace beast = boost::beast;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;


bool HttpClient::is_success(http::response<http::string_body> res) {
    int status_code = res.result_int();
    if (status_code >= 200 && status_code < 300) {
        return true;
    }
    return false;
}

void HttpClient::get(std::string url) {
    boost::urls::url_view url_view = url;
    auto parsed_url = boost::urls::url(url_view);

    get(parsed_url);
}

void HttpClient::get(boost::urls::url url) {
    url_ = url;
    request_ = http::request<http::string_body>(
        http::verb::post, 
        url_.encoded_target(), 
        11
    ); 

    request_.set(http::field::host, url_.host());
}

void HttpClient::post(std::string url) {
    boost::urls::url_view url_view = url;
    auto parsed_url = boost::urls::url(url_view);

    post(parsed_url);
}

void HttpClient::post(boost::urls::url url) {
    url_ = url;
    request_ = http::request<http::string_body>(
        http::verb::post, 
        url_.encoded_target(), 
        11
    ); 

    request_.set(http::field::host, url_.host());
}

void HttpClient::header(http::field header, std::string value) {
    request_.set(header, value);
}

void HttpClient::body(std::string body) {
    request_.body() = body;
}

void HttpClient::body(json::value body) {
    std::string str = json::serialize(body);
    request_.body() = str;
}

http::response<http::string_body> HttpClient::send() {
    std::string content_length = std::to_string(request_.body().size());
    request_.set(http::field::content_length, content_length);

    net::io_context ioc;
    
    ssl::context ctx{ssl::context::tls_client};
    ctx.set_default_verify_paths();

    tcp::resolver resolver(ioc);
    ssl::stream<tcp::socket> stream{ioc, ctx};

    auto const results = resolver.resolve(url_.host(), https_port_);

    net::connect(stream.next_layer(), results);

    stream.set_verify_mode(ssl::verify_peer);
    stream.set_verify_callback(
        ssl::host_name_verification(url_.host())
    );

    if (!SSL_set_tlsext_host_name(stream.native_handle(), url_.host().c_str())) {
        throw std::runtime_error("Failed to set SNI");
    }

    stream.handshake(ssl::stream_base::client);

    http::write(stream, request_);

    beast::flat_buffer buffer;
    http::response<http::string_body> res;

    http::read(stream, buffer, res);

    return res;
}
