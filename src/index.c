#include <stdint.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "util.h"
#include "index.h"
#include "DB.h"

int indexExist(void *);
void *getNodeName(const int);
int indexInit(void *);
void closeIndex();
int cmpData(void *, void *);
int nodeNum(void *, void *, int, int);
void *newNode(const int);
void *loadNode(const int);
int makeIndex(void *, void *);
void insertIndex(void *);
void insertIntoNode(void *, void *, int, const int);
int indexSearch(void *, const int, void *);
int indexSearch2(void *, const int, void *);

extern DbInfo dbInfo;
extern void getRecords(void *, void *);
extern int setExist(void *);
extern int setInit(void *);

int indexExist(void *indexName)
{
    //path: ./dbs/dbName/setName/index/indexName
    char path[BUFF_LEN] = {0};
    strncpy(path, dbInfo.setInfo.path, BUFF_LEN);
    strncat(path, "/index/", BUFF_LEN);
    strncat(path, indexName, BUFF_LEN);

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

void *getNodeName(const int tag)
{
    char *pathName = calloc(1, BUFF_LEN * sizeof(char));
    char temp[BUFF_LEN];
    strcpy(pathName, dbInfo.setInfo.indexInfo.path);
    strcat(pathName, "/");
    snprintf(temp, sizeof(temp), "%d", tag);
    strncat(pathName, temp, BUFF_LEN);

    return pathName;
}

int indexInit(void *indexName)
{
    if (dbInfo.setInfo.indexInfo.properties)
    {
        if (strncmp(dbInfo.setInfo.indexInfo.properties->name, indexName, BUFF_LEN) == 0)
        {
            return 1;
        }
        else
        {
            closeIndex();
        }
    }

    char path[BUFF_LEN] = {0};
    int fd = -1;
    //dbInfo.setInfo.indexInfo.path = ./dbs/dbName/setName/index/indexName
    strncpy(dbInfo.setInfo.indexInfo.path, dbInfo.setInfo.path, BUFF_LEN);
    strncat(dbInfo.setInfo.indexInfo.path, "/index/", BUFF_LEN);
    strncat(dbInfo.setInfo.indexInfo.path, indexName, BUFF_LEN);

    //path = ./dbs/dbName/setName/index/indexName/properties
    strncpy(path, dbInfo.setInfo.indexInfo.path, BUFF_LEN);
    strncat(path, "/properties", BUFF_LEN);

    if ((fd = open(path, O_RDWR, 0644)) < 0)
    {
        if ((fd = open(path, O_RDWR | O_CREAT, 0644)) >= 0)
        {
            IndexProperties new = {.root = -1, .tags = 0};
            strncpy(new.name, indexName, BUFF_LEN);
            if (write(fd, &new, sizeof(IndexProperties)) != sizeof(IndexProperties))
            {
                perror("Write index properties file failed");
                return 0;
            }
        }
        else
        {
            perror("Create index-properties file failed");
            return 0;
        }
    }

    dbInfo.setInfo.indexInfo.properties = mmap(NULL, sizeof(IndexProperties), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    if (dbInfo.setInfo.indexInfo.properties == MAP_FAILED)
    {
        dbInfo.setInfo.indexInfo.properties = NULL;
        perror("Map index-propserties file failed");
        return 0;
    }

    if (dbInfo.setInfo.indexInfo.properties->root == -1)
    {
        Node *root = (Node *)newNode(0);
        dbInfo.setInfo.indexInfo.properties->root = root->tag;
        munmap(root, sizeof(Node));
    }
    return 1;
}

void closeIndex()
{
    munmap(dbInfo.setInfo.indexInfo.properties, sizeof(IndexProperties));
    dbInfo.setInfo.indexInfo.properties = NULL;
    dbInfo.setInfo.indexInfo.path[0] = 0;
}

int cmpData(void *str1, void *str2)
{
    return strncmp(str1, str2, INDEX_LEN);
}

// find the position while data(default: string) try to add into node n
// method: Binary search
int nodeNum(void *node, void *str, int start, int end)
{
    Node *n = node;
    if (start <= end)
    {
        int mid = start + (end - start) / 2;
        int cmp = cmpData(str, n->element[mid].data);

        if (start == end)
        {
            if (cmp > 0)
            {
                start += 1;
            }
        }
        else
        {
            if (cmp > 0)
            {
                return nodeNum(n, str, mid + 1, end);
            }
            else
            {
                return nodeNum(n, str, start, mid - 1);
            }
        }
    }
    return start;

    // method: linear search
    /*
    int i;
    for (i = 0; i < (n->length); i++)
    {
        if (compareStr(str, (n->element[i]).data) <= 0)
        {
            break;
        }
    }
    return i;
    */
}

void *newNode(const int level)
{
    if (dbInfo.setInfo.indexInfo.properties)
    {
        //init node
        Node *n = calloc(1, sizeof(Node));
        n->tag = dbInfo.setInfo.indexInfo.properties->tags;
        n->level = level;
        n->length = 0;
        n->prevTag = -1;
        n->nextTag = -1;
        for (int i = 0; i < ORDER + 1; i++)
        {
            (n->childTag)[i] = -1;
        }

        char *path = (char *)getNodeName(dbInfo.setInfo.indexInfo.properties->tags);
        dbInfo.setInfo.indexInfo.properties->tags++;

        int fd = open(path, O_RDWR | O_CREAT, 0666);
        if (fd != -1)
        {
            write(fd, n, sizeof(Node));
            free(n);

            n = mmap(NULL, sizeof(Node), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            close(fd);

            return n;
        }
    }
    return NULL;
}

void *loadNode(const int tag)
{
    char *path = (char *)getNodeName(tag);

    int fd = open(path, O_RDWR, 0666);
    free(path);

    if (fd >= 0)
    {
        return mmap(NULL, sizeof(Node), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    }
    return NULL;
}

/*
** return -1: indexName syntax error.
** return -2: index has already existed.
** return -3: create directory failed.
** return -4: Can not init index.(index error)
** return -5: set error
*/
int makeIndex(void *setName, void *str)
{
    if (setExist(setName) && setInit(setName))
    {
        char *indexName, *end;
        if ((indexName = strchr(str, '"')) && (end = strrchr(str, '"')))
        {
            indexName++;
            *end = 0;
        }
        else
        {
            return -1;
        }

        if (!indexExist(indexName))
        {
            //dirPath: ./dbs/dbName/setName/index/indexName
            char dirPath[BUFF_LEN] = {0};
            strncpy(dirPath, dbInfo.setInfo.path, BUFF_LEN);
            strncat(dirPath, "/index/", BUFF_LEN);
            strncat(dirPath, indexName, BUFF_LEN);

            if (mkdir(dirPath, 0750) == 0)
            {
                printf("Create index directory: %s\n", (char *)indexName);

                indexInit(indexName);
                if (dbInfo.setInfo.indexInfo.properties)
                {
                    Block buffer[TMPSIZE];
                    int readSize = 0;
                    int counter = 0;
                    Index new;
                    ArrayExt records = {.arr1 = NULL, .arr2 = NULL, .len = 0};

                    lseek(dbInfo.setInfo.file, 0, SEEK_SET);
                    while ((readSize = read(dbInfo.setInfo.file, &buffer, TMPSIZE * sizeof(Block))) > 0)
                    {
                        for (int i = 0; i < (int)(readSize / sizeof(Block)); i++)
                        {
                            if (!(buffer[i].delete))
                            {
                                getRecords(&records, buffer[i].data);
                                for (int j = 0; j < records.len; j++)
                                {
                                    //seq search;to imporve, use binary search
                                    if (strncmp(indexName, records.arr2[2 * j], DATA_SIZE) == 0)
                                    {
                                        new.offset = (lseek(dbInfo.setInfo.file, 0, SEEK_CUR) - readSize + i * sizeof(Block));
                                        new.rid = buffer[i].rid;
                                        strncpy(new.data, records.arr2[2 * j + 1], INDEX_LEN);

                                        insertIndex(&new);
                                        counter++;
                                        break;
                                    }
                                }
                                free(records.arr2);
                            }
                        }
                    }

                    return counter;
                }
                else
                {
                    //Can not init index
                    return -4;
                }
            }
            else
            {
                perror("Create index directory failed");
                return -3;
            }
        }
        else
        {
            return -2;
        }
    }
    else
    {
        return -5;
    }
}

void insertIndex(void *index)
{
    Index *e = index;
    Node *n = loadNode(dbInfo.setInfo.indexInfo.properties->root);
    int num = -1;
    for (num = nodeNum(n, e->data, 0, n->length - 1); n->level != 0; num = nodeNum(n, e->data, 0, n->length - 1))
    {
        int child = n->childTag[num];
        munmap(n, sizeof(Node));
        n = loadNode(child);
    }
    insertIntoNode(n, e, num, -1);
}

void insertIntoNode(void *node, void *index, int num, const int newChildTag)
{
    Node *n = node;
    Index *e = index;

    for (int i = (n->length); i > num; i--)
    {
        memcpy(&(n->element[i]), &(n->element[i - 1]), sizeof(Index));
        n->childTag[i + 1] = n->childTag[i];
    }

    (n->length)++;
    memcpy(&(n->element[num]), e, sizeof(Index));

    if (newChildTag != -1)
    {
        n->childTag[num + 1] = newChildTag;
    }

    if ((n->length) >= ORDER)
    {
        Node *new = (Node *)newNode(n->level);
        Node *prev = NULL;
        int up = -1;

        if (n->nextTag != -1)
        {
            new->nextTag = n->nextTag;
        }

        if (n->prevTag == -1)
        {
            prev = (Node *)newNode((n->level) + 1);
            prev->childTag[0] = n->tag;
            n->prevTag = prev->tag;
            //update root
            dbInfo.setInfo.indexInfo.properties->root = prev->tag;
        }
        else
        {
            prev = (Node *)loadNode(n->prevTag);
        }

        n->nextTag = new->tag;
        new->prevTag = prev->tag;

        for (int i = ORDER / 2 + 1, j = 0; i < (n->length); i++, j++)
        {
            memcpy(&(new->element[j]), &(n->element[i]), sizeof(Index));
            new->childTag[j] = n->childTag[i];

            //update new->childTag[j]->prevTag
            if (n->level != 0)
            {
                Node *child = loadNode(new->childTag[j]);
                child->prevTag = new->tag;
                munmap(child, sizeof(Node));
            }
        }
        new->childTag[ORDER / 2] = n->childTag[ORDER];
        if (n->level != 0)
        {
            //update new->childTag[ORDER/2]->prevTag
            Node *child = loadNode(new->childTag[ORDER / 2]);
            child->prevTag = new->tag;
            munmap(child, sizeof(Node));

            //update length
            n->length = ORDER / 2;
            up = n->length;
        }
        else
        {
            n->length = ORDER / 2 + 1;
            up = (n->length) - 1;
        }
        new->length = ORDER / 2;

        munmap(new, sizeof(Node));
        insertIntoNode(prev, &(n->element[up]), nodeNum(prev, n->element[up].data, 0, prev->length - 1), n->nextTag);
    }
    munmap(n, sizeof(Node));
}

// method(inside the node): linear search
int indexSearch(void *array, const int tag, void *str)
{
    ArrayExt *result = array;
    Node *n = loadNode(tag);

    int i = 0, count = 0;
    for (i = 0; i < (n->length); i++)
    {
        int cmp = cmpData(str, n->element[i].data);
        if (cmp == 0 && n->level == 0)
        {
            count++;
        }
        else if (cmp <= 0)
        {
            break;
        }
    }

    if (count > 0)
    {
        //search nextNode
        if (n->nextTag != -1 && i == (n->length))
        {
            indexSearch(result, n->nextTag, str);
        }

        if (result->arr1)
        {
            void *tmp = realloc(result->arr1, (count + (result->len)) * sizeof(Index));
            if (tmp)
            {
                result->arr1 = tmp;
            }
            else
            {
                //error
            }
        }
        else
        {
            result->arr1 = calloc(count, sizeof(Index));
        }

        memcpy(&(((Index *)result->arr1)[result->len]), &(n->element[i - count]), count * sizeof(Index));
        result->len += count;
    }
    else if (n->level != 0)
    {
        int child = n->childTag[i];
        munmap(n, sizeof(Node));
        return indexSearch(result, child, str);
    }

    munmap(n, sizeof(Node));
    return result->len;
}

/*
** for case: (key!value) and (key=*)
*/
int indexSearch2(void *result, const int tag, void *value)
{
    Node *n = loadNode(tag);
    if (n->level != 0)
    {
        indexSearch2(result, n->childTag[0], value);
    }
    else
    {
        ArrayExt *array = result;
        if (value && cmpData(value, n->element[0].data) >= 0 && cmpData(value, n->element[n->length - 1].data) <= 0)
        {
            //linear search
            for (int i = 0; i < n->length; i++)
            {
                if (cmpData(value, n->element[i].data) != 0)
                {
                    memcpy(&(((Index *)array->arr1)[array->len]), &(n->element[i]), sizeof(Index));
                    array->len++;
                }
            }
        }
        else
        {
            memcpy(&(((Index *)array->arr1)[array->len]), n->element, n->length * sizeof(Index));
            array->len += n->length;
        }

        if (n->nextTag != -1)
        {
            indexSearch2(result, n->nextTag, value);
        }
    }
    munmap(n, sizeof(Node));
    return ((ArrayExt *)result)->len;
}