#define ORDER 5
#define INDEX_LEN 112

struct INDEX
{
    uint64_t rid;
    off_t offset;
    char data[INDEX_LEN];
};
typedef struct INDEX Index;

struct NODE
{
    struct INDEX element[ORDER];

    int level;
    int tag;
    int length;

    int padding;

    int childTag[ORDER + 1];
    int prevTag;
    int nextTag;
};
typedef struct NODE Node;

struct INDEXPROPERTIES
{
    char name[BUFF_LEN];
    int root;
    int tags;
};
typedef struct INDEXPROPERTIES IndexProperties;

struct INDEXINFO
{
    char path[BUFF_LEN];
    struct INDEXPROPERTIES *properties;
};
typedef struct INDEXINFO IndexInfo;