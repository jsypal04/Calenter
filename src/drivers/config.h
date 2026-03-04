#ifndef CONFIG_H
#define CONFIG_H

#define CONFIG_DIR "/.config/calenter/"
#define CONFIG_FILE "config"


typedef struct _config {
    char* remote_url;
} Config;


Config read_config();


#endif
