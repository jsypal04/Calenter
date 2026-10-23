/**
 * oauth.h
 *
 * This file exposes the OAuth class under a C compatible API.
 * */

#ifndef OAUTH_H
#define OAUTH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

struct oauth2_tokens {
    char*   access_token;
    long    expires_at; // Seconds since unix epoch
    char*   token_type;
    Array*  scope;
    char*   refresh_token;
};

typedef struct oauth_c OAuth_C;

OAuth_C* new_oauth_client();

void destroy_oauth_client(OAuth_C* oc);

struct oauth2_tokens oauth_get_credentials(OAuth_C* oc);

int oauth_authenticate(OAuth_C* oc, struct oauth2_tokens* tokens);

void destroy_oauth2_tokens(struct oauth2_tokens* tokens);

#ifdef __cplusplus
}
#endif

#endif
