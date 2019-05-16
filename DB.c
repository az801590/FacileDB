#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#include <sys/mman.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <dirent.h>

#define MAX_DATA_SIZE 2048 
#define INPUT_BUFFER 128
#define MAX_FILENAME 32

typedef struct
{
    size_t amountOfData;
    size_t createTime;
} fileInfo;

typedef struct
{
    uint64_t rid;
    size_t data[(MAX_DATA_SIZE/sizeof(size_t))+1];
    size_t nextOffset;
    size_t createTime;
    bool delete;
} blockBuffer;

void putOneRow(int, fileInfo*, char*);
void inputByFile(FILE*, int, fileInfo*);
bool findOneRow(char*, char*, char*, char*);
int seqFind(int, char*);
char **splitKeyValue(char*);


time_t getTime()
{
    return time(NULL);
}

double timeSpend(clock_t t1, clock_t t2)
{
    return (t2-t1)/(double)(CLOCKS_PER_SEC);
}

void blockInit(blockBuffer *inputBuffer, fileInfo *fileDescription)
{
    inputBuffer->rid = fileDescription->amountOfData;
    fileDescription->amountOfData ++;

    memset(inputBuffer->data, '\0', sizeof(((blockBuffer*)0)->data));
    inputBuffer->nextOffset = -1;
    inputBuffer->createTime = getTime();
    inputBuffer->delete = false;
}

void fileInfoInit(fileInfo *fileDescription)
{
    fileDescription->amountOfData = 0;
    fileDescription->createTime = getTime();
}

