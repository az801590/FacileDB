#include <time.h>

#include "db_utils.h"

/*
** reference: (https://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way)
*/
char *strTrim(char *input)
{
    char *str = input;
    while (isspace(*str))
    {
        *str = 0;
        str++;
    }

    // all space = false
    if (*str != 0)
    {
        char *end = str + strlen(str) - 1;
        while (end > str && isspace(*end))
        {
            end--;
        }

        *(end + 1) = '\0';
    }

    return str;
}

time_t getTime()
{
    return time(NULL);
}
