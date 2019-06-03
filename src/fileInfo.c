#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "fileInfo.h"


void fileInfoInit(FileInfo *fileDescription)
{
    fileDescription->amountOfData = 0;
    fileDescription->createTime = getTime();
}

FileInfo *getFileInfo(int fd)
{
    return (FileInfo*) mmap(NULL, sizeof(FileInfo), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
}