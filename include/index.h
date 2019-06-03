#ifndef __INDEX_H__
#define __INDEX_H__

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

#define INDEX_FILE_SIZE (1<<13) //8K
#define MAX_INDEX_SIZE 112
#define MAX_FILENAME 64


struct INDEX
{
    uint64_t rid;
    char indexValue[MAX_INDEX_SIZE];
    off_t offset;
};
typedef struct INDEX Index;

bool indexExisted(const char*, const char*);
int makeIndex(Index*, int, char*);
Index *readIndex(int, const char*, const char*);
void saveIndex(Index*, int, const char*, const char*);
void indexSearch(Index*, int, const char*, char*, char*, int);
off_t findIndexFile(int, char*);
char *findValueWithKey(char*, char*);


extern bool findOneRow(char*, char*, char*, char*);


#endif