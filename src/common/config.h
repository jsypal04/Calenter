#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

#include "types.h"

#define CONFIG_DIR "/.config/calenter/"
#define CONFIG_FILE "config"
#define INI_CONFIG_FILE "config.ini"


typedef struct _config {
    Array* remote_urls;
    bool enable_notifications;
    int  notify_time;
} Config;


Config read_config();


#endif
