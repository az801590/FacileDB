#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#define __INDEX_TEST__ (1)
#define INDEX_ORDER (3)
#define INDEX_PAYLOAD_SIZE (8)

#include "hash.c"
#include "index.c"

void test_start(char *case_name)
{
    printf("Test Case: %s START!\n", case_name);
}

void test_end(char *case_name)
{
    printf("Test Case: %s END!\n\n", case_name);
}

void check_index_properties(INDEX_PROPERTIES_T *p_index_properties_1, INDEX_PROPERTIES_T *p_index_properties_2)
{
    assert(p_index_properties_1->tag_num == p_index_properties_2->tag_num);
    assert(p_index_properties_1->root_tag == p_index_properties_2->root_tag);
    assert(p_index_properties_1->key_size == p_index_properties_2->key_size);
    assert(memcmp(p_index_properties_1->p_key, p_index_properties_2->p_key, p_index_properties_1->key_size) == 0);
}

void check_index_node(INDEX_NODE_T *p_index_node_1, INDEX_NODE_T *p_index_node_2)
{
    uint32_t i;

    assert(p_index_node_1->tag == p_index_node_2->tag);
    assert(p_index_node_1->level == p_index_node_2->level);
    assert(p_index_node_1->length == p_index_node_2->length);
    assert(p_index_node_1->parent_tag == p_index_node_2->parent_tag);
    assert(p_index_node_1->next_tag == p_index_node_2->next_tag);
    for (i = 0; i < p_index_node_1->length; i++)
    {
        assert(p_index_node_1->child_tag[i] == p_index_node_2->child_tag[i]);
        if (p_index_node_1->level == 0)
        {
            // The index node is a leaf node
            assert(memcmp(&(p_index_node_1->elements[i]), &(p_index_node_2->elements[i]), sizeof(INDEX_ELEMENT_T)) == 0);
        }
    }
    assert(p_index_node_1->child_tag[i] == p_index_node_2->child_tag[i]);
}

void print_index_node(INDEX_NODE_T *p_index_node)
{
    printf("\ntag: %d\n", p_index_node->tag);
    printf("level: %d\n", p_index_node->level);
    printf("length: %d\n", p_index_node->length);
    printf("parnet_tag: %d\n", p_index_node->parent_tag);
    printf("next_tag: %d\n", p_index_node->next_tag);
    for (int i = 0; i < p_index_node->length + 1; i++)
    {
        printf("child_tag[%d]: %d\n", i, p_index_node->child_tag[i]);
    }
    for (int i = 0; i < p_index_node->length; i++)
    {
        printf("element_index_id[%d]: %d\n", i, p_index_node->elements[i].index_id);
        if (INDEX_PAYLOAD_SIZE > 0)
        {
            printf("\telement_payload_first_char: %c\n", *((char *)(p_index_node->elements[i].index_payload)));
        }
    }
}

/*----------------------------*/

char test_index_directory[] = "./bin/test_index_files/";

void test_hash()
{
    char case_name[] = "hash";

    test_start(case_name);

    uint8_t s[] = "dog";
    HASH_VALUE_T result = Hash(s, strlen((char *)s));
    HASH_VALUE_T expected = 0x0b886aff;

    // printf("0x%llx\n", result);
    assert(Hash_Compare(result, expected) == HASH_VALUE_EQUAL);

    test_end(case_name);
}

void test_index_init()
{
    char case_name[] = "test_index_init";
    test_start(case_name);

    Index_Api_Init(test_index_directory);

    assert(strcmp(index_directory_path, test_index_directory) == 0);

    test_end(case_name);
}

void test_index_close()
{
    char case_name[] = "test_index_close";
    test_start(case_name);

    char p_index_key[] = "test_index_close.index";
    uint32_t target = 1;
    char payload[100] = "aaaaa";

    Index_Api_Init(test_index_directory);
    Index_Api_Insert_Element(p_index_key, (uint8_t *)&target, sizeof(uint32_t), (uint8_t *)payload, strlen(payload) * sizeof(char));
    Index_Api_Close();

    assert(index_info_instance.index_file == NULL);

    test_end(case_name);
}

