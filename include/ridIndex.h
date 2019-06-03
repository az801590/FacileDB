#ifndef __RID_INDEX__
#define __RID_INDEX__

#include <stdint.h>
#include <sys/types.h>

#define MAX_FILENAME 64


struct DATAINFO
{
    uint64_t rid;
    off_t offset;

    struct DATAINFO *left;
    struct DATAINFO *right;
};
typedef struct DATAINFO DataInfo;

DataInfo *makeRidIndex(DataInfo*, int);
DataInfo *arrayToBst(DataInfo*, int, int);
void saveRidIndex(void*, char*);
DataInfo *readRidIndex(int, const char*);
DataInfo *bstTraversal(DataInfo*, int);


#endif