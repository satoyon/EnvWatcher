#pragma once

#include "tinykernel/kernel.hpp"

#define DHT11_SDA  16

namespace Dht11 {
    // DHT-11センサーのタスクを登録する
    tk::TaskHandle regist(uint32_t gp = DHT11_SDA);
};

