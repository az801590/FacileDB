#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#include "index.h"
#include "block.h"

bool indexExisted(const char *dir, const char *indexName)
{
    struct dirent *de; 
    DIR *dr = opendir(dir);
    bool find = false;

    if(dr == NULL) 
    { 
        printf("Could not open current directory"); 
    }
    else
    {
        while((de = readdir(dr)) != NULL)
        {
            if(strcmp(de->d_name, indexName) == 0)
            {
                find = true;
                break;
            }
        }

        closedir(dr);
    }

    return find;
}

int makeIndex(Index *indexArr, int fd, char *key)
{
    int count = 0;
    char *findIndexValue = NULL;

    Block current;
    while(read(fd, &current, sizeof(Block)))
    {
        if(!current.delete)
        {
            findIndexValue = NULL;
            if((findIndexValue = findValueWithKey((char*)current.data, key)) != NULL)
            {
                indexArr[count].rid = current.rid;
                strncpy(indexArr[count].indexValue, findIndexValue, MAX_INDEX_SIZE);
                indexArr[count].offset = lseek(fd, -sizeof(Block), SEEK_CUR);
                lseek(fd, sizeof(Block), SEEK_CUR);

                count++;
            }
        }
    }

    return count;
}

Index *readIndex(int amountOfData, const char *currentIndexDir, const char *indexName)
{
    int fd = -1;

    char dir[MAX_FILENAME] = {0};
    char indexFileName[MAX_FILENAME] = {0};

    int count = (amountOfData / (INDEX_FILE_SIZE / sizeof(Index))) + 1;
    Index *indexArr = (Index*) calloc (count, sizeof(Index));

    for(int i = 0; i < count; i++)
    {
        sprintf(indexFileName, "%d", i);
        strcpy(dir, currentIndexDir);
        strcat(dir, "/");
        strcat(dir, indexName);
        strcat(dir, "/");
        strcat(dir, indexFileName);

        if((fd = open(dir, O_RDWR, 0666)) != -1)
        {
            if(!read(fd, &indexArr[i], sizeof(Index)))
            {
                printf("read failed!\n");
            }
                 
            close(fd);   
        }
        else
        {
            break;
        }
    }

    return indexArr;
}

void saveIndex(Index *indexArr, int count, const char *dirPath, const char *indexName)
{
    int num = -1;
    int fd = -1;
    char finalFilename[MAX_FILENAME] = {0};
    char toNum[MAX_FILENAME] = {0};

    for(int i = 0; i < count; i++)
    {
        if(i % (INDEX_FILE_SIZE / sizeof(Index)) == 0)
        {
            if(num != -1)
            {
                close(fd);
            }

            num++;
            sprintf(toNum, "%d", num);

            strcpy(finalFilename, dirPath);
            strcat(finalFilename, "/");
            strcat(finalFilename, indexName);
            strcat(finalFilename, "/");
            strcat(finalFilename, toNum);

            fd = open(finalFilename, O_RDWR | O_CREAT, 0666);
        }

        if(!write(fd, &indexArr[i], sizeof(Index)))
        {
            printf("Write error\n");
        }
    }
    if(fd != -1)
    {
        close(fd);
    }
}

void indexSearch(Index *indexArr, int amountOfData, const char *dir, char *indexName, char *value, int currentDbFile)
{
    int fd = -1;
    char finalFilename[MAX_FILENAME] = {0};
    char toNum[MAX_FILENAME] = {0};
 
    for(int i = 0; i < ((amountOfData / (INDEX_FILE_SIZE / sizeof(Index))) + 1); i++)
    {
        if(i == amountOfData/64)
        {
            sprintf(toNum, "%d", i);
            strcpy(finalFilename, dir);
            strcat(finalFilename, "/");
            strcat(finalFilename, indexName);
            strcat(finalFilename, "/");
            strcat(finalFilename, toNum);

            fd = open(finalFilename, O_RDWR, 0666);
            off_t offset = findIndexFile(fd, value);
            if(offset > 0)
            {
                char output[MAX_DATA_SIZE] = {0};
                strcpy(output, "------------\n");
                lseek(currentDbFile, offset, SEEK_SET);
                Block current;
                if(read(currentDbFile, &current, sizeof(current)))
                {
                    if(findOneRow((char*)current.data, indexName, value, output))
                    {
                        printf("%s\n", output);
                    }
                }
            }

            close(fd);
        }
        else
        {
            if(strncmp(indexArr[i+1].indexValue, value, MAX_INDEX_SIZE) > 0)
            {
                sprintf(toNum, "%d", i);
                strcpy(finalFilename, dir);
                strcat(finalFilename, "/");
                strcat(finalFilename, indexName);
                strcat(finalFilename, "/");
                strcat(finalFilename, toNum);

                fd = open(finalFilename, O_RDWR, 0666);
                off_t offset = findIndexFile(fd, value);
                if(offset > 0)
                {
                    char output[MAX_DATA_SIZE] = {0};
                    strcpy(output, "------------\n");
                    lseek(currentDbFile, offset, SEEK_SET);
                    Block current;
                    if(read(currentDbFile, &current, sizeof(current)))
                    {
                        if(findOneRow((char*)current.data, indexName, value, output))
                        {
                            printf("%s\n", output);
                        }
                    }
                }

                close(fd);
            }
        }
    }
}

off_t findIndexFile(int fd, char *value)
{
    Index current;
    off_t offset = -1;

    while(read(fd, &current, sizeof(current)))
    {
        int cmp = strncmp(current.indexValue, value, MAX_INDEX_SIZE);
        if(cmp > 0)
        {
            break;
        }
        else if(cmp == 0)
        {
            offset = current.offset;
            break;
        }
    }

    return offset;
}

char *findValueWithKey(char *str, char *key)
{
    bool keyPare = false;
    char *p = str;
    char *value = NULL;

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

        p += strlen(p) + 1;

        if(keyPare)
        {
            value = p;
            break;
        }

        p += strlen(p) + 2;
    }

    return value;
}
