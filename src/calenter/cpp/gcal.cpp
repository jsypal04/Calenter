#include <exception>
#include <optional>
#include <sstream>
#include <stdexcept>

#include "gcal.h"
#include "gcal.hpp"
#include "http.hpp"
#include "oauth.hpp"

typedef struct gcal {
    GoogleCalendar gcal;
} GCal;

extern "C" {
    GCal* new_gcal_client() {
        return new GCal();
    }

    void destroy_gcal_client(GCal* gcal) {
        delete gcal;
    }

    int gcal_authenticate(GCal* gcal) {
        try {
            gcal->gcal.authenticate();
            return 0;
        } catch (std::exception& e) {
            return 1;
        }
    }

    int gcal_get_calendar_id(GCal* gcal, char** calendar_id) {
        try {
           *calendar_id = strdup(gcal->gcal.get_calendar_id().c_str());
            return 0;
        } catch (std::exception& e) {
            std::cout << e.what() << std::endl;
            return 1;
        }
    }

    int gcal_insert_event(GCal* gcal, struct event event) {
        try {
            gcal->gcal.insert_event(event);
            return 0;
        } catch (std::exception& e) {
            std::cout << e.what() << std::endl;
            return 1;
        }
    }
}

GoogleCalendar::GoogleCalendar() : http_client_(), oc_() {
    base_url_ = "https://www.googleapis.com/calendar/v3/";
    calendar_id_ = std::nullopt;

    try {
        tokens_ = oc_.get_credentials();
        authenticated_ = true;
    } catch (std::exception& e) {
        tokens_ = {0};
        authenticated_ = false;
    }
}

bool GoogleCalendar::is_authenticated() {
    if (!authenticated_)
        return false;

    auto now = OAuth::now();
    bool access_token_expired = tokens_.expires_at <= now;

    if (!access_token_expired)
        return true;

    try {
        struct oauth2_tokens new_tokens = oc_.refresh(
            std::string(tokens_.refresh_token)
        );
        destroy_oauth2_tokens(&tokens_);
        tokens_ = new_tokens;
        return true;
    } catch (std::exception& e) {
        return false;
    }
}

void GoogleCalendar::authenticate() {
    try {
        struct oauth2_tokens new_tokens = oc_.authenticate();
        destroy_oauth2_tokens(&tokens_);
        tokens_ = new_tokens;
        authenticated_ = true;
    } catch (std::exception& e) {
        std::stringstream msg;
        msg << "Failed to authenticate: " << e.what();
        throw std::runtime_error(msg.str());
    }
}

std::string GoogleCalendar::get_calendar_id() {
    if (calendar_id_.has_value()) {
        return calendar_id_.value();
    }

    if (!is_authenticated())
        throw NotAuthenticated("You are not authenticated.");

    auto url = base_url_ + "users/me/calendarList"; 

    auto access_token = std::string("Bearer ") + tokens_.access_token;

    http_client_.get(url);
    http_client_.header(http::field::authorization, access_token);
    auto res = http_client_.send();

    if (!HttpClient::is_success(res)) {
        std::stringstream msg_stream;
        msg_stream << "Google Calendar http request failed with code "; 
        msg_stream << res.result_int(); 
        throw HttpException(msg_stream.str());
    }


    auto body = json::parse(res.body());
    auto items = body.at("items").as_array();
    
    for (auto e : items) {
        bool primary = false;
        try {
            primary = e.at("primary").as_bool();
        } catch (std::exception& e) {
            continue;
        }
        if (!primary) continue;
        calendar_id_ = std::string(e.at("id").as_string());
        break;
    }

    return calendar_id_.value();
}

// TODO: This function is not complete yet.
void GoogleCalendar::insert_event(struct event event) {
    if (!is_authenticated())
        throw NotAuthenticated("You are not authenticated.");

    std::string id = get_calendar_id();
    std::string url = base_url_ + "calendars/" + id + "/events";

    std::cout << "url = " << url << std::endl;

    auto access_token = std::string("Bearer ") + tokens_.access_token;

    json::value body = {
        {"summary", std::string(event.summary)}
    };

    http_client_.post(url);
    http_client_.header(http::field::authorization, access_token);
    http_client_.body(body);
    auto res = http_client_.send();

    if (!HttpClient::is_success(res)) {
        std::stringstream msg_stream;
        msg_stream << "Google Calendar http request failed with code "; 
        msg_stream << res.result_int(); 
        throw HttpException(msg_stream.str());
    }

    std::cout << res << std::endl;
}
