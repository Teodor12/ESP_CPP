#pragma once

#include "esp_mac.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <cstring>


#define MAC_ADDR_BYTE_NUM 6
#define MAC_ADDR_CS_LEN 13

#define WIFI_LOG_TAG "WIFI"

namespace wifi
{
    class Wifi
    {

    constexpr static const char *ssid{"CUMYNET_REP1"};
    constexpr static const char *password{"123456789a"};

    public:
        enum class wifi_state_t
        {
            NOT_INITIALISED,
            INITIALISED,
            READY_TO_CONNECT,
            CONNECTING,
            WAITING_FOR_IP,
            CONNECTED,
            DISCONNECTED,
            ERROR
        };

        Wifi(void);                              // default constructor
        ~Wifi(void) = default;                   // destructor
        Wifi(const Wifi &) = default;            // copy constructor
        Wifi(Wifi &&) = default;                 // move constructor
        Wifi &operator=(const Wifi &) = default; // assignmet operator for rvalues
        Wifi &operator=(Wifi &&) = default;      // assignment operator for lvalues

        esp_err_t wifi_init(void);
        esp_err_t wifi_start(void);

        char *wifi_get_mac_address(void) const
        {
            return this->mac_address_cstring;
        }

        private:
            static char mac_address_cstring[MAC_ADDR_CS_LEN]; // this buffer stores the formatted, c-string mac-address
            static wifi_state_t wifi_state;
            static SemaphoreHandle_t init_mutex;
            static SemaphoreHandle_t connect_mutex;
            static SemaphoreHandle_t state_mutex;
            esp_err_t _get_mac_address(void);
            static esp_err_t _init(void);
            static esp_err_t begin(void);
            static wifi_init_config_t wifi_init_config;
            static wifi_config_t wifi_config;
            static const char* wifi_state_to_name(wifi_state_t wifi_state);
            static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
            static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
            static void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    };
};