#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "faciledb.h"
#include "faciledb_utils.h"
#include "index.h"

#ifndef DB_SET_INFO_INSTANCE_NUM
#define DB_SET_INFO_INSTANCE_NUM (1)
#endif // DB_SET_INFO_INSTANCE_NUM

// static variables
static DB_SET_INFO_T db_set_info_instance[DB_SET_INFO_INSTANCE_NUM];
static char db_directory_path[DB_FILE_PATH_BUFFER_LENGTH] = {0};
// End of static vaiables

// Local function declaration
bool set_db_directory_path(char *p_db_directory_path);
bool check_db_directory_init();
void db_set_info_instances_init();
void db_set_info_init(DB_SET_INFO_T *p_db_set_info);
DB_SET_INFO_T *request_empty_db_set_info_instance();
DB_SET_INFO_T *query_db_set_info_loaded(void *p_db_set_name, uint32_t set_name_size);
bool is_db_set_file_exist(char *p_db_set_file_path);
void get_db_set_file_path_by_db_set_name(char *p_db_set_name, char *p_db_set_file_path);
DB_SET_INFO_T *load_db_set_info(char *p_db_set_name);
void free_db_set_info_resources(DB_SET_INFO_T *p_db_set_info);
void close_db_set_info(DB_SET_INFO_T *p_db_set_info);
void close_db_set_info_instances();
void db_set_properties_init(DB_SET_PROPERTIES_T *p_db_set_properties);
size_t get_db_set_properties_size(DB_SET_PROPERTIES_T *p_db_set_properties);
bool allocate_db_set_properties_resources(DB_SET_PROPERTIES_T *p_db_set_properties, uint32_t set_name_size);
void free_db_set_properties_resources(DB_SET_PROPERTIES_T *p_db_set_properties);
void write_db_set_properties(DB_SET_INFO_T *p_db_set_info);
void read_db_set_properties(DB_SET_INFO_T *p_db_set_info);
void db_block_init(DB_BLOCK_T *p_db_block);
off_t get_db_block_offset(DB_SET_PROPERTIES_T *p_db_set_properties, uint64_t block_tag);
void write_db_block(DB_BLOCK_T *p_db_block, DB_SET_INFO_T *p_db_set_info);
void read_db_block(DB_SET_INFO_T *p_db_set_info, uint64_t block_tag, DB_BLOCK_T *p_db_block);
DB_RECORD_INFO_T *extract_db_records_from_db_blocks(uint64_t block_tag, DB_SET_INFO_T *p_db_set_info, uint32_t *p_record_num);
void db_record_info_init(DB_RECORD_INFO_T *p_db_record_info);
bool allocate_db_record_info_resources(DB_RECORD_INFO_T *p_db_record_info);
void free_db_record_info_resources(DB_RECORD_INFO_T *p_db_record_info);
size_t get_db_record_properties_size();
void db_record_init(DB_RECORD_T *p_db_record);
bool allocate_db_record_resources(DB_RECORD_T *p_db_record, uint32_t key_size, uint32_t value_size);
void free_db_record_resources(DB_RECORD_T *p_db_record);
void insert_db_records(DB_SET_INFO_T *p_db_set_info, FACILEDB_RECORD_T *p_faciledb_records, uint32_t faciledb_record_length, uint64_t da_tag);
void update_db_block_next_block_tag(uint64_t block_tag, uint64_t next_block_tag, DB_SET_INFO_T *p_db_set_info);
uint64_t insert_db_records_handler_write_new_db_block(DB_BLOCK_T *p_db_block, DB_SET_INFO_T *p_db_set_info);
void insert_db_records_handler_assign_db_block_value(DB_BLOCK_T *p_db_block, uint64_t prev_block_tag, uint32_t valid_record_num, uint64_t data_tag);
uint64_t add_db_set_properties_valid_record_num(DB_SET_PROPERTIES_T *p_db_set_properties);
// End of local function declaration

void FacileDB_Api_Init(char *p_db_directory_path)
{
    set_db_directory_path(p_db_directory_path);
    db_set_info_instances_init();
}

void FacileDB_Api_Close()
{
    close_db_set_info_instances();
    set_db_directory_path("");
}

bool FacileDB_Api_Check_Set_Exist(char *p_db_set_name)
{
    char db_set_file_path[DB_FILE_PATH_BUFFER_LENGTH] = {0};

    // TODO: Check inputed p_db_set_name before using it.
    get_db_set_file_path_by_db_set_name(p_db_set_name, db_set_file_path);
    return is_db_set_file_exist(db_set_file_path);
}

