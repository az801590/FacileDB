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

#define MAX_DATA_SIZE 1<<8 //size:2k
#define INPUT_BUFFER 1<<8
#define MAX_FILENAME 32


typedef struct
{
    size_t amountOfData;
    size_t createTime;
} fileInfo;

typedef struct
{
    uint64_t rid;
    size_t data[MAX_DATA_SIZE];
    size_t nextOffset;
    size_t createTime;
    bool delete;
} blockBuffer;

//size_t *base64Encode()


time_t getTime()
{
    return time(NULL);
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

void findAll(int fd)
{
    blockBuffer current;
    char data[MAX_DATA_SIZE] = {0};

    while(read(fd, &current, sizeof(blockBuffer)))
    {
        if(!current.delete)
        {
            memcpy(data, current.data, MAX_DATA_SIZE);

            for(char *p = NULL; p = strstr(data, ",");)
            {
                *p = '\n';
            }
            
            printf("------------\n");
            printf("%s\n", data);
            printf("------------\n");
        }
    }
}

void deleteAll(int fd)
{
    blockBuffer current;

    while(read(fd, &current, sizeof(blockBuffer)))
    {
        lseek(fd, -sizeof(blockBuffer), SEEK_CUR);

        current.delete = true;
        write(fd, &current, sizeof(blockBuffer));
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
        write(fd, &new, sizeof(fileInfo));
    }

    //close(fd);
    return fd;
}

int main(int argc, char *argv[])
{
    int status = 0;
    char buff[INPUT_BUFFER];
    char *argu = NULL;
    int currentDbFile;
    bool fileSelected = false;
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
                    if(currentDbFile = useDB(argu))
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
                            blockBuffer inputBuffer;
                            blockInit(&inputBuffer, fileDescription);

                            memcpy(inputBuffer.data, argu, MAX_DATA_SIZE);
                            lseek(currentDbFile, 0, SEEK_END);
                            write(currentDbFile, &inputBuffer, sizeof(blockBuffer));
                        }
                        else if(status == 4)
                        {
                            //find
                            lseek(currentDbFile, sizeof(fileInfo), SEEK_SET);

                            if(strcmp(argu, "*") == 0)
                            {
                                findAll(currentDbFile);
                            }

                        }
                        else if(status == 5)
                        {
                            //delete
                            lseek(currentDbFile, sizeof(fileInfo), SEEK_SET);

                            if(strcmp(argu, "*") == 0)
                            {
                                deleteAll(currentDbFile);
                            }

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
    /*
    FILE *f = fopen("data", "r");
    fseek(f, 10, SEEK_SET);
    char s[100]={0};
    fread(s, sizeof(char), 10, f);
    printf("%s\n", s);
    fclose(f);
    */
/*
    FILE *f = fopen("data", "wb+");
    inputBuffer buffer;
    char buff[MAX_DATA_SIZE];

    fgets(buff, MAX_DATA_SIZE, stdin);
    buff[strlen(buff)-1]='\0';
    memcpy(buffer.data, buff, MAX_DATA_SIZE);
    buffer.rid=1;
    buffer.nextOffset=-1;

    fwrite(&buffer, sizeof(inputBuffer), 1, f);
    fclose(f);

    f = fopen("data", "rb");
    inputBuffer readBuff;
    fread(&readBuff, sizeof(inputBuffer), 1, f);

    printf("%d\t%s\t%zd\n", readBuff.rid, readBuff.data, readBuff.nextOffset);
    fclose(f);
*/
/*
    int fd = open("./data", O_RDWR , 0666);
    printf("%d\n", fd);
    if(fd<0)
    {
        fd = open("./data", O_RDWR | O_CREAT, 0666);
        fileInfo new;
        fileInfoInit(&new);
        write(fd, &new, sizeof(fileInfo));
    }
    fileInfo *profile = (fileInfo*) mmap(NULL, sizeof(fileInfo), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    //fileInfo rrr;
    //read(fd, &rrr, sizeof(fileInfo));
    close(fd);

    //printf("%ld\t%ld\n", rrr.amountOfData, rrr.createTime);
*/

    if(fileDescription)
    {
        //printf("%ld\t%ld\n", fileDescription->amountOfData, fileDescription->createTime);
        munmap(fileDescription, sizeof(fileInfo));
    }

    close(currentDbFile);
    return 0;
}