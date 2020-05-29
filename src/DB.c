#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>

#include "util.h"
#include "index.h"
#include "DB.h"

void init();
time_t getTime();
int listAllDirs(void *, void *);

int useDb(void *);
int loadDbProperties(void *);

void closeDb();

void *splitKeyValue(void *);
int putData(void *, void *);
void putFormat(void *, void *);

void *seqSearch(void *);
void *findData(void *, void *);
void *getRecords(void *);

int deleteData(off_t);

int setExist(void *);
int setInit(void *);
int loadSetProperties(void *);
void closeSet();

extern int indexExist(void *);
extern void indexInit(void *);
extern void closeIndex();
extern void insertIndex(void *);
extern int makeIndex(void *, void *);
extern void *indexSearch(const int, void *);

DbInfo dbInfo = {.setInfo = {.file = -1}, .path = {0}, .properties = NULL};

//Check directory ./dbs existed , if not, create one.
void init()
{
    char dirPath[BUFF_LEN] = "./dbs";
    DIR *dir = opendir(dirPath);

    if (dir)
    {
        printf("Access db directory: %s\n", dirPath);
        closedir(dir);
    }
    else
    {
        if (mkdir(dirPath, 0750) == 0)
        {
            printf("Create db directory: %s\n", dirPath);
        }
        else
        {
            perror("Create db directory failed");
            exit(EXIT_FAILURE);
        }
    }
}

time_t getTime()
{
    return time(NULL);
}

int listAllDirs(void *arrExt, void *path)
{
    struct dirent *de;
    DIR *dr = opendir(path);

    if (dr == NULL)
    {
        perror("Could not open current directory");
        return 0;
    }

    ArrayExt *result = arrExt;
    int arrSize = 8;
    result->arr2 = calloc(arrSize, sizeof(char *));

    while ((de = readdir(dr)) != NULL)
    {
        //exception: . & .. & properties
        if (strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0 && strcmp(de->d_name, "properties") != 0)
        {
            if (result->len >= arrSize)
            {
                arrSize *= 2;
                //avoid mem leak
                void *tmp = realloc(result->arr2, arrSize * sizeof(char *));
                if (tmp)
                {
                    result->arr2 = tmp;
                }
                else
                {
                    //error
                }
            }

            result->arr2[result->len] = calloc(MAX_FILENAME, sizeof(char));
            strncpy(result->arr2[result->len], de->d_name, MAX_FILENAME);
            result->len++;
        }
    }
    closedir(dr);

    return 1;
}

void closeDb()
{
    if (dbInfo.setInfo.properties)
    {
        closeSet();
    }

    char oldName[BUFF_LEN] = {0};
    strncpy(oldName, dbInfo.properties->name, BUFF_LEN);

    munmap(dbInfo.properties, sizeof(DbProperties));
    dbInfo.properties = NULL;

    printf("Close Db %s.\n", oldName);
}

int useDb(void *dbName)
{
    if (dbInfo.properties)
    {
        if (strncmp(dbInfo.properties->name, dbName, BUFF_LEN) == 0)
        {
            return 1;
        }
        else
        {
            /*
            ** Close opened db 
            */
            closeDb();
        }
    }

    //dbInfo.dbPath = ./dbs/dbName
    strcpy(dbInfo.path, "./dbs/");
    strncat(dbInfo.path, dbName, BUFF_LEN);

    /*
    ** Create db directories and files structure
    */
    DIR *dir = opendir(dbInfo.path);
    if (!dir)
    {
        if ((mkdir(dbInfo.path, 0750) != 0))
        {
            perror("Create directory failed");
            return 0;
        }
    }
    closedir(dir);

    /*
    ** Map db properties file
    */
    if (!loadDbProperties(dbName))
    {
        dbInfo.properties = NULL;
        return 0;
    }

    return 1;
}

