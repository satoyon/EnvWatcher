#pragma once

#include <variant>
#include <cstdint>
#include <stdio.h>
#include <array>
#include "pico/stdlib.h"
#include "tinykernel/kernel.hpp"
#include "tinykernel/actor.hpp"
#include "sensor_msg.hpp"

// センサーハブ
class SensorHub : public tk::Actor<EnvData, 8> {
protected:
    void on_message(const EnvData& msg) override {
        char buf[32];
        switch(msg.sensor_id) {
            case SensorId::InternalTemp:
                if(auto *d = std::get_if<InternalTempData>(&msg.value)) {
                    snprintf(buf, sizeof(buf), "SoC温度 %3.2f", d->temp);
                    // 2行目
                    tk::publish(LcdMsg::put_str(1,17,buf));
                }
            break;
            case SensorId::Dht11:
                if(auto *d = std::get_if<DhtData>(&msg.value)) {
                    snprintf(buf, sizeof(buf), "温度 %2d, 湿度 %2d", d->temp, d->hum);
                    // 1行目
                    tk::publish(LcdMsg::put_str(1,1, buf));
                }
            break;

            default:
            break;
        }
    }
};
