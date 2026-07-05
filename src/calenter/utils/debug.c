#include <stdarg.h>

#include "debug.h"


#ifdef DEBUG

#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#define DEBUG_LOG_FILE "/.calendar/logs/debug.log"

void debug_log(const char* format, ...) {
    time_t raw_time = time(NULL);
    struct tm* info = localtime(&raw_time);
    char buffer[80];

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", info);

    char log_path[1024] = "\0";
    char* home = getenv("HOME");
    if (home == NULL) return;

    sprintf(log_path, "%s%s", home, DEBUG_LOG_FILE);

    va_list args;
    va_start(args, format);

    FILE* debug_log_file = fopen(log_path, "a");

    fprintf(debug_log_file, "[%s] ", buffer);
    vfprintf(debug_log_file, format, args);
    va_end(args);

    fclose(debug_log_file);
}
#else
void debug_log(const char* format, ...) {
    return;
}
#endif
