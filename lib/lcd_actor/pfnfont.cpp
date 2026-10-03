#include <stdio.h>
#include <string.h>
#include "pfnfont.h"

PFNFont::PFNFont(uint8_t *pfn_font, size_t pfnfont_size, void (*draw_font)(font_gryph_t *,int,int))
{
    is_valid = false;
    font_data = pfn_font;
    font_size = pfnfont_size;
    draw_font_callback = draw_font;
    aligned_bitmap_buffer = NULL;

    // ヘッダサイズとシグニチャを検証
    if (pfnfont_size < sizeof(pfn_font_header_t) || memcmp(font_data, "PINOTFN", PFN_SIG_SIZE) != 0) {
        return; // 不正なフォントデータ
    }
    // first blockの先頭アドレス
    first_block = font_data + sizeof(pfn_font_header_t);
    if (first_block >= font_data + pfnfont_size) return; // データサイズが不正

    // フォント名のプロパティをセット
    memcpy(font_name, ((pfn_font_header_t *)font_data)->fontname, PFN_FONT_NAME_SIZE);
    font_name[PFN_FONT_NAME_SIZE] = '\0';

    // ブロックをたどりフォントビットマップの最大サイズを調べる
    bitmap_buffer_size = 0;
    max_font_height = 0;
    max_font_width = 0;
    uint8_t *current_block_ptr = first_block;
    while(current_block_ptr < (font_data + pfnfont_size)) {
        pfn_block_header_t *current_block_header = (pfn_block_header_t *)current_block_ptr;
        int bitmap_size = (current_block_header->width * current_block_header->height + 7) / 8;
        max_font_height = max(max_font_height, current_block_header->height);
        max_font_width = max(max_font_width, current_block_header->width);

        // このブロックのデータ部分の合計サイズ
        int block_data_size = current_block_header->num_glyphs * (bitmap_size + current_block_header->codepoint_size);
        // 次のブロックへポインタを進める (ヘッダサイズ + データサイズ)
        current_block_ptr += sizeof(pfn_block_header_t) + block_data_size;
    }
    // ビットマップの最大サイズ（バイト単位）
    bitmap_buffer_size = (max_font_width * max_font_height + 7) / 8;

    // 4バイトアラインメントの配列としてメモリを確保
    if (bitmap_buffer_size > 0) {
        aligned_bitmap_buffer = (uint8_t *)new uint32_t[(bitmap_buffer_size + 3) / 4];
    } else {
        aligned_bitmap_buffer = NULL;
    }

    is_valid = true;
}

PFNFont::~PFNFont()
{
    // 確保したメモリを開放
    delete[] aligned_bitmap_buffer;
}

// UTF-8文字列へのポインタを受け取り、1文字デコードしてコードポイントを返す
uint32_t PFNFont::utf8_to_codepoint(const char **utf8_str)
{
    uint32_t codepoint = 0;
    const uint8_t* s = (const uint8_t*)*utf8_str;
    
    if (s[0] < 0x80) { // 1-byte (ASCII)
        codepoint = s[0];
        *utf8_str += 1;
    } else if ((s[0] & 0xE0) == 0xC0) { // 2-byte sequence
        codepoint = ((s[0] & 0x1F) << 6) | (s[1] & 0x3F);
        *utf8_str += 2;
    } else if ((s[0] & 0xF0) == 0xE0) { // 3-byte sequence
        codepoint = ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
        *utf8_str += 3;
    } else if ((s[0] & 0xF8) == 0xF0) { // 4-byte sequence
        codepoint = ((s[0] & 0x07) << 18) | ((s[1] & 0x3F) << 12) | ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);
        *utf8_str += 4;
    } else {
        // 不正なUTF-8シーケンスの場合は、1バイト進めてエラーを示す
        codepoint = '?'; 
        *utf8_str += 1;
    }
    return codepoint;
}

// コードポイントサイズに合わせてgryphのコードポイントを得るユーティリティ関数
inline uint32_t PFNFont::get_codepoint_from_glyph(uint8_t *glyph_data, uint8_t codepoint_size)
{
    uint32_t codepoint = 0;

    if(codepoint_size == 1) {
        pfn_glyph1_t *glyph = (pfn_glyph1_t *)glyph_data;
        codepoint = glyph->codepoint;
    }
    else if(codepoint_size == 2){
        pfn_glyph2_t *glyph = (pfn_glyph2_t *)glyph_data;
        codepoint = glyph->codepoint;
    }
    else if(codepoint_size == 4){
        pfn_glyph4_t *glyph = (pfn_glyph4_t *)glyph_data;
        codepoint = glyph->codepoint;
    }

    return codepoint;
}

