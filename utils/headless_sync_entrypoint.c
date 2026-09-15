#include "../src/calenter/utils/config.h"
#include "../src/calenter/utils/sync.h"


int main() {

    Config config = read_config();
    sync_calendar_wrapper(config.remote_url);

    return 0;
}
