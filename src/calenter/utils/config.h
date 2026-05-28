#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

#define CONFIG_DIR "/.config/calenter/"
#define CONFIG_FILE "config"


typedef struct _config {
    char* remote_url;
    bool enable_notifications;
    int  notify_time;
} Config;


Config read_config();


#endif
