#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "tinykernel/kernel.hpp"
#include "tinykernel/actor.hpp"
#include "sensor_msg.hpp"
#include "intl_sensor.hpp"

#define INTL_TEMP_SENSOR 4

namespace InternalSensor {

static void intl_task(void *arg) {

    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(INTL_TEMP_SENSOR);

    while(true) {
        const float conversion_factor = 3.3f / (1 << 12);
        EnvData msg;
        uint16_t result = adc_read();

        msg.sensor_id = SensorId::InternalTemp;
        msg.value = InternalTempData {
            (27.0f - (result*conversion_factor - 0.706f)/ 0.001721f),
        };
        tk::publish(msg);
        tk::sleep_ms(1000);
    }
}

// 内部センサータスクを登録する
tk::TaskHandle regist(void) {
    return tk::create(intl_task, nullptr, tk::PRIO_NORMAL);
}


} // namespace InternalSensor
