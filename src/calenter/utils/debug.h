#ifndef DEBUG_H
#define DEBUG_H


#define LOG_LOCATION() debug_log("running %s:%d\n", __FILE_NAME__, __LINE__)

#define LOG_FUNC(x) debug_log("\n"); debug_log("%s\n", x)

#define LOG_DIM() debug_log("LINES = %d, COLS = %d\n", LINES, COLS);

/*
 * Function to write output to a logfile instead of the terminal
 */
void debug_log(const char *format, ...);

#endif
