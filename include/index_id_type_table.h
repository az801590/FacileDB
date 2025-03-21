/*
Format: INDEX_ID_TYPE_CONFIG(type_name, sizeof_type, compare_function)

type_name: enum name of index id type
sizeof_type: size of the index id, bytes.
compare_function: function name too compare index id. NULL is unavailable.
*/

INDEX_ID_TYPE_CONFIG(INDEX_ID_TYPE_HASH, 4, Index_Id_Type_Api_Hash_Compare)
INDEX_ID_TYPE_CONFIG(INDEX_ID_TYPE_UINT32, 4, Index_Id_Type_Api_Uint32_Compare)