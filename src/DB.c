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

/*
** DataInfo *ridIndex
*/
int deleteAll(int fd, void *ridIndex)
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
    while((de = readdir(dr))!=NULL)
    {
        if(strcmp(de->d_name, ".")!=0 && strcmp(de->d_name, "..")!=0)
        {
            printf("%s\n", de->d_name);
        }
    }
    printf("----------------\n");

    closedir(dr);     
}

/*
*/
void putByIn(void*, char*);
void putOneRow2(ArrayExt*, void*);
DataInfo *bstInsert(DataInfo*, DataInfo*);
void *putFormat(void*);

void saveRidIndex2(void*);
void closeDb(void*);
void ridIndexInOrderWrite(int, DataInfo*);

/*
** Array *sysInfo
** arr[0] = int currentDbFile
** arr2[0] = FileInfo *fileDescription
** arr2[1] = DataInfo *ridIndex
*/
void putByIn(void *sysInfo, char *str)
{
    void *formatStr = putFormat(splitKeyValue(str));
    putOneRow2(sysInfo, formatStr);
}

/*
** Array *sysInfo
** arr[0] = int currentDbFile
** arr2[0] = FileInfo *fileDescription
** arr2[1] = DataInfo *ridIndex
*/
void putOneRow2(ArrayExt *sysInfo, void *data)
{
    FileInfo *fileDescription = sysInfo->arr2[0];
    int currentDbFile = ((int*) sysInfo->arr)[0];

    Block inputBuffer;
    blockInit(&inputBuffer, fileDescription);
    memcpy(inputBuffer.data, data, MAX_DATA_SIZE);

    lseek(currentDbFile, 0, SEEK_END);

    DataInfo *ridOffset = malloc(sizeof(DataInfo));
    ridOffset->rid = inputBuffer.rid;
    ridOffset->offset = lseek(currentDbFile, 0, SEEK_CUR);
    ridOffset->left = NULL;
    ridOffset->right = NULL;

    //write into file
    if(!(write(currentDbFile, &inputBuffer, sizeof(Block))))
    {
        fprintf(stderr, "Write error!\n");
    }

    //write into ridBST
    if(!(sysInfo->arr2[1] = bstInsert((DataInfo*) sysInfo->arr2[1], ridOffset))) 
    {
        fprintf(stderr, "rid write error!\n");
    }
}

DataInfo *bstInsert(DataInfo *node, DataInfo *insert)
{
    if(node == NULL)
    {
        node = insert;
    }
    else
    {
        if((node->rid) < (insert->rid))
        {
            node->right = bstInsert(node->right, insert);
        }
        else if((node->rid) > (insert->rid))
        {
            node->left = bstInsert(node->left, insert);
        }
    }
    return node;
}

/*
** ArrayExt *sysInfo
** arr2[0] = FileInfo *fileDescription
** arr2[1] = DataInfo *ridIndex
** arr2[2] = char *indexDirPath
*/
void closeDb(void *sysInfo)
{
    if(((ArrayExt*) sysInfo)->arr2[0] != NULL)
    {
        munmap(((ArrayExt*) sysInfo)->arr2[0], sizeof(FileInfo));
    }
    if(((ArrayExt*) sysInfo)->arr2[1])
    {
        saveRidIndex2(sysInfo);
    }
}

/*
** ArrayExt *ridInfo
** arr2[0] = FileInfo *fileDescription
** arr2[1] = DataInfo *bst
** arr2[2] = char *dirPath
*/
void saveRidIndex2(void *ridInfo)
{
    char finalFilename[MAX_FILENAME] = {0};
    strcpy(finalFilename, (char*) ((ArrayExt*) ridInfo)->arr2[2]);
    strcat(finalFilename, "/rid/0");

    int fd = open(finalFilename, O_RDWR | O_CREAT, 0666);

    if(fd)
    {
        ridIndexInOrderWrite(fd, (DataInfo*) ((ArrayExt*) ridInfo)->arr2[1]);
    }
    else
    {
        fprintf(stderr, "%d: %s(rid save)\n", errno, strerror(errno));
    }
}


void ridIndexInOrderWrite(int fd, DataInfo *node)
{
    if(node != NULL)
    {
        ridIndexInOrderWrite(fd, node->left);

        if(!write(fd, &node->rid, sizeof(((DataInfo*)0)->rid)))
        {
            fprintf(stderr, "%d %s\n", errno, strerror(errno));
        }
        else
        {
            if(!write(fd, &node->offset, sizeof(((DataInfo*)0)->offset)))
            {
                fprintf(stderr, "%d %s\n", errno, strerror(errno));
            }
        }

        ridIndexInOrderWrite(fd, node->right);
    }
}


/*
** input: ArrayExt* , saved with key-value pair in arr2 and length in len.
** output: A space size MAX_DATA_SIZE and filled with key-value in format.
** format: key \0 value \0 \0 key2 ... 
** exception: over max-data-size, not done
*/
void *putFormat(void *keyValue)
{
    char *output = (char*) calloc(1, sizeof(MAX_DATA_SIZE));
    char *p = output;

    for(int i = 0; i < (((ArrayExt*) keyValue)->len); i++)
    {
        strcat(p, (char*) (((ArrayExt*) keyValue)->arr2)[2 * i]);
        p += strlen((char*) (((ArrayExt*) keyValue)->arr2)[2 * i]) + 1;

        strcat(p, (char*) (((ArrayExt*) keyValue)->arr2)[2 * i + 1]);
        p += strlen((char*) (((ArrayExt*) keyValue)->arr2)[2 * i + 1]) + 2;
    }

    return output;
}
/*
*/

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
        strcat(finalFilename, "/rid");
        mkdir(finalFilename, 0777);
        
        FileInfo new;
        fileInfoInit(&new);
        if(!(write(fd, &new, sizeof(FileInfo))))
        {
            printf("Write error!\n");
        }
    }

    return fd;
}

