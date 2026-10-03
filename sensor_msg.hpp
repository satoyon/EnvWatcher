#pragma once
#include <variant>
#include <cstdint>
#include <stdio.h>
#include <array>
#include "pico/stdlib.h"
#include "tinykernel/kernel.hpp"
#include "tinykernel/actor.hpp"
#include "lcd_actor.hpp"

enum class SensorId : uint8_t {
    InternalTemp = 0x01,
    Dht11        = 0x02,
};

// DHT-11
struct DhtData   {  uint16_t temp; uint16_t hum; };
// オンチップ温度センサー
struct InternalTempData { float temp; };

// センサーの型のバリエーションを定義
using EnvPayload = std::variant<
    int32_t,
    float,
    DhtData,
    InternalTempData,
    std::array<uint8_t, 8>
>;

// 環境センサーメッセージ
struct EnvData {
    SensorId sensor_id;
    EnvPayload value;
};

