// Compile-check only: FreeRTOS declarations that Arduino.h provides on the ESP32.
#pragma once
#include <stdint.h>
typedef void* SemaphoreHandle_t;
typedef void* TaskHandle_t;
typedef int BaseType_t;
#define pdTRUE 1
#define pdFALSE 0
#define pdPASS 1
#define portMAX_DELAY 0xFFFFFFFFu
#define pdMS_TO_TICKS(ms) (ms)
inline SemaphoreHandle_t xSemaphoreCreateMutex() { return nullptr; }
inline BaseType_t xSemaphoreTake(SemaphoreHandle_t, uint32_t) { return pdTRUE; }
inline BaseType_t xSemaphoreGive(SemaphoreHandle_t) { return pdTRUE; }
inline BaseType_t xTaskCreate(void (*)(void*), const char*, uint32_t, void*, unsigned, TaskHandle_t*) { return pdPASS; }
inline void vTaskDelay(uint32_t) {}
inline void vTaskDelete(TaskHandle_t) {}
typedef int portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(m) ((void)(m))
#define portEXIT_CRITICAL(m) ((void)(m))
