#ifndef __INDEX_H__
#define __INDEX_H__

#include <stdlib.h>
#include <stdio.h>
#include "hash.h"

#ifndef __INDEX_TEST__
#define __INDEX_TEST__ (0)
#endif


#ifndef INDEX_ORDER
#define INDEX_ORDER (5)
#endif

#ifndef INDEX_PAYLOAD_SIZE
// ((INDEX_PAYLOAD_SIZE + sizeof(HASH_VALUE_T)) * INDEX_ORDER) should be divisible by 4.
#define INDEX_PAYLOAD_SIZE (16)
#endif

#ifndef INDEX_FILE_PATH_BUFFER_LENGTH
#define INDEX_FILE_PATH_BUFFER_LENGTH (128)
#endif

#define INDEX_CHILD_TAG_ORDER (INDEX_ORDER + 1)
#define INDEX_FILE_PATH_MAX_LENGTH (INDEX_FILE_PATH_BUFFER_LENGTH - 1)


void Index_Api_Init(char *p_index_directory_path);
void Index_Api_Insert_Element(char *p_index_key, uint8_t *p_target, uint32_t target_size, uint8_t *p_index_payload, uint32_t payload_size);
uint8_t *Index_Api_Search(char *p_index_key, uint8_t *p_target, uint32_t target_size, uint32_t *result_length);
void Index_Api_Close();

#endif // __INDEX_H__