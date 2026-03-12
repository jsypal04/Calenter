/*
 * ics.c
 *
 * This file contains functions to parse events in a ICS
 * file into data structures that can be written to calendat.txt
 * using the functions in calendartxt.c.
 *
 * NOTE: This file does not work yet and is not actually compiled and
 * linked into the project yet.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define INIT_EVENTS_SIZE 500

typedef struct _ics_line {
    char* line;
    int length;
    FILE* ics_file;
} Line;

typedef struct _param {
    char* name;
    char** values;
} Param;

typedef struct _content_line {
    char* name;
    Param* params;
} ContentLine;

/*
 * Gets the next line in the ics file (accounting for line folding).
 * Returns the number of characters read or EOF if the end of file was reached.
 * */
int get_line(Line* line) {
    int buf_len = 2048;
    char* buffer = malloc(sizeof(char) * buf_len);
    memset(buffer, '\0', buf_len);
    
    char c;
    int index = 0;

    // I know this is probably dumb to have while (true) loop but I don't care
    while (true) {
        while ((c = fgetc(line->ics_file)) != '\r') {
            if (c == EOF) return EOF;

            if (index == buf_len) {
                char* bigger_buffer = malloc(sizeof(char) * 2 * buf_len);
                memset(buffer, '\0', buf_len);

                for (int i = 0; i < buf_len; i++) {
                    bigger_buffer[i] = buffer[i];
                }
                buf_len *= 2;

                free(buffer);
                buffer = bigger_buffer;
            }

            buffer[index] = c;
            index++;
        }

        c = fgetc(line->ics_file); // takes care of the newline
        c = fgetc(line->ics_file);
        if (c == ' ' || c == '\t') {
            ungetc(c, line->ics_file);
        } else {
            break;
        }
    }
    ungetc(c, line->ics_file);

    line->line = buffer;
    line->length = buf_len;

    return index;
}

ContentLine parse_content_line(Line line) {
    ContentLine cline = {0};

    // parse name
    // parse the list of params
    // parse value

    return cline;
}


void parse_ics(char* path) {
    Line line = {0};
    line.ics_file = fopen(path, "r");

    while (get_line(&line) != EOF) {
        printf("%s\n", line.line);
        free(line.line);
        line.line = NULL;
    }

    fclose(line.ics_file);
}


int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: ./ics path/to/ics/file\n");
    }

    parse_ics(argv[1]);

    return 0;
}