bool FacileDB_Api_Insert_Element(char *p_db_set_name, FACILEDB_DATA_T *p_faciledb_data)
{
    DB_SET_INFO_T *p_db_set_info = NULL;
    uint64_t data_tag = 0;

    if (check_db_directory_init() == false || p_faciledb_data == NULL || p_faciledb_data->data_num == 0)
    {
        return false;
    }

    p_db_set_info = query_db_set_info_loaded(p_db_set_name, strlen(p_db_set_name));
    if (p_db_set_info == NULL)
    {
        p_db_set_info = load_db_set_info(p_db_set_name);

        if (p_db_set_info == NULL)
        {
            // load db set fail.
            return false;
        }
    }

    data_tag = add_db_set_properties_valid_record_num(&(p_db_set_info->db_set_properties));

    insert_db_records(p_db_set_info, p_faciledb_data->p_data_records, p_faciledb_data->data_num, data_tag);

    // write due to add valid record number.
    // TODO: update when close operation.
    write_db_set_properties(p_db_set_info);

    return true;
}

bool set_db_directory_path(char *p_db_directory_path)
{
    struct stat db_directory_stat;
    size_t str_length = 0;
    char temp_path_buffer[DB_FILE_PATH_BUFFER_LENGTH] = {0};

    strncpy(db_directory_path, p_db_directory_path, DB_FILE_PATH_MAX_LENGTH);
    db_directory_path[DB_FILE_PATH_MAX_LENGTH] = '\0';

    str_length = strlen(db_directory_path);
    if (str_length <= 0)
    {
        return false;
    }

    // The last char is not '/'.
    if (db_directory_path[str_length - 1] != '/')
    {
        if (str_length == DB_FILE_PATH_MAX_LENGTH)
        {
            // String length reaches the max buffer length, modify the last char to '/'.
            db_directory_path[str_length - 1] = '/';
        }
        else
        {
            // Append '/' at the end of the path.
            db_directory_path[str_length] = '/';
            db_directory_path[str_length + 1] = '\0';
        }
    }

    // Check if db directory exists or not.
    if ((stat(db_directory_path, &db_directory_stat) == 0) && (S_ISDIR(db_directory_stat.st_mode)))
    {
        // Existed
        return true;
    }

    // Index directory doesen't exist. Create a new direcotry
    for (char *p = strchr(db_directory_path + 1, '/'); p != NULL; p = strchr(p + 1, '/'))
    {
        strncpy(temp_path_buffer, db_directory_path, p - db_directory_path);
        temp_path_buffer[p - db_directory_path + 1] = '\0';

        if (stat(temp_path_buffer, &db_directory_stat) != 0 || !(S_ISDIR(db_directory_stat.st_mode)))
        {
            if (mkdir(temp_path_buffer, 0755) == -1)
            {
                perror("Create index directory fail");
                // clear the path buffer
                db_directory_path[0] = '\0';
                return false;
            }
        }
    }

    return true;
}

bool check_db_directory_init()
{
    if (db_directory_path[0] == '\0')
    {
        return false;
    }

    return true;
}

void db_set_info_instances_init()
{
    for (uint32_t i = 0; i < DB_SET_INFO_INSTANCE_NUM; i++)
    {
        db_set_info_init(&(db_set_info_instance[i]));
    }
}

void db_set_info_init(DB_SET_INFO_T *p_db_set_info)
{
    p_db_set_info->file = NULL;
    db_set_properties_init(&(p_db_set_info->db_set_properties));
}

DB_SET_INFO_T *request_empty_db_set_info_instance()
{
    // Current: only one instance
#if DB_SET_INFO_INSTANCE_NUM == 1
    DB_SET_INFO_T *p_db_set_info = &(db_set_info_instance[0]);
    close_db_set_info(p_db_set_info);

    return p_db_set_info;
#else
    // TODO: FIFO, LRU
#endif
}

DB_SET_INFO_T *query_db_set_info_loaded(void *p_db_set_name, uint32_t set_name_size)
{
#if DB_SET_INFO_INSTANCE_NUM == 1
    DB_SET_INFO_T *p_target_db_set_info = &(db_set_info_instance[0]);

    if ((p_target_db_set_info->file) &&
        (p_target_db_set_info->db_set_properties.set_name_size == set_name_size) &&
        (memcmp(p_target_db_set_info->db_set_properties.p_set_name, p_db_set_name, set_name_size) == 0))
    {
        return p_target_db_set_info;
    }
    else
    {
        return NULL;
    }
#else
#endif
}

bool is_db_set_file_exist(char *p_db_set_file_path)
{
    // TODO: check if set file is in the db_set_directory

    // unistd.h
    if (access(p_db_set_file_path, F_OK) == 0)
    {
        // db set file exists
        return true;
    }
    else
    {
        // db set file doesn't exist.
        return false;
    }
}

