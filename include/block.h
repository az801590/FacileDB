#ifndef __BLOCK_H__
#define __BLOCK_H__

#define MAX_DATA_SIZE 2048

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "fileInfo.h"

/*
** data size: 2k
*/
struct BLOCK
{
    uint64_t rid;
    size_t data[(MAX_DATA_SIZE/sizeof(size_t))+1];
    size_t nextOffset;
    size_t createTime;
    bool delete;
};
typedef struct BLOCK Block;

void blockInit(Block*, FileInfo*);

extern time_t getTime();

#endif