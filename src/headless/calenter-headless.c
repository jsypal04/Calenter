#include <stdio.h>

#include "../common/config.h"
#include "../common/sync.h"
#include "../common/array.h"


int main() {

    Config config = read_config();
    for (int i = 0; i < array_len(config.remote_urls); i++) {
        char* url = get_string(config.remote_urls, i);
        printf("remote_url = %s\n", url);
    }
    // sync_calendar_wrapper(config.remote_urls);

    return 0;
}
