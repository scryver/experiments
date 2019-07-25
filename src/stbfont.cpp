#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "../libberdip/perlin.h"
#include "../libberdip/suballoc.h"
#include "interface.h"
DRAW_IMAGE(draw_image);

#define SUBPIXEL_LOCATION 1
// NOTE(michiel): Make one or none 1, others 0
#define FONT_POS_ROUNDING 1
#define FONT_POS_FLOOR    0
#define FONT_POS_CEIL     0
//


//
// NOTE(michiel): STB Truetype helpers
//
internal void *
STBTT_memcpy(void *dest, void *src, umm size)
{
    copy(size, src, dest);
    return dest;
}

internal void *
STBTT_memset(void *dest, s32 value, umm size)
{
    copy_single(size, (u32)value, dest);
    return dest;
}

#define stbtt_uint8        u8
#define stbtt_int8         s8
#define stbtt_uint16       u16
#define stbtt_int16        s16
#define stbtt_uint32       u32
#define stbtt_int32        s32

#define STBTT_ifloor(x)    floor((s32)(x))
#define STBTT_iceil(x)     ceil((s32)(x))
#define STBTT_sqrt(x)      square_root(x)
#define STBTT_pow(x, y)    pow(x, y)
#define STBTT_fmod(x, y)   modulus(x, y)
#define STBTT_cos(x)       cos(x)
#define STBTT_acos(x)      acos(x)
#define STBTT_fabs(x)      absolute(x)

#define STBTT_malloc(x, u) sub_alloc((SubAllocator *)u, x)
#define STBTT_free(x, u)   sub_dealloc((SubAllocator *)u, x)

//#define STBTT_assert(x)    i_expect_simple(x)
#define STBTT_assert(x)

#define STBTT_strlen(x)    string_length(x)

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "../libberdip/stb_truetype.h"

//
//
//

#include "main.cpp"

#include "../libberdip/std_file.c"
#include "../libberdip/drawing.cpp"
#include "../libberdip/suballoc.cpp"

#define FONT_REGULAR_TOKEN   0x06
#define FONT_BOLD_TOKEN      0x15
#define FONT_START_REGULAR   "\x06"
#define FONT_START_BOLD      "\x15"

struct SingleFont
{
    Buffer  memory;
    stbtt_fontinfo stbFont;
    
    f32 fontScale;
    
    f32 ascender;
    f32 descender;
    f32 lineGap;
};

struct DualFont
{
    SingleFont regular;
    SingleFont bold;
};

#define MAX_FALLBACK_FONTS 8
struct StbFont
{
    SubAllocator *allocator;
    
    // NOTE(michiel): Using font 0 -> count to search for glyphs. Only using the next font if the glyph is undefined
    u32 fontCount;
    DualFont fonts[MAX_FALLBACK_FONTS];
    
    f32 pixelHeight;
};

internal f32
get_line_advance(SingleFont *font)
{
    f32 result = font->ascender - font->descender + font->lineGap;
    return result;
}

struct TextImage
{
    String text;
    Image8 image;
};

struct StbState
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    u32 prevMouseDown;
    
    SubAllocator allocator;
    
    StbFont font;
    
    TextImage textA;
    TextImage textB;
};

internal void
init_stb_font(StbFont *font, f32 pixelHeight)
{
    font->fontCount = 0;
    font->pixelHeight = pixelHeight;
}

internal SingleFont
add_stb_font(StbFont *font, String filename)
{
    SingleFont result = {};
    result.memory = read_entire_file(filename);
    if (result.memory.size)
    {
        s32 numFonts = stbtt_GetNumberOfFonts(result.memory.data);
        
        if (numFonts > 0)
        {
            int offset = stbtt_GetFontOffsetForIndex(result.memory.data, 0);
            if (stbtt_InitFont(&result.stbFont, result.memory.data, offset) != 0)
            {
                result.stbFont.userdata = font->allocator;
                
                fprintf(stdout, "Found %d font%s in '%.*s'\n", numFonts, numFonts == 1 ? "" : "s", STR_FMT(filename));
                
                s32 ascent, descent, lineGap;
                stbtt_GetFontVMetrics(&result.stbFont, &ascent, &descent, &lineGap);
                
                //f32 scale = font->pixelHeight / (f32)(ascent - descent);
                f32 scale = stbtt_ScaleForPixelHeight(&result.stbFont, font->pixelHeight);
                
                result.ascender = scale * (f32)ascent;
                result.descender = scale * (f32)descent;
                result.lineGap = scale * (f32)lineGap;
                result.fontScale = scale;
            }
            else
            {
                fprintf(stderr, "Could not initialize the stb truetype font '%.*s'\n", STR_FMT(filename));
            }
        }
        else
        {
            fprintf(stderr, "There are no fonts in the file '%.*s'\n", STR_FMT(filename));
        }
    }
    else
    {
        fprintf(stderr, "Could not load font file '%.*s'\n", STR_FMT(filename));
    }
    return result;
}

