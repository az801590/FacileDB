#include <stdio.h>
#include <string.h>
#include <assert.h>

#define __INDEX_TEST__  (1)
#define INDEX_ORDER (3)
#define INDEX_PAYLOAD_SIZE (0)

#include "hash.c"
#include "index.c"

void test_start(char *case_name);
void test_end(char *case_name);
void print_index_node(INDEX_NODE_T *p_index_node);

void test_start(char *case_name)
{
    printf("Test Case: %s START!\n", case_name);
}

void test_end(char *case_name)
{
    printf("Test Case: %s END!\n\n", case_name);
}

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

    char p_index_directory_path[] = "./bin/";
    Index_Api_Init(p_index_directory_path);

    assert(strcmp(index_directory_path, p_index_directory_path) == 0);

    test_end(case_name);
}


void test_index_insert_element_case1()
{
    char case_name[] = "test_index_insert_element_case1";
    test_start(case_name);

    char p_index_directory_path[] = "./bin/";
    char p_index_key[] = "test_index.index";

    Index_Api_Init(p_index_directory_path);

    uint32_t target = 1;
    char payload[100] = "aaaaa";
    Index_Api_Insert_Element(p_index_key, (uint8_t *)&target, sizeof(uint32_t), (uint8_t *) payload, strlen(payload) * sizeof(char));

    target = 4;
    strcpy(payload, "bbbbb");
    Index_Api_Insert_Element(p_index_key, (uint8_t *)&target, sizeof(uint32_t), (uint8_t *) payload, strlen(payload) * sizeof(char));

    target = 9;
    strcpy(payload, "bbbbb");
    Index_Api_Insert_Element(p_index_key, (uint8_t *)&target, sizeof(uint32_t), (uint8_t *) payload, strlen(payload) * sizeof(char));

    target = 10;
    strcpy(payload, "bbbbb");
    Index_Api_Insert_Element(p_index_key, (uint8_t *)&target, sizeof(uint32_t), (uint8_t *) payload, strlen(payload) * sizeof(char));

    target = 11;
    strcpy(payload, "bbbbb");
    Index_Api_Insert_Element(p_index_key, (uint8_t *)&target, sizeof(uint32_t), (uint8_t *) payload, strlen(payload) * sizeof(char));

    target = 12;
    strcpy(payload, "bbbbb");
    Index_Api_Insert_Element(p_index_key, (uint8_t *)&target, sizeof(uint32_t), (uint8_t *) payload, strlen(payload) * sizeof(char));

    target = 13;
    strcpy(payload, "bbbbb");
    Index_Api_Insert_Element(p_index_key, (uint8_t *)&target, sizeof(uint32_t), (uint8_t *) payload, strlen(payload) * sizeof(char));

    target = 15;
    strcpy(payload, "bbbbb");
    Index_Api_Insert_Element(p_index_key, (uint8_t *)&target, sizeof(uint32_t), (uint8_t *) payload, strlen(payload) * sizeof(char));

    target = 16;
    strcpy(payload, "bbbbb");
    Index_Api_Insert_Element(p_index_key, (uint8_t *)&target, sizeof(uint32_t), (uint8_t *) payload, strlen(payload) * sizeof(char));

    target = 20;
    strcpy(payload, "bbbbb");
    Index_Api_Insert_Element(p_index_key, (uint8_t *)&target, sizeof(uint32_t), (uint8_t *) payload, strlen(payload) * sizeof(char));

    target = 25;
    strcpy(payload, "bbbbb");
    Index_Api_Insert_Element(p_index_key, (uint8_t *)&target, sizeof(uint32_t), (uint8_t *) payload, strlen(payload) * sizeof(char));

    INDEX_NODE_T index_node;
    for (int i = 1; i <= 8; i++)
    {
        read_index_node(&index_info_instance, i, &index_node);
        print_index_node(&index_node);
    }

    Index_Api_Close();

    test_end(case_name);
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

int main()
{
    // hash
    test_hash();
    // index
    test_index_init();
    test_index_insert_element_case1();
    return 0;
}