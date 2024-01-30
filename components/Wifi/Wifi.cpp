#include "include/Wifi.h"



namespace wifi
{
    std::mutex Wifi::init_mutex{};
    char Wifi::mac_address_cstring[MAC_ADDR_CS_LEN] = {};
    Wifi::wifi_state_t Wifi::wifi_state{wifi_state_t::NOT_INITIALISED};
    wifi_init_config_t Wifi::wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    wifi_config_t Wifi::wifi_config{};

    Wifi::Wifi(void)
    {
        std::lock_guard<std::mutex> guard(init_mutex);

        if(wifi_get_mac_address()[0] == '\0')
        {
            if(_get_mac_address() != ESP_OK)
            {
                esp_restart();
            }

        }
    }


    esp_err_t Wifi::wifi_init(void)
    {
        return _init();
    }

    esp_err_t Wifi::_init(void)
    {
        std::lock_guard<std::mutex> init_guard(init_mutex); // create a mutex, and lock (take) it
        esp_err_t ret{ESP_OK};

        if (Wifi::wifi_state != wifi_state_t::NOT_INITIALISED)
        {
            return ESP_OK;
        }
        else if (wifi_state == wifi_state_t::ERROR)
        {
            return ESP_FAIL;
        }

        ret = esp_netif_init();
        if (ret != ESP_OK)
        {
            return ret;
        }

        esp_netif_t *wifi_station = esp_netif_create_default_wifi_sta();
        if (wifi_station == NULL)
        {
            return ESP_FAIL;
        }

        ret = esp_wifi_init(&wifi_init_config);
        if (ret != ESP_OK)
        {
            return ret;
        }

        ret = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, nullptr, nullptr);
        if (ret != ESP_OK)
        {
            return ret;
        }

        ret = esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, ip_event_handler, nullptr, nullptr);
        if (ret != ESP_OK)
        {
            return ret;
        }

        ret = esp_wifi_set_mode(WIFI_MODE_STA);
        if (ret != ESP_OK)
        {
            return ret;
        }

        memcpy(wifi_config.sta.ssid, ssid, strlen(ssid));
        memcpy(wifi_config.sta.password, password, strlen(password));
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
        wifi_config.sta.pmf_cfg.capable = true;
        wifi_config.sta.pmf_cfg.required = false;

        ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
        if (ret != ESP_OK)
            return ret;

        ret = esp_wifi_start();
        if (ret != ESP_OK)
            return ret;

        wifi_state = wifi_state_t::INITIALISED;
        return ESP_OK;
    }



    void Wifi::event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
    {
        if(event_base == WIFI_EVENT)
        {
            return wifi_event_handler(arg, event_base, event_id, event_data);
        }
        else if (event_base == IP_EVENT)
        {
            return ip_event_handler(arg, event_base, event_id, event_data);
        }
        else
        {
            ESP_LOGE("myWIFI", "Unexpected event: %s", event_base); // TODO logging
        }
    }

    void Wifi::wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
    {
        wifi_event_t event_type = static_cast<wifi_event_t>(event_id); /* compile time int32_t -> enum conversion */
        switch (event_type)
        {
        case WIFI_EVENT_STA_START:
        {
            std::lock_guard<std::mutex> state_guard(state_mutex);
            wifi_state = wifi_state_t::READY_TO_CONNECT;
            break;
        }
        case WIFI_EVENT_STA_CONNECTED:
        {
            std::lock_guard<std::mutex> state_guard(state_mutex);
            wifi_state = wifi_state_t::WAITING_FOR_IP;
            break;
        }
        case WIFI_EVENT_STA_DISCONNECTED:
        {
            std::lock_guard<std::mutex> state_guard(state_mutex);
            wifi_state = wifi_state_t::DISCONNECTED;
            break;
        }
        default:
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
                std::lock_guard<std::mutex> guard(state_mutex);
                wifi_state = wifi_state_t::CONNECTED;
                break;
            }
            case IP_EVENT_STA_LOST_IP:
            {
                std::lock_guard<std::mutex> guard(state_mutex);
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

        if(status == ESP_OK)
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
