#define DATA_SIZE 1024
#define TMPSIZE 5 //use in seqSearch
#define MAX_FILENAME 256 //linux definition: 255bytes

/*
** size: 1072 bytes
*/
struct BLOCK
{
    uint64_t rid;
    char data[DATA_SIZE];
    off_t nextOffset;
    time_t createdTime;
    uint64_t recordNum;
    int64_t delete;

    int64_t padding;
};
typedef struct BLOCK Block;

struct BLOCKLIST
{
    struct BLOCK block;
    off_t offset;
    struct BLOCKLIST *next;
};
typedef struct BLOCKLIST BlockList;

struct SETPROPERTIES
{
    char name[BUFF_LEN];
    size_t amountOfData;
    size_t createdTime;
    size_t modifiedTime;
};
typedef struct SETPROPERTIES SetProperties;

struct SETINFO
{
    int file;
    char path[BUFF_LEN];
    struct SETPROPERTIES *properties;

    struct INDEXINFO indexInfo;
};
typedef struct SETINFO SetInfo;

struct DBPROPERTIES
{
    char name[BUFF_LEN];
    unsigned int amountOfData;
    size_t createdTime;
    size_t modifiedTime;
};
typedef struct DBPROPERTIES DbProperties;

struct DBINFO
{
    //int file;
    char path[BUFF_LEN];
    struct DBPROPERTIES *properties;
    struct SETINFO setInfo;
};
typedef struct DBINFO DbInfo;