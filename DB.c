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
#define MAX_FILENAME 64
#define MAX_INDEX_SIZE 120


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

typedef struct
{
    uint64_t rid;
    char indexValue[MAX_INDEX_SIZE];
    off_t offset;
} indexBuffer;

void putOneRow(int, fileInfo*, char*);
void inputByFile(FILE*, int, fileInfo*);
bool findOneRow(char*, char*, char*, char*);
int seqFind(int, char*, char*);
char **splitKeyValue(char*);
char *findValueWithKey(char*, char*);
off_t findIndexFile(int, char*);


time_t getTime()
{
    return time(NULL);
}

time_t timeSpend2(time_t t1, time_t t2)
{
    return t2-t1;
}

double timeSpend(clock_t t1, clock_t t2)
{
    return (t2-t1)/(double)(CLOCKS_PER_SEC);
}

void blockInit(blockBuffer *inputBuffer, fileInfo *fileDescription)
{
    fileDescription->amountOfData ++;
    inputBuffer->rid = fileDescription->amountOfData;

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

//index start
int makeIndex(indexBuffer *indexArr, int fd, char *key)
{
    int count = 0;

    char *findIndexValue = NULL;

    blockBuffer current;
    while(read(fd, &current, sizeof(blockBuffer)))
    {
        if(!current.delete)
        {
            findIndexValue = NULL;
            findIndexValue = findValueWithKey((char*)current.data, key);
            if(findIndexValue != NULL)
            {
                indexArr[count].rid = current.rid;
                strncpy(indexArr[count].indexValue, findIndexValue, MAX_INDEX_SIZE);
                indexArr[count].offset = lseek(fd, -sizeof(blockBuffer), SEEK_CUR);
                lseek(fd, sizeof(blockBuffer), SEEK_CUR);
                count++;
            }
        }
    }

    return count;
}

int cmp(const void *a, const void *b)
{
    indexBuffer *f = (indexBuffer*)a;
    indexBuffer *l = (indexBuffer*)b;

    return strcmp(f->indexValue, l->indexValue);
}

void saveIndex(indexBuffer *indexArr, int count, const char *dirPath, const char *indexName)
{
    int num = -1;
    int fd = -1;
    char finalFilename[MAX_FILENAME] = {0};
    char toNum[MAX_FILENAME] = {0};

    for(int i=0;i<count;i++)
    {
        if(i % 64 == 0)
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
            //printf("%s\n", finalFilePath);
        }

        if(!write(fd, &indexArr[i], sizeof(indexBuffer)))
        {
            printf("Write error\n");
        };
    }
    if(fd != -1)
    {
        close(fd);
    }
}

