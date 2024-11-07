#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "hash.h"
#include "index.h"

// Structure definition
typedef struct
{
    HASH_VALUE_T index_id;
    uint8_t index_payload[INDEX_PAYLOAD_SIZE];
} INDEX_ELEMENT_T;

typedef struct
{
    // tag is a 1-based number, tag=0 means null
    uint32_t tag;

    uint32_t level;
    uint32_t length; // length of elements array

    uint32_t parent_tag;
    uint32_t next_tag;
    // child_tag is a 1-based number and initilized as 0.
    uint32_t child_tag[INDEX_CHILD_TAG_ORDER];

    INDEX_ELEMENT_T elements[INDEX_ORDER];
} INDEX_NODE_T;

typedef struct
{
    // tag_num == max(tag)
    uint32_t tag_num;
    uint32_t root_tag;

    uint32_t key_size; // bytes
    uint8_t *p_key;

    // HASH_VALUE_T integrity;
} INDEX_PROPERTIES_T;

typedef struct
{
    FILE *index_file;
    INDEX_PROPERTIES_T index_properties;
} INDEX_INFO_T;
// End of structure definition

// Static Varialbes
static INDEX_INFO_T index_info_instance;
static char index_directory_path[INDEX_FILE_PATH_BUFFER_LENGTH];
// End of Static Variables

// Local function declaration.
void index_info_instances_init();
bool set_index_directory_path(char *p_index_directory_path);
INDEX_INFO_T *load_index_info(char *p_key);
INDEX_INFO_T *request_empty_index_info_instance();
void close_index(INDEX_INFO_T *index_info);
INDEX_INFO_T *query_index_info_loaded(uint8_t *p_index_key, uint32_t index_key_size);
void get_index_file_path_by_index_key(char *p_index_file_path, char *p_index_key);
void index_properties_init(INDEX_INFO_T *p_index_info, uint8_t *p_key, uint32_t key_size);
void read_index_properties(INDEX_INFO_T *p_index_info);
void write_index_properties(INDEX_INFO_T *p_index_info);
void write_index_node(INDEX_INFO_T *p_index_info, INDEX_NODE_T *p_index_node);
void setup_index_element(INDEX_ELEMENT_T *p_index_element, uint8_t *p_target, uint32_t target_size, uint8_t *p_payload, uint32_t payload_size);
void index_node_init(INDEX_NODE_T *p_index_node, uint32_t tag);
off_t get_node_offset(INDEX_INFO_T *p_index_info, uint32_t tag);
bool read_index_node(INDEX_INFO_T *p_index_info, uint32_t tag, INDEX_NODE_T *p_index_node);
uint32_t find_element_position_in_the_node(INDEX_NODE_T *p_index_node, INDEX_ELEMENT_T *p_index_element);
void split_index_elements_into_two_index_node(INDEX_ELEMENT_T *p_index_element_buffer, uint32_t index_element_buffer_length, INDEX_NODE_T *p_index_node_current, INDEX_NODE_T *p_index_node_sibling);
void split_child_tags_into_two_index_node(INDEX_INFO_T *p_index_info, uint32_t *p_child_tag_buffer, uint32_t child_tag_buffer_length, INDEX_NODE_T *p_index_node_current, INDEX_NODE_T *p_index_node_sibling);
void insert_index_element_handler(INDEX_INFO_T *p_index_info, INDEX_NODE_T *p_index_node, INDEX_ELEMENT_T *p_index_element, uint32_t new_child_tag);
void insert_index_element(INDEX_INFO_T *p_index_info, uint32_t tag, INDEX_ELEMENT_T *p_index_element);
uint8_t *search_index_element(INDEX_INFO_T *p_index_info, uint32_t tag, INDEX_ELEMENT_T *p_target_index_element, uint32_t *result_length);
uint8_t *search_index_element_handler(INDEX_INFO_T *p_index_info, INDEX_NODE_T *p_index_node, INDEX_ELEMENT_T *p_target_index_element, uint32_t *result_length);
// End of local function declaration

