#include "include/Wifi.hpp"

namespace wifi
{

    std::mutex Wifi::init_mutex{};
    std::mutex Wifi::state_mutex{};
    std::mutex Wifi::connect_mutex{};

    Wifi::wifi_state_t Wifi::wifi_state{wifi_state_t::NOT_INITIALISED};
    wifi_init_config_t Wifi::wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    wifi_config_t Wifi::wifi_config{};

    char Wifi::mac_address_cstring[MAC_ADDR_CS_LEN] = {};

    Wifi::Wifi(void)
    {
        std::lock_guard<std::mutex> guard(init_mutex);

        if (wifi_get_mac_address()[0] == '\0')
        {
            if (_get_mac_address() != ESP_OK)
            {
                esp_restart();
            }
        }
    }

    esp_err_t Wifi::wifi_start(void)
    {
        ESP_LOGI(WIFI_LOG_TAG, "%s:%d Waiting for connect_mutex", __func__, __LINE__);
        std::lock_guard<std::mutex> connect_guard(connect_mutex);

        esp_err_t ret{ESP_OK};

        ESP_LOGI(WIFI_LOG_TAG, "%s:%d Waiting for state_mutex", __func__, __LINE__);
        std::lock_guard<std::mutex> state_guard(state_mutex);
        switch (wifi_state)
        {
        case wifi_state_t::READY_TO_CONNECT:
            ESP_LOGI(WIFI_LOG_TAG, "%s:%d Calling esp_wifi_connect", __func__, __LINE__);
            ret = esp_wifi_connect();
            ESP_LOGI(WIFI_LOG_TAG, "%s:%d esp_wifi_connect:%s", __func__, __LINE__, esp_err_to_name(ret));
            if (ESP_OK == ret)
            {
                wifi_state = wifi_state_t::CONNECTING;
            }
            break;
        case wifi_state_t::CONNECTING:
        case wifi_state_t::WAITING_FOR_IP:
        case wifi_state_t::CONNECTED:
            break;
        case wifi_state_t::INITIALISED:
        case wifi_state_t::NOT_INITIALISED:
        case wifi_state_t::DISCONNECTED:
        case wifi_state_t::ERROR:
            ESP_LOGE(WIFI_LOG_TAG, "%s:%d Error state", __func__, __LINE__);
            ret = ESP_FAIL;
            break;
        }
        return ret;
    }

    esp_err_t Wifi::wifi_init(void)
    {
        return _init();
    }