/*
** ArrayExt *sysInfo
** arr2[0] = char *currentDbFilePath
** arr2[1] = FileInfo *fileDescription
** arr2[2] = char *currentIndexDir
** arr2[3] = DataInfo *ridIndex
** arr2[4] = char *argu2
**
** arr[0] = int currentDbFile
*/
void loadDbInfo(void *sysInfo)
{
    strcpy((char*) ((ArrayExt*) sysInfo)->arr2[0], "./dbs/");
    strcat((char*) ((ArrayExt*) sysInfo)->arr2[0], (char*) ((ArrayExt*) sysInfo)->arr2[4]);

    strcpy((char*) ((ArrayExt*) sysInfo)->arr2[2], (char*) ((ArrayExt*) sysInfo)->arr2[0]);
    strcat((char*) ((ArrayExt*) sysInfo)->arr2[2], "/index");

    ((ArrayExt*) sysInfo)->arr2[1] = getFileInfo(((int*) ((ArrayExt*) sysInfo)->arr)[0]);

    //load ridIndex
    ((ArrayExt*) sysInfo)->arr2[3] = readRidIndex(((FileInfo*) ((ArrayExt*) sysInfo)->arr2[1])->amountOfData, (char*) ((ArrayExt*) sysInfo)->arr2[2]);
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
    bool ridIndexDirty = false;

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
            ArrayExt old;
            old.arr2 = malloc(3 * sizeof(void*));

            old.arr2[0] = fileDescription;
            old.arr2[1] = ridIndex;
            old.arr2[2] = currentIndexDir;
            closeDb(&old);

            break;
        }
        else if(strcmp(argu, "list") == 0)
        {
            listAllDbFiles("./dbs");
        }
        else if(strcmp(argu, "use") == 0)
        {
            if(fileSelected && ridIndexDirty)
            {
                /*
                ** ArrayExt *sysInfo
                ** arr2[0] = FileInfo *fileDescription
                ** arr2[1] = DataInfo *ridIndex
                ** arr2[2] = char *indexDirPath
                */
                ArrayExt old;
                old.arr2 = malloc(3 * sizeof(void*));

                old.arr2[0] = fileDescription;
                old.arr2[1] = ridIndex;
                old.arr2[2] = currentIndexDir;
                closeDb(&old);
            }

            if((currentDbFile = useDB(argu2)))
            {
                /*
                ** ArrayExt *dbInfo
                ** arr2[0] = char *currentDbFilePath
                ** arr2[1] = FileInfo *fileDescription
                ** arr2[2] = char *currentIndexDir
                ** arr2[3] = DataInfo *ridIndex
                ** arr2[4] = char *argu2
                **
                ** arr[0] = int currentDbFile
                */
                ArrayExt dbInfo;
                dbInfo.arr = malloc(sizeof(int));
                dbInfo.arr2 = malloc(5 * sizeof(void*));

                ((int*) dbInfo.arr)[0] = currentDbFile;
                dbInfo.arr2[0] = currentDbFilePath;
                dbInfo.arr2[1] = fileDescription;
                dbInfo.arr2[2] = currentIndexDir;
                dbInfo.arr2[3] = ridIndex;
                dbInfo.arr2[4] = argu2;

                loadDbInfo(&dbInfo);
                fileDescription = dbInfo.arr2[1];
                ridIndex = dbInfo.arr2[3];

                printf("Switch to %s.\n", argu2);
            }

            ridIndexDirty = false;
            fileSelected = true;
        }
        else if(strcmp(argu, "put") == 0)
        {
            if(fileSelected)
            {
                //put with stdin
                start = clock();
                t1 = getTime();

                /*
                ** sysInfo
                ** arr[0] = int currentDbFile
                ** arr2[0] = FileInfo *fileDescription
                ** arr2[1] = DataInfo *ridIndex
                */
                ArrayExt sysInfo;
                sysInfo.arr = malloc(1 * sizeof(int));
                sysInfo.arr2 = malloc(2 * sizeof(void*));

                ((int*) sysInfo.arr)[0] = currentDbFile;
                sysInfo.arr2[0] = fileDescription;
                sysInfo.arr2[1] = ridIndex;

                putByIn(&sysInfo, argu2);
                ridIndex = sysInfo.arr2[1];

                t2 = getTime();
                end = clock();
                printf("Time: %lfsec\t%ldsec\n", timeSpend(start, end), timeSpend2(t1, t2));

                ridIndexDirty = true;
            }
        }
        else if(strcmp(argu, "fput") == 0)
        {
            if(fileSelected)
            {
                //put by file
                start = clock();
                t1 = getTime();

                FILE *f = fopen(argu2, "r");
                printf("Records: %d\t", putByFile(f, currentDbFile, fileDescription, ridIndex, currentIndexDir));
                fclose(f);

                //read ridIndex
                ridIndex = readRidIndex(fileDescription->amountOfData, currentIndexDir);

                //rid dirty bit
                ridIndexDirty = true;

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