int loadDbProperties(void *dbName)
{
    char path[BUFF_LEN] = {0};
    int propertiesFile = -1;

    //path = ./dbs/dbName/properties
    strncpy(path, dbInfo.path, BUFF_LEN);
    strncat(path, "/", BUFF_LEN);
    strncat(path, "properties", BUFF_LEN);

    //if not exist, create it.
    if ((propertiesFile = open(path, O_RDWR, 0640)) < 0)
    {
        if ((propertiesFile = open(path, O_RDWR | O_CREAT, 0640)) < 0)
        {
            perror("Create properties file failed");
            return 0;
        }
        else
        {
            DbProperties new = {.amountOfData = 0, .createdTime = getTime()};
            strncpy(new.name, dbName, BUFF_LEN);
            new.modifiedTime = new.createdTime;

            if (write(propertiesFile, &new, sizeof(DbProperties)) != sizeof(DbProperties))
            {
                close(propertiesFile);
                perror("Write properties file failed");
                return 0;
            }
        }
    }

    dbInfo.properties = mmap(NULL, sizeof(DbProperties), PROT_READ | PROT_WRITE, MAP_SHARED, propertiesFile, 0);
    if (dbInfo.properties == MAP_FAILED)
    {
        close(propertiesFile);
        dbInfo.properties = NULL;
        perror("Map properties file failed");
        return 0;
    }

    close(propertiesFile);
    return 1;
}

void *splitKeyValue(void *str)
{
    if (*(char *)str)
    {
        char *p = str;
        int count = 1;
        while ((p = strstr(p, "\",")))
        {

            p++;
            count++;
        }

        ArrayExt *array = calloc(1, sizeof(ArrayExt));
        array->arr2 = calloc(count * 2, sizeof(char *));

        for (int i = 0; i < count; i++)
        {
            char *start, *end;
            if ((p = strstr(str, "\",")))
            {
                p++;
                *p = 0;
            }

            //key
            if (!((start = strchr(str, '"')) && (end = strstr(start, "\":"))))
            {
                free(array->arr2);
                free(array);
                return NULL;
            }
            start++;
            *end = 0;
            if (!(*start))
            {
                free(array->arr2);
                free(array);
                return NULL;
            }
            array->arr2[2 * i] = start;
            str = end + 1;

            //value
            //strrchr: strchr from end of string
            if (!((start = strchr(str, '"')) && (end = strrchr(start, '"')) && (end > start)))
            {
                free(array->arr2);
                free(array);
                return NULL;
            }
            start++;
            *end = 0;
            array->arr2[2 * i + 1] = start;

            if (p)
            {
                str = p + 1;
            }
        }

        array->len = count;
        return array;
    }
    else
    {
        return NULL;
    }
}

/*
** Size of data: DATA_SIZE
** Format: key1 0 value1 0 0 key2 0 value2 0 0...
*/
//safety problem: out of bounded
void putFormat(void *data, void *array)
{
    char *p = data;

    for (int i = 0; i < ((ArrayExt *)array)->len; i++)
    {
        char *key = ((ArrayExt *)array)->arr2[2 * i];
        char *value = ((ArrayExt *)array)->arr2[2 * i + 1];

        strncat(p, key, DATA_SIZE);
        p += strlen(key) + 1;
        strncat(p, value, DATA_SIZE);
        p += strlen(value) + 2;
    }

    ((char *)data)[DATA_SIZE / sizeof(char) - 1] = 0;
}

/*
** return 0: syntax error
** return -1: write error
** return -2: setinit error
*/
int putData(void *setName, void *str)
{
    if (setInit(setName))
    {

        ArrayExt *obj;
        if (!(obj = splitKeyValue(str)))
        {
            return 0;
        }
        else
        {
            Block new = {
                .rid = dbInfo.setInfo.properties->amountOfData,
                .nextOffset = 0,
                .createdTime = getTime(),
                .recordNum = obj->len,
                .delete = 0};
            putFormat(new.data, obj);

            dbInfo.setInfo.properties->amountOfData++;
            dbInfo.setInfo.properties->modifiedTime = new.createdTime;

            lseek(dbInfo.setInfo.file, 0, SEEK_END);
            if (write(dbInfo.setInfo.file, &new, sizeof(Block)) != sizeof(Block))
            {
                perror("put");
                return -1;
            }
            else
            {
                for (int i = 0; i < obj->len; i++)
                {
                    if (indexExist(obj->arr2[2 * i]))
                    {
                        indexInit(obj->arr2[2 * i]);
                        Index newIndex = {
                            .rid = new.rid,
                            .offset = (lseek(dbInfo.setInfo.file, 0, SEEK_CUR) - sizeof(Block))};
                        strncpy(newIndex.data, obj->arr2[2 * i + 1], INDEX_LEN);

                        insertIndex(&newIndex);
                    }
                }
            }

            free(((ArrayExt *)obj)->arr2);
            free((ArrayExt *)obj);

            return 1;
        }
    }
    else
    {
        return -2;
    }
}

