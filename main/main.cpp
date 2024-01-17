#include <stdio.h>
#include <esp_log.h>
#include "main.h"

#define LOG_TAG "MAIN"

static Main main_entry;

esp_err_t Main::setup(void)
{
    return ESP_OK;
}

void Main::loop()
{
    while (1)
    {
        ESP_LOGI(LOG_TAG, "%s\n", "Hello from ESP-IDF CPP!");
    }
}

extern "C" void app_main(void)
{
    ESP_ERROR_CHECK(main_entry.setup());
    main_entry.loop();

}
