#pragma once

#include "esp_wifi.h"
#include "esp_mac.h"
#include <algorithm>
#include <cstring>
#include <atomic>

#define MAC_ADDR_BYTE_NUM 6
#define MAC_ADDR_CS_LEN 13

namespace wifi
{
    class Wifi
    {

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

            Wifi(void);                                     // default constructor
            ~Wifi(void) =                   default;        // destructor
            Wifi(const Wifi &) =            default;        // copy constructor
            Wifi(Wifi &&) =                 default;        // move constructor
            Wifi &operator=(const Wifi &) = default;        // assignmet operator for rvalues
            Wifi &operator=(Wifi &&) =      default;        // assignment operator for lvalues

            esp_err_t init(void);
            esp_err_t begin(void);

            char* wifi_get_mac_address(void)const
            {
                return this->cstring_mac_address;
            }

        private:
            static char cstring_mac_address[MAC_ADDR_CS_LEN]; // this buffer stores the formatted, c-string mac-address
            esp_err_t _get_mac_address(void);                 // function to load the hard-coded EFUSE mac-address to the 'cstring_mac_address' member variable
            static std::atomic_bool first_call;
            void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) ;
            void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
            void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
    };
};