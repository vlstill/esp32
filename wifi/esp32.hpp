#pragma once
#include "nvs_flash.h"
#include "esp_log.h"

void esp_main();

void init() {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

extern "C" void app_main()
{
    init();
    ESP_LOGI("esp32.hpp", "going to start main");
    esp_main();
}