void test_index_insert_case1()
{
    char case_name[] = "test_index_insert_case1";
    test_start(case_name);

    char p_index_key[] = "test_index_insert_case1.index";
    uint32_t target = 1;
    char payload[INDEX_PAYLOAD_SIZE / sizeof(char)] = "aaaaa";

    // Operations
    Index_Api_Init(test_index_directory);
    Index_Api_Insert_Element(p_index_key, (uint8_t *)&target, sizeof(uint32_t), (uint8_t *)payload, strlen(payload) * sizeof(char));
    Index_Api_Close();

    // check
    INDEX_INFO_T *p_index_info;
    INDEX_PROPERTIES_T expected_index_properties;
    INDEX_NODE_T index_node, expected_index_node;

    // check index info
    p_index_info = load_index_info(p_index_key);
    assert(p_index_info->index_file != NULL);

    // check index properties
    read_index_properties(p_index_info);

    expected_index_properties.tag_num = 1;
    expected_index_properties.root_tag = 1;
    expected_index_properties.key_size = strlen(p_index_key);
    expected_index_properties.p_key = calloc(strlen(p_index_key) + 1, sizeof(char));
    memcpy(expected_index_properties.p_key, p_index_key, strlen(p_index_key) + 1);
    check_index_properties(&(p_index_info->index_properties), &expected_index_properties);

    // check index node
    read_index_node(p_index_info, 1, &index_node);
    index_node_init(&expected_index_node, 1);
    expected_index_node.level = 0;
    expected_index_node.length = 1;
    setup_index_element(&(expected_index_node.elements[0]), (uint8_t *)&target, sizeof(target), (uint8_t *)payload, strlen(payload) * sizeof(char));
    check_index_node(&index_node, &expected_index_node);

    Index_Api_Close();
    // End of the check

    test_end(case_name);
}