void get_db_set_file_path_by_db_set_name(char *p_db_set_name, char *p_db_set_file_path)
{
    // file path: /db/directory/path/set_name.faciledb
    // TODO: check p_db_set_name contains sensitive keywords. e.g. '/'

    char file_extension[] = ".faciledb";

    if ((strlen(db_directory_path) + strlen(p_db_set_name) + strlen(file_extension)) > DB_FILE_PATH_MAX_LENGTH)
    {
        p_db_set_file_path[0] = '\0';
    }
    else
    {
        strcpy(p_db_set_file_path, db_directory_path);
        strcat(p_db_set_file_path, p_db_set_name);
        strcat(p_db_set_file_path, file_extension);

        p_db_set_file_path[DB_FILE_PATH_MAX_LENGTH] = '\0';
    }
}

DB_SET_INFO_T *load_db_set_info(char *p_db_set_name)
{
    DB_SET_INFO_T *p_db_set_info = NULL;
    char db_set_file_path[DB_FILE_PATH_BUFFER_LENGTH] = {0};

    p_db_set_info = request_empty_db_set_info_instance();
    if (p_db_set_info == NULL)
    {
        return NULL;
    }
    else
    {
        db_set_info_init(p_db_set_info);
    }

    get_db_set_file_path_by_db_set_name(p_db_set_name, db_set_file_path);
    if (is_db_set_file_exist(db_set_file_path))
    {
        // open existed db_set_file as read and write permission.
        if (!((access(db_set_file_path, R_OK | W_OK) == 0) && ((p_db_set_info->file = fopen(db_set_file_path, "rb+")) != NULL)))
        {
            // db set file unreadable or unwritable
            perror("DB set file unavailable:");
            return NULL;
        }

        // read db properties from file and check set_name
        read_db_set_properties(p_db_set_info);
        if (!((strlen(p_db_set_name) == p_db_set_info->db_set_properties.set_name_size) &&
              (memcmp(p_db_set_name, p_db_set_info->db_set_properties.p_set_name, p_db_set_info->db_set_properties.set_name_size) == 0)))
        {
            close_db_set_info(p_db_set_info);
            return NULL;
        }
    }
    else
    {
        uint64_t current_time = 0;

        // db set file doesn't exist. Create a new db set file.
        p_db_set_info->file = fopen(db_set_file_path, "wb+");
        if (p_db_set_info->file == NULL)
        {
            // create db set file failed.
            perror("DB set file unavailable: ");
            return NULL;
        }

        // Setup set_name nad write new db properties into file.
        // store the db_set_name without the '\0'.
        // TODO: Setup variables
        allocate_db_set_properties_resources(&(p_db_set_info->db_set_properties), strlen(p_db_set_name));
        current_time = get_current_time();
        p_db_set_info->db_set_properties.set_name_size = strlen(p_db_set_name);
        memcpy(p_db_set_info->db_set_properties.p_set_name, p_db_set_name, p_db_set_info->db_set_properties.set_name_size);
        p_db_set_info->db_set_properties.created_time = current_time;
        p_db_set_info->db_set_properties.modified_time = current_time;
        write_db_set_properties(p_db_set_info);
    }

    return p_db_set_info;
}

void free_db_set_info_resources(DB_SET_INFO_T *p_db_set_info)
{
    if (p_db_set_info->file != NULL)
    {
        fclose(p_db_set_info->file);
        p_db_set_info->file = NULL;
    }

    free_db_set_properties_resources(&(p_db_set_info->db_set_properties));
}

void close_db_set_info(DB_SET_INFO_T *p_db_set_info)
{
    // Reset static variables

    // Free dynamic buffers
    free_db_set_info_resources(p_db_set_info);
}

void close_db_set_info_instances()
{
    for (uint32_t i = 0; i < DB_SET_INFO_INSTANCE_NUM; i++)
    {
        close_db_set_info(&(db_set_info_instance[i]));
    }
}

void db_set_properties_init(DB_SET_PROPERTIES_T *p_db_set_properties)
{
    p_db_set_properties->block_num = 0;
    p_db_set_properties->created_time = 0;
    p_db_set_properties->modified_time = 0;
    p_db_set_properties->valid_record_num = 0;
    p_db_set_properties->set_name_size = 0;

    p_db_set_properties->p_set_name = NULL;
}

size_t get_db_set_properties_size(DB_SET_PROPERTIES_T *p_db_set_properties)
{
    size_t set_properties_size = 0;

    // static variable
    set_properties_size = sizeof(p_db_set_properties->block_num) + sizeof(p_db_set_properties->created_time) + sizeof(p_db_set_properties->modified_time) + sizeof(p_db_set_properties->valid_record_num) + sizeof(p_db_set_properties->set_name_size);
    // dynamic variables
    set_properties_size += p_db_set_properties->set_name_size;

    return set_properties_size;
}

bool allocate_db_set_properties_resources(DB_SET_PROPERTIES_T *p_db_set_properties, uint32_t set_name_size)
{
    void *ptr = NULL;

    ptr = calloc(set_name_size, sizeof(uint8_t));
    if (ptr != NULL)
    {
        p_db_set_properties->p_set_name = ptr;
        return true;
    }

    return false;
}

