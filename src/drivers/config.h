#ifndef CONFIG_H
#define CONFIG_H

#define CONFIG_PATH "/.config/calenter/config"


typedef struct _config {
    char* remote_url;
} Config;


Config read_config();


#endif
