#include <main.hpp>

#define LOG_TAG "MAIN"

static Main main_entry;

esp_err_t Main::setup(void)
{
    esp_err_t ret{ESP_OK};

    ret = wifi_handle.wifi_init();
    if(ret == ESP_OK)
    {
        ret = wifi_handle.wifi_start();
    }

    return ret;
}

void Main::loop()
{
    while (1)
    {
        vTaskDelay(PDSECOND);
    }
}

extern "C" void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(main_entry.setup());
    main_entry.loop();

}
