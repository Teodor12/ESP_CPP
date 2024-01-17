#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

#define PDSECOND pdMS_TO_TICKS(1000)

class Main final{
    public:
        esp_err_t setup(void);
        void loop(void);
};