fileInfo *getFileInfo(int fd)
{
    return (fileInfo*) mmap(NULL, sizeof(fileInfo), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
}

int findAll(int fd)
{
    blockBuffer current;
    char *p = NULL;
    int count = 0;

    while(read(fd, &current, sizeof(blockBuffer)))
    {
        if(!current.delete)
        {
            printf("------------\n");
            p = (char*)current.data;

            while(*p)
            {
                printf("%s", p);
                p += strlen(p) + 1;
                printf(" : ");
                printf("%s\n", p);
                p +=strlen(p) + 2;
            }

            printf("------------\n");

            count++;
        }
    }

    return count;
}

char **splitKeyValue(char *str)
{
    char **keyValue = malloc(2*sizeof(char*));
    keyValue[0] = str;
    keyValue[1] = strchr(str, ':');

    *keyValue[1] = '\0';
    keyValue[1]++;

    return keyValue;
}

int seqFind(int fd, char *str)
{
    char output[MAX_DATA_SIZE] = {0};
    int count = 0;

    char **temp = splitKeyValue(str);
    char *key = temp[0];
    char *value = temp[1];

    blockBuffer current;
    while(read(fd, &current, sizeof(blockBuffer)))
    {
        if(!current.delete)
        {
            memset(output, 0, MAX_DATA_SIZE);
            strcpy(output, "------------\n");
            //char *p = (char*) current.data;

            if(findOneRow((char*)current.data, key, value, output))
            {
                count++;
                printf("%s", output);
            }
        }
    }

    free(temp);
    return count;
}

bool findOneRow(char *str, char *key, char*value, char *output)
{
    bool keyPare = false, valuePare = false;
    char *p = str;

    while(*p)
    {
        if(strcmp(p, key) == 0)
        {
            keyPare = true;
        }
        else
        {
            keyPare = false;
        }

        strcat(output, p);
        strcat(output, " : ");
        p += strlen(p) + 1;

        if(keyPare && strcmp(p, value) == 0)
        {
            valuePare = true;
        }

        strcat(output, p);
        strcat(output, "\n");
        p += strlen(p) + 2;
    }

    strcat(output, "------------\n");

    if(valuePare)
    {
        return true;
    }
    else
    {
        return false;
    }
}

int deleteAll(int fd)
{
    blockBuffer current;
    int count = 0;

    while(read(fd, &current, sizeof(blockBuffer)))
    {
        if(!(current.delete))
        {
            current.delete = true;
            lseek(fd, -sizeof(blockBuffer), SEEK_CUR);
            if(!(write(fd, &current, sizeof(blockBuffer))))
            {
                printf("Write error!\n");
            }
            count++;
        }
    }
    return count;
}

int putByFile(FILE *f, int fd, fileInfo *fileDescription)
{
    char temp[MAX_DATA_SIZE] = {0};
    char buff[MAX_DATA_SIZE] = {0};
    char output[MAX_DATA_SIZE] = {0};
    char *key = NULL, *value = NULL;
    char *position = NULL;
    int count = 0;

    int i = 0;
    while(fgets(buff, MAX_DATA_SIZE, f) != NULL)
    {
        buff[strlen(buff)-1] = '\0';
        
        if(i == 0)
        {
            position = output;
            strcpy(temp, buff);
            i++;
        }
        else
        {
            if(*buff == '@')
            {
                /*
                ** For bug in file youtube.rec
                **
                */
                if((value = strchr(temp, ':')))
                {
                    //format it
                    key = temp + 1;
                    //value = strchr(temp, ':');
                    *value = '\0';
                    value ++;

                    strcpy(position, key);
                    position += strlen(key) + 1;

                    if(strcmp(value, "") == 0)
                    {
                        strcpy(position, " ");
                    }
                    else
                    {
                        strcpy(position, value);
                    }

                    position += strlen(value) + 2;
                    //end of format
                    i++;
                }
                else
                {
                    strcat(temp, buff);
                }

                /*
                **
                */

                if(*(buff + 1) == '\0')
                {
                    putOneRow(fd, fileDescription, output);
                    memset(output, 0, MAX_DATA_SIZE);
                    count++;
                    i = 0;
                    //printf("%d\n", count);
                }
                else
                {
                    key = value = NULL;
                    memset(temp, 0, MAX_DATA_SIZE);
                    strcpy(temp, buff);
                }
            }
            else
            {
                strcat(temp, buff);
            }
        }
    }

    return count;
}

int deleteFromFind(int fd, char *str)
{
    blockBuffer current;
    char output[MAX_DATA_SIZE] = {0};
    int count = 0;
    char *p = NULL;

    char **temp = splitKeyValue(str);
    char *key = temp[0];
    char *value = temp[1];

    while(read(fd, &current, sizeof(blockBuffer)))
    {
        if(!current.delete)
        {
            memset(output, 0, MAX_DATA_SIZE);
            strcpy(output, "------------\n");
            p = (char*)current.data;

            if(findOneRow(p, key, value, output))
            {
                count++;
                current.delete = true;
                lseek(fd, -sizeof(blockBuffer), SEEK_CUR);
                if(!(write(fd, &current, sizeof(blockBuffer))))
                {
                    printf("Write error!\n");
                }
                lseek(fd, sizeof(blockBuffer), SEEK_CUR);
            }
        }
    }

    free(temp);
    return count;
}

void putOneRow(int currentDbFile, fileInfo *fileDescription, char *one)
{
    blockBuffer inputBuffer;
    blockInit(&inputBuffer, fileDescription);

    memcpy(inputBuffer.data, one, MAX_DATA_SIZE);
    lseek(currentDbFile, 0, SEEK_END);
    if(!(write(currentDbFile, &inputBuffer, sizeof(blockBuffer))))
    {
        printf("Write error!\n");
    }
}

void listAllDbFiles(const char *dir)
{
    struct dirent *de; 
    DIR *dr = opendir(dir);

    printf("----------------\n");
    if(dr == NULL) 
    { 
        printf("Could not open current directory"); 
        return; 
    }
    while((de=readdir(dr))!=NULL)
    {
        if(strcmp(de->d_name, ".")!=0 && strcmp(de->d_name, "..")!=0)
        {
            printf("%s\n", de->d_name);
        }
    }
    printf("----------------\n");

    closedir(dr);     
}

int useDB(const char *filename)
{
    char finalFilename[MAX_FILENAME] = "./dbs/";
    strcat(finalFilename, filename);

    int fd = open(finalFilename, O_RDWR , 0666);

    if(fd < 0)
    {
        fd = open(finalFilename, O_RDWR | O_CREAT, 0666);
        fileInfo new;
        fileInfoInit(&new);
        if(!(write(fd, &new, sizeof(fileInfo))))
        {
            printf("Write error!\n");
        }
    }

    //close(fd);
    return fd;
}

int main(int argc, char *argv[])
{
    int status = 0;
    char buff[INPUT_BUFFER];
    char *argu = NULL;
    int currentDbFile = -1;
    bool fileSelected = false;
    clock_t start, end;
    fileInfo *fileDescription = NULL;

    printf("Database start.\n> ");

    while(fgets(buff, INPUT_BUFFER, stdin) != NULL)
    {
        buff[strlen(buff)-1]='\0';

        argu = strtok(buff, " ");
        for(int i = 0;argu != NULL && i<2;i++)
        {
            if(status == 0)
            {
                if(strcmp(argu, "exit") == 0)
                {
                    //close the database
                    status = -1;
                    break;
                }
                else if(strcmp(argu, "list") == 0)
                {
                    status = 1;
                    listAllDbFiles("./dbs");
                    break;
                }
                else if(strcmp(argu, "use") == 0)
                {
                    status = 2;
                    fileSelected = true;
                }
                else if(strcmp(argu, "put") == 0)
                {
                    status = 3;
                }
                else if (strcmp(argu, "find") == 0)
                {
                    status = 4;
                }
                else if(strcmp(argu, "delete") == 0)
                {
                    status = 5;
                }
                else
                {
                    printf("Command not found!\n");
                    break;
                }
            }
            else
            {
                if(status == 2)
                {
                    //use
                    if((currentDbFile = useDB(argu)))
                    {
                        fileDescription = getFileInfo(currentDbFile);
                        printf("Switch to %s.\n", argu);
                    }
                }
                else 
                {
                    if(fileSelected)
                    {
                        if(status == 3)
                        {
                            //put
                            start = clock();
                            /*
                            blockBuffer inputBuffer;
                            blockInit(&inputBuffer, fileDescription);

                            memcpy(inputBuffer.data, argu, MAX_DATA_SIZE);
                            lseek(currentDbFile, 0, SEEK_END);
                            write(currentDbFile, &inputBuffer, sizeof(blockBuffer));
                            */

                            //put by file
                            FILE *f = fopen(argu, "r");
                            printf("Records: %d\t", putByFile(f, currentDbFile, fileDescription));
                            fclose(f);

                            end = clock();
                            printf("Time: %lfsec\n", timeSpend(start, end));
                        }
                        else if(status == 4)
                        {
                            //find
                            start = clock();
                            lseek(currentDbFile, sizeof(fileInfo), SEEK_SET);

                            if(strcmp(argu, "*") == 0)
                            {
                                printf("Records: %d\t", findAll(currentDbFile));
                            }
                            else
                            {
                                printf("Records: %d\t", seqFind(currentDbFile, argu));
                            }
                            end = clock();

                            printf("Time: %lfsec\n", timeSpend(start, end));
                        }
                        else if(status == 5)
                        {
                            //delete
                            start = clock();
                            lseek(currentDbFile, sizeof(fileInfo), SEEK_SET);

                            if(strcmp(argu, "*") == 0)
                            {
                                printf("Delete: %d\t", deleteAll(currentDbFile));
                            }
                            else
                            {
                                printf("Relete: %d\t", deleteFromFind(currentDbFile, argu));
                            }
                            end = clock();
                            printf("Time: %lfsec\n", timeSpend(start, end));
                        }

                    }
                    else
                    {
                        printf("Choose a db file first.\n");
                    }

                }
            }

            argu = strtok(NULL, " ");
        }

        if(status < 0)
        {
            break;
        }
        else
        {
            status = 0;
            printf("> ");
        }
    }

    if(fileDescription)
    {
        munmap(fileDescription, sizeof(fileInfo));
    }

    close(currentDbFile);
    return 0;
}