#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"

// SSD1306コマンド定義
#define SSD1306_SET_MEM_MODE        _u(0x20)
#define SSD1306_SET_COL_ADDR        _u(0x21)
#define SSD1306_SET_PAGE_ADDR       _u(0x22)
#define SSD1306_SET_HORIZ_SCROLL    _u(0x26)
#define SSD1306_SET_SCROLL          _u(0x2E)

#define SSD1306_SET_DISP_START_LINE _u(0x40)

#define SSD1306_SET_CONTRAST        _u(0x81)
#define SSD1306_SET_CHARGE_PUMP     _u(0x8D)

#define SSD1306_SET_SEG_REMAP       _u(0xA0)
#define SSD1306_SET_ENTIRE_ON       _u(0xA4)
#define SSD1306_SET_ALL_ON          _u(0xA5)
#define SSD1306_SET_NORM_DISP       _u(0xA6)
#define SSD1306_SET_INV_DISP        _u(0xA7)
#define SSD1306_SET_MUX_RATIO       _u(0xA8)
#define SSD1306_SET_DISP            _u(0xAE)
#define SSD1306_SET_COM_OUT_DIR     _u(0xC0)
#define SSD1306_SET_COM_OUT_DIR_FLIP _u(0xC0)

#define SSD1306_SET_DISP_OFFSET     _u(0xD3)
#define SSD1306_SET_DISP_CLK_DIV    _u(0xD5)
#define SSD1306_SET_PRECHARGE       _u(0xD9)
#define SSD1306_SET_COM_PIN_CFG     _u(0xDA)
#define SSD1306_SET_VCOM_DESEL      _u(0xDB)

#define SSD1306_PAGE_HEIGHT         _u(8)
#define SSD1306_NUM_PAGES           (SSD1306_HEIGHT / SSD1306_PAGE_HEIGHT)
#define SSD1306_BUF_LEN             (SSD1306_NUM_PAGES * SSD1306_WIDTH)

#define SSD1306_WRITE_MODE         _u(0xFE)
#define SSD1306_READ_MODE          _u(0xFF)

#define SSD1306_CONTROL_CMD        _u(0x80)
#define SSD1306_CONTROL_DATA       _u(0x40)


inline int ssd1306_buf_len(ssd1306_rect_t *r)
{
    return (r->end_col-r->start_col+1)*(r->end_page-r->start_page+1);
}

// コマンド発行
void ssd1306_send_cmd(uint8_t cmd)
{
    uint8_t buf[2] = {SSD1306_CONTROL_CMD, cmd};   // 0x80 = コマンド
    i2c_write_blocking(i2c0, SSD1306_ADDR, buf, 2, false);
}

// SSD1306を初期化
void ssd1306_init()
{
    /* memory mapping */
    ssd1306_send_cmd(SSD1306_SET_MEM_MODE); // set memory address mode
    ssd1306_send_cmd(0x00); // horizontal addressing mode

    /* resolution and layout */
    ssd1306_send_cmd(SSD1306_SET_DISP_START_LINE); // set display start line to 0

    ssd1306_send_cmd(SSD1306_SET_SEG_REMAP | 0x01); // set segment re-map
    // column address 127 is mapped to SEG0

    ssd1306_send_cmd(SSD1306_SET_MUX_RATIO); // set multiplex ratio
    ssd1306_send_cmd(SSD1306_HEIGHT - 1); // 32 pixels high

    ssd1306_send_cmd(SSD1306_SET_COM_OUT_DIR | 0x08); // set COM (common) output scan direction
    // scan from bottom up, COM[N-1] to COM0

    ssd1306_send_cmd(SSD1306_SET_DISP_OFFSET); // set display offset
    ssd1306_send_cmd(0x00); // no offset

    ssd1306_send_cmd(SSD1306_SET_COM_PIN_CFG); // set COM (common) pins hardware configuration
    ssd1306_send_cmd(0x02); // manufacturer magic number

    /* timing and driving scheme */
    ssd1306_send_cmd(SSD1306_SET_DISP_CLK_DIV); // set display clock divide ratio
    ssd1306_send_cmd(0x80); // div ratio of 1, standard freq

    ssd1306_send_cmd(SSD1306_SET_PRECHARGE); // set pre-charge period
    ssd1306_send_cmd(0xF1); // Vcc internally generated on our board

    ssd1306_send_cmd(SSD1306_SET_VCOM_DESEL); // set VCOMH deselect level
    ssd1306_send_cmd(0x30); // 0.83xVcc

    /* display */
    ssd1306_send_cmd(SSD1306_SET_CONTRAST); // set contrast control
    ssd1306_send_cmd(0xFF);

    ssd1306_send_cmd(SSD1306_SET_ENTIRE_ON); // set entire display on to follow RAM content

    ssd1306_send_cmd(SSD1306_SET_NORM_DISP); // set normal (not inverted) display

    ssd1306_send_cmd(SSD1306_SET_CHARGE_PUMP); // set charge pump
    ssd1306_send_cmd(0x14); // Vcc internally generated on our board

    ssd1306_send_cmd(SSD1306_SET_SCROLL | 0x00); // deactivate horizontal scrolling if set
    // this is necessary as memory writes will corrupt if scrolling was enabled

    ssd1306_send_cmd(SSD1306_SET_DISP | 0x01); // turn display on
}

