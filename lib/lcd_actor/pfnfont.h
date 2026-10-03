#ifndef __PFNFONT
#define __PFNFONT 1

#include <stdint.h>

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#define PFN_FONT_NAME_SIZE  8
#define PFN_SIG_SIZE        7
#define PFN_VERSION         1

typedef struct __attribute__((packed)) {
    char    signature[PFN_SIG_SIZE];       // シグニチャ、文字列PINOTFN固定
    uint8_t version;            // バージョン(1)
    char    fontname[PFN_FONT_NAME_SIZE];        // フォント名（文字列shnmk14u）
} pfn_font_header_t;

typedef struct __attribute__((packed)) {
    uint8_t     width;          // フォントの幅
    uint8_t     height;         // フォントの高さ
    uint8_t     codepoint_size; // コードポイントのサイズ（1～4）
    uint8_t     attribute;      // 予約（未使用）
    uint16_t    num_glyphs;     // このブロックに含まれるグリフの数
} pfn_block_header_t;

typedef struct __attribute__((packed)) {
    uint8_t     codepoint;
    uint8_t     glyph_data[];
} pfn_glyph1_t;

typedef struct __attribute__((packed)) {
    uint16_t     codepoint;
    uint8_t     glyph_data[];
} pfn_glyph2_t;

typedef struct __attribute__((packed)) {
    uint32_t     codepoint;
    uint8_t     glyph_data[];
} pfn_glyph4_t;

typedef struct __attribute__((packed)) {
    uint8_t     width;          // フォントの幅
    uint8_t     height;         // フォントの高さ
    uint8_t     *bitmap;        // フォントのビットマップ
} font_gryph_t;


class PFNFont {
 private:
    uint8_t *font_data;     // フォントデータの先頭アドレス
    size_t font_size;       // フォントデータのサイズ
    uint8_t *first_block;    // 最初のフォントブロック
    font_gryph_t gryph_data;        // get_gryph_dataが返すグリフデータ
    uint8_t *aligned_bitmap_buffer; // グリフのビットマップ用バッファ
    size_t bitmap_buffer_size;      // ビットマップバッファのサイズ
    bool is_valid;                  // フォントデータが有効かどうかのフラグ
    char font_name[PFN_FONT_NAME_SIZE+1];    // フォント名
    uint8_t max_font_width;             // フォントの最大幅
    uint8_t max_font_height;            // フォントの最大高

    // コードポイントを含むフォントブロックを返す
    uint8_t* find_glyph_in_block(uint32_t codepoint);
    // グリフのコードポイントを返す
    inline uint32_t get_codepoint_from_glyph(uint8_t *glyph_data, uint8_t codepoint_size);
    // 文字列を描画するコールバック関数
    void (*draw_font_callback)(font_gryph_t *,int x, int y);

 public:
    // PFNFontコンストラクタ
    // uint8_t *pfn_font: pfnフォントデータの先頭アドレス
    // size_t pfnfont_size: pfnフォントデータのサイズ
    // void *draw_font(font_gryph_t *,int,int): 文字描画のコールバック関数
    PFNFont(uint8_t *pfn_font, size_t pfnfont_size, void (*draw_font)(font_gryph_t *,int,int));
    ~PFNFont();
    // フォントが有効かチェックする
    bool isValid() { return is_valid; }
    // UTF-8をUTF-32コードポイントに変換
    uint32_t utf8_to_codepoint(const char **utf8_str);
    // コードポイントからフォントグリフのデータを得る
    font_gryph_t *get_gryph_data(uint32_t codepoint);
    // 文字列を描画する
    void draw_string(const char *str, int x, int y);
    // このフォントデータの最大フォント高を返す
    uint8_t get_max_font_height() { return max_font_height; }
    // このフォントデータの最大フォント幅を返す
    uint8_t get_max_font_width() { return max_font_width; }
    // フォント名を返す
    const char *get_font_name() { return font_name; }
};
#endif
