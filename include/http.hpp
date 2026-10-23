#pragma once

#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <boost/url/url.hpp>
#include <boost/url/url_view.hpp>

namespace http = boost::beast::http;
namespace json = boost::json;

class HttpClient {
    private:
        http::request<http::string_body> request_;
        boost::urls::url url_;

        const std::string http_port_  = "80";
        const std::string https_port_ = "443";


    public:

        static bool is_success(http::response<http::string_body> res);

        void get(std::string url);
        void get(boost::urls::url url);

        void post(std::string url);
        void post(boost::urls::url url);

        void header(http::field header, std::string value);
        void body(std::string body);
        void body(json::value body);

        http::response<http::string_body> send();

};

