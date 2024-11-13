// Format: INDEX_ID_TYPE_CONFIG(type_name, sizeof type, compare function)

INDEX_ID_TYPE_CONFIG(INDEX_ID_TYPE_HASH, sizeof(HASH_VALUE_T), index_id_type_hash_compare)
INDEX_ID_TYPE_CONFIG(INDEX_ID_TYPE_UINT32, sizeof(uint32_t), index_id_type_uint32_compare)