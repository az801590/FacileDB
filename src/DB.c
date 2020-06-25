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
#include <regex.h>
#include <ctype.h>

#include "util.h"
#include "index.h"
#include "DB.h"

void init();
time_t getTime();
int listAllDirs(void *, void *);

int useDb(void *);
int loadDbProperties(void *);

void closeDb();

int splitKeyValue(void *, void *);
int putData(void *, void *);
int putFormat(void *, void *);

int seqSearch(void *, void *);
int findData(void *, void *, void *);
int cmpByOffset(const void *, const void *);
void getRecords(void *, void *);

int deleteData(off_t);

int setExist(void *);
int setInit(void *);
int loadSetProperties(void *);
void closeSet();

int queryDecode(void *, void *);

extern int indexExist(void *);
extern void indexInit(void *);
extern void closeIndex();
extern void insertIndex(void *);
extern int makeIndex(void *, void *);
extern int indexSearch(void *, const int, void *);
extern int indexSearch2(void *, const int, void *);

DbInfo dbInfo = {.setInfo = {.file = -1}, .path = {0}, .properties = NULL};

/*
** reference: (https://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way)
*/
void *strTrim(void *input)
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

int splitKeyValue(void *arrExt, void *str)
{
    /*
    ** regular expression(without space detection): ^("[^"\\]*(\\.[^"\\]*)*":"[^"\\]*(\\.[^"\\]*)*",)*("[^"\\]*(\\.[^"\\]*)*":"[^"\\]*(\\.[^"\\]*)*")$
    ** PCRE(without space detection): ^("[^"\\]+(?:\\.[^"\\]*)*":"[^"\\]*(?:\\.[^"\\]*)*",)*("[^"\\]+(?:\\.[^"\\]*)*":"[^"\\]*(?:\\.[^"\\]*)*")$
    ** regular expression(with space detection): ^\s*("[^"\\]*(\\.[^"\\]*)*"\s*:\s*"[^"\\]*(\\.[^"\\]*)*"\s*,\s*)*("[^"\\]*(\\.[^"\\]*)*"\s*:\s*"[^"\\]*(\\.[^"\\]*)*")\s*$
    ** reference: (https://stackoverflow.com/questions/6525556/regular-expression-to-match-escaped-characters-quotes)
    */
    ArrayExt *array = arrExt;
    int arrPair = 2;
    array->arr2 = calloc(arrPair * 2, sizeof(char *));
    array->len = 0;

    regex_t regex;
    regmatch_t match;
    char pattern[] = "^\\s*(\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"\\s*:\\s*\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"\\s*,\\s*)*(\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"\\s*:\\s*\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\")\\s*$";
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0)
    {
        //regerror
    }

    if (regexec(&regex, str, 1, &match, 0) == REG_NOMATCH)
    {
        /*
        ** syntax error
        ** function : regerror(...), for error information.
        */
        regfree(&regex);
        free(array->arr2);
        return 0;
    }
    else
    {
        regfree(&regex);
        strcpy(pattern, "\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"");

        if (regcomp(&regex, pattern, REG_EXTENDED) != 0)
        {
            //regerror
        }

        char *key, *value;
        char *p = str;
        while (*p)
        {
            if (regexec(&regex, p, 1, &match, 0) == REG_NOMATCH)
            {
                break;
            }
            key = p + match.rm_so + 1;
            *(p + match.rm_eo - 1) = 0;
            p += match.rm_eo;

            if (regexec(&regex, p, 1, &match, 0) == REG_NOMATCH)
            {
                break;
            }
            value = p + match.rm_so + 1;
            *(p + match.rm_eo - 1) = 0;
            p += match.rm_eo;

            if (strlen(key) == 0)
            {
                free(array->arr2);
                array->len = 0;
                regfree(&regex);
                return 0;
            }

            if ((array->len) >= arrPair)
            {
                arrPair *= 2;
                void *tmp = realloc(array->arr2, 2 * arrPair * sizeof(char *));
                if (tmp)
                {
                    array->arr2 = tmp;
                }
                else
                {
                    //error
                }
            }
            array->arr2[2 * (array->len)] = key;
            array->arr2[2 * (array->len) + 1] = value;
            array->len++;
        }
    }
    regfree(&regex);
    return 1;
}

