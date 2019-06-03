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
/*
struct CPArray
{
    char **arr;
    int len;
};

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

int main()
{
    char buff[1024] = {0};
    while(fgets(buff, 1024, stdin)!=NULL)
    {
        buff[strlen(buff) - 1] = '\0';

        struct CPArray *current = NULL;
        current = splitKeyValue(buff);

        for(int i = 0;i<(current->len);i++)
        printf("%s\n%s\n", current->arr[2*i], current->arr[2*i+1]);

    }
}
*/

void saveRidIndex(DataInfo *indexArr, int count, char *dirPath)
{
    char finalFilename[MAX_FILENAME] = {0};
    strcpy(finalFilename, dirPath);
    strcat(finalFilename, "/rid/0");

    int fd = open(finalFilename, O_RDWR | O_CREAT, 0666);

    if(fd != -1)
    {
        for(int i = 0; i < count; i ++)
        {        
            if(!write(fd, &indexArr[i].rid, sizeof(((DataInfo*)0)->rid)))
            {
                printf("Write error\n");
            }
            else
            {
                if(!write(fd, &indexArr[i].offset, sizeof(((DataInfo*)0)->offset)))
                {
                    printf("Write error\n");
                }
            }
        }
        close(fd);
    }
    else
    {
        fprintf(stderr, "%d: %s(rid index file)\n", errno, strerror(errno));
    }
}