internal void
add_dual_stb_font(StbFont *font, String regular, String bold)
{
    i_expect(font->fontCount < array_count(font->fonts));
    DualFont *dual = font->fonts + font->fontCount++;
    dual->regular  = add_stb_font(font, regular);
    dual->bold     = add_stb_font(font, bold);
}

internal SingleFont *
get_font_from_codepoint(StbFont *font, u32 codepoint, b32 useBold = false, u32 *glyphIndex = 0)
{
    i_expect(font->fontCount);
    SingleFont *result = 0;
    
    s32 index = 0;
    for (u32 fontIdx = 0; fontIdx < font->fontCount; ++fontIdx)
    {
        DualFont *dFont = font->fonts + fontIdx;
        SingleFont *test = useBold ? &dFont->bold : &dFont->regular;
        
        index = stbtt_FindGlyphIndex(&test->stbFont, codepoint);
        if (index > 0) {
            if (glyphIndex) {
                *glyphIndex = index;
            }
            result = test;
            break;
        }
    }
    
    if (!result) {
        // NOTE(michiel): Use the main font for glyphs that are not found
        result = useBold ? &font->fonts[0].bold : &font->fonts[0].regular;
    }
    
    return result;
}

internal TextImage
create_text_image(StbFont *font, String text)
{
    i_expect(font->fontCount);
    
    TextImage result = {};
    result.text = text;
    result.image.height = s32_from_f32_round(get_line_advance(&font->fonts[0].regular));
    
    // TODO(michiel): Merge two groups somehow, temp memory maybe?
    u8 *scan = text.data;
    b32 useBold = false;
    u32 prevGlyph = 0;
    f32 maxX = 0.0f;
    for (u32 textIdx = 0; textIdx < text.size;)
    {
        if ((scan[0] != FONT_REGULAR_TOKEN) &&
            (scan[0] != FONT_BOLD_TOKEN))
        {
            u32 codepoint = 0;
            u32 advance = get_code_point_from_utf8(scan, &codepoint);
            if (!advance) {
                fprintf(stderr, "Invalid utf character 0x%02X\n", text.data[textIdx]);
                advance = 1;
            }
            
            u32 glyphIndex = 0;
            SingleFont *curFont = get_font_from_codepoint(font, codepoint, useBold, &glyphIndex);
            
            if (is_end_of_line(codepoint))
            {
                u32 maxXint = u32_from_f32_ceil(maxX);
                if (result.image.width < maxXint) {
                    result.image.width = maxXint;
                }
                maxX = 0.0f;
                result.image.height += u32_from_f32_ceil(get_line_advance(curFont) * 1.2f);
            }
            else
            {
                if (prevGlyph && glyphIndex)
                {
                    s32 kerning = stbtt_GetGlyphKernAdvance(&curFont->stbFont, prevGlyph, glyphIndex);
                    maxX += curFont->fontScale * (f32)kerning;
                }
                
                s32 stbAdv, stbLsb;
                stbtt_GetGlyphHMetrics(&curFont->stbFont, glyphIndex, &stbAdv, &stbLsb);
                maxX += curFont->fontScale * (f32)stbAdv;
            }
            
            prevGlyph = glyphIndex;
            
            scan += advance;
            textIdx += advance;
        }
        else
        {
            useBold = scan[0] == FONT_BOLD_TOKEN;
            ++scan;
            ++textIdx;
        }
    }
    
    u32 maxXint = u32_from_f32_ceil(maxX);
    if (result.image.width < maxXint) {
        result.image.width = maxXint;
    }
    
    // TODO(michiel): Put this on the same sub alloc as stb, make sub alloc macros for struct/array
    result.image.pixels = allocate_array(u8, result.image.width * result.image.height);
    
    scan = text.data;
    useBold = false;
    prevGlyph = 0;
    f32 x = 0;
    f32 y = font->fonts[0].regular.ascender;
    for (u32 textIdx = 0; textIdx < text.size;)
    {
        if ((scan[0] != FONT_REGULAR_TOKEN) &&
            (scan[0] != FONT_BOLD_TOKEN))
        {
            u32 codepoint = 0;
            u32 advance = get_code_point_from_utf8(scan, &codepoint);
            if (!advance) {
                fprintf(stderr, "Invalid utf character 0x%02X\n", text.data[textIdx]);
                advance = 1;
            }
            
            u32 glyphIndex = 0;
            SingleFont *curFont = get_font_from_codepoint(font, codepoint, useBold, &glyphIndex);
            
            if (is_end_of_line(codepoint))
            {
                x = 0;
                y += get_line_advance(curFont) * 1.2f;
            }
            else
            {
                f32 modKern = 0.0f;
                if (glyphIndex && prevGlyph)
                {
                    s32 kerning = stbtt_GetGlyphKernAdvance(&curFont->stbFont, prevGlyph, glyphIndex);
                    modKern = curFont->fontScale * (f32)kerning;
                }
                
                s32 stbAdv, stbLsb;
                stbtt_GetGlyphHMetrics(&curFont->stbFont, glyphIndex, &stbAdv, &stbLsb);
                
                f32 atX = x + curFont->fontScale * (f32)stbLsb + modKern;
                f32 atY = y;
                
                s32 ix0, ix1, iy0, iy1;
                
#if SUBPIXEL_LOCATION
                stbtt_GetGlyphBitmapBoxSubpixel(&curFont->stbFont, glyphIndex, curFont->fontScale, curFont->fontScale,
                                                atX - (s32)atX, atY - (s32)atY,
                                                &ix0, &iy0, &ix1, &iy1);
#else
                stbtt_GetGlyphBitmapBox(&curFont->stbFont, glyphIndex, curFont->fontScale, curFont->fontScale,
                                        &ix0, &iy0, &ix1, &iy1);
#endif
                
                atY += (f32)iy0;
                
#if FONT_POS_ROUNDING
                s32 modX = s32_from_f32_round(atX);
                s32 modY = s32_from_f32_round(atY);
#elif FONT_POS_FLOOR
                s32 modX = s32_from_f32_floor(atX);
                s32 modY = s32_from_f32_floor(atY);
#elif FONT_POS_CEIL
                s32 modX = s32_from_f32_ceil(atX);
                s32 modY = s32_from_f32_ceil(atY);
#else
                s32 modX = s32_from_f32_truncate(atX);
                s32 modY = s32_from_f32_truncate(atY);
#endif
                
                u8 *start = result.image.pixels + modY * result.image.width + modX;
                
#if SUBPIXEL_LOCATION
                stbtt_MakeGlyphBitmapSubpixel(&curFont->stbFont, start, 
                                              ix1 - ix0 + 5, iy1 - iy0 + 5,
                                              result.image.width,
                                              curFont->fontScale, curFont->fontScale,
                                              atX - (s32)atX, atY - (s32)atY, glyphIndex);
#else
                stbtt_MakeGlyphBitmap(&curFont->stbFont, start, ix1 - ix0 + 5, iy1 - iy0 + 5, result.image.width,
                                      curFont->fontScale, curFont->fontScale, glyphIndex);
#endif
                
                x += curFont->fontScale * (f32)stbAdv;
            }
            
            prevGlyph = glyphIndex;
            
            scan += advance;
            textIdx += advance;
        }
        else
        {
            useBold = scan[0] == FONT_BOLD_TOKEN;
            ++scan;
            ++textIdx;
        }
    }
    
    return result;
}

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(StbState) <= state->memorySize);
    
    //v2 size = V2((f32)image->width, (f32)image->height);
    
    StbState *stbState = (StbState *)state->memory;
    if (!state->initialized)
    {
        // stbState->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        stbState->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        
        u32 dataSize = megabytes(128);
        u8 *dataMem = allocate_array(u8, dataSize);
        b32 success = init_sub_allocator(&stbState->allocator, dataSize, dataMem);
        i_expect(success);
        stbState->font.allocator = &stbState->allocator;
        
        //String regularFont = static_string("/usr/share/fonts/truetype/noto/NotoSerifDisplay-Regular.ttf");
        //String boldFont    = static_string("/usr/share/fonts/truetype/noto/NotoSerifDisplay-Bold.ttf");
        //String regularFont = static_string("/usr/share/fonts/truetype/freefont/FreeSans.ttf");
        //String regularFont = static_string("/usr/share/fonts/truetype/malayalam/Suruma.ttf");
        //String regularFont = static_string("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
        //String regularFont = static_string("/usr/share/fonts/truetype/nanum/NanumMyeongjo.ttf");
        //String regularFont = static_string("/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf");
        //String regularFont = static_string("/usr/share/fonts/truetype/noto/NotoSans-SemiCondensedBlack.ttf");
        
        String regularFont = static_string("/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf");
        String boldFont    = static_string("/usr/share/fonts/truetype/noto/NotoSans-Bold.ttf");
        String regularCJKFont = static_string("/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc");
        String boldCJKFont    = static_string("/usr/share/fonts/opentype/noto/NotoSansCJK-Bold.ttc");
        String regularThaiFont = static_string("/usr/share/fonts/truetype/noto/NotoSansThai-Regular.ttf");
        String boldThaiFont    = static_string("/usr/share/fonts/truetype/noto/NotoSansThai-Bold.ttf");
        //String regularArabFont = static_string("/usr/share/fonts/truetype/noto/NotoSansArabic-Regular.ttf");
        //String boldArabFont    = static_string("/usr/share/fonts/truetype/noto/NotoSansArabic-Bold.ttf");
        // TODO(michiel): This font gives a stbtt_assert when setting the font scale to above 32.0f
        String regularDevaFont = static_string("/usr/share/fonts/truetype/noto/NotoSansDevanagari-Regular.ttf");
        String boldDevaFont    = static_string("/usr/share/fonts/truetype/noto/NotoSansDevanagari-Bold.ttf");
        
        init_stb_font(&stbState->font, 45.0f);
        add_dual_stb_font(&stbState->font, regularFont, boldFont);
        add_dual_stb_font(&stbState->font, regularCJKFont, boldCJKFont);
        add_dual_stb_font(&stbState->font, regularThaiFont, boldThaiFont);
        //add_dual_stb_font(&stbState->font, regularArabFont, boldArabFont); // NOTE(michiel): Add rtl support
        add_dual_stb_font(&stbState->font, regularDevaFont, boldDevaFont);
        
        String testStrA = static_string(FONT_START_BOLD "Hello, world!" FONT_START_REGULAR " <And greetings... \xCE\x94 \xCF\x80 +=->\n"
                                        "\xEB\xB8\xA0\xEB\xB8\xA1\xEB\xB8\xA2\xEB\xB8\xA3\n"
                                        "And some more text for\nVAWA jij and fij ffi\n"
                                        "\xD0\x81 \xD1\xAA");
        
        struct timespec start = get_wall_clock();
        stbState->textA = create_text_image(&stbState->font, testStrA);
        stbState->textB = create_text_image(&stbState->font, static_string("Test B | T.W.Lewis\nficellé fffffi. VAV.\nتسجّل يتكلّم\nДуо вёжи дёжжэнтиюнт ут\n緳 踥踕\nहालाँकि प्रचलित रूप पूजा"));
        struct timespec end = get_wall_clock();
        
        fprintf(stdout, "Generated fonts in %f seconds\n", get_seconds_elapsed(start, end));
        
        state->initialized = true;
    }
    
    i_expect(stbState->font.fontCount);
    i_expect(stbState->font.fonts[0].regular.memory.size);
    i_expect(stbState->font.fonts[0].bold.memory.size);
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    if (keyboard->keys[Key_A].isDown) {
        draw_image(image, 20, 20, &stbState->textB.image);
    }else {
        draw_image(image, 20, 20, &stbState->textA.image, V4(1, 1, 0, 1));
    }
    
    sub_coalesce(&stbState->allocator);
    
    stbState->prevMouseDown = mouse.mouseDowns;
    stbState->seconds += dt;
    ++stbState->ticks;
    if (stbState->seconds > 1.0f)
    {
        stbState->seconds -= 1.0f;
        fprintf(stdout, "Ticks: %4u | Time: %fms\n", stbState->ticks,
                1000.0f / (f32)stbState->ticks);
        stbState->ticks = 0;
    }
}
