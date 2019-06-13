#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "ridIndex.h"
#include "util.h"

DataInfo *makeRidIndex(DataInfo *array, int amount)
{
    DataInfo *bst = arrayToBst(array, 0, amount - 1);
    free(array);
    return bst;
}

DataInfo *arrayToBst(DataInfo *array, int start, int end)
{
    if(start > end)
    {
        return NULL;
    }
    else
    {
        int mid = start + (end - start) / 2;

        DataInfo *current = (DataInfo*) malloc(sizeof(DataInfo));
        current->rid = array[mid].rid;
        current->offset = array[mid].offset;

        current->left = arrayToBst(array, start, mid-1);
        current->right = arrayToBst(array, mid+1, end);
        return current;
    }
}

DataInfo *bstTraversal(DataInfo *node, int key)
{
    if(node->rid == key)
    {
        return node;
    }
    else if(node->rid < key)
    {
        return bstTraversal(node->right, key);
    }
    else
    {
        return bstTraversal(node->left, key);
    }
}

/*
** ArrayExt *indexArr
** arr = DataInfo index[]
** len = arr.length
*/
void saveRidIndex(void *indexArr, char *dirPath)
{
    char finalFilename[MAX_FILENAME] = {0};
    strcpy(finalFilename, dirPath);
    strcat(finalFilename, "/rid/0");

    DataInfo *index = ((ArrayExt*)indexArr)->arr;

    int fd = open(finalFilename, O_RDWR | O_CREAT, 0666);

    if(fd != -1)
    {
        for(int i = 0; i < ((ArrayExt*)indexArr)->len; i ++)
        {
            if(!write(fd, &index[i].rid, sizeof(((DataInfo*)0)->rid)))
            {
                fprintf(stderr, "%d %s\n", errno, strerror(errno));
            }
            else
            {
                if(!write(fd, &index[i].offset, sizeof(((DataInfo*)0)->offset)))
                {
                    fprintf(stderr, "%d %s\n", errno, strerror(errno));
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

DataInfo *readRidIndex(int amountOfData, const char *currentIndexDir)
{
    char dir[MAX_FILENAME] = {0};
    strcpy(dir, currentIndexDir);
    strcat(dir, "/rid/0");

    int fd = open(dir, O_RDWR, 0666);
    int count = 0;

    if(fd == -1)
    {
        //fprintf(stderr, "%d %s(rid index file)\n", errno, strerror(errno));

        return NULL;
    }


    DataInfo *indexArr = (DataInfo*) calloc (amountOfData, sizeof(DataInfo));

    while(read(fd, &indexArr[count].rid, sizeof(((DataInfo*)0)->rid)) && read(fd, &indexArr[count].offset, sizeof(((DataInfo*)0)->offset)))
    {
        count ++;
    }

    close(fd);

    if(indexArr != NULL)
    {
        return makeRidIndex(indexArr, amountOfData);
    }
    else
    {
        return indexArr;
    }
}
