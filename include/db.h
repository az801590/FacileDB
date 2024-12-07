#ifndef __DB_H__
#define __DB_H__

#include <stdint.h>
#include <stdio.h>

#define DB_DATA_SIZE (1024)
#define DB_FILE_PATH_BUFFER_LENGTH (256) // linux definition: 255bytes


typedef enum
{
    DB_RECORD_VALUE_TYPE_NUM
} DB_RECORD_VALUE_TYPE_E;

// Record: A key-value pair
typedef struct 
{   
    uint32_t key_size;
    uint32_t value_size;

    union 
    {
        DB_RECORD_VALUE_TYPE_E record_value_type;
        uint32_t record_value_type_32;
    };

    void *key;
    void *value;
} DB_RECORD_T;

// in-memory
typedef struct
{
    DB_RECORD_T db_record;
    off_t offset;
} DB_RECORD_INFO_T;

typedef struct
{
    uint64_t block_id;
    int64_t prev_block_offset;
    int64_t next_block_offset;
    uint64_t created_time;
    uint64_t modified_time;
    uint32_t deleted;
} DB_DATA_BLOCK_ATTRIBUTES_T;

// DB data: contains lots of db records.
typedef struct
{
    DB_DATA_BLOCK_ATTRIBUTES_T db_data_block_attributes;
    uint8_t data[DB_DATA_SIZE / sizeof(uint8_t)];
} DB_DATA_BLOCK_T;

typedef struct
{
    uint64_t db_data_block_num;
    uint64_t created_time;
    uint64_t modified_time;
    uint32_t set_name_size;
    void *p_set_name;
} DB_SET_PROPERTIES_T;

typedef struct 
{
    FILE *file;
    DB_SET_PROPERTIES_T db_set_properties;
} DB_SET_INFO_T;

#endif // __DB_H__
