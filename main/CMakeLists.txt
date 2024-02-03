set(SOURCES main.cpp)

idf_component_register(SRCS ${SOURCES} INCLUDE_DIRS "include" REQUIRES freertos esp_common nvs_flash esp_common esp_event nvs_flash Wifi)

