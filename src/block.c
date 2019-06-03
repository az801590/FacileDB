#include <string.h>

#include "block.h"

void blockInit(Block *inputBuffer, FileInfo *fileDescription)
{
    fileDescription->amountOfData ++;
    inputBuffer->rid = fileDescription->amountOfData;

    memset(inputBuffer->data, '\0', sizeof(((Block*)0)->data));
    inputBuffer->nextOffset = -1;
    inputBuffer->createTime = getTime();
    inputBuffer->delete = false;
}