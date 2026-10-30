#include <cstring>
#include <exception>
#include <optional>
#include <sstream>

#include "gcal.h"
#include "calendartxt.h"
#include "gcal.hpp"
#include "http.hpp"
#include "oauth.hpp"
#include "debug.h"

struct gcal {
    GoogleCalendar client;

    gcal(bool silence) : client(silence) {}
};


extern "C" {
    GCal* new_gcal_client(bool silence) {
        return new GCal(silence);
    }

    void destroy_gcal_client(GCal* gcal) {
        delete gcal;
        asm("nop");
    }

    int gcal_authenticate(GCal* gcal) {
        try {
            gcal->client.authenticate();
            return 0;
        } catch (std::exception& e) {
            debug_log("Failed to authenticate with Google: %s\n", e.what());
            return 1;
        }
    }

    int gcal_get_calendar_id(GCal* gcal, char** calendar_id) {
        try {
           *calendar_id = strdup(gcal->client.get_calendar_id().c_str());
            return 0;
        } catch (std::exception& e) {
            debug_log("Failed to get calendar id: %s\n", e.what());
            return 1;
        }
    }

    int gcal_insert_event(GCal* gcal, struct event event) {
        try {
            gcal->client.insert_event(event);
            return 0;
        } catch (std::exception& e) {
            debug_log("Failed to insert Google Calendar event: %s\n", e.what());
            return 1;
        }
    }
}

GoogleCalendar::GoogleCalendar() : http_client_(), oc_(), cout_() {
    basic_setup();
}

GoogleCalendar::GoogleCalendar(bool silence) 
    : http_client_(), oc_(silence), cout_(silence)
{
    basic_setup();
}

void GoogleCalendar::basic_setup() {
    base_url_ = "https://www.googleapis.com/calendar/v3/";
    calendar_id_ = std::nullopt;

    try {
        tokens_ = oc_.get_credentials();
        authenticated_ = true;
    } catch (std::exception& e) {
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
        tokens_ = new_tokens;
        debug_log("Refreshed access token\n");
        return true;
    } catch (std::exception& e) {
        return false;
    }
}

void GoogleCalendar::authenticate() {
    struct oauth2_tokens new_tokens = oc_.authenticate();
    tokens_ = new_tokens;
    authenticated_ = true;
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

// TODO: End time is weird with Calenter
void GoogleCalendar::insert_event(struct event event) {
    if (!is_authenticated())
        throw NotAuthenticated("You are not authenticated.");

    std::string id = get_calendar_id();
    std::string url = base_url_ + "calendars/" + id + "/events";

    auto access_token = std::string("Bearer ") + tokens_.access_token;

    char     key[16] = "\0";
    char s_value[32] = "\0";
    char e_value[32] = "\0";

    if (event.all_day) {
        strncpy(key, "date", 15);
        fmt_ISO_8601_date(s_value, &event.datetime);
        fmt_ISO_8601_date(e_value, &event.datetime);
    } else {
        strncpy(key, "dateTime", 15);
        fmt_ISO_8601_timestamp(s_value, &event.datetime, true);

        struct tm end_tm = event.datetime;
        end_tm.tm_hour += 1;
        fmt_ISO_8601_timestamp(e_value, &end_tm, true);
    }

    json::value start = {
        {key, s_value},
        {"timezone", event.datetime.tm_zone}
    };

    json::value end = {
        {key, e_value},
        {"timezone", event.datetime.tm_zone}
    };

    json::value body = {
        {"summary", event.summary},
        {"start", start},
        {"end", end}
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
}