// コードポイントがあるブロックを返す
uint8_t* PFNFont::find_glyph_in_block(uint32_t codepoint)
{
    uint8_t *current_block = first_block;

    do {
        pfn_block_header_t *current_block_header = (pfn_block_header_t *)current_block;
        uint8_t codepoint_size = current_block_header->codepoint_size;
        uint8_t font_height = current_block_header->height;
        uint8_t font_width = current_block_header->width;
        uint16_t num_glyphs = current_block_header->num_glyphs;

        int glyph_size = (font_width * font_height + 7) / 8 + codepoint_size;
        // ブロックのデータ部分のサイズ
        int block_data_size = glyph_size * num_glyphs;

        uint8_t *first_glyph = current_block + sizeof(pfn_block_header_t);
        uint32_t first_codepoint = get_codepoint_from_glyph(first_glyph, codepoint_size);
        uint8_t *last_glyph = first_glyph + (num_glyphs - 1) * glyph_size;
        uint32_t last_codepoint = get_codepoint_from_glyph(last_glyph, codepoint_size);

        if(codepoint < first_codepoint || codepoint > last_codepoint) {
            // 次のブロックへ移動 (ヘッダサイズ + データサイズ)
            current_block += sizeof(pfn_block_header_t) + block_data_size;
            if(current_block >= font_data + font_size) {
                current_block = NULL;
                break;
            }
        }
        else {
            break;
        }
    } while(true);

    return current_block;
}

// コードポイントからフォントグリフを得る
font_gryph_t *PFNFont::get_gryph_data(uint32_t codepoint)
{
    uint8_t *block = find_glyph_in_block(codepoint);
    if (block == NULL) {
        // 要求されたグリフが見つからない場合、代替文字('?')を探すか、NULLを返す
        // ここでは'?'が存在すればそれを返し、なければNULLを返す
        if (codepoint != '?') {
            return get_gryph_data('?');
        }
        return NULL;
    }

    pfn_block_header_t *block_header = (pfn_block_header_t *)block;
    uint8_t font_height = block_header->height;
    uint8_t font_width = block_header->width;
    uint16_t num_glyphs = block_header->num_glyphs;
    uint8_t codepoint_size = block_header->codepoint_size;
    uint8_t *first_glyph = block + sizeof(pfn_block_header_t);  
    int glyph_size = (font_width * font_height + 7) / 8 + codepoint_size;

    // 二分探索でグリフを検索
    int low = 0;
    int high = num_glyphs - 1;
    uint8_t *found_glyph = NULL;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        uint8_t *current_glyph = first_glyph + mid * glyph_size;
        uint32_t current_codepoint = get_codepoint_from_glyph(current_glyph, codepoint_size);

        if (current_codepoint == codepoint) {
            found_glyph = current_glyph;
            break;
        } else if (current_codepoint < codepoint) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    if (found_glyph == NULL) {
        // find_glyph_in_blockが成功していればここには到達しないはずだが、念のため
        return (codepoint != '?') ? get_gryph_data('?') : NULL;
    }

    int bitmap_data_size = (font_width * font_height + 7) / 8;
    gryph_data.width = font_width;
    gryph_data.height = font_height;
    // ビットマップデータをアラインされたバッファにコピー
    memcpy(aligned_bitmap_buffer, found_glyph + codepoint_size, bitmap_data_size);
    gryph_data.bitmap = aligned_bitmap_buffer;

    return &gryph_data;
}

// 文字列を描画する
void PFNFont::draw_string(const char *str, int x, int y)
{
    const char *p = str;
    while(*p != '\0') {
        uint32_t codepoint = utf8_to_codepoint(&p);
        font_gryph_t *gryph = get_gryph_data(codepoint);
        if(gryph != NULL && draw_font_callback != NULL) {
            draw_font_callback(gryph, x, y);
            x += gryph->width;
        }
    }
}
