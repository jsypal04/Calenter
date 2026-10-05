#ifndef DEBUG_H
#define DEBUG_H


#define printf(format, ...) lib_printf(format, ##__VA_ARGS__)

#define LOG_LOCATION() debug_log("Running %s:%d\n", __FILE__, __LINE__);

void lib_printf(const char* format, ...);

/*
 * Function to write output to a logfile instead of the terminal
 */
void debug_log(const char* format, ...);

#endif
