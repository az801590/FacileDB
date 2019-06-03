#ifndef __FILE_INFO_H__
#define __FILE_INFO_H__

#include <time.h>

struct FILEINFO
{
    size_t amountOfData;
    size_t createTime;
};
typedef struct FILEINFO FileInfo;

void fileInfoInit(FileInfo*);
FileInfo *getFileInfo(int);

extern time_t getTime();

#endif