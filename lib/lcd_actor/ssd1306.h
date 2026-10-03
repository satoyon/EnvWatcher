#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SSD1306_ADDR    0x3C            // SSD1306のデフォルトのI2Cアドレス
#define SSD1306_HEIGHT     32
#define SSD1306_WIDTH      128

// SSD1306フレームバッファの領域指定
typedef struct {
    uint8_t start_col;
    uint8_t end_col;
    uint8_t start_page;
    uint8_t end_page;
} ssd1306_rect_t;

void ssd1306_send_cmd(uint8_t cmd);
void ssd1306_init(void);
void ssd1306_send_data(uint8_t *buf, uint len);
void ssd1306_write_area(uint8_t *buf, ssd1306_rect_t *rect);
void ssd1306_clear(void);

#ifdef __cplusplus
}
#endif
