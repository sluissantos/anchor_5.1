#pragma once
#include "esp_system.h"
#include "esp_log.h"
#define BUTTON_TAG "BUTTON"
#define BUTTON_INFO(fmt, ...)   ESP_LOGI(BUTTON_TAG, fmt, ##__VA_ARGS__)
#define BUTTON_ERROR(fmt, ...)  ESP_LOGE(BUTTON_TAG, fmt, ##__VA_ARGS__)

void button_init(void);
void gpio_isr_handler(void* arg);
//void button_deinit(void);
