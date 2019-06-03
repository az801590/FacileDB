#include "libraries.h"

#include "analyse.h"
#include "block.h"
#include "fileInfo.h"
#include "index.h"
#include "ridIndex.h"
#include "util.h"

#define INPUT_BUFFER 128
#define MAX_PAIR_NUM 15 // 7+8n<=128 n<=15


void putOneRow(int, FileInfo*, char*, DataInfo*, int);
bool findOneRow(char*, char*, char*, char*);
int seqFind(int, char*, char*);

//char **splitKeyValue(char*);
void *splitKeyValue(char*);

char *findValueWithKey(char*, char*);
DataInfo *arrayToBst(DataInfo*, int, int);



//index start


int cmp(const void *a, const void *b)
{
    Index *f = (Index*)a;
    Index *l = (Index*)b;

    return strcmp(f->indexValue, l->indexValue);
}


int makeIndexKeyDir(const char *currentIndexDir, const char *indexName)
{
    char indexKeyDir[MAX_FILENAME] = {0};
    strcpy(indexKeyDir, currentIndexDir);
    strcat(indexKeyDir, "/");
    strcat(indexKeyDir, indexName);

    return mkdir(indexKeyDir, 0777);
}

// end

int findAll(int fd)
{
    Block current;
    char *p = NULL;
    int count = 0;

    while(read(fd, &current, sizeof(Block)))
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

void findByOffset(int fd, off_t offset)
{
    Block current;
    char *p = NULL;

    lseek(fd, offset, SEEK_SET);

    if(read(fd, &current, sizeof(Block)))
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
        }
    }
}

/*
char **splitKeyValue(char *str)
{
    char **keyValue = malloc(2*sizeof(char*));
    keyValue[0] = str;
    keyValue[1] = strchr(str, ':');

    *keyValue[1] = '\0';
    keyValue[1]++;

    return keyValue;
}
*/
void *splitKeyValue(char *str)
{
    int count = 0;
    ArrayExt *current = (ArrayExt*) malloc(sizeof(ArrayExt)); 
    current->arr2 = (void**) malloc (MAX_PAIR_NUM * 2 * sizeof(char*));

    do
    {
        char *start = strchr(str, '"') + 1;
        char *end = strstr(start, "\":");
        *end = '\0';
        current->arr2[count] = start;
        str = end + 1;

        start = strchr(str, '"') + 1;
        end = strchr(start, '"');
        *end = '\0';
        current->arr2[count + 1] = start;
        str = end + 1;

        count += 2;
    }
    while((str = strstr(str, ",\"")) != NULL);

    current->len = count / 2;
    return current;
}

int seqFind(int fd, char *key, char *value)
{
    char output[MAX_DATA_SIZE] = {0};
    int count = 0;

    Block current;
    while(read(fd, &current, sizeof(Block)))
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
    Block current;
    int count = 0;

    while(read(fd, &current, sizeof(Block)))
    {
        if(!(current.delete))
        {
            current.delete = true;
            lseek(fd, -sizeof(Block), SEEK_CUR);
            if(!(write(fd, &current, sizeof(Block))))
            {
                printf("Write error!\n");
            }
            count++;
        }
    }
    return count;
}

int putByFile(FILE *f, int fd, FileInfo *fileDescription, DataInfo *ridIndex, char *indexDirPath)
{
    char temp[MAX_DATA_SIZE] = {0};
    char buff[MAX_DATA_SIZE] = {0};
    char output[MAX_DATA_SIZE] = {0};
    char *key = NULL, *value = NULL;
    char *position = NULL;
    int count = 0;

    ridIndex = (DataInfo*) malloc(9312084 * sizeof(DataInfo));
    makeIndexKeyDir(indexDirPath, "rid");
    int counter = 0;

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
                    putOneRow(fd, fileDescription, output, ridIndex, counter);
                    memset(output, 0, MAX_DATA_SIZE);
                    counter ++;
                    count ++;
                    i = 0;
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

    ArrayExt *tempArrExt = malloc(sizeof(ArrayExt));
    tempArrExt->arr = ridIndex;
    tempArrExt->len = counter;
    saveRidIndex(tempArrExt, indexDirPath);
    free(tempArrExt);
    //ridIndex = makeRidIndex(ridIndex, counter);

    return count;
}

void deleteByOffset(off_t offset, int fd, FileInfo *fileDescription)
{
    Block current;

    lseek(fd, offset, SEEK_SET);

    if(read(fd, &current, sizeof(Block)))
    {
        if(!current.delete)
        {
            current.delete = true;
            fileDescription->amountOfData --;
            lseek(fd, -sizeof(Block), SEEK_CUR);
            if(!(write(fd, &current, sizeof(Block))))
            {
                printf("Write error!\n");
            }
        }
    }
}

int deleteFromFind(int fd, char *str, FileInfo *fileDescription)
{
    Block current;
    char output[MAX_DATA_SIZE] = {0};
    int count = 0;
    char *p = NULL;

    /*
    char **temp = splitKeyValue(str);
    char *key = temp[0];
    char *value = temp[1];
    */
    ArrayExt *keyValue = (ArrayExt*) splitKeyValue(str);
    char *key = (char*) keyValue->arr2[0];
    char *value = (char*) keyValue->arr2[1];

    while(read(fd, &current, sizeof(Block)))
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
                lseek(fd, -sizeof(Block), SEEK_CUR);
                if(!(write(fd, &current, sizeof(Block))))
                {
                    printf("Write error!\n");
                }
                lseek(fd, sizeof(Block), SEEK_CUR);
            }
        }
    }

    //free(temp);
    return count;
}