void *findData(void *setName, void *str)
{
    ArrayExt *result = calloc(1, sizeof(ArrayExt));
    if (setExist(setName) && setInit(setName))
    {
        ArrayExt *obj;
        if (!(obj = splitKeyValue(str)))
        {
            free(result);
            return NULL;
        }

        if (indexExist(obj->arr2[0]))
        {
            indexInit(obj->arr2[0]);
            ArrayExt *found = indexSearch(dbInfo.setInfo.indexInfo.properties->root, obj->arr2[1]);

            if (found != NULL)
            {
                Index *indexArr = found->arr1;
                for (int i = 0; i < (found->len); i++)
                {
                    off_t displacement = (indexArr[i].offset) - lseek(dbInfo.setInfo.file, 0, SEEK_CUR);
                    lseek(dbInfo.setInfo.file, displacement, SEEK_CUR);

                    BlockList *tmp = calloc(1, sizeof(BlockList));
                    if (read(dbInfo.setInfo.file, &tmp->block, sizeof(Block)) > 0)
                    {
                        if (!tmp->block.delete)
                        {
                            tmp->offset = indexArr[i].offset;
                            tmp->next = result->arr1;
                            result->arr1 = tmp;
                            result->len++;
                        }
                        else
                        {
                            free(tmp);
                        }
                    }
                    else
                    {
                        perror("findData/indexSearch");
                    }
                }
            }
        }
        else
        {
            lseek(dbInfo.setInfo.file, 0, SEEK_SET);
            return seqSearch(obj);
        }
    }
    else
    {
        result->len = 0;
    }
    return result;
}

/*
** Get every key-value pair from file with format "key 0 value 0 0..."
*/
void *getRecords(void *block)
{
    Block *b = (Block *)block;
    char *p = b->data;
    char *end = p + (DATA_SIZE - 1) / sizeof(char);

    ArrayExt *records = calloc(1, sizeof(ArrayExt));
    records->arr2 = calloc(2 * (b->recordNum), sizeof(char *));
    records->len = b->recordNum;

    for (int i = 0; (i < (int)(b->recordNum)) && (p <= end) && (*p); i++)
    {
        records->arr2[2 * i] = p;
        p += strlen(p) + 1;

        records->arr2[2 * i + 1] = p;
        p += strlen(p) + 2;
    }

    return records;
}

/*
** current version only support single key-value pair target search
** key: target->arr2[0], value: target->arr2[1]
** return ArrayExt pointer with arr1: BlockList, len: length
*/
void *seqSearch(void *target)
{
    Block buffer[TMPSIZE];
    ArrayExt *result = calloc(1, sizeof(ArrayExt));
    int readSize = 0;

    while ((readSize = read(dbInfo.setInfo.file, &buffer, TMPSIZE * sizeof(Block))) > 0)
    {
        for (int i = 0; i < (int)(readSize / sizeof(Block)); i++)
        {
            if (!(buffer[i].delete))
            {
                ArrayExt *records = getRecords(&buffer[i]);
                for (int j = 0; j < records->len; j++)
                {
                    //seq search;to imporve, use binary search
                    if (strncmp(((ArrayExt *)target)->arr2[0], records->arr2[2 * j], DATA_SIZE) == 0)
                    {
                        if ((strncmp(((ArrayExt *)target)->arr2[1], records->arr2[2 * j + 1], DATA_SIZE) == 0) || strncmp(((ArrayExt *)target)->arr2[1], "*", DATA_SIZE) == 0)
                        {
                            BlockList *found = calloc(1, sizeof(BlockList));
                            memcpy(&(found->block), &buffer[i], sizeof(Block));
                            found->offset = (lseek(dbInfo.setInfo.file, 0, SEEK_CUR) - readSize + i * sizeof(Block));
                            found->next = result->arr1;
                            result->arr1 = found;
                            result->len++;
                            break;
                        }
                    }
                }
            }
        }
    }

    return result;
}

int deleteData(off_t offset)
{
    off_t displacement = offset - lseek(dbInfo.setInfo.file, 0, SEEK_CUR);
    lseek(dbInfo.setInfo.file, displacement, SEEK_CUR);

    Block tmp;
    if (read(dbInfo.setInfo.file, &tmp, sizeof(Block)) > 0)
    {
        tmp.delete = 1;
        lseek(dbInfo.setInfo.file, -sizeof(Block), SEEK_CUR);
        write(dbInfo.setInfo.file, &tmp, sizeof(Block));
        return 1;
    }

    return 0;
}