void free_db_set_properties_resources(DB_SET_PROPERTIES_T *p_db_set_properties)
{
    // free set_name buffer
    if (p_db_set_properties->p_set_name != NULL)
    {
        free(p_db_set_properties->p_set_name);
        p_db_set_properties->p_set_name = NULL;
    }
}

/*
** The file variable in db_set_info should be set before calling this function.
*/
void write_db_set_properties(DB_SET_INFO_T *p_db_set_info)
{
    FILE *p_db_set_file = p_db_set_info->file;
    DB_SET_PROPERTIES_T *p_db_set_properties = &(p_db_set_info->db_set_properties);

    fseek(p_db_set_file, 0, SEEK_SET);

    // write static variables
    fwrite(&(p_db_set_properties->block_num), sizeof(p_db_set_properties->block_num), 1, p_db_set_file);
    fwrite(&(p_db_set_properties->created_time), sizeof(p_db_set_properties->created_time), 1, p_db_set_file);
    fwrite(&(p_db_set_properties->modified_time), sizeof(p_db_set_properties->modified_time), 1, p_db_set_file);
    fwrite(&(p_db_set_properties->valid_record_num), sizeof(p_db_set_properties->valid_record_num), 1, p_db_set_file);
    fwrite(&(p_db_set_properties->set_name_size), sizeof(p_db_set_properties->set_name_size), 1, p_db_set_file);

    // write dynamic variables
    fwrite(p_db_set_properties->p_set_name, p_db_set_properties->set_name_size, 1, p_db_set_file);
}

/*
** The file variable in db_set_info should be set before calling this function.
*/
void read_db_set_properties(DB_SET_INFO_T *p_db_set_info)
{
    FILE *p_db_set_file = p_db_set_info->file;
    DB_SET_PROPERTIES_T *p_db_set_properties = &(p_db_set_info->db_set_properties);

    fseek(p_db_set_file, 0, SEEK_SET);

    // read ststic variables
    fread(&(p_db_set_properties->block_num), sizeof(p_db_set_properties->block_num), 1, p_db_set_file);
    fread(&(p_db_set_properties->created_time), sizeof(p_db_set_properties->created_time), 1, p_db_set_file);
    fread(&(p_db_set_properties->modified_time), sizeof(p_db_set_properties->modified_time), 1, p_db_set_file);
    fread(&(p_db_set_properties->valid_record_num), sizeof(p_db_set_properties->valid_record_num), 1, p_db_set_file);
    fread(&(p_db_set_properties->set_name_size), sizeof(p_db_set_properties->set_name_size), 1, p_db_set_file);

    // allocate dynamic variable buffer and read dynamic variables from file
    allocate_db_set_properties_resources(p_db_set_properties, p_db_set_properties->set_name_size);
    fread(p_db_set_properties->p_set_name, p_db_set_properties->set_name_size, 1, p_db_set_file);
}

void db_block_init(DB_BLOCK_T *p_db_block)
{
    p_db_block->block_tag = 0;
    p_db_block->data_tag = 0;
    p_db_block->prev_block_tag = 0;
    p_db_block->next_block_tag = 0;
    p_db_block->created_time = 0;
    p_db_block->modified_time = 0;
    p_db_block->deleted = 0;
    p_db_block->valid_record_num = 0;
    p_db_block->record_properties_num = 0;

    memset(p_db_block->block_data, 0, DB_BLOCK_DATA_SIZE);
}

off_t get_db_block_offset(DB_SET_PROPERTIES_T *p_db_set_properties, uint64_t block_tag)
{
    size_t set_properties_size = get_db_set_properties_size(p_db_set_properties);
    size_t block_size = sizeof(DB_BLOCK_T);

    return (set_properties_size + ((block_tag - 1) * block_size));
}

// TODO: split each element for memory alignment
void write_db_block(DB_BLOCK_T *p_db_block, DB_SET_INFO_T *p_db_set_info)
{
    FILE *p_db_set_file = p_db_set_info->file;
    DB_SET_PROPERTIES_T *p_db_set_properties = &(p_db_set_info->db_set_properties);
    uint64_t block_tag = p_db_block->block_tag;
    off_t block_offset = get_db_block_offset(p_db_set_properties, block_tag);

    fseek(p_db_set_file, block_offset, SEEK_SET);

    // write static variables
    fwrite(p_db_block, sizeof(DB_BLOCK_T), 1, p_db_set_file);
}

// TODO: split each ement for memory alignment
void read_db_block(DB_SET_INFO_T *p_db_set_info, uint64_t block_tag, DB_BLOCK_T *p_db_block)
{
    FILE *p_db_set_file = p_db_set_info->file;
    off_t block_offset = get_db_block_offset(&(p_db_set_info->db_set_properties), block_tag);

    fseek(p_db_set_file, block_offset, SEEK_SET);

    // read static variables
    fread(p_db_block, sizeof(DB_BLOCK_T), 1, p_db_set_file);
}

