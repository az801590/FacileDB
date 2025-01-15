#ifndef __FACILEDB_H__
#define __FACILEDB_H__

#include <stdint.h>
#include <stdio.h>

#ifndef DB_BLOCK_DATA_SIZE
#define DB_BLOCK_DATA_SIZE (1024)
#endif

#ifndef DB_FILE_PATH_BUFFER_LENGTH
#define DB_FILE_PATH_BUFFER_LENGTH (256) // linux definition: 255bytes
#endif

#define DB_FILE_PATH_MAX_LENGTH (DB_FILE_PATH_BUFFER_LENGTH - 1)
#define DB_RECORD_VALUE_TYPE_DYNAMIC_SIZE ((uint32_t)~0)

typedef enum
{
#ifdef FACILEDB_RECORD_VALUE_TYPE_CONFIG
#undef FACILEDB_RECORD_VALUE_TYPE_CONFIG
#endif
#define FACILEDB_RECORD_VALUE_TYPE_CONFIG(faciledb_record_value_type, faciledb_record_value_size, faciledb_record_value_compare_function) faciledb_record_value_type,
#include "faciledb_record_value_type_table.h"
#undef FACILEDB_RECORD_VALUE_TYPE_CONFIG
    FACILEDB_RECORD_VALUE_TYPE_NUM,
    FACILEDB_RECORD_VALUE_TYPE_INVALID
} FACILEDB_RECORD_VALUE_TYPE_E;

// user input format
typedef struct
{
    uint32_t key_size;
    uint32_t value_size;
    union
    {
        FACILEDB_RECORD_VALUE_TYPE_E record_value_type;
        uint32_t record_value_type_32;
    };
    void *p_key;
    void *p_value;
} FACILEDB_RECORD_T;

// user input format
typedef struct
{
    uint32_t data_num;           // array length
    FACILEDB_RECORD_T *p_data_records; // array of DB_RECORD_T structure.
} FACILEDB_DATA_T;

typedef struct
{
    uint32_t deleted;
    uint32_t key_size;
    uint32_t value_size;
    union
    {
        FACILEDB_RECORD_VALUE_TYPE_E record_value_type;
        uint32_t record_value_type_32;
    };
} DB_RECORD_PROPERTIES_T;

// Record: A key-value pair
typedef struct
{
    void *p_key;
    void *p_value;
} DB_RECORD_T;

// in-memory
typedef struct
{
    DB_RECORD_PROPERTIES_T db_record_properties;
    DB_RECORD_T db_record;
    off_t db_record_properties_offset; // The offset value from the block data starting address to the record properties address.
} DB_RECORD_INFO_T;

typedef struct
{
    uint64_t block_tag; // 1-based number, block_tag = 0 means null
    uint64_t prev_block_tag;
    uint64_t next_block_tag;
    uint64_t created_time;
    uint64_t modified_time;
    uint32_t deleted;
    uint32_t valid_record_num;
    uint32_t record_properties_num; // numbers of record in the data block

    uint8_t block_data[DB_BLOCK_DATA_SIZE]; // contains lots of db records.
} DB_BLOCK_T;

typedef struct
{
    uint64_t block_num;
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

#endif // __FACILEDB_H__
