#ifndef __INDEX_ID_TYPE_COMPARE_H__
#define __INDEX_ID_TYPE_COMPARE_H__

#include <stdint.h>

typedef enum
{
    INDEX_ID_COMPARE_RIGHT_GREATER = -1,
    INDEX_ID_COMPARE_EQUAL = 0,
    INDEX_ID_COMPARE_LEFT_GREATER = 1
} INDEX_ID_COMPARE_RESULT_E;

INDEX_ID_COMPARE_RESULT_E Index_Id_Type_Api_Hash_Compare(void *value1, void *value2);
INDEX_ID_COMPARE_RESULT_E Index_Id_Type_Api_Uint32_Compare(void *value1, void *value2);


#endif