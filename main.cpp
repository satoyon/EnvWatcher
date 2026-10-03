#include <stdio.h>
#include "pico/stdlib.h"
#include "tinykernel/kernel.hpp"
#include "tinykernel/actor.hpp"
#include "tinykernel/sync.hpp"

#include "intl_sensor.hpp"
#include "SensorHub.hpp"
#include "dht11_sensor.hpp"
#include "lcd_actor.hpp"

int main()
{
    // SensorHub
    static SensorHub sensorhub;
    sensorhub.start();
    // 内部温度センサー
    InternalSensor::regist();
    // Dht11
    Dht11::regist();
    // lcd_actor
    static LcdActor lcd;
    lcd.start();
    // tkkernel起動
    tk::start();

    while(true);
}