    esp_err_t Wifi::_init(void)
    {
        ESP_LOGI(WIFI_LOG_TAG, "%s:%d Waiting for init_mutex", __func__, __LINE__);
        std::lock_guard<std::mutex> init_guard(init_mutex);

        esp_err_t ret{ESP_OK};

        ESP_LOGI(WIFI_LOG_TAG, "%s:%d Waiting for state_mutex", __func__, __LINE__);
        std::lock_guard<std::mutex> state_guard(state_mutex);

        if (wifi_state == wifi_state_t::NOT_INITIALISED) /* start the initialization of the wifi driver hasn't been initialized yet*/
        {
            ESP_LOGI(WIFI_LOG_TAG, "%s:%d Calling esp_netif_init", __func__, __LINE__);
            ret = esp_netif_init(); /* initialize the TCP/IP stack */
            ESP_LOGI(WIFI_LOG_TAG, "%s:%d esp_netif_init:%s", __func__, __LINE__, esp_err_to_name(ret));

            if (ret == ESP_FAIL)
            {
                ESP_LOGE(WIFI_LOG_TAG, "esp_netif_init returned:%s", esp_err_to_name(ret));
                return ret;
            }

            ESP_LOGI(WIFI_LOG_TAG, "%s:%d Calling esp_event_loop_create_default", __func__, __LINE__);
            ret = esp_event_loop_create_default(); /* create a default System Event loop */
            ESP_LOGI(WIFI_LOG_TAG, "%s:%d esp_event_loop_create_default:%s", __func__, __LINE__, esp_err_to_name(ret));

            if (ret == ESP_FAIL)
            {
                return ret;
            }

            ESP_LOGI(WIFI_LOG_TAG, "%s:%d Calling esp_netif_create_default_wifi_sta", __func__, __LINE__);
            const esp_netif_t *const p_netif = esp_netif_create_default_wifi_sta(); /* create default network interface instance binding station with TCP/IP stack */
            ESP_LOGI(WIFI_LOG_TAG, "%s:%d esp_netif_create_default_wifi_sta:%s", __func__, __LINE__, (p_netif != NULL) ? "Success" : "Fail");

            if (p_netif == NULL)
            {
                ret = ESP_FAIL;
                return ret;
            }

            ESP_LOGI(WIFI_LOG_TAG, "%s:%d Calling esp_wifi_init", __func__, __LINE__);
            ret = esp_wifi_init(&wifi_init_config);
            ESP_LOGI(WIFI_LOG_TAG, "%s:%d esp_wifi_init:%s", __func__, __LINE__, esp_err_to_name(ret));

            if (ret == ESP_FAIL)
            {
                return ret;
            }

            /**
             * 2. "Wi-Fi Configuration Phase"
             */

            ESP_LOGI(WIFI_LOG_TAG, "%s:%d Calling esp_wifi_init", __func__, __LINE__);
            ret = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
            ESP_LOGI(WIFI_LOG_TAG, "%s:%d esp_event_handler_instance_register WIFI_EVENT:%s", __func__, __LINE__, esp_err_to_name(ret));

            if (ret == ESP_FAIL)
            {
                return ret;
            }

            ESP_LOGI(WIFI_LOG_TAG, "%s:%d Calling esp_event_handler_instance_register", __func__, __LINE__);
            ret = esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL, NULL);
            ESP_LOGI(WIFI_LOG_TAG, "%s:%d esp_event_handler_instance_register IP_EVENT:%s", __func__, __LINE__, esp_err_to_name(ret));

            if (ret == ESP_FAIL)
            {
                return ret;
            }

            ESP_LOGI(WIFI_LOG_TAG, "%s:%d Calling esp_wifi_set_mode", __func__, __LINE__);
            ret = esp_wifi_set_mode(WIFI_MODE_STA);
            ESP_LOGI(WIFI_LOG_TAG, "%s:%d esp_wifi_set_mode:%s", __func__, __LINE__, esp_err_to_name(ret));

            if (ret == ESP_FAIL)
            {
                return ret;
            }

            memcpy(wifi_config.sta.ssid, ssid, strlen(ssid));
            memcpy(wifi_config.sta.password, password, strlen(password));
            wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
            wifi_config.sta.pmf_cfg.capable = true;
            wifi_config.sta.pmf_cfg.required = false;

            ESP_LOGI(WIFI_LOG_TAG, "%s:%d Calling esp_wifi_set_config", __func__, __LINE__);
            ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
            ESP_LOGI(WIFI_LOG_TAG, "%s:%d esp_wifi_set_config:%s", __func__, __LINE__, esp_err_to_name(ret));

            if (ret == ESP_FAIL)
            {
                return ret;
            }

            /**
             * 3. "Wi-Fi Start Phase"
             */
            ESP_LOGI(WIFI_LOG_TAG, "%s:%d Calling esp_wifi_start", __func__, __LINE__);
            ret = esp_wifi_start(); /* start the Wi-Fi driver, WIFI_STA_START event will arise */
            ESP_LOGI(WIFI_LOG_TAG, "%s:%d esp_wifi_start:%s", __func__, __LINE__, esp_err_to_name(ret));

            if (ret == ESP_FAIL)
            {
                return ret;
            }
            wifi_state = wifi_state_t::INITIALISED;
        }
        else if (wifi_state == wifi_state_t::ERROR)
        {
            ret = ESP_FAIL;
        }
        return ret;
    }

    void Wifi::event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
    {
        if (event_base == WIFI_EVENT)
        {
            ESP_LOGI(WIFI_LOG_TAG, "%s:%d Got a WIFI_EVENT", __func__, __LINE__);
            return wifi_event_handler(arg, event_base, event_id, event_data);
        }
        else if (event_base == IP_EVENT)
        {
            ESP_LOGI(WIFI_LOG_TAG, "%s:%d Got an IP_EVENT", __func__, __LINE__);
            return ip_event_handler(arg, event_base, event_id, event_data);
        }
        else
        {
            ESP_LOGE(WIFI_LOG_TAG, "%s:%d Unexpected event: %s", __func__, __LINE__, event_base);
        }
    }

    void Wifi::wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
    {
        wifi_event_t event_type = static_cast<wifi_event_t>(event_id); /* compile time int32_t -> enum conversion */
        ESP_LOGI(WIFI_LOG_TAG, "%s:%d Event ID %ld", __func__, __LINE__, event_id);

        switch (event_type)
        {
        case WIFI_EVENT_STA_START:
        {
            ESP_LOGI(WIFI_LOG_TAG, "%s:%d WIFI_EVENT_STA_START, waiting for state_mutx", __func__, __LINE__);
            std::lock_guard<std::mutex> state_guard(state_mutex);
            ESP_LOGI(WIFI_LOG_TAG, "%s:%d READY_TO_CONNECT", __func__, __LINE__);
            wifi_state = wifi_state_t::READY_TO_CONNECT;
            break;
        }
        case WIFI_EVENT_STA_CONNECTED:
        {
            ESP_LOGI(WIFI_LOG_TAG, "%s:%d WIFI_EVENT_STA_CONNECTED, waiting for state_mutx", __func__, __LINE__);
            std::lock_guard<std::mutex> state_guard(state_mutex);
            ESP_LOGI(WIFI_LOG_TAG, "%s:%d WAITING_FOR_IP", __func__, __LINE__);
            wifi_state = wifi_state_t::WAITING_FOR_IP;
            break;
        }
        case WIFI_EVENT_STA_DISCONNECTED:
        {
            ESP_LOGI(WIFI_LOG_TAG, "%s:%d WIFI_EVENT_STA_DISCONNECTED, waiting for state_mutx", __func__, __LINE__);
            std::lock_guard<std::mutex> state_guard(state_mutex);
            wifi_state = wifi_state_t::DISCONNECTED;
            break;
        }
        default:
            ESP_LOGW(WIFI_LOG_TAG, "%s:%d Default switch case (%ld)", __func__, __LINE__, event_id);
            break;
        }
    }

    void Wifi::ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
    {
        wifi_event_t event_type = static_cast<wifi_event_t>(event_id); /* compile time int32_t -> enum conversion */
        switch (event_type)
        {
        case IP_EVENT_STA_GOT_IP:
        {
            ESP_LOGI(WIFI_LOG_TAG, "%s:%d IP_EVENT_STA_GOT_IP, waiting for state_mutx", __func__, __LINE__);
            std::lock_guard<std::mutex> guard(state_mutex);
            ESP_LOGI(WIFI_LOG_TAG, "%s:%d The device connected to the access point!", __func__, __LINE__);
            wifi_state = wifi_state_t::CONNECTED;
            break;
        }
        case IP_EVENT_STA_LOST_IP:
        {
            ESP_LOGI(WIFI_LOG_TAG, "%s:%d IP_EVENT_STA_LOST_IP, waiting for state_mutx", __func__, __LINE__);
            std::lock_guard<std::mutex> guard(state_mutex);
            ESP_LOGI(WIFI_LOG_TAG, "%s:%d The device hast lost it's IP address!", __func__, __LINE__);
            wifi_state = wifi_state_t::WAITING_FOR_IP;
            break;
        }
        default:
            break;
        }
    }

    esp_err_t Wifi::_get_mac_address(void)
    {
        uint8_t mac_address_bytes[6] = {};
        esp_err_t status = esp_efuse_mac_get_default(mac_address_bytes);

        if (status == ESP_OK)
        {
            snprintf(
                mac_address_cstring,
                sizeof(mac_address_cstring),
                "%02X%02X%02X%02X%02X%02X",
                mac_address_bytes[0],
                mac_address_bytes[1],
                mac_address_bytes[2],
                mac_address_bytes[3],
                mac_address_bytes[4],
                mac_address_bytes[5]);
        }
        return status;
    }

} // namespace wifi
