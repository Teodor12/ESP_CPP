set(SOURCES Wifi.cpp)

idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS .
                        REQUIRES
                        esp_wifi
                        esp_event
                        esp_netif
                        lwip
                        Wifi
                        )