void Index_Api_Init(char *p_index_directory_path)
{
    // Initialize index dorectory path.
    set_index_directory_path(p_index_directory_path);

    // Initialize index info instances.
    index_info_instances_init();
}

void index_info_instances_init()
{
    index_info_instance.index_file = NULL;
    memset(&(index_info_instance.index_properties), 0, sizeof(INDEX_PROPERTIES_T));
}

bool set_index_directory_path(char *p_index_directory_path)
{
    struct stat index_directory_stat;
    char temp_path_buffer[INDEX_FILE_PATH_BUFFER_LENGTH] = {0};

    strncpy(index_directory_path, p_index_directory_path, INDEX_FILE_PATH_MAX_LENGTH);
    index_directory_path[INDEX_FILE_PATH_MAX_LENGTH] = '\0';

    // add '/' at the end of the path if the last char is not '/'
    if (index_directory_path[strlen(index_directory_path) - 1] != '/')
    {
        index_directory_path[strlen(index_directory_path) - 1] = '/';
    }

    // Check if index directory exists or not.
    if ((stat(index_directory_path, &index_directory_stat) == 0) && (S_ISDIR(index_directory_stat.st_mode)))
    {
        // Existed
        return true;
    }

    // Index directory doesen't exist. Create a new direcotry
    for (char *p = strchr(index_directory_path + 1, '/'); p != NULL; p = strchr(p + 1, '/'))
    {
        strncpy(temp_path_buffer, index_directory_path, p - index_directory_path);
        temp_path_buffer[p - index_directory_path + 1] = '\0';

        if (stat(temp_path_buffer, &index_directory_stat) != 0 || !(S_ISDIR(index_directory_stat.st_mode)))
        {
            if (mkdir(temp_path_buffer, 0755) == -1)
            {
                perror("Create index directory fail");
                return false;
            }
        }
    }

    return true;
}

void Index_Api_Insert_Element(char *p_index_key, uint8_t *p_target, uint32_t target_size, uint8_t *p_index_payload, uint32_t payload_size)
{
    INDEX_INFO_T *p_index_info = query_index_info_loaded((uint8_t *)p_index_key, strlen(p_index_key));
    uint32_t root_tag = 0;
    INDEX_ELEMENT_T index_element;

    if (p_index_info == NULL)
    {
        p_index_info = load_index_info(p_index_key);
    }
    root_tag = p_index_info->index_properties.root_tag;

    setup_index_element(&index_element, p_target, target_size, (uint8_t *)p_index_payload, payload_size);
    insert_index_element(p_index_info, root_tag, &index_element);
}

INDEX_INFO_T *load_index_info(char *p_key)
{
    INDEX_INFO_T *p_index_info = request_empty_index_info_instance();
    char index_file_path[INDEX_FILE_PATH_BUFFER_LENGTH] = {0};

    get_index_file_path_by_index_key(index_file_path, p_key);

    if (access(index_file_path, F_OK) == 0)
    {
        // Index file exists
        if (access(index_file_path, R_OK | W_OK) == 0)
        {
            // Index file readable & writable
            p_index_info->index_file = fopen(index_file_path, "rb+");
        }
        else
        {
            // index file unreadable or unwritable
            perror("Index file unavailable");
            return NULL;
        }
    }
    else
    {
        // Index file doesn't exist. Create a new index file.
        p_index_info->index_file = fopen(index_file_path, "wb+");
        if (p_index_info->index_file == NULL)
        {
            // create index file failed.
            perror("Index file unavailable: ");
            return NULL;
        }
    }

    index_properties_init(p_index_info, (uint8_t *)p_key, strlen(p_key));
    return p_index_info;
}

// todo: using fifo queue or lru algorithm.
INDEX_INFO_T *request_empty_index_info_instance()
{
    if (index_info_instance.index_file != NULL)
    {
        close_index(&index_info_instance);
    }

    return &index_info_instance;
}

void close_index(INDEX_INFO_T *index_info)
{
    // writeback_index_properties();

    fclose(index_info->index_file);
    index_info->index_file = NULL;
}