/*
** Size of data: DATA_SIZE
** Format: key1 0 value1 0 0 key2 0 value2 0 0...
** safety problem: out of bound
*/
int putFormat(void *data, void *array)
{
    char *p = data;
    int length = DATA_SIZE;

    int i = 0;
    for (int i = 0; i < ((ArrayExt *)array)->len; i++)
    {
        void *key = ((ArrayExt *)array)->arr2[2 * i];
        void *value = ((ArrayExt *)array)->arr2[2 * i + 1];

        if ((length -= (strlen(key) + strlen(value) + 3)) >= 0)
        {
            strncat(p, key, length);
            p += strlen(key) + 1;

            strncat(p, value, DATA_SIZE);
            p += strlen(value) + 2;
        }
        else
        {
            break;
        }
    }

    ((char *)data)[DATA_SIZE / sizeof(char) - 1] = 0;
    return i;
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

        ArrayExt obj = {.arr1 = NULL, .arr2 = NULL, .len = 0};
        if (!(splitKeyValue(&obj, str)))
        {
            return 0;
        }
        else
        {
            Block new = {
                .rid = dbInfo.setInfo.properties->amountOfData,
                .nextOffset = 0,
                .createdTime = getTime(),
                .delete = 0};

            if (putFormat(new.data, &obj) != obj.len)
            {
                //out of bound
            }

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
                for (int i = 0; i < obj.len; i++)
                {
                    if (indexExist(obj.arr2[2 * i]))
                    {
                        indexInit(obj.arr2[2 * i]);
                        Index newIndex = {
                            .rid = new.rid,
                            .offset = (lseek(dbInfo.setInfo.file, 0, SEEK_CUR) - sizeof(Block))};
                        strncpy(newIndex.data, obj.arr2[2 * i + 1], INDEX_LEN);

                        insertIndex(&newIndex);
                    }
                }
            }

            free(obj.arr2);
            return 1;
        }
    }
    else
    {
        return -2;
    }
}

int findData(void *result, void *setName, void *str)
{
    if (setExist(setName) && setInit(setName))
    {
        ArrayExt obj = {.arr1 = NULL, .arr2 = NULL, .len = 0};

        if (!queryDecode(&obj, str))
        {
            free(obj.arr1);
            free(obj.arr2);
            return -2;
        }

        //case: key!* using seqSearch
        if (*((char *)obj.arr2[0]) == '"' && !(((char *)obj.arr1)[0] == '!' && *((char *)obj.arr2[1]) == '*') && indexExist(&((char *)obj.arr2[0])[1]))
        {
            indexInit(&((char *)obj.arr2[0])[1]);
            ArrayExt found = {.arr1 = NULL, .arr2 = NULL, .len = 0};

            if (((char *)obj.arr1)[0] == '=' && *((char *)obj.arr2[1]) == '"')
            {
                //case: key=value
                indexSearch(&found, dbInfo.setInfo.indexInfo.properties->root, &((char *)obj.arr2[1])[1]);
            }
            else
            {
                int num = dbInfo.setInfo.indexInfo.properties->tags * (ORDER - 1) + 1;
                found.arr1 = calloc(num, sizeof(Index));
                if (((char *)obj.arr1)[0] == '=' && *((char *)obj.arr2[1]) == '*')
                {
                    //case: key=*
                    indexSearch2(&found, dbInfo.setInfo.indexInfo.properties->root, NULL);
                }
                else
                {
                    //case: key!value
                    indexSearch2(&found, dbInfo.setInfo.indexInfo.properties->root, &((char *)obj.arr2[1])[1]);
                }
            }

            Index *indexArr = found.arr1;
            //sort indexArr by offset
            qsort(indexArr, found.len, sizeof(Index), cmpByOffset);
            off_t displacement = 0;
            int readSize = 0;
            for (int i = 0; i < (found.len); i++)
            {
                displacement = (indexArr[i].offset) - lseek(dbInfo.setInfo.file, 0, SEEK_CUR);
                lseek(dbInfo.setInfo.file, displacement, SEEK_CUR);

                BlockList *tmp = calloc(1, sizeof(BlockList));
                if ((readSize = read(dbInfo.setInfo.file, &tmp->block, sizeof(Block))) > 0)
                {
                    if (!tmp->block.delete)
                    {
                        tmp->offset = indexArr[i].offset;
                        tmp->next = ((ArrayExt *)result)->arr1;
                        ((ArrayExt *)result)->arr1 = tmp;
                        ((ArrayExt *)result)->len++;
                    }
                    else
                    {
                        free(tmp);
                    }
                }
                else if (readSize == -1)
                {
                    //read error
                    perror("findData/indexSearch");
                    break;
                }
            }

            if (found.arr1)
            {
                free(found.arr1);
            }
        }
        else
        {
            lseek(dbInfo.setInfo.file, 0, SEEK_SET);
            seqSearch(result, &obj);
        }

        free(obj.arr1);
        free(obj.arr2);
        return ((ArrayExt *)result)->len;
    }
    else
    {
        //load set error or set isn't existed
        return -1;
    }
}