DB_RECORD_INFO_T *extract_db_records_from_db_blocks(uint64_t block_tag, DB_SET_INFO_T *p_db_set_info, uint32_t *p_record_num)
{
    DB_BLOCK_T db_block;
    uint32_t record_num = 0;
    DB_RECORD_INFO_T *result = NULL;
    uint64_t next_block_tag = 0;
    uint8_t *p_block_data = NULL;
    uint8_t *p_block_end_address = NULL;

    db_block_init(&db_block);
    read_db_block(p_db_set_info, block_tag, &db_block);
    record_num = db_block.valid_record_num;
    result = malloc(record_num * sizeof(DB_RECORD_INFO_T));
    next_block_tag = db_block.next_block_tag;
    p_block_data = db_block.block_data;
    p_block_end_address = ((uint8_t *)&(db_block)) + sizeof(DB_BLOCK_T);

    if (result == NULL)
    {
        *p_record_num = 0;
        return NULL;
    }

    for (uint32_t i = 0; i < record_num; i++)
    {
        bool find_valid_db_record = false;
        uint32_t remaining_size = 0;

        db_record_info_init(&(result[i]));

        // check if valid record (deleted == false) and reaches to the valid one.
        while (find_valid_db_record == false)
        {
            if ((p_block_data + get_db_record_properties_size()) > p_block_end_address)
            {
                // clear the current block and read next block from next_block_tag and update the variables.
                db_block_init(&db_block);
                read_db_block(p_db_set_info, next_block_tag, &db_block);
                next_block_tag = db_block.next_block_tag;
                p_block_data = db_block.block_data;
                p_block_end_address = ((uint8_t *)&(db_block)) + sizeof(DB_BLOCK_T);
            }

            // check if the record deleted or not.
            if (((DB_RECORD_PROPERTIES_T *)p_block_data)->deleted == 0)
            {
                find_valid_db_record = true;
                break;
            }
            else
            {
                // record was deleted, bypass it.
                uint32_t key_size = ((DB_RECORD_PROPERTIES_T *)p_block_data)->key_size;
                uint32_t value_size = ((DB_RECORD_PROPERTIES_T *)p_block_data)->value_size;

                find_valid_db_record = false;
                p_block_data += get_db_record_properties_size();

                remaining_size = key_size;
                while (remaining_size > 0)
                {
                    uint32_t remaining_block_size = p_block_end_address - p_block_data;
                    uint32_t forward_size = (remaining_block_size > remaining_size) ? (remaining_size) : (remaining_block_size);

                    p_block_data += forward_size;
                    if (forward_size == 0)
                    {
                        // p_block_data reaches the end of the block, load next block and update variables.
                        db_block_init(&db_block);
                        read_db_block(p_db_set_info, next_block_tag, &db_block);
                        next_block_tag = db_block.next_block_tag;
                        p_block_data = db_block.block_data;
                        p_block_end_address = ((uint8_t *)&(db_block)) + sizeof(DB_BLOCK_T);
                    }
                    remaining_size -= forward_size;
                }

                remaining_size = value_size;
                while (remaining_size > 0)
                {
                    uint32_t remaining_block_size = p_block_end_address - p_block_data;
                    uint32_t forward_size = (remaining_block_size > remaining_size) ? (remaining_size) : (remaining_block_size);

                    p_block_data += forward_size;
                    if (forward_size == 0)
                    {
                        // p_block_data reaches the end of the block, load next block and update variables.
                        db_block_init(&db_block);
                        read_db_block(p_db_set_info, next_block_tag, &db_block);
                        next_block_tag = db_block.next_block_tag;
                        p_block_data = db_block.block_data;
                        p_block_end_address = ((uint8_t *)&(db_block)) + sizeof(DB_BLOCK_T);
                    }
                    remaining_size -= forward_size;
                }
            }
        }
        // Setting record properties offset and copy db_record_properties from db_block_data.
        result[i].db_record_properties_offset = get_db_block_offset(&(p_db_set_info->db_set_properties), block_tag) + (p_block_data - ((uint8_t *)&db_block));
        memcpy(&(result[i].db_record_properties), p_block_data, get_db_record_properties_size());
        p_block_data += get_db_record_properties_size();

        // Copy db_record from the db blocks.
        allocate_db_record_resources(&(result[i].db_record), result[i].db_record_properties.key_size, result[i].db_record_properties.value_size);

        // copy key of db_record from db block data.
        remaining_size = result[i].db_record_properties.key_size;
        while (remaining_size > 0)
        {
            uint32_t remaining_block_size = p_block_end_address - p_block_data;
            uint32_t copy_size = (remaining_block_size > remaining_size) ? (remaining_size) : (remaining_block_size);
            uint8_t *p_key = NULL;

            if (copy_size == 0)
            {
                // read next block and update variables.
                db_block_init(&db_block);
                read_db_block(p_db_set_info, next_block_tag, &db_block);
                next_block_tag = db_block.next_block_tag;
                p_block_data = db_block.block_data;
                p_block_end_address = ((uint8_t *)&(db_block)) + sizeof(DB_BLOCK_T);

                continue;
            }

            p_key = result[i].db_record.p_key + result[i].db_record_properties.key_size - remaining_size;
            memcpy(p_key, p_block_data, copy_size);
            p_block_data += copy_size;
            remaining_size -= copy_size;
        }

        remaining_size = result[i].db_record_properties.value_size;
        while (remaining_size > 0)
        {
            uint32_t remaining_block_size = p_block_end_address - p_block_data;
            uint32_t copy_size = (remaining_block_size > remaining_size) ? (remaining_size) : (remaining_block_size);
            uint8_t *p_value = NULL;

            if (copy_size == 0)
            {
                // read next block and update variables.
                db_block_init(&db_block);
                read_db_block(p_db_set_info, next_block_tag, &db_block);
                next_block_tag = db_block.next_block_tag;
                p_block_data = db_block.block_data;
                p_block_end_address = ((uint8_t *)&(db_block)) + sizeof(DB_BLOCK_T);
            }

            p_value = result[i].db_record.p_value + result[i].db_record_properties.value_size - remaining_size;
            memcpy(p_value, p_block_data, copy_size);
            p_block_data += copy_size;
            remaining_size -= copy_size;
        }
    }

    *p_record_num = record_num;
    return result;
}