void Index_Api_Close()
{
    if (index_info_instance.index_file != NULL)
    {
        close_index(&index_info_instance);
    }
}

INDEX_INFO_T *query_index_info_loaded(uint8_t *p_index_key, uint32_t index_key_size)
{
    if (index_info_instance.index_file)
    {
        uint32_t min_key_compare_length = (index_key_size > index_info_instance.index_properties.key_size) ? (index_info_instance.index_properties.key_size) : (index_key_size);
        if (memcmp(index_info_instance.index_properties.p_key, p_index_key, min_key_compare_length) == 0)
        {
            return &index_info_instance;
        }
    }

    return NULL;
}

void get_index_file_path_by_index_key(char *p_index_file_path, char *p_index_key)
{
    // Default: ./index_key.index
    // TODO: check p_key contains sensitive keywords. e.g. '/'
    strncpy(p_index_file_path, index_directory_path, INDEX_FILE_PATH_MAX_LENGTH);
    p_index_file_path[INDEX_FILE_PATH_MAX_LENGTH] = '\0';
    strncat(p_index_file_path, p_index_key, INDEX_FILE_PATH_MAX_LENGTH - strlen(p_index_file_path));
    p_index_file_path[INDEX_FILE_PATH_MAX_LENGTH] = '\0';
}

// Read index_properties or init an index_properties and a first node and write it into file
void index_properties_init(INDEX_INFO_T *p_index_info, uint8_t *p_key, uint32_t key_size)
{
    off_t file_size = 0;
    INDEX_PROPERTIES_T *p_index_properties = &(p_index_info->index_properties);

    fseek(p_index_info->index_file, 0, SEEK_END);
    file_size = ftell(p_index_info->index_file);

    if (file_size == 0)
    {
        INDEX_NODE_T first_node;

        p_index_properties->p_key = malloc(key_size * sizeof(uint8_t));
        memcpy(p_index_properties->p_key, p_key, key_size);
        p_index_properties->key_size = key_size;
        p_index_properties->root_tag = 0;
        p_index_properties->tag_num = 0;

        // insert an empty node with node tag: 1
        index_node_init(&first_node, ++(p_index_properties->tag_num));
        p_index_properties->root_tag = first_node.tag;

        // write to file
        write_index_properties(p_index_info);
        write_index_node(p_index_info, &first_node);
    }
    else
    {
        // TODO: Checker, node number matches, size matches
        read_index_properties(p_index_info);
    }
}

void read_index_properties(INDEX_INFO_T *p_index_info)
{
    FILE *p_index_file = p_index_info->index_file;
    INDEX_PROPERTIES_T *p_index_properties = &(p_index_info->index_properties);

    fseek(p_index_file, 0, SEEK_SET);
    // Read tag_num and root_tag
    fread(&(p_index_properties->tag_num), sizeof(p_index_properties->tag_num), 1, p_index_file);
    fread(&(p_index_properties->root_tag), sizeof(p_index_properties->root_tag), 1, p_index_file);

    // Read key_size
    fread(&(p_index_properties->key_size), sizeof(p_index_properties->key_size), 1, p_index_file);
    // Read key
    p_index_properties->p_key = malloc(p_index_properties->key_size);
    fread(p_index_properties->p_key, p_index_properties->key_size, 1, p_index_file);
}

void write_index_properties(INDEX_INFO_T *p_index_info)
{
    FILE *p_index_file = p_index_info->index_file;
    INDEX_PROPERTIES_T *p_index_properties = &(p_index_info->index_properties);

    fseek(p_index_file, 0, SEEK_SET);

    // Write tag_num & root_tag
    fwrite(&(p_index_properties->tag_num), sizeof(p_index_properties->tag_num), 1, p_index_file);
    fwrite(&(p_index_properties->root_tag), sizeof(p_index_properties->root_tag), 1, p_index_file);

    // Write key_size
    fwrite(&(p_index_properties->key_size), sizeof(p_index_properties->key_size), 1, p_index_file);
    // Write key
    fwrite(p_index_properties->p_key, p_index_properties->key_size, 1, p_index_file);
}