// データ送信
void ssd1306_send_data(uint8_t *buf, uint len)
{
    uint8_t *temp_buf = malloc(len+1);

    for (int i = 1; i < len + 1; i++) {
        temp_buf[i] = buf[i - 1];
    }
    temp_buf[0] = SSD1306_CONTROL_DATA; // データ

    i2c_write_blocking(i2c0, SSD1306_ADDR, temp_buf, len+1, false);

    free(temp_buf);
}

// OLEDエリア書き込み
void ssd1306_write_area(uint8_t *buf, ssd1306_rect_t *rect)
{
    uint8_t cmd[6];

    ssd1306_send_cmd(SSD1306_SET_COL_ADDR);
    ssd1306_send_cmd(rect->start_col);
    ssd1306_send_cmd(rect->end_col);
    ssd1306_send_cmd(SSD1306_SET_PAGE_ADDR);
    ssd1306_send_cmd(rect->start_page);
    ssd1306_send_cmd(rect->end_page);
    ssd1306_send_data(buf, ssd1306_buf_len(rect));
}


/* ペンディング
void ssd1306_write_dma_area(uint8_t *buf, ssd1306_rect_t *rect)
{
    uint8_t cmd[1] = {SSD1306_CONTROL_DATA};
    size_t len = ssd1306_buf_len(rect);

    dma_buffer = malloc(len * sizeof(uint16_t));
    for(int i = 0; i < len; i++) {
        dma_buffer[i] = buf[i];
    }

    dma_ch = dma_claim_unused_channel(true);
    dmac_config = dma_channel_get_default_config(dma_ch);
    channel_config_set_transfer_data_size(&dmac_config, DMA_SIZE_16);
    channel_config_set_read_increment(&dmac_config, true);
    channel_config_set_write_increment(&dmac_config, false);
    // channel_config_set_bswap(&dmac_config, true);
    uint dreq = i2c_get_dreq(i2c0, true);
    channel_config_set_dreq(&dmac_config, dreq);
    // DMAチャンネルの設定
    dma_channel_configure(
        dma_ch,                 // DMAチャンネル
        &dmac_config,           // 構成情報
        &i2c0_hw->data_cmd,     // 書き込みアドレス
        dma_buffer,             // 転送元レジスタアドレス
        len,                    // 転送データサイズ
        false                   // すぐに開始ならtrue
    );

    // エリア設定
    ssd1306_send_cmd(SSD1306_SET_COL_ADDR);
    ssd1306_send_cmd(rect->start_col);
    ssd1306_send_cmd(rect->end_col);
    ssd1306_send_cmd(SSD1306_SET_PAGE_ADDR);
    ssd1306_send_cmd(rect->start_page);
    ssd1306_send_cmd(rect->end_page);

    // SSD1306_CONTROL_DATAをバースト書き込み
    i2c_write_burst_blocking(i2c0, SSD1306_ADDR, cmd, 1);
    // DMA転送開始
    dma_channel_start(dma_ch);
}
*/