void test_index_insert_case11()
{
    char case_name[] = "test_index_insert_case11";
    test_start(case_name);

    char p_index_key[] = "test_index_insert_case11.index";
    uint32_t target[11] = {1, 4, 9, 10, 11, 12, 13, 15, 16, 20, 25};
    char payload[11][100];

    // Operation: 11 elements
    Index_Api_Init(test_index_directory);
    for (uint32_t i = 0; i < 11; i++)
    {
        payload[i][0] = (target[i] % 26) + 'a';
        payload[i][1] = '\0';
        Index_Api_Insert_Element(p_index_key, (uint8_t *)&(target[i]), sizeof(uint32_t), (uint8_t *)payload[i], strlen(payload[i]) * sizeof(char));
    }

    Index_Api_Close();

    // check
    INDEX_INFO_T *p_index_info;
    INDEX_PROPERTIES_T expected_index_properties;
    INDEX_NODE_T index_node, expected_index_node;

    // check index info
    p_index_info = load_index_info(p_index_key);
    assert(p_index_info->index_file != NULL);

    // check index properties
    read_index_properties(p_index_info);
    expected_index_properties.tag_num = 8;
    expected_index_properties.root_tag = 8;
    expected_index_properties.key_size = strlen(p_index_key);
    expected_index_properties.p_key = calloc(strlen(p_index_key) + 1, sizeof(char));
    memcpy(expected_index_properties.p_key, p_index_key, strlen(p_index_key) + 1);
    check_index_properties(&(p_index_info->index_properties), &expected_index_properties);

    // check index nodes
    // Index node tag 1 (leaf node)
    read_index_node(p_index_info, 1, &index_node);
    // print_index_node(&index_node);
    index_node_init(&expected_index_node, 1);
    expected_index_node.level = 0;
    expected_index_node.length = 2;
    expected_index_node.parent_tag = 3;
    expected_index_node.next_tag = 2;
    setup_index_element(&(expected_index_node.elements[0]), (uint8_t *)&(target[0]), sizeof(target[0]), (uint8_t *)payload[0], strlen(payload[0]) * sizeof(char));
    setup_index_element(&(expected_index_node.elements[1]), (uint8_t *)&(target[1]), sizeof(target[1]), (uint8_t *)payload[1], strlen(payload[1]) * sizeof(char));
    check_index_node(&index_node, &expected_index_node);

    // Index node tag 2 (leaf node)
    read_index_node(p_index_info, 2, &index_node);
    // print_index_node(&index_node);
    index_node_init(&expected_index_node, 2);
    expected_index_node.level = 0;
    expected_index_node.length = 2;
    expected_index_node.parent_tag = 3;
    expected_index_node.next_tag = 4;
    setup_index_element(&(expected_index_node.elements[0]), (uint8_t *)&(target[2]), sizeof(target[2]), (uint8_t *)payload[2], strlen(payload[2]) * sizeof(char));
    setup_index_element(&(expected_index_node.elements[1]), (uint8_t *)&(target[3]), sizeof(target[3]), (uint8_t *)payload[3], strlen(payload[3]) * sizeof(char));
    check_index_node(&index_node, &expected_index_node);

    // Index node tag 3 (non-leaf node)
    read_index_node(p_index_info, 3, &index_node);
    // print_index_node(&index_node);
    index_node_init(&expected_index_node, 3);
    expected_index_node.level = 1;
    expected_index_node.length = 2;
    expected_index_node.parent_tag = 8;
    expected_index_node.next_tag = 7;
    expected_index_node.child_tag[0] = 1;
    expected_index_node.child_tag[1] = 2;
    expected_index_node.child_tag[2] = 4;
    check_index_node(&index_node, &expected_index_node);

    // Index node tag 4 (leaf node)
    read_index_node(p_index_info, 4, &index_node);
    // print_index_node(&index_node);
    index_node_init(&expected_index_node, 4);
    expected_index_node.level = 0;
    expected_index_node.length = 2;
    expected_index_node.parent_tag = 3;
    expected_index_node.next_tag = 5;
    setup_index_element(&(expected_index_node.elements[0]), (uint8_t *)&(target[4]), sizeof(target[4]), (uint8_t *)payload[4], strlen(payload[4]) * sizeof(char));
    setup_index_element(&(expected_index_node.elements[1]), (uint8_t *)&(target[5]), sizeof(target[5]), (uint8_t *)payload[5], strlen(payload[5]) * sizeof(char));
    check_index_node(&index_node, &expected_index_node);

    // Index node tag 5 (leaf node)
    read_index_node(p_index_info, 5, &index_node);
    // print_index_node(&index_node);
    index_node_init(&expected_index_node, 5);
    expected_index_node.level = 0;
    expected_index_node.length = 2;
    expected_index_node.parent_tag = 7;
    expected_index_node.next_tag = 6;
    setup_index_element(&(expected_index_node.elements[0]), (uint8_t *)&(target[6]), sizeof(target[6]), (uint8_t *)payload[6], strlen(payload[6]) * sizeof(char));
    setup_index_element(&(expected_index_node.elements[1]), (uint8_t *)&(target[7]), sizeof(target[7]), (uint8_t *)payload[7], strlen(payload[7]) * sizeof(char));
    check_index_node(&index_node, &expected_index_node);

    Index_Api_Close();
    // End of the check

    test_end(case_name);
}

void test_index_search_case1()
{
    char case_name[] = "test_index_search_case1";
    test_start(case_name);

    char p_index_key[] = "test_index_search_case1.index";
    uint32_t target = 1;
    char payload[INDEX_PAYLOAD_SIZE / sizeof(char)] = "aaaaa";

    // Operations
    Index_Api_Init(test_index_directory);
    Index_Api_Insert_Element(p_index_key, (uint8_t *)&target, sizeof(uint32_t), (uint8_t *)payload, strlen(payload) * sizeof(char));
    Index_Api_Close();

    // check
    void *result = NULL;
    uint32_t result_length = 0;
    result = (void *)Index_Api_Search(p_index_key, (uint8_t *)&target, sizeof(target), &result_length);

    assert(result_length == 1);
    assert(memcmp(payload, result, INDEX_PAYLOAD_SIZE) == 0);
    free(result);
    // End of the check

    test_end(case_name);
}