void putOneRow(int currentDbFile, FileInfo *fileDescription, char *data, DataInfo *ridIndex, int count)
{
    Block inputBuffer;
    blockInit(&inputBuffer, fileDescription);
    memcpy(inputBuffer.data, data, MAX_DATA_SIZE);

    lseek(currentDbFile, 0, SEEK_END);

    //rid index
    /*
    if(count > 0 && count % (1<<6) == 0)
    {
        ridIndex = realloc(ridIndex, (count + (1<<6)) * sizeof(DataInfo));
    }
    */
    ridIndex[count].rid = inputBuffer.rid;
    ridIndex[count].offset = lseek(currentDbFile, 0, SEEK_CUR);

    //write into file
    if(!(write(currentDbFile, &inputBuffer, sizeof(Block))))
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
        
        FileInfo new;
        fileInfoInit(&new);
        if(!(write(fd, &new, sizeof(FileInfo))))
        {
            printf("Write error!\n");
        }
    }

    //close(fd);
    return fd;
}


int main(int argc, char *argv[])
{
    char buff[INPUT_BUFFER];
    char *argu = NULL;
    char *argu2 = NULL;

    FileInfo *fileDescription = NULL;
    bool fileSelected = false;

    int currentDbFile = -1;
    char currentDbFilePath[MAX_FILENAME] = {0};

    char currentIndexDir[MAX_FILENAME] = {0};
    char currentIndex[MAX_FILENAME] = {0};
    Index *index = NULL;
    DataInfo *ridIndex = NULL;

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

                start = clock();
                t1 = getTime();

                ridIndex = readRidIndex(fileDescription->amountOfData, currentIndexDir);
                if(ridIndex != NULL)
                {
                    ridIndex = makeRidIndex(ridIndex, fileDescription->amountOfData);
                }
                end = clock();
                t2 = getTime();
                printf("Time: %lfsec\t%ldsec\n", timeSpend(start, end), timeSpend2(t1, t2));


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
                Block inputBuffer;
                blockInit(&inputBuffer, fileDescription);

                memcpy(inputBuffer.data, argu2, MAX_DATA_SIZE);
                lseek(currentDbFile, 0, SEEK_END);
                write(currentDbFile, &inputBuffer, sizeof(Block));
                */

                //put by file
                FILE *f = fopen(argu2, "r");
                printf("Records: %d\t", putByFile(f, currentDbFile, fileDescription, ridIndex, currentIndexDir));
                fclose(f);

                //read ridIndex
                ridIndex = readRidIndex(fileDescription->amountOfData, currentIndexDir);
                if(ridIndex != NULL)
                {
                    ridIndex = makeRidIndex(ridIndex, fileDescription->amountOfData);
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
        else if (strcmp(argu, "find") == 0)
        {
            if(fileSelected)
            {
                start = clock();
                t1 = getTime();
                lseek(currentDbFile, sizeof(FileInfo), SEEK_SET);

                if(strcmp(argu2, "*") == 0)
                {
                    printf("Records: %d\t", findAll(currentDbFile));
                }
                else
                {
                    /*
                    char **temp = splitKeyValue(argu2);
                    char *key = temp[0];
                    char *value = temp[1];
                    free(temp);
                    */
                    ArrayExt *keyValue = (ArrayExt*) splitKeyValue(argu2);
                    char *key = (char*) keyValue->arr2[0];
                    char *value = (char*) keyValue->arr2[1];
                    printf("%s\t%s\n", key, value);

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

                lseek(currentDbFile, sizeof(FileInfo), SEEK_SET);

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
                        if(ridIndex != NULL)
                        {
                            DataInfo *current = bstTraversal(ridIndex, random[i]);
                            deleteByOffset(current->offset, currentDbFile, fileDescription);
                        }
                        else
                        {
                            printf("No index!\n");
                        }
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
                        if(ridIndex != NULL)
                        {
                            DataInfo *current = bstTraversal(ridIndex, random[i]);
                            deleteByOffset(current->offset, currentDbFile, fileDescription);
                        }
                        else
                        {
                            printf("No index!\n");
                        }
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
                        lseek(currentDbFile, sizeof(FileInfo), SEEK_SET);

                        Index *indexArr = (Index*) calloc((fileDescription->amountOfData), sizeof(Index));

                        int indexCount = makeIndex(indexArr, currentDbFile, argu2);
                        qsort(indexArr, indexCount, sizeof(Index), cmp);
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
            else
            {
                printf("Choose a db file first.\n");
            }
        }
        else if(strcmp(argu, "getByRid()") == 0)
        {
            if(fileSelected)
            {
                start = clock();
                t1 = getTime();

                uint64_t rid = (uint64_t) atoi(argu2);
                if(ridIndex != NULL)
                {
                    DataInfo *current = bstTraversal(ridIndex, rid);
                    findByOffset(currentDbFile, current->offset);
                }
                else
                {
                    printf("No index!\n");
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
        else if(strcmp(argu, "getByRid(1000000)") == 0)
        {
            int total = fileDescription->amountOfData;
            int *random = getRandom(total);
            printf("Random success!\n");

            start = clock();
            t1 = getTime();
            for(int i = 0; i < 1000000; i++)
            {
                if(ridIndex != NULL)
                {
                    DataInfo *current = bstTraversal(ridIndex, (uint64_t) random[i]);
                    findByOffset(currentDbFile, current->offset);
                }
                else
                {
                    printf("No index!\n");
                }
            }
            free(random);

            t2 = getTime();
            end = clock();
            printf("Time: %lfsec\t%ldsec\n", timeSpend(start, end), timeSpend2(t1, t2));

        }
        else
        {
            printf("Command not found!\n");
        }

        printf("> ");
    }

    if(fileDescription)
    {
        munmap(fileDescription, sizeof(FileInfo));
    }
    if(index != NULL)
    {
        free(index);
    }

    close(currentDbFile);
    return 0;
}