void write_index_node(INDEX_INFO_T *p_index_info, INDEX_NODE_T *p_index_node)
{
    uint32_t node_tag = p_index_node->tag;
    off_t node_offset = get_node_offset(p_index_info, node_tag);
    FILE *p_index_file = p_index_info->index_file;

    fseek(p_index_file, node_offset, SEEK_SET);
    // TODO: split
    fwrite(p_index_node, sizeof(INDEX_NODE_T), 1, p_index_file);
}

void setup_index_element(INDEX_ELEMENT_T *p_index_element, uint8_t *p_target, uint32_t target_size, uint8_t *p_payload, uint32_t payload_size)
{
#if __INDEX_TEST__
    p_index_element->index_id = *((HASH_VALUE_T *)p_target);
#else
    p_index_element->index_id = Hash(p_target, target_size);
#endif

    payload_size = (payload_size > INDEX_PAYLOAD_SIZE) ? INDEX_PAYLOAD_SIZE : payload_size;

    memset(p_index_element->index_payload, 0, INDEX_PAYLOAD_SIZE);
    if (payload_size > 0 && p_payload != NULL)
    {
        memcpy(p_index_element->index_payload, p_payload, payload_size);
    }
}

void index_node_init(INDEX_NODE_T *p_index_node, uint32_t tag)
{
    memset(p_index_node, 0, sizeof(INDEX_NODE_T));
    p_index_node->tag = tag;
}

off_t get_node_offset(INDEX_INFO_T *p_index_info, uint32_t tag)
{
    size_t index_properties_size = 0;
    size_t index_node_size = 0;
    INDEX_PROPERTIES_T *p_index_properties = &(p_index_info->index_properties);

    index_properties_size = sizeof(p_index_properties->tag_num) + sizeof(p_index_properties->root_tag) + sizeof(p_index_properties->key_size);
    index_properties_size += p_index_properties->key_size;

    index_node_size = sizeof(INDEX_NODE_T);

    // tag is a 1-based number.
    return (index_properties_size + ((tag - 1) * index_node_size));
}

// return sucessful or not
bool read_index_node(INDEX_INFO_T *p_index_info, uint32_t tag, INDEX_NODE_T *p_index_node)
{
    // tag is a 1-index number and it should smaller than the tag number.
    if (p_index_info->index_properties.tag_num < tag)
    {
        // Index node doesn't exist
        return false;
    }

    FILE *p_index_file = p_index_info->index_file;
    off_t node_offset = get_node_offset(p_index_info, tag);

    fseek(p_index_file, node_offset, SEEK_SET);
    fread(p_index_node, sizeof(INDEX_NODE_T), 1, p_index_file);

    return true;
}

// Find the array position in the node where the new element should insert into.
// Return the minimum element position where the value is equal or greater than the inputed value.
uint32_t find_element_position_in_the_node(INDEX_NODE_T *p_index_node, INDEX_ELEMENT_T *p_index_element)
{
    // binary search index_element position.
    // find equal or upper bound (ceiling child_tag in the node)
    uint32_t start = 0;
    uint32_t end = p_index_node->length; // [start, end)

    while (start < end)
    {
        uint32_t mid = start + ((end - start) / 2);
        HASH_VALUE_COMPARE_RESULT cmp_result = Hash_Compare(p_index_element->index_id, p_index_node->elements[mid].index_id);

        if (cmp_result == HASH_VALUE_LEFT_GREATER)
        {
            // grater than [mid]
            start = mid + 1;
        }
        else
        {
            // Smaller or equals to [mid]
            end = mid;
        }
    }
    assert((start == end) && (start <= p_index_node->length));
    return start;
}

