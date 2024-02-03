set(SOURCES Wifi.cpp)

idf_component_register(SRCS ${SOURCES} INCLUDE_DIRS "include" REQUIRES nvs_flash esp_netif esp_wifi lwip esp_event)