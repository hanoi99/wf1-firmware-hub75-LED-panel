#pragma once
#include <stdint.h>
typedef struct { char label[17]; uint32_t address; uint32_t size; } esp_partition_t;
enum { ESP_PARTITION_TYPE_DATA = 1, ESP_PARTITION_SUBTYPE_DATA_SPIFFS = 0x82 };
inline const esp_partition_t* esp_partition_find_first(int, int, const char*) { return nullptr; }
