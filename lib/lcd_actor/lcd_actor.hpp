#pragma once

#include "tinykernel/actor.hpp"
#include "pfnfont.h"
#include <cstdint>
#include <cstddef>
#include <cstring>

#include "hardware/i2c.h"

#define I2C_PORT i2c0
#define I2C_SCL PICO_DEFAULT_I2C_SCL_PIN
#define I2C_SDA PICO_DEFAULT_I2C_SDA_PIN

struct LcdMsg {
    enum Type : uint8_t {
        CLEAR = 1,
        PUT_STR,
    };
    Type type{CLEAR};
    uint8_t x{0};
    uint8_t y{0};
    char text[48]{};

    static LcdMsg clear() {
        LcdMsg m;
        m.type = CLEAR;
        return m;
    }

    static LcdMsg put_str(uint8_t x, uint8_t y, const char* str) {
        LcdMsg m;
        m.type = PUT_STR;
        m.x = x;
        m.y = y;
        if (str) {
            std::strncpy(m.text, str, sizeof(m.text) - 1);
            m.text[sizeof(m.text) - 1] = '\0';
        }
        return m;
    }
};

class LcdActor : public tk::Actor<LcdMsg, 8> {
public:
    explicit LcdActor(i2c_inst_t* i2c = I2C_PORT,
                      uint8_t sda_pin = I2C_SDA,
                      uint8_t scl_pin = I2C_SCL,
                      uint32_t baudrate = 400 * 1000);
    ~LcdActor() override;

    // --- 送信側タスク向けAPI ---
    bool clear() {
        return post(LcdMsg::clear());
    }

    bool put_str(uint8_t x, uint8_t y, const char* str) {
        return post(LcdMsg::put_str(x, y, str));
    }

protected:
    void on_start() override;
    void on_message(const LcdMsg& msg) override;

private:
    static constexpr size_t WIDTH = 128;
    static constexpr size_t HEIGHT = 32;
    static constexpr size_t PAGES = HEIGHT / 8;
    static constexpr size_t VRAM_SIZE = WIDTH * PAGES; // 512 bytes

    uint8_t vram_[VRAM_SIZE]{};
    PFNFont* font_{nullptr};

    i2c_inst_t* i2c_{I2C_PORT};
    uint8_t sda_pin_{I2C_SDA};
    uint8_t scl_pin_{I2C_SCL};
    uint32_t baudrate_{400 * 1000};

    static LcdActor* instance_;

    static void draw_glyph_callback(font_gryph_t* glyph, int x, int y);
    void render_glyph(font_gryph_t* glyph, int x, int y);
    void draw_pixel(int x, int y, bool on);
    void clear_vram();
    void flush_vram();
};