void db_record_properties_init(DB_RECORD_PROPERTIES_T *p_db_record_properties)
{
    p_db_record_properties->deleted = 0;
    p_db_record_properties->key_size = 0;
    p_db_record_properties->value_size = 0;
    p_db_record_properties->record_value_type = FACILEDB_RECORD_VALUE_TYPE_INVALID;
}

void copy_db_record_properties_to_db_block_data(DB_BLOCK_T *p_db_block, DB_RECORD_INFO_T *p_db_record_info)
{
    uint8_t *p_write = p_db_block->block_data + p_db_record_info->db_record_properties_offset;

    memcpy(p_write, &(p_db_record_info->db_record_properties), sizeof(DB_RECORD_PROPERTIES_T));
}

size_t get_db_record_properties_size()
{
    return sizeof(DB_RECORD_PROPERTIES_T);
}

void db_record_init(DB_RECORD_T *p_db_record)
{
    p_db_record->p_key = NULL;
    p_db_record->p_value = NULL;
}

bool allocate_db_record_resources(DB_RECORD_T *p_db_record, uint32_t key_size, uint32_t value_size)
{
    if (p_db_record->p_key || p_db_record->p_value)
    {
        free_db_record_resources(p_db_record);
    }

    p_db_record->p_key = calloc(1, key_size * sizeof(uint8_t));
    if (p_db_record->p_key == NULL)
    {
        return false;
    }

    p_db_record->p_value = calloc(1, value_size * sizeof(uint8_t));
    if (p_db_record->p_value == NULL)
    {
        free_db_record_resources(p_db_record);
        return false;
    }

    return true;
}

void free_db_record_resources(DB_RECORD_T *p_db_record)
{
    if (p_db_record->p_key != NULL)
    {
        free(p_db_record->p_key);
        p_db_record->p_key = NULL;
    }

    if (p_db_record->p_value != NULL)
    {
        free(p_db_record->p_value);
        p_db_record->p_value = NULL;
    }
}

// p_db_record_info->db_record_properties should be set before.
void write_db_record(DB_RECORD_INFO_T *p_db_record_info, DB_SET_INFO_T *p_db_set_info)
{
    FILE *p_db_set_info_file = p_db_set_info->file;
    DB_RECORD_T *p_db_record = &(p_db_record_info->db_record);
    off_t record_offset = p_db_record_info->db_record_properties_offset + get_db_record_properties_size();
    uint32_t key_size = p_db_record_info->db_record_properties.key_size;
    uint32_t value_size = p_db_record_info->db_record_properties.value_size;

    fseek(p_db_set_info_file, record_offset, SEEK_SET);
    fwrite(p_db_record->p_key, key_size, 1, p_db_set_info_file);
    fwrite(p_db_record->p_value, value_size, 1, p_db_set_info_file);
}

// p_db_record_info->db_record_properties_offset should be set before.
// void copy_db_record_into_db_block_data(uint8_t, DB_RECORD_INFO_T *p_db_record_info)
// {
//     uint8_t *p_write = p_db_block->block_data + p_db_record_info->db_record_properties_offset + sizeof(DB_RECORD_PROPERTIES_T);
//     uint32_t key_size = p_db_record_info->db_record_properties.key_size;
//     uint32_t value_size = p_db_record_info->db_record_properties.value_size;