void split_index_elements_into_two_index_node(INDEX_ELEMENT_T *p_index_element_buffer, uint32_t index_element_buffer_length, INDEX_NODE_T *p_index_node_current, INDEX_NODE_T *p_index_node_sibling)
{
    uint32_t start_position, copy_length;
    bool is_leaf_node = (p_index_node_current->child_tag[0] == 0) ? true : false;

    // Copy first half of index elements into current node.
    start_position = 0;
    copy_length = index_element_buffer_length / 2;
    memcpy(p_index_node_current->elements, p_index_element_buffer, sizeof(INDEX_ELEMENT_T) * copy_length);
    p_index_node_current->length = copy_length;

    // Copy second half of index elements into the sibling node.
    if (is_leaf_node)
    {
        start_position = copy_length;
        copy_length = index_element_buffer_length - copy_length;
    }
    else
    {
        // If current node and sibling node are not leaf nodes, it doesn't need to copy the [mid] element to sibling node.
        // The [mid] element should be inserted to parent node.
        start_position = copy_length + 1;
        copy_length = index_element_buffer_length - copy_length - 1;
    }
    memcpy(p_index_node_sibling->elements, &(p_index_element_buffer[start_position]), sizeof(INDEX_ELEMENT_T) * copy_length);
    p_index_node_sibling->length = copy_length;
}

void split_child_tags_into_two_index_node(INDEX_INFO_T *p_index_info, uint32_t *p_child_tag_buffer, uint32_t child_tag_buffer_length, INDEX_NODE_T *p_index_node_current, INDEX_NODE_T *p_index_node_sibling)
{
    uint32_t first_half_length = (child_tag_buffer_length / 2) + (child_tag_buffer_length % 2);
    uint32_t second_half_length = child_tag_buffer_length - first_half_length;
    INDEX_NODE_T second_half_child_index_node;

    // Copy first half of child tags to current node.
    memcpy(p_index_node_current->child_tag, p_child_tag_buffer, sizeof(uint32_t) * first_half_length);
    // Copy second half of child tags to sibling node.
    memcpy(p_index_node_sibling->child_tag, &(p_child_tag_buffer[first_half_length]), sizeof(uint32_t) * second_half_length);
    // update the parent tag of the second half child nodes to sibling node tag.
    for (uint32_t i = 0; i < second_half_length; i++)
    {
        read_index_node(p_index_info, p_index_node_sibling->child_tag[i], &second_half_child_index_node);
        second_half_child_index_node.parent_tag = p_index_node_sibling->tag;
        write_index_node(p_index_info, &second_half_child_index_node);
    }
}

