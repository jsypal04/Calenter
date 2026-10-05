#ifndef JSON_H
#define JSON_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Channel_C Channel_C;

/**
 * Creates a new channel
 * */
Channel_C* new_channel();

/**
 * Cleans up the given channel
 * */
void destroy_channel(Channel_C* channel);

/**
 * Sends a string over the given channel. Returns 0 on success
 * and -1 on error (I think it only fails if the channel is closed)
 * */
int channel_send(Channel_C* channel, char* value);

/**
 * Receives a value from the given channel and returns it.
 * Returns NULL if no value was received.
 * */
char* channel_receive(Channel_C* channel, size_t* value_size);

/**
 * Closes the given channel
 * */
void channel_close(Channel_C* channel);

#ifdef __cplusplus
}
#endif

#endif
