#include "include/Wifi.h"

namespace wifi
{
    std::atomic_bool Wifi::first_call = false;

    char Wifi::cstring_mac_address[] = {};


    Wifi::Wifi(void)
    {
        if(!first_call)
        {
            _get_mac_address();
            first_call = true;
        }
    }


    esp_err_t Wifi::_get_mac_address(void)
    {
        uint8_t mac_address_bytes[6] = {};
        esp_err_t status = esp_efuse_mac_get_default(mac_address_bytes);

        if(status == ESP_OK)
        {
            snprintf(
                cstring_mac_address,
                sizeof(cstring_mac_address),
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