void insert_index_element_handler(INDEX_INFO_T *p_index_info, INDEX_NODE_T *p_index_node, INDEX_ELEMENT_T *p_index_element, uint32_t new_child_tag)
{
    uint32_t position = find_element_position_in_the_node(p_index_node, p_index_element);
    uint32_t tag_position = position + 1;
    INDEX_PROPERTIES_T *p_index_properties = &(p_index_info->index_properties);

    if (p_index_node->length >= INDEX_ORDER)
    {
        // index node is full.
        // split current node into two nodes and insert the [INDEX_ORDER / 2] element to the parent node.
        INDEX_NODE_T parent_node, new_sibling_node;
        INDEX_ELEMENT_T index_elements_buffer[INDEX_ORDER + 1];
        uint32_t child_tags_buffer[INDEX_CHILD_TAG_ORDER + 1];
        uint32_t mid_position = (INDEX_ORDER + 1) / 2;
        bool is_leaf_node = (p_index_node->child_tag[0] == 0) ? true : false;

        // insert the current node and the new element into buffer by order.
        if (position > 0)
        {
            memcpy(index_elements_buffer, &(p_index_node->elements[0]), sizeof(INDEX_ELEMENT_T) * position);
        }
        memcpy(&(index_elements_buffer[position]), p_index_element, sizeof(INDEX_ELEMENT_T));
        if (position < INDEX_ORDER)
        {
            memcpy(&(index_elements_buffer[position + 1]), &(p_index_node->elements[position]), sizeof(INDEX_ELEMENT_T) * (INDEX_ORDER - position));
        }

        // Insert the current child tags and new_child_tag into buffer by order.
        // new_child_tag is the child tag that greater than the inputted element.
        if (tag_position > 0)
        {
            memcpy(child_tags_buffer, &(p_index_node->child_tag[0]), sizeof(uint32_t) * tag_position);
        }
        child_tags_buffer[tag_position] = new_child_tag;
        if (tag_position < INDEX_CHILD_TAG_ORDER)
        {
            memcpy(&(child_tags_buffer[tag_position + 1]), &(p_index_node->child_tag[tag_position]), sizeof(uint32_t) * (INDEX_CHILD_TAG_ORDER - tag_position));
        }

        // Create and initialize the sibling Node
        p_index_properties->tag_num++;
        index_node_init(&new_sibling_node, p_index_properties->tag_num);
        new_sibling_node.level = p_index_node->level;
        new_sibling_node.parent_tag = p_index_node->parent_tag;
        new_sibling_node.next_tag = p_index_node->next_tag;
        p_index_node->next_tag = new_sibling_node.tag;

        // Create or load parent node.
        if (p_index_node->parent_tag == 0)
        {
            // no parent node, create a new one.
            p_index_properties->tag_num++;
            index_node_init(&parent_node, p_index_properties->tag_num);
            parent_node.child_tag[0] = p_index_node->tag;
            parent_node.level = p_index_node->level + 1;
            p_index_properties->root_tag = parent_node.tag;

            p_index_node->parent_tag = parent_node.tag;
            new_sibling_node.parent_tag = parent_node.tag;
        }
        else
        {
            read_index_node(p_index_info, p_index_node->parent_tag, &parent_node);
        }

        // Insert first half of the elements in the buffer into current node and insert the second half of the elements into sibling node.
        // If the current node is a leaf node, copy the [mid] element to new sibling node to keep it in the leaf.
        // If current node is not a leaf node, we don't have to keep it in any node in the current node level.
        split_index_elements_into_two_index_node(index_elements_buffer, INDEX_ORDER + 1, p_index_node, &new_sibling_node);
        if (is_leaf_node == false)
        {
            split_child_tags_into_two_index_node(p_index_info, child_tags_buffer, INDEX_CHILD_TAG_ORDER + 1, p_index_node, &new_sibling_node);
        }

        // write sibling node into file
        write_index_node(p_index_info, &new_sibling_node);

        // write current node into file.
        write_index_node(p_index_info, p_index_node);

        write_index_properties(p_index_info);

        // Insert the [mid] element to the parent node.
        insert_index_element_handler(p_index_info, &parent_node, &(index_elements_buffer[mid_position]), new_sibling_node.tag);
        // the above function will write parent node into file.
    }
    else
    {
        // index_node isn't full
        // insertion sort
        for (uint32_t i = p_index_node->length; i > position; i--)
        {
            // move the elements in the node which greater than the inserted element to the next position in the array.
            memcpy(&(p_index_node->elements[i]), &(p_index_node->elements[i - 1]), sizeof(INDEX_ELEMENT_T));
            // move child_tags
            p_index_node->child_tag[i + 1] = p_index_node->child_tag[i];
        }

        memcpy(&(p_index_node->elements[position]), p_index_element, sizeof(INDEX_ELEMENT_T));
        p_index_node->child_tag[tag_position] = new_child_tag;
        p_index_node->length++;

        // write current node into file.
        write_index_node(p_index_info, p_index_node);
    }
}

void insert_index_element(INDEX_INFO_T *p_index_info, uint32_t tag, INDEX_ELEMENT_T *p_index_element)
{
    INDEX_NODE_T index_node;

    if (read_index_node(p_index_info, tag, &index_node) == false)
    {
        return;
    }

    // Check if the leaf node existed or not by child_tag[0], and process if reached leaf node.
    if (index_node.child_tag[0] != 0)
    {
        // Current node is not a leaf node.
        // find the insert position (and also the child_tag position). Position is in the range of [0, index_node.length].
        uint32_t position = find_element_position_in_the_node(&index_node, p_index_element);
        return insert_index_element(p_index_info, index_node.child_tag[position], p_index_element);
    }
    else
    {
        // Current node is a leaf node.
        return insert_index_element_handler(p_index_info, &index_node, p_index_element, 0);
    }
}

