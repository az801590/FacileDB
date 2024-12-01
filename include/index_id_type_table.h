// Format: INDEX_ID_TYPE_CONFIG(type_name, sizeof type, compare function)

INDEX_ID_TYPE_CONFIG(INDEX_ID_TYPE_HASH, 4, Index_Id_Type_Api_Hash_Compare)
INDEX_ID_TYPE_CONFIG(INDEX_ID_TYPE_UINT32, sizeof(uint32_t), Index_Id_Type_Api_Uint32_Compare)