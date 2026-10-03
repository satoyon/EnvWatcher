#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "tinykernel/actor.hpp"
#include "tinykernel/kernel.hpp"
#include "tinykernel/sync.hpp"
#include "sensor_msg.hpp"
#include "dht11_sensor.hpp"

// pioasm
#include "dht11.pio.h"

#define PIO_SM  0
#define DHT11_START_PULSE_US (20 * 1000)        // スタートパルス幅(20ms)
#define DHT11_TRANS_SIZE    5                   // DMAデータ数

namespace Dht11 {   // namespcace Dht11

static uint32_t g_dht11_sda = DHT11_SDA;


static uint8_t dht11_buff[DHT11_TRANS_SIZE];    // DMA転送バッファ

static int  dma_ch;                 // DMAチャンネル
static bool dht11_error = false;    // エラーフラグ
static tk::BinarySemaphore dma_sem(0, 1); // DMA完了待ち用のセマフォ（初期値 0）

dma_channel_config dmac_config;

volatile static uint temperature = 0;   // 現在温度
volatile static uint humidity = 0;      // 現在湿度

static void dma_irq_handler(void)
{
    // 割り込みフラグクリア
    dma_channel_acknowledge_irq0(dma_ch);
    
    uint8_t checksum = (dht11_buff[0]+dht11_buff[1]+dht11_buff[2]+dht11_buff[3]) & 0xFF;
    if( checksum == dht11_buff[4]) {
        // データ正常
        dht11_error = false;
        // DHT-11の小数部は信用できないので切り捨て
        temperature = dht11_buff[2];
        humidity = dht11_buff[0];
    }
    else dht11_error = true;    // エラー
    // セマフォリリース
    dma_sem.release();
}

static void dht11_task(void *arg) {
    // 未使用のDMAチャンネルを得る
    dma_ch = dma_claim_unused_channel(true);
    // DMA設定情報を取得
    dmac_config = dma_channel_get_default_config(dma_ch);
    // 転送データサイズの設定
    channel_config_set_transfer_data_size(&dmac_config, DMA_SIZE_8);
    // 読み出し側インクリメントの可否
    channel_config_set_read_increment(&dmac_config, false);
    // 書き込み側インクリメントの可否
    channel_config_set_write_increment(&dmac_config, true);
    // DREQの設定
    uint dreq = pio_get_dreq(pio0, PIO_SM, false);
    channel_config_set_dreq(&dmac_config, dreq);
    // DMAチャンネルの設定
    dma_channel_configure(
        dma_ch,        // DMAチャンネル
        &dmac_config,  // 設定情報
        dht11_buff,    // 書き込みアドレス
        &pio0_hw->rxf[PIO_SM],  // 転送元レジスタアドレス
        DHT11_TRANS_SIZE, // 転送データサイズ
        false           // すぐに開始ならtrue
    );
    // DMA割り込みを設定
    // DMAチャンネルをDMA_IRQ_0にルーティング
    dma_channel_set_irq0_enabled(dma_ch, true);
    // 割り込みハンドラとして静的関数を登録
    irq_set_exclusive_handler(DMA_IRQ_0, dma_irq_handler);
    // DMA_IRQ_0を有効化
    irq_set_enabled(DMA_IRQ_0, true);    
    // DHT-11 PIOの起動
    uint offset = pio_add_program(pio0, &dht11_program);
    dht11_program_init(pio0, PIO_SM, offset, g_dht11_sda);
    pio_sm_set_enabled(pio0, PIO_SM, true);

    while(true) {
        // 念のためRX FIFOをクリアしておく
        pio_sm_clear_fifos(pio0, PIO_SM);
        // DMA転送をトリガー
        dma_channel_set_write_addr(dma_ch, dht11_buff, false);
        dma_channel_set_trans_count(dma_ch, DHT11_TRANS_SIZE, true);    
        // スタートパルス幅をプッシュしてPIOをスタート
        pio_sm_put(pio0, 0, DHT11_START_PULSE_US);
        // DMA転送完了待ち
        dma_sem.acquire();
        if( !dht11_error ) { // エラーがない
            EnvData msg;
            msg.sensor_id = SensorId::Dht11;
            msg.value = DhtData {
                (uint16_t)temperature,
                (uint16_t)humidity
            };
            tk::publish(msg);
        }
        dht11_error = false;    // フラグを戻す
        tk::sleep_ms(2000);     // 測定は少なくとも2秒以上開ける
    }
}

tk::TaskHandle regist(uint32_t gp) {
    g_dht11_sda = gp;
    return tk::create( dht11_task, nullptr, tk::PRIO_NORMAL );
}


} // namespace Dht11