indexBuffer *readIndex(int amountOfData, const char *currentIndexDir, char *indexName)
{
    int fd = -1;

    char dir[MAX_FILENAME] = {0};
    char indexFileName[MAX_FILENAME] = {0};

    indexBuffer *indexArr = (indexBuffer*) calloc ((amountOfData/64)+1, sizeof(indexBuffer));
    int count = 0;

    for(int i=0;i<(amountOfData/64)+1;i++)
    {
        sprintf(indexFileName, "%d", i);
        strcpy(dir, currentIndexDir);
        strcat(dir, "/");
        strcat(dir, indexName);
        strcat(dir, "/");
        strcat(dir, indexFileName);

        if((fd = open(dir, O_RDWR, 0666)) != -1)
        {
            if(!read(fd, &indexArr[count], sizeof(indexBuffer)))
            {
                printf("read failed!\n");
            } 
            else
            {
                count++;
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

int makeIndexKeyDir(const char *currentIndexDir, const char *indexName)
{
    char indexKeyDir[MAX_FILENAME] = {0};
    strcpy(indexKeyDir, currentIndexDir);
    strcat(indexKeyDir, "/");
    strcat(indexKeyDir, indexName);

    return mkdir(indexKeyDir, 0777);
}

//return value pointer
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

void indexSearch(indexBuffer *indexArr, int amountOfData, const char *dir, char *indexName, char *value, int currentDbFile)
{
    int fd = -1;
    char finalFilename[MAX_FILENAME] = {0};
    char toNum[MAX_FILENAME] = {0};

    //printf("%s\n", finalFilename);
    
    for(int i=0;i<amountOfData/64+1;i++)
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
            if(offset>0)
            {
                char output[MAX_DATA_SIZE] = {0};
                strcpy(output, "------------\n");
                lseek(currentDbFile, offset, SEEK_SET);
                blockBuffer current;
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
                if(offset>0)
                {
                    char output[MAX_DATA_SIZE] = {0};
                    strcpy(output, "------------\n");
                    lseek(currentDbFile, offset, SEEK_SET);
                    blockBuffer current;
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
    indexBuffer current;
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
// end

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

int seqFind(int fd, char *key, char *value)
{
    char output[MAX_DATA_SIZE] = {0};
    int count = 0;

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

    return valuePare;
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

void deleteByRid(int rid, int fd, fileInfo *fileDescription)
{
    blockBuffer current;

    while(read(fd, &current, sizeof(blockBuffer)))
    {
        if(!current.delete)
        {
            if(current.rid == rid)
            {
                current.delete = true;
                fileDescription->amountOfData --;
                lseek(fd, -sizeof(blockBuffer), SEEK_CUR);
                if(!(write(fd, &current, sizeof(blockBuffer))))
                {
                    printf("Write error!\n");
                }
                break;
            }
        }
    }
}

int deleteFromFind(int fd, char *str, fileInfo *fileDescription)
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
                fileDescription->amountOfData --;
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

bool indexExisted(const char *dir, char *indexName)
{
    struct dirent *de; 
    DIR *dr = opendir(dir);
    bool find = false;

    if(dr == NULL) 
    { 
        printf("Could not open current directory"); 
        return false; 
    }
    while((de=readdir(dr))!=NULL)
    {
        if(strcmp(de->d_name, ".")!=0 && strcmp(de->d_name, "..")!=0)
        {
            if(strcmp(de->d_name, indexName) == 0)
            {
                find = true;
                break;
            }
        }
    }

    closedir(dr);
    return find;
}

int useDB(const char *filename)
{
    char finalFilename[MAX_FILENAME] = "./dbs/";
    strcat(finalFilename, filename);
    mkdir(finalFilename, 0777);

    strcat(finalFilename, "/");
    strcat(finalFilename, filename);
    int fd = open(finalFilename, O_RDWR , 0666);

    if(fd < 0)
    {
        fd = open(finalFilename, O_RDWR | O_CREAT, 0666);
        strcpy(finalFilename, "./dbs/");
        strcat(finalFilename, filename);
        strcat(finalFilename, "/index");
        mkdir(finalFilename, 0777);
        
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

int *getRandom(int size)
{
    int temp, pos;
    int *poker = (int*) malloc(size * sizeof(int));
    srand(time(NULL));

    poker[0] = size;
    for(int i = 1; i < size; i++)
    {
        poker[i] = i;
    }
    
    for(int i = 0; i < size; i++)
    {
        pos = (size - 1) * rand() / RAND_MAX;
        temp = poker[i];
        poker[i] = poker[pos];
        poker[pos] = temp;
    } 
     
    return poker;
}

int main(int argc, char *argv[])
{
    char buff[INPUT_BUFFER];
    char *argu = NULL;
    char *argu2 = NULL;

    fileInfo *fileDescription = NULL;
    bool fileSelected = false;

    int currentDbFile = -1;
    char currentDbFilePath[MAX_FILENAME] = {0};

    char currentIndexDir[MAX_FILENAME] = {0};
    char currentIndex[MAX_FILENAME] = {0};
    indexBuffer *index = NULL;

    clock_t start, end;
    time_t t1, t2;

    printf("Database start.\n> ");

    while(fgets(buff, INPUT_BUFFER, stdin) != NULL)
    {
        buff[strlen(buff)-1]='\0';

        argu = buff;
        if((argu2 = strstr(buff, " ")))
        {
            *argu2 = '\0';
            argu2++;
        }

        if(strcmp(argu, "exit") == 0)
        {
            break;
        }
        else if(strcmp(argu, "list") == 0)
        {
            listAllDbFiles("./dbs");
        }
        else if(strcmp(argu, "use") == 0)
        {
            fileSelected = true;

            if((currentDbFile = useDB(argu2)))
            {
                strcpy(currentDbFilePath, "./dbs/");
                strcat(currentDbFilePath, argu2);
                fileDescription = getFileInfo(currentDbFile);

                strcpy(currentIndexDir, currentDbFilePath);
                strcat(currentIndexDir, "/index");

                printf("Switch to %s.\n", argu2);
            }
        }
        else if(strcmp(argu, "put") == 0)
        {
            if(fileSelected)
            {
                //put
                start = clock();
                t1 = getTime();
                /*
                blockBuffer inputBuffer;
                blockInit(&inputBuffer, fileDescription);

                memcpy(inputBuffer.data, argu2, MAX_DATA_SIZE);
                lseek(currentDbFile, 0, SEEK_END);
                write(currentDbFile, &inputBuffer, sizeof(blockBuffer));
                */

                //put by file
                FILE *f = fopen(argu2, "r");
                printf("Records: %d\t", putByFile(f, currentDbFile, fileDescription));
                fclose(f);

                t2 = getTime();
                end = clock();
                printf("Time: %lfsec\t%ldsec\n", timeSpend(start, end), timeSpend2(t1, t2));
            }
            else
            {
                printf("Choose a db file first.\n");
            }
        }
        else if (strcmp(argu, "find") == 0)
        {
            if(fileSelected)
            {
                start = clock();
                t1 = getTime();
                lseek(currentDbFile, sizeof(fileInfo), SEEK_SET);

                if(strcmp(argu2, "*") == 0)
                {
                    printf("Records: %d\t", findAll(currentDbFile));
                }
                else
                {
                    char **temp = splitKeyValue(argu2);
                    char *key = temp[0];
                    char *value = temp[1];
                    free(temp);

                    if(indexExisted(currentIndexDir, key))
                    {
                        if(strcmp(currentIndex, key) != 0)
                        {
                            if(index!=NULL)
                            {
                                free(index);
                            }

                            index = readIndex(fileDescription->amountOfData, currentIndexDir, key);
                            strcpy(currentIndex, key);
                        }

                        indexSearch(index, fileDescription->amountOfData, currentIndexDir, key, value, currentDbFile);

                    }
                    else
                    {
                        printf("Records: %d\t", seqFind(currentDbFile, key, value));
                    }
                    
                }
                t2 = getTime();
                end = clock();

                printf("Time: %lfsec\t%ldsec\n", timeSpend(start, end), timeSpend2(t1, t2));
            }
            else
            {
                printf("Choose a db file first.\n");
            }
        }
        else if(strcmp(argu, "delete") == 0)
        {
            if(fileSelected)
            {
                start = clock();
                time_t t1 = getTime();

                lseek(currentDbFile, sizeof(fileInfo), SEEK_SET);

                if(strcmp(argu2, "*") == 0)
                {
                    fileDescription->amountOfData = 0;
                    printf("Delete: %d\t", deleteAll(currentDbFile));
                }
                /*
                **
                */
                else if(strcmp(argu2, "1%") == 0)
                {
                    int total = fileDescription->amountOfData;
                    int *random = getRandom(total);
                    printf("Random success!\n");
                    for(int i = 0; i < total / 100; i++)
                    {
                        lseek(currentDbFile, sizeof(fileInfo), SEEK_SET);
                        deleteByRid(random[i], currentDbFile, fileDescription);
                    }
                    free(random);
                    printf("Delete: %d\t", total / 100);
                }
                else if(strcmp(argu2, "10%") == 0)
                {
                    int total = fileDescription->amountOfData;
                    int *random = getRandom(total);
                    for(int i = 0; i < total / 10; i++)
                    {
                        lseek(currentDbFile, sizeof(fileInfo), SEEK_SET);
                        deleteByRid(random[i], currentDbFile, fileDescription);
                    }
                    free(random);
                    printf("Delete: %d\t", total / 10);
                }
                /*
                **
                */
                else
                {
                    printf("Relete: %d\t", deleteFromFind(currentDbFile, argu2, fileDescription));
                }

                t2 = getTime();
                end = clock();
                printf("Time: %lfsec\t%ldsec\n", timeSpend(start, end), timeSpend2(t1, t2));
            }
            else
            {
                printf("Choose a db file first.\n");
            }
        }
        else if(strcmp(argu, "index") == 0)
        {
            if(fileSelected)
            {
                if(fileDescription->amountOfData)
                {
                    start = clock();
                    t1 = getTime();
                    //make seq index
                    if(makeIndexKeyDir(currentIndexDir, argu2) == 0)
                    {
                        lseek(currentDbFile, sizeof(fileInfo), SEEK_SET);

                        indexBuffer *indexArr = (indexBuffer*) calloc((fileDescription->amountOfData), sizeof(indexBuffer));

                        int indexCount = makeIndex(indexArr, currentDbFile, argu2);
                        qsort(indexArr, indexCount, sizeof(indexBuffer), cmp);
                        saveIndex(indexArr, indexCount, currentIndexDir, argu2);

                        printf("Index: %d\t", indexCount);
                        free(indexArr);
                    }
                    else
                    {
                        printf("Make index dir failed.\n");
                    }
                    
                    t2 = getTime();
                    end = clock();
                    printf("Time: %lfsec\t%ldsec\n", timeSpend(start, end), timeSpend2(t1, t2));
                }
                else
                {
                    printf("No data in the database!\n");
                }
            }
            else
            {
                printf("Choose a db file first.\n");
            }
        }
        else if(strcmp(argu, "getAmountOfData()") == 0)
        {
            if(fileSelected)
            {
                printf("Amount: %ld\n", fileDescription->amountOfData);
            }
        }
        else
        {
            printf("Command not found!\n");
        }

        printf("> ");
    }

    if(fileDescription)
    {
        munmap(fileDescription, sizeof(fileInfo));
    }
    if(index!=NULL)
    {
        free(index);
    }

    close(currentDbFile);
    return 0;
}