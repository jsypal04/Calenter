#include <ctype.h>
#include <string.h>

void trim(char* str) {
    int index1 = 0;
    int index2 = 0;

    while (isspace(str[index1])) index1++;

    while (index1 < strlen(str)) {
        str[index2] = str[index1];
        index1++;
        index2++;
    }

    while (index2 < strlen(str)) {
        str[index2] = '\0';
        index2++;
    }

    index1 = strlen(str) - 1;
    while (isspace(str[index1])) {
        str[index1] = '\0';
        index1--;
    }
}
