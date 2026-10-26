#pragma once

#include <optional>
#include <string>

#include "types.h"
#include "http.hpp"
#include "oauth.hpp"

class GoogleCalendar {
    private:
        std::string base_url_;
        bool authenticated_;
        struct oauth2_tokens tokens_;
        HttpClient http_client_;
        OAuth oc_; 
        std::optional<std::string> calendar_id_;

    public:
        GoogleCalendar();

        GoogleCalendar(OAuth& oc);

        ~GoogleCalendar() {
            destroy_oauth2_tokens(&tokens_);
        }

        /**
         * Checks if the gcal client instance is authenticated. If the 
         * client is authenticated but the access token has expired it
         * attempts to refresh the access token.
         *
         * Returns true if the client is authenticated and false otherwise.
         * */
        bool is_authenticated();

        /**
         * Executes the oauth authentication flow regardless of whether the
         * client is already authenticated.
         * */
        void authenticate();

        /**
         * Returns the id of the authenticated user's primary calendar.
         *
         * Throws either a NotAuthenticated exception or an HttpException
         * for expected errors.
         * */
        std::string get_calendar_id();

        void insert_event(struct event event);
};

/**
 * GoogleCalendar Exceptions
 * */

class NotAuthenticated : public std::exception {
    private:
        std::string msg_;

    public:
        NotAuthenticated(std::string msg) : msg_(std::move(msg)) {}

        const char* what() const noexcept override {
            return msg_.c_str();
        }
};