int cmpByOffset(const void *a, const void *b)
{
    const Index *aa = a, *bb = b;
    return (aa->offset - bb->offset);
}

/*
** Get every key-value pair from file with format "key 0 value 0 0..."
** Safty problem
*/
void getRecords(void *arrExt, void *data)
{
    ArrayExt *records = arrExt;
    char *p = data;
    char *dataEnd = p + DATA_SIZE;

    records->len = 0;
    int arrPair = 8;
    records->arr2 = calloc(2 * arrPair, sizeof(char *));

    while ((p < dataEnd) && (*p))
    {
        char *key = p;
        p += strlen(key) + 1;
        char *value = p;
        p += strlen(value) + 2;

        if (!((p - 1) < dataEnd))
        {
            break;
        }

        if (records->len >= arrPair)
        {
            arrPair *= 2;
            void *tmp = realloc(records->arr2, 2 * arrPair * sizeof(char *));

            if (tmp)
            {
                records->arr2 = tmp;
            }
            else
            {
                //no memory to allocate
                break;
            }
        }

        records->arr2[2 * (records->len)] = key;
        records->arr2[2 * (records->len) + 1] = value;
        records->len++;
    }
}

/*
** current version only support single key-value pair target search
** key: target->arr2[0], value: target->arr2[1]
** result: ArrayExt pointer with arr1: BlockList, len: length
*/
int seqSearch(void *result, void *target)
{
    void **targetArr = ((ArrayExt *)target)->arr2;
    char *eq = ((ArrayExt *)target)->arr1;

    if (*((char *)targetArr[0]) == '*' && *((char *)targetArr[1]) == '*' && eq[0] == '!')
    {
        //Case: *!* -> null
        return 1;
    }

    Block buffer[TMPSIZE];

    int readSize = 0;
    ArrayExt records = {.arr1 = NULL, .arr2 = NULL, .len = 0};

    while ((readSize = read(dbInfo.setInfo.file, &buffer, TMPSIZE * sizeof(Block))) > 0)
    {
        for (int i = 0; i < (int)(readSize / sizeof(Block)); i++)
        {
            if (!(buffer[i].delete))
            {
                int found = 0;
                if (*((char *)targetArr[0]) != '*' || *((char *)targetArr[1]) != '*')
                {
                    getRecords(&records, buffer[i].data);
                    for (int j = 0; j < records.len; j++)
                    {
                        //seq search;to imporve, use binary search
                        if (eq[0] == '!' && (*((char *)targetArr[0]) == '*' || *((char *)targetArr[1]) == '*'))
                        {
                            if (*((char *)targetArr[1]) == '*')
                            {
                                //Case: "key"!* -> a is not existed; a is null.
                                if (strncmp(&((char *)targetArr[0])[1], records.arr2[2 * j], DATA_SIZE) != 0)
                                {
                                    found = 1;
                                }
                                else
                                {
                                    found = 0;
                                    break;
                                }
                            }
                            else
                            {
                                //case: *!"value"
                                if (strncmp(&((char *)targetArr[1])[1], records.arr2[2 * j + 1], DATA_SIZE) == 0)
                                {
                                    found = 0;
                                    break;
                                }
                                else
                                {
                                    found = 1;
                                }
                            }
                        }
                        else
                        {
                            if (*((char *)targetArr[0]) == '*' || strncmp(&((char *)targetArr[0])[1], records.arr2[2 * j], DATA_SIZE) == 0)
                            {
                                if (eq[0] == '=')
                                {
                                    if (*((char *)targetArr[1]) == '*' || strncmp(&((char *)targetArr[1])[1], records.arr2[2 * j + 1], DATA_SIZE) == 0)
                                    {
                                        found = 1;
                                        break;
                                    }
                                }
                                else
                                {
                                    //case: "key"!"value"
                                    if (strncmp(&((char *)targetArr[1])[1], records.arr2[2 * j + 1], DATA_SIZE) != 0)
                                    {
                                        found = 1;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    free(records.arr2);
                }
                else
                {
                    //Case: *=*
                    found = 1;
                }

                if (found)
                {
                    BlockList *new = calloc(1, sizeof(BlockList));
                    memcpy(&(new->block), &buffer[i], sizeof(Block));
                    new->offset = (lseek(dbInfo.setInfo.file, 0, SEEK_CUR) - readSize + i * sizeof(Block));
                    new->next = ((ArrayExt *)result)->arr1;
                    ((ArrayExt *)result)->arr1 = new;
                    ((ArrayExt *)result)->len++;
                }
            }
        }
    }
    return 1;
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

/*
** Decode query to ArrayExt object. For example, 
** query: *=* & "key"!"value" (all=all and key!=value)
** arrExt->arr2: [ *, *, "key, "value] (* : all, "... : string(...))
** arrExt->arr1: [=, &, !] 
*/
int queryDecode(void *arrExt, void *str)
{
    ArrayExt *array = arrExt;
    array->len = 0;
    int arrPair = 2;
    array->arr2 = calloc(2 * arrPair, sizeof(char *));
    array->arr1 = calloc(2 * arrPair, sizeof(char));

    /*
    ** regular expression for 'find': ^\s*((("[^"\\]*(\\.[^"\\]*)*")|\*)\s*(=|!|>|<)\s*(("[^"\\]*(\\.[^"\\]*)*")|\*)\s*(&|\|)\s*)*((("[^"\\]*(\\.[^"\\]*)*")|\*)\s*(=|!|>|<)\s*(("[^"\\]*(\\.[^"\\]*)*")|\*))\s*$
    */
    regex_t regex;
    regmatch_t match;
    char pattern[] = "^\\s*(((\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\")|\\*)\\s*(=|!|>|<)\\s*((\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\")|\\*)\\s*(&|\\|)\\s*)*(((\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\")|\\*)\\s*(=|!|>|<)\\s*((\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\")|\\*))\\s*$";
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0)
    {
        //regerror
    }

    if (regexec(&regex, str, 1, &match, 0) == REG_NOMATCH)
    {
        /*
        ** syntax error
        ** function : regerror(...), for error information.
        */
        regfree(&regex);
        return 0;
    }
    else
    {
        regfree(&regex);
        /*
        ** regular expression: ("[^"\\]*(\\.[^"\\]*)*")|\*
        */
        strcpy(pattern, "(\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\")|\\*");

        if (regcomp(&regex, pattern, REG_EXTENDED) != 0)
        {
            //regerror
        }

        char *key, *value, eq = 0, lop = 0;
        char *p = str;
        while (*p)
        {
            if (regexec(&regex, p, 1, &match, 0) == REG_NOMATCH)
            {
                break;
            }
            key = p + match.rm_so;
            if (*key == '"')
            {
                *(p + match.rm_eo - 1) = 0;
            }
            p += match.rm_eo;
            p = strTrim(p);

            eq = *p;
            *p = 0;
            p++;

            if (regexec(&regex, p, 1, &match, 0) == REG_NOMATCH)
            {
                break;
            }
            value = p + match.rm_so;
            if (*value == '"')
            {
                *(p + match.rm_eo - 1) = 0;
            }
            p += match.rm_eo;
            p = strTrim(p);

            lop = *p;
            if (*p)
            {
                *p = 0;
                p++;
            }

            if (*key == '"' && strlen(key) <= 1)
            {
                regfree(&regex);
                array->len = 0;
                return 0;
            }

            if ((array->len) >= arrPair)
            {
                arrPair *= 2;
                void *tmp1 = realloc(array->arr1, 2 * arrPair * sizeof(char));
                void *tmp2 = realloc(array->arr2, 2 * arrPair * sizeof(char *));
                if (tmp1 && tmp2)
                {
                    array->arr1 = tmp1;
                    array->arr2 = tmp2;
                }
                else
                {
                    //error
                }
            }
            ((char *)array->arr1)[2 * array->len] = eq;
            ((char *)array->arr1)[2 * array->len + 1] = lop;
            array->arr2[2 * array->len] = key;
            array->arr2[2 * array->len + 1] = value;
            array->len++;
        }
    }
    regfree(&regex);
    return 1;
}