// return value: result array
// result_length: integer, number of results in result array
uint8_t *Index_Api_Search(char *p_index_key, uint8_t *p_target, uint32_t target_size, uint32_t *result_length)
{
    INDEX_INFO_T *p_index_info = query_index_info_loaded((uint8_t *)p_index_key, strlen(p_index_key));
    uint32_t root_tag = 0;
    INDEX_ELEMENT_T target_index_element;

    if (p_index_info == NULL)
    {
        p_index_info = load_index_info(p_index_key);
    }
    root_tag = p_index_info->index_properties.root_tag;
    setup_index_element(&target_index_element, p_target, target_size, NULL, 0);

    return search_index_element(p_index_info, root_tag, &target_index_element, result_length);
}

uint8_t *search_index_element(INDEX_INFO_T *p_index_info, uint32_t tag, INDEX_ELEMENT_T *p_target_index_element, uint32_t *result_length)
{
    INDEX_NODE_T index_node;

    if (read_index_node(p_index_info, tag, &index_node) == false)
    {
        *result_length = 0;
        return NULL;
    }

    if (index_node.child_tag[0] != 0)
    {
        // non-leaf
        uint32_t position = find_element_position_in_the_node(&index_node, p_target_index_element);
        return search_index_element(p_index_info, index_node.child_tag[position], p_target_index_element, result_length);
    }
    else
    {
        // leaf-node
        return search_index_element_handler(p_index_info, &index_node, p_target_index_element, result_length);
    }
}

uint8_t *search_index_element_handler(INDEX_INFO_T *p_index_info, INDEX_NODE_T *p_index_node, INDEX_ELEMENT_T *p_target_index_element, uint32_t *result_length)
{
    uint8_t *p_search_result = NULL;
    uint32_t position = find_element_position_in_the_node(p_index_node, p_target_index_element);
    uint32_t compare_equal_length = 0, i = 0;
    bool only_current_node_result = true;

    for (i = position; i < p_index_node->length; i++)
    {
        if (Hash_Compare(p_target_index_element->index_id, p_index_node->elements[i].index_id) == HASH_VALUE_EQUAL)
        {
            compare_equal_length++;
        }
        else
        {
            break;
        }
    }

    if ((i == p_index_node->length) && (p_index_node->next_tag != 0))
    {
        // The target node may also existed in the next node, search next node.
        INDEX_NODE_T next_index_node;
        uint8_t *next_index_node_search_result = NULL;

        if (read_index_node(p_index_info, p_index_node->next_tag, &next_index_node) == true)
        {
            next_index_node_search_result = search_index_element_handler(p_index_info, &next_index_node, p_target_index_element, result_length);
            compare_equal_length += *result_length;
            p_search_result = realloc(next_index_node_search_result, compare_equal_length * INDEX_PAYLOAD_SIZE);
            if (p_search_result == NULL)
            {
                // allocate more memory error.
                // Error handling: return next search result, and doesn't attach current result.
                return next_index_node_search_result;
            }
            only_current_node_result = false;
        }
        else
        {
            // read next node error
            only_current_node_result = true;
        }
    }

    if (only_current_node_result == true)
    {
        // The right most element is not equal to the target. No need to search next node.
        // Or next node doesn't exist.
        // Or next node read error.
        *result_length = 0;
        if (compare_equal_length > 0)
        {
            p_search_result = malloc(compare_equal_length * INDEX_PAYLOAD_SIZE);
            if (p_search_result == NULL)
            {
                // Allocate memory error
                // return NULL pointer and length = 0
                return NULL;
            }
        }
    }

    for (i = *result_length; i < compare_equal_length; i++)
    {
        memcpy(p_search_result + (INDEX_PAYLOAD_SIZE * i), p_index_node->elements[i].index_payload, INDEX_PAYLOAD_SIZE);
    }

    *result_length = compare_equal_length;
    return p_search_result;
}