int setExist(void *setName)
{
    //path: ./dbs/dbName/setName
    char path[BUFF_LEN] = {0};
    strncpy(path, dbInfo.path, BUFF_LEN);
    strcat(path, "/");
    strncat(path, setName, BUFF_LEN);

    DIR *dir = opendir(path);
    if (!dir)
    {
        return 0;
    }
    else
    {
        closedir(dir);
        return 1;
    }
}

int setInit(void *setName)
{
    if ((dbInfo.setInfo.properties) && (strncmp(dbInfo.setInfo.properties->name, setName, BUFF_LEN) == 0))
    {
        return 1;
    }

    if (dbInfo.setInfo.properties)
    {
        closeSet();
    }

    char tmp[BUFF_LEN];

    //dbInfo.setInfo.path: ./dbs/dbName/setName
    strncpy(dbInfo.setInfo.path, dbInfo.path, BUFF_LEN);
    strcat(dbInfo.setInfo.path, "/");
    strncat(dbInfo.setInfo.path, setName, BUFF_LEN);

    /*
    ** Create set directories and file structure
    */
    DIR *dir = opendir(dbInfo.setInfo.path);
    if (!dir)
    {
        //tmp: ./dbs/dbName/setName/index
        strncpy(tmp, dbInfo.setInfo.path, BUFF_LEN);
        strncat(tmp, "/index", BUFF_LEN);

        if ((mkdir(dbInfo.setInfo.path, 0750) != 0) || (mkdir(tmp, 0750) != 0))
        {
            perror("Create directory failed");
            return 0;
        }
    }
    closedir(dir);

    /*
    ** Map set-properties file
    */
    if (!(loadSetProperties(setName)))
    {
        dbInfo.setInfo.properties = NULL;
        return 0;
    }

    /*
    ** Open set-file
    ** tmp: ./dbs/dbName/setName/setName.fac
    */
    strncpy(tmp, dbInfo.setInfo.path, BUFF_LEN);
    strcat(tmp, "/");
    strncat(tmp, setName, BUFF_LEN);
    strcat(tmp, ".fac");
    if ((dbInfo.setInfo.file = open(tmp, O_RDWR | O_CREAT, 0666)) < 0)
    {
        munmap(dbInfo.setInfo.properties, sizeof(SetProperties));
        dbInfo.setInfo.properties = NULL;
        dbInfo.setInfo.file = -1;

        perror("Load set-file failed");
        return 0;
    }

    return 1;
}

int loadSetProperties(void *setName)
{
    char path[BUFF_LEN] = {0};
    int propertiesFile = -1;

    //path: ./dbs/dbName/setName/properties
    strncpy(path, dbInfo.setInfo.path, BUFF_LEN);
    strncat(path, "/properties", BUFF_LEN);

    //if properties file not existed, create it.
    if ((propertiesFile = open(path, O_RDWR, 0640)) < 0)
    {
        if ((propertiesFile = open(path, O_RDWR | O_CREAT, 0640)) < 0)
        {
            perror("Create set-properties file failed");
            return 0;
        }
        else
        {
            SetProperties new = {.amountOfData = 0, .createdTime = getTime()};
            strncpy(new.name, setName, BUFF_LEN);
            new.modifiedTime = new.createdTime;

            if (write(propertiesFile, &new, sizeof(SetProperties)) != sizeof(SetProperties))
            {
                close(propertiesFile);
                perror("Write set-properties file failed");
                return 0;
            }
        }
    }

    dbInfo.setInfo.properties = mmap(NULL, sizeof(SetProperties), PROT_READ | PROT_WRITE, MAP_SHARED, propertiesFile, 0);
    close(propertiesFile);
    if (dbInfo.setInfo.properties == MAP_FAILED)
    {

        dbInfo.setInfo.properties = NULL;
        perror("Map set-properties file failed");
        return 0;
    }

    return 1;
}

void closeSet()
{
    if (dbInfo.setInfo.indexInfo.properties)
    {
        closeIndex();
    }

    close(dbInfo.setInfo.file);
    dbInfo.setInfo.file = -1;

    munmap(dbInfo.setInfo.properties, sizeof(SetProperties));
    dbInfo.setInfo.properties = NULL;
}
