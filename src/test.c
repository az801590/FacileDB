#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#define INPUT_BUFFER 128
#define MAX_PAIR_NUM 15

struct CPArray
{
    char **arr;
    int len;
};
/*
struct CPArray *splitKeyValue(char *str)
{
    int count = 0;
    struct CPArray *current =(struct CPArray*) malloc(sizeof(struct CPArray)); 
    current->arr = malloc( MAX_PAIR_NUM * 2 * sizeof(char*));

    do
    {
        char *start = strchr(str, '"') + 1;
        char *end = strstr(start, "\":");
        *end = '\0';
        current->arr[count] = start;
        str = end + 1;

        start = strchr(str, '"') + 1;
        end = strchr(start, '"');
        *end = '\0';
        current->arr[count + 1] = start;
        str = end + 1;

        count += 2;
    }
    while((str = strstr(str, ",\"")) != NULL);

    current->len = count / 2;
    return current;
}
*/
int main()
{
    printf("%d\t%d\n", sizeof(struct CPArray*), sizeof(char*));
}
