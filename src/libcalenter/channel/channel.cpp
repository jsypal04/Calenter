#include <condition_variable>
#include <cstring>
#include <exception>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <strings.h>
#include <utility>
#include <cstring>

#include "channel.h"

template <typename T>
class Channel {
private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool closed_ = false;

public:
    void send(T const& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (closed_) throw std::runtime_error("Sent message to closed channel");
        queue_.push(value);
        cv_.notify_one();
    }

    bool receive(T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() { return !queue_.empty() || closed_; });

        if (queue_.empty() && closed_) {
            return false;
        }

        value = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    void close() {
        std::unique_lock<std::mutex> lock(mutex_);
        closed_ = true;
        cv_.notify_all();
    }
};

typedef struct Channel_C {
    Channel<std::string> ch;
} Channel_C;

extern "C" {
    Channel_C* new_channel() {
        Channel_C* channel = new Channel_C();
        return channel;
    }

    void destroy_channel(Channel_C* channel) {
        delete channel;
    }

    int channel_send(Channel_C* channel, char* value) {
        try {
            channel->ch.send(std::string(value));
        } catch (std::exception& e) {
            return -1;
        }

        return 0;
    }

    char* channel_receive(Channel_C* channel, size_t* value_size) {
        std::string str_value = std::string(""); 
        bool result = channel->ch.receive(str_value);

        if (!result) return NULL;

        *value_size = strlen(str_value.c_str()) + 1;
        char* value = (char*) malloc(sizeof(char) * (*value_size));
        bzero(value, *value_size);

        strncpy(value, str_value.c_str(), *value_size);
        return value;
    }

    void channel_close(Channel_C* channel) {
        channel->ch.close();
    }

}
