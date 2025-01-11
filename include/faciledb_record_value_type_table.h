// Format: FACILEDB_RECORD_VALUE_TYPE_CONFIG(type name, sizeof(type) in bytes, compare function)

FACILEDB_RECORD_VALUE_TYPE_CONFIG(FACILEDB_RECORD_VALUE_TYPE_HASH, 4, FACILEDB_Record_Value_Type_Api_Hash_Compare)
FACILEDB_RECORD_VALUE_TYPE_CONFIG(FACILEDB_RECORD_VALUE_TYPE_UINT32, 4, FACILEDB_Record_Value_Type_Api_Uint32_Compare)
FACILEDB_RECORD_VALUE_TYPE_CONFIG(FACILEDB_RECORD_VALUE_TYPE_OBJECT, FACILEDB_RECORD_VALUE_TYPE_DYNAMIC_SIZE, FACILEDB_Record_Value_Type_Api_Object_Compare)