void test_index_search_case11()
{
    char case_name[] = "test_index_search_case11";
    test_start(case_name);

    char p_index_key[] = "test_index_search_case11.index";
    uint32_t target[11] = {1, 9, 9, 10, 11, 12, 12, 12, 12, 20, 20};
    char payload[11][INDEX_PAYLOAD_SIZE];

    // Operation: 11 elements
    Index_Api_Init(test_index_directory);
    for (uint32_t i = 0; i < 11; i++)
    {
        memset(payload[i], 0, INDEX_PAYLOAD_SIZE);
        payload[i][0] = (i % 26) + 'a';
        Index_Api_Insert_Element(p_index_key, (uint8_t *)&(target[i]), sizeof(uint32_t), (uint8_t *)payload[i], strlen(payload[i]) * sizeof(char));
    }

    Index_Api_Close();

    // check
    void *result = NULL;
    uint32_t reuslt_length = 0;

    result = Index_Api_Search(p_index_key, (uint8_t *)&(target[0]), sizeof(target[0]), &reuslt_length);
    assert(reuslt_length == 1);
    assert(memcmp(result, payload[0], INDEX_PAYLOAD_SIZE) == 0);
    free(result);

    result = Index_Api_Search(p_index_key, (uint8_t *)&(target[1]), sizeof(target[1]), &reuslt_length);
    assert(reuslt_length == 2);
    assert(
        (memcmp(result, payload[1], INDEX_PAYLOAD_SIZE) == 0 && memcmp(result + INDEX_PAYLOAD_SIZE, payload[2], INDEX_PAYLOAD_SIZE) == 0) ||
        (memcmp(result, payload[2], INDEX_PAYLOAD_SIZE) == 0 && memcmp(result + INDEX_PAYLOAD_SIZE, payload[1], INDEX_PAYLOAD_SIZE) == 0)
    );
    // printf("%s\n", result + INDEX_PAYLOAD_SIZE * 0);
    // printf("%s\n", result + INDEX_PAYLOAD_SIZE * 1);
    free(result);

    result = Index_Api_Search(p_index_key, (uint8_t *)&(target[3]), sizeof(target[3]), &reuslt_length);
    assert(reuslt_length == 1);
    assert(memcmp(result, payload[3], INDEX_PAYLOAD_SIZE) == 0);
    free(result);

    result = Index_Api_Search(p_index_key, (uint8_t *)&(target[4]), sizeof(target[4]), &reuslt_length);
    assert(reuslt_length == 1);
    assert(memcmp(result, payload[4], INDEX_PAYLOAD_SIZE) == 0);
    free(result);

    result = Index_Api_Search(p_index_key, (uint8_t *)&(target[5]), sizeof(target[5]), &reuslt_length);
    assert(reuslt_length == 4);
    assert(memcmp(result + INDEX_PAYLOAD_SIZE * 0, payload[5], INDEX_PAYLOAD_SIZE) == 0);
    assert(memcmp(result + INDEX_PAYLOAD_SIZE * 1, payload[6], INDEX_PAYLOAD_SIZE) == 0);
    assert(memcmp(result + INDEX_PAYLOAD_SIZE * 2, payload[7], INDEX_PAYLOAD_SIZE) == 0);
    assert(memcmp(result + INDEX_PAYLOAD_SIZE * 3, payload[8], INDEX_PAYLOAD_SIZE) == 0);    
    free(result);

    result = Index_Api_Search(p_index_key, (uint8_t *)&(target[9]), sizeof(target[9]), &reuslt_length);
    assert(reuslt_length == 2);
    assert(memcmp(result + INDEX_PAYLOAD_SIZE * 0, payload[9], INDEX_PAYLOAD_SIZE) == 0);
    assert(memcmp(result + INDEX_PAYLOAD_SIZE * 1, payload[10], INDEX_PAYLOAD_SIZE) == 0);
    free(result);
    // End of the check

    test_end(case_name);
}

int main()
{
    // hash
    test_hash();
    // index
    test_index_init();
    test_index_close();
    test_index_insert_case1();
    test_index_insert_case11();
    test_index_search_case1();
    test_index_search_case11();
    return 0;
}