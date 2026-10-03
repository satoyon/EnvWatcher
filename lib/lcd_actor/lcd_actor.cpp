#include "lcd_actor.hpp"
#include "ssd1306.h"
#include <cstring>
#include <cstdio>

// 14ドット日本語フォント (shnmk14u.pfn) をFlash (.rodata) に配置
__asm__(
  ".section \".rodata\" \n"
  ".balign 4\n"
  ".global _font_shnmk14u\n"
  ".global _font_shnmk14u_end\n"
  "_font_shnmk14u:\n"
  ".incbin \"shnmk14u.pfn\"\n"
  "_font_shnmk14u_end:\n"
  ".balign 4\n"
  ".section \".text\"\n"
);

extern "C" {
extern const uint8_t _font_shnmk14u[];
extern const uint8_t _font_shnmk14u_end[];
}

LcdActor* LcdActor::instance_ = nullptr;

LcdActor::LcdActor(i2c_inst_t* i2c, uint8_t sda_pin, uint8_t scl_pin, uint32_t baudrate)
    : i2c_(i2c), sda_pin_(sda_pin), scl_pin_(scl_pin), baudrate_(baudrate) {
    instance_ = this;
}

LcdActor::~LcdActor() {
    delete font_;
    if (instance_ == this) {
        instance_ = nullptr;
    }
}

void LcdActor::on_start() {
    // 1. I2CとGPIOの初期化
    i2c_init(i2c_, baudrate_);
    gpio_set_function(sda_pin_, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin_, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin_);
    gpio_pull_up(scl_pin_);

    // 2. SSD1306のハードウェア初期化
    ssd1306_init();

    // 3. フォント初期化
    size_t font_size = reinterpret_cast<uintptr_t>(_font_shnmk14u_end) - reinterpret_cast<uintptr_t>(_font_shnmk14u);
    font_ = new PFNFont(const_cast<uint8_t*>(_font_shnmk14u), font_size, &LcdActor::draw_glyph_callback);

    // 4. VRAM初期クリアと画面反映
    clear_vram();
    flush_vram();
}

void LcdActor::on_message(const LcdMsg& msg) {
    switch (msg.type) {
        case LcdMsg::CLEAR:
            clear_vram();
            flush_vram();
            break;

        case LcdMsg::PUT_STR:
            if (font_ && font_->isValid()) {
                font_->draw_string(msg.text, msg.x, msg.y);
            }
            flush_vram();
            break;

        default:
            break;
    }
}

void LcdActor::draw_glyph_callback(font_gryph_t* glyph, int x, int y) {
    if (instance_) {
        instance_->render_glyph(glyph, x, y);
    }
}

void LcdActor::render_glyph(font_gryph_t* glyph, int x, int y) {
    if (!glyph || !glyph->bitmap) return;

    for (int gy = 0; gy < glyph->height; gy++) {
        for (int gx = 0; gx < glyph->width; gx++) {
            int bit_idx = gy * glyph->width + gx;
            bool is_on = (glyph->bitmap[bit_idx / 8] & (0x80 >> (bit_idx % 8))) != 0;
            draw_pixel(x + gx, y + gy, is_on);
        }
    }
}

void LcdActor::draw_pixel(int x, int y, bool on) {
    if (x < 0 || x >= static_cast<int>(WIDTH) || y < 0 || y >= static_cast<int>(HEIGHT)) {
        return;
    }

    int page = y / 8;
    int bit  = y % 8;
    int idx  = page * WIDTH + x;

    if (on) {
        vram_[idx] |= (1 << bit);
    } else {
        vram_[idx] &= ~(1 << bit);
    }
}

void LcdActor::clear_vram() {
    memset(vram_, 0, sizeof(vram_));
}

void LcdActor::flush_vram() {
    ssd1306_rect_t rect;
    rect.start_col = 0;
    rect.end_col = WIDTH - 1;
    rect.start_page = 0;
    rect.end_page = PAGES - 1;
    ssd1306_write_area(vram_, &rect);
}