//     memcpy(p_write, p_db_record_info->db_record.p_key, key_size);
//     p_write += key_size;
//     memcpy(p_write, p_db_record_info->db_record.p_value, value_size);
// }

// p_db_record_info->db_record_properties should be set before.
void read_db_record(DB_RECORD_INFO_T *p_db_record_info, DB_SET_INFO_T *p_db_set_info)
{
    FILE *p_db_set_info_file = p_db_set_info->file;
    DB_RECORD_T *p_db_record = &(p_db_record_info->db_record);
    off_t record_offset = p_db_record_info->db_record_properties_offset + get_db_record_properties_size();
    uint32_t key_size = p_db_record_info->db_record_properties.key_size;
    uint32_t value_size = p_db_record_info->db_record_properties.value_size;

    fseek(p_db_set_info_file, record_offset, SEEK_SET);
    fread(p_db_record->p_key, key_size, 1, p_db_set_info_file);
    fread(p_db_record->p_value, value_size, 1, p_db_set_info_file);
}

void db_record_info_init(DB_RECORD_INFO_T *p_db_record_info)
{
    db_record_properties_init(&(p_db_record_info->db_record_properties));
    db_record_init(&(p_db_record_info->db_record));
    p_db_record_info->db_record_properties_offset = 0;
}

bool allocate_db_record_info_resources(DB_RECORD_INFO_T *p_db_record_info)
{
    uint32_t key_size = p_db_record_info->db_record_properties.key_size;
    uint32_t value_size = p_db_record_info->db_record_properties.value_size;

    return allocate_db_record_resources(&(p_db_record_info->db_record), key_size, value_size);
}

void free_db_record_info_resources(DB_RECORD_INFO_T *p_db_record_info)
{
    free_db_record_resources(&(p_db_record_info->db_record));
}

void faciledb_record_to_db_record_info(FACILEDB_RECORD_T *p_faciledb_record, DB_RECORD_INFO_T *p_db_record_info)
{
    DB_RECORD_PROPERTIES_T *p_db_record_properties = &(p_db_record_info->db_record_properties);
    DB_RECORD_T *p_db_record = &(p_db_record_info->db_record);

    p_db_record_properties->deleted = 0;
    p_db_record_properties->key_size = p_faciledb_record->key_size;
    p_db_record_properties->value_size = p_faciledb_record->value_size;
    p_db_record_properties->record_value_type = p_faciledb_record->record_value_type;

    allocate_db_record_resources(p_db_record, p_db_record_properties->key_size, p_db_record_properties->value_size);
    memcpy(p_db_record->p_key, p_faciledb_record->p_key, p_db_record_properties->key_size);
    memcpy(p_db_record->p_value, p_faciledb_record->p_value, p_db_record_properties->value_size);
}

// p_faciledb_records: array of faciledb_records
// faciledb_record_length: length of the db_records array,the value should greater than 0.
void insert_db_records(DB_SET_INFO_T *p_db_set_info, FACILEDB_RECORD_T *p_faciledb_records, uint32_t faciledb_record_length, uint64_t data_tag)
{
    DB_BLOCK_T new_db_block;
    uint8_t *p_db_block_write = new_db_block.block_data;
    uint8_t *p_db_block_end = new_db_block.block_data + DB_BLOCK_DATA_SIZE;
    size_t db_record_properties_size = get_db_record_properties_size();
    uint64_t last_block_tag = 0;

    db_block_init(&new_db_block);
    // Init the db_block and set first block prev/next block tag as 0.
    insert_db_records_handler_assign_db_block_value(&new_db_block, 0, faciledb_record_length, data_tag);
    for (uint32_t i = 0; i < faciledb_record_length; i++)
    {
        uint32_t remaining_size = 0;
        DB_RECORD_INFO_T record_info;

        db_record_info_init(&record_info);
        faciledb_record_to_db_record_info(&(p_faciledb_records[i]), &record_info);

        if ((p_db_block_write + db_record_properties_size) > p_db_block_end)
        {
            // new_db_block is full, write new_db_block into file.
            // Clear the new_block and local variables for new data.
            uint64_t prev_block_tag = insert_db_records_handler_write_new_db_block(&new_db_block, p_db_set_info);
            db_block_init(&new_db_block);
            insert_db_records_handler_assign_db_block_value(&new_db_block, prev_block_tag, faciledb_record_length, data_tag);

            // update local variables.
            p_db_block_write = new_db_block.block_data;
            p_db_block_end = new_db_block.block_data + DB_BLOCK_DATA_SIZE;
        }

        // copy record properties to block data.
        memcpy(p_db_block_write, &(record_info.db_record_properties), db_record_properties_size);
        new_db_block.record_properties_num++;
        p_db_block_write += db_record_properties_size;

        // copy record key to block data.
        remaining_size = record_info.db_record_properties.key_size;
        while (remaining_size > 0)
        {
            assert(p_db_block_end >= p_db_block_write);

            uint32_t remaining_block_size = (p_db_block_end - p_db_block_write);
            uint32_t copy_size = (remaining_block_size < remaining_size) ? (remaining_block_size) : (remaining_size);
            uint8_t *p_key = NULL;

            if (copy_size == 0)
            {
                // Current db_block is full, write db_block into file.
                // Clear the current block data and local variables for new data.
                uint64_t prev_block_tag = insert_db_records_handler_write_new_db_block(&new_db_block, p_db_set_info);
                db_block_init(&new_db_block);
                insert_db_records_handler_assign_db_block_value(&new_db_block, prev_block_tag, faciledb_record_length, data_tag);

                p_db_block_write = new_db_block.block_data;
                p_db_block_end = new_db_block.block_data + DB_BLOCK_DATA_SIZE;

                continue;
            }

            p_key = record_info.db_record.p_key + record_info.db_record_properties.key_size - remaining_size;
            memcpy(p_db_block_write, p_key, copy_size);
            p_db_block_write += copy_size;
            remaining_size -= copy_size;
        }

        // copy record value to block data
        remaining_size = record_info.db_record_properties.value_size;
        while (remaining_size > 0)
        {
            assert(p_db_block_end >= p_db_block_write);

            uint32_t remaining_block_size = p_db_block_end - p_db_block_write;
            uint32_t copy_size = (remaining_block_size < remaining_size) ? (remaining_block_size) : (remaining_size);
            uint8_t *p_value = NULL;

            if (copy_size == 0)
            {
                // block is full, write into file and clear the current block and pointer for new data.
                uint64_t prev_block_tag = insert_db_records_handler_write_new_db_block(&new_db_block, p_db_set_info);
                db_block_init(&new_db_block);
                insert_db_records_handler_assign_db_block_value(&new_db_block, prev_block_tag, faciledb_record_length, data_tag);

                // update local variables / pointers
                p_db_block_write = new_db_block.block_data;
                p_db_block_end = new_db_block.block_data + DB_BLOCK_DATA_SIZE;

                continue;
            }

            p_value = record_info.db_record.p_value + record_info.db_record_properties.value_size - remaining_size;
            memcpy(p_db_block_write, p_value, copy_size);
            p_db_block_write += copy_size;
            remaining_size -= copy_size;
        }

        free_db_record_info_resources(&record_info);
    }

    last_block_tag = insert_db_records_handler_write_new_db_block(&new_db_block, p_db_set_info);

    // recurssively update the next block tag of each db blocks.
    update_db_block_next_block_tag(last_block_tag, 0, p_db_set_info);

    // free db block resources if needed.
    // currently there is no dynamic resources in db block structure.
}

// traverse backward
void update_db_block_next_block_tag(uint64_t block_tag, uint64_t next_block_tag, DB_SET_INFO_T *p_db_set_info)
{
    DB_BLOCK_T db_block;
    db_block_init(&db_block);

    read_db_block(p_db_set_info, block_tag, &db_block);
    db_block.next_block_tag = next_block_tag;
    // TODO: only write the next_block_tag value
    write_db_block(&db_block, p_db_set_info);

    if (db_block.prev_block_tag != 0)
    {
        update_db_block_next_block_tag(db_block.prev_block_tag, block_tag, p_db_set_info);
    }
}

// return value: new block_tag
uint64_t insert_db_records_handler_write_new_db_block(DB_BLOCK_T *p_db_block, DB_SET_INFO_T *p_db_set_info)
{
    uint64_t block_tag = 0;
    uint64_t current_time = 0;

    p_db_set_info->db_set_properties.block_num++;
    block_tag = p_db_set_info->db_set_properties.block_num;
    p_db_block->block_tag = block_tag;

    current_time = (uint64_t)get_current_time();
    p_db_block->created_time = current_time;
    p_db_block->modified_time = current_time;
    // update db_set_properties time records
    p_db_set_info->db_set_properties.modified_time = current_time;

    write_db_block(p_db_block, p_db_set_info);

    return block_tag;
}

void insert_db_records_handler_assign_db_block_value(DB_BLOCK_T *p_db_block, uint64_t prev_block_tag, uint32_t valid_record_num, uint64_t data_tag)
{
    p_db_block->data_tag = data_tag;
    p_db_block->prev_block_tag = prev_block_tag;
    p_db_block->next_block_tag = 0;
    p_db_block->deleted = 0;
    p_db_block->record_properties_num = 0;
    p_db_block->valid_record_num = valid_record_num;
}

// return the updated value
uint64_t add_db_set_properties_valid_record_num(DB_SET_PROPERTIES_T *p_db_set_properties)
{
    // add and return the added value.
    return ++(p_db_set_properties->valid_record_num);
}
