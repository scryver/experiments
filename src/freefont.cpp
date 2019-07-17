#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "../libberdip/perlin.h"
#include "interface.h"
DRAW_IMAGE(draw_image);

#undef internal
#include <ft2build.h>
#include FT_FREETYPE_H
#define internal static

#include <hb.h>
#include <hb-ft.h>

#include "main.cpp"

#include "../libberdip/std_file.c"
#include "../libberdip/drawing.cpp"

#define FONT_REGULAR_TOKEN   0xC1
#define FONT_BOLD_TOKEN      0xC2
#define FONT_START_REGULAR   "\xC1\xC1"
#define FONT_START_BOLD      "\xC2\xC2"

struct SingleFont
{
    Buffer  memory;
    FT_Face face;
};

struct DualFont
{
    SingleFont regular;
    SingleFont bold;
};

#define MAX_FALLBACK_FONTS 8
struct FTFont
{
    FT_Library library;
    
    // NOTE(michiel): Using font 0 -> count to search for glyphs. Only using the next font if the glyph is undefined
    u32 fontCount;
    DualFont fonts[MAX_FALLBACK_FONTS];
    
    f32 pixelHeight;
    b32 hasKerning;
    
    f32 lineAdvance;
};

struct HBFont
{
    hb_buffer_t *regularBuffer;
    hb_font_t   *regularFont;
    hb_buffer_t *boldBuffer;
    hb_font_t   *boldFont;
};

struct TextImage
{
    String text;
    Image image;
};

struct FTState
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    u32 prevMouseDown;
    
    FT_Library ftLibrary;
    FTFont font;
    
    b32 useHarfbuzz;
    HBFont hbFont;
};

internal void
init_ft_font(FT_Library ftLibrary, FTFont *font, f32 pixelHeight)
{
    font->library = ftLibrary;
    font->fontCount = 0;
    font->pixelHeight = pixelHeight;
    font->hasKerning = false;
    font->lineAdvance = 0.0f;
}

internal void
load_ft_font(FTFont *ftFont, Buffer buffer, FT_Face *face, f32 pixelHeight)
{
    b32 error = FT_New_Memory_Face(ftFont->library, buffer.data, buffer.size, 0, face);
    if (error == FT_Err_Unknown_File_Format)
    {
        fprintf(stderr, "Unsupported font format!\n");
    }
    else if (error)
    {
        fprintf(stderr, "Could not load font file!\n");
    }
    i_expect((*face)->face_flags & FT_FACE_FLAG_SCALABLE);
    
    // NOTE(michiel): Horizontal = 0 (same as vertical), Vertical = x pixels
    error = FT_Set_Pixel_Sizes(*face, 0, s32_from_f32_round(pixelHeight));
    if (error)
    {
        fprintf(stderr, "Could not set font pixel height!\n");
    }
    
    fprintf(stdout, "Got %ld font face%s\n", (*face)->num_faces, (*face)->num_faces == 1 ? "" : "s");
}

internal SingleFont
add_ft_font(FTFont *font, String filename)
{
    SingleFont result = {};
    result.memory = read_entire_file(filename);
    i_expect(result.memory.size);
    load_ft_font(font, result.memory, &result.face, font->pixelHeight);
    return result;
}

internal void
add_dual_ft_font(FTFont *font, String regular, String bold)
{
    i_expect(font->fontCount < array_count(font->fonts));
    DualFont *dual = font->fonts + font->fontCount++;
    dual->regular  = add_ft_font(font, regular);
    dual->bold     = add_ft_font(font, bold);
}

internal void
finalize_ft_font(FTFont *font)
{
    b32 hasKerning = (font->fontCount > 0);
    f32 lineAdvance = 0.0f;
    f32 oneOver64 = 1.0f / 64.0f;
    
    for (u32 fontIdx = 0; fontIdx < font->fontCount; ++fontIdx)
    {
        DualFont *dualFont = font->fonts + fontIdx;
        
        i_expect(dualFont->regular.memory.size && dualFont->bold.memory.size);
        hasKerning = hasKerning && FT_HAS_KERNING(dualFont->regular.face) && FT_HAS_KERNING(dualFont->bold.face);
        f32 regAdvance = (f32)(dualFont->regular.face->size->metrics.height) * oneOver64;
        f32 boldAdvance = (f32)(dualFont->bold.face->size->metrics.height) * oneOver64;
        i_expect(regAdvance == boldAdvance);
        if (lineAdvance < regAdvance) {
            lineAdvance = regAdvance;
        }
    }
    
    font->hasKerning = hasKerning;
    font->lineAdvance = lineAdvance;
    
    fprintf(stdout, "This font has%s got kerning support!\n", font->hasKerning ? "" : "n't");
}

internal FT_GlyphSlot
get_glyph_from_index(FT_Face face, u32 glyphIndex)
{
    FT_GlyphSlot result = face->glyph;
    
    b32 error = FT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT);
    if (error) {
        fprintf(stderr, "Could not load glyph %u\n", glyphIndex);
    }
    
    error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
    if (error) {
        fprintf(stderr, "Could not render glyph %u\n", glyphIndex);
    }
    
    return result;
}

internal FT_GlyphSlot
get_glyph_from_codepoint(FTFont *font, u32 codepoint, b32 useBold = false, u32 *glyphIndex = 0)
{
    i_expect(font->fontCount);
    FT_GlyphSlot result = 0;
    FT_Face face = useBold ? font->fonts[0].bold.face : font->fonts[0].regular.face;
    
    u32 index = 0;
    for (u32 fontIdx = 0; fontIdx < font->fontCount; ++fontIdx)
    {
        DualFont *dFont = font->fonts + fontIdx;
        face = useBold ? dFont->bold.face : dFont->regular.face;
        
        index = FT_Get_Char_Index(face, codepoint);
        if (index) {
            if (glyphIndex) {
                *glyphIndex = index;
            }
            break;
        }
    }
    result = get_glyph_from_index(face, index);
    
    return result;
}

internal void
init_hb_font(HBFont *font, FTFont *ftFont)
{
    if (ftFont->fontCount) {
        if (ftFont->fonts[0].regular.memory.size) {
            font->regularBuffer = hb_buffer_create();
            font->regularFont = hb_ft_font_create(ftFont->fonts[0].regular.face, NULL);
        }
        if (ftFont->fonts[0].bold.memory.size) {
            font->boldBuffer = hb_buffer_create();
            font->boldFont = hb_ft_font_create(ftFont->fonts[0].bold.face, NULL);
        }
    }
}

internal void
draw_glyph(Image *screen, u32 xStart, u32 yStart, FT_Bitmap *glyph, v4 modColour = V4(1, 1, 1, 1))
{
    i_expect(glyph->pixel_mode == FT_PIXEL_MODE_GRAY);
    
    u8 *glyphAt = glyph->buffer;
    for (u32 y = yStart; (y < (yStart + glyph->rows)) && (y < screen->height); ++y)
    {
        u8 *glyphRow = glyphAt;
        for (u32 x = xStart; (x < (xStart + glyph->width)) && (x < screen->width); ++x)
        {
            u8 gray = glyphRow[x - xStart];
            v4 pixel = V4((f32)gray, (f32)gray, (f32)gray, (f32)gray);
            pixel = linear1_from_sRGB255(pixel);
            pixel.rgb = pixel.rgb * (1.0f - modColour.a) + modColour.rgb;
            draw_pixel(screen, x, y, pixel);
        }
        glyphAt += glyph->pitch;
    }
}


DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(FTState) <= state->memorySize);
    
    //v2 size = V2((f32)image->width, (f32)image->height);
    
    FTState *ftState = (FTState *)state->memory;
    if (!state->initialized)
    {
        // ftState->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        ftState->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        
        b32 error = FT_Init_FreeType(&ftState->ftLibrary);
        if (error)
        {
            fprintf(stderr, "Could not load the freetype library!\n");
            state->closeProgram = true;
        }
        else
        {
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
            
            init_ft_font(ftState->ftLibrary, &ftState->font, 32.0f);
            add_dual_ft_font(&ftState->font, regularFont, boldFont);
            add_dual_ft_font(&ftState->font, regularCJKFont, boldCJKFont);
            finalize_ft_font(&ftState->font);
            
            init_hb_font(&ftState->hbFont, &ftState->font);
        }
        
        state->initialized = true;
    }
    
    i_expect(ftState->font.fontCount);
    i_expect(ftState->font.fonts[0].regular.memory.size);
    i_expect(ftState->font.fonts[0].bold.memory.size);
    
    if (keyboard->keys[Key_Space].isPressed) {
        ftState->font.hasKerning = !ftState->font.hasKerning;
        fprintf(stdout, "This font has%s got kerning support!\n", ftState->font.hasKerning ? "" : "n't");
    }
    
    if (keyboard->keys[Key_H].isPressed) {
        ftState->useHarfbuzz = !ftState->useHarfbuzz;
        fprintf(stdout, "%sing Harfbuzz to support lygitures\n",
                ftState->useHarfbuzz ? "U" : "Not u");
    }
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    {
        // NOTE(michiel): Loading glyphs
        u32 startX = 20;
        s32 x = startX;
        s32 y = startX + s32_from_f32_round(ftState->font.lineAdvance);
        
        String testStr = static_string(FONT_START_BOLD "Hello, world!" FONT_START_REGULAR " <And greetings... \xCE\x94 \xCF\x80 +=->\n"
                                       "\xEB\xB8\xA0\xEB\xB8\xA1\xEB\xB8\xA2\xEB\xB8\xA3\n"
                                       "And some more text for\nVAWA jij and fij ffi\n"
                                       "\xD0\x81 \xD1\xAA");
        
        b32 useRegular = true;
        while (testStr.size)
        {
            if ((testStr.data[0] == FONT_REGULAR_TOKEN) &&
                (testStr.size > 1) &&
                (testStr.data[1] == FONT_REGULAR_TOKEN))
            {
                useRegular = true;
                testStr.data += 2;
                testStr.size -= 2;
            }
            else if ((testStr.data[0] == FONT_BOLD_TOKEN) &&
                     (testStr.size > 1) &&
                     (testStr.data[1] == FONT_BOLD_TOKEN))
            {
                useRegular = false;
                testStr.data += 2;
                testStr.size -= 2;
            }
            else
            {
                String text = testStr;
                
                for (u32 idx = 1; idx < text.size; ++idx)
                {
                    if ((text.data[idx - 1] == FONT_REGULAR_TOKEN) &&
                        (text.data[idx] == FONT_REGULAR_TOKEN))
                    {
                        text.size = idx - 1;
                        break;
                    }
                    else if ((text.data[idx - 1] == FONT_BOLD_TOKEN) &&
                             (text.data[idx] == FONT_BOLD_TOKEN))
                    {
                        text.size = idx - 1;
                        break;
                    }
                }
                
                if (ftState->useHarfbuzz)
                {
                    hb_buffer_t *buffer = useRegular ? ftState->hbFont.regularBuffer : ftState->hbFont.boldBuffer;
                    hb_font_t   *hbFont = useRegular ? ftState->hbFont.regularFont : ftState->hbFont.boldFont;
                    hb_buffer_add_utf8(buffer, (char *)text.data, text.size, 0, text.size);
                    hb_buffer_guess_segment_properties(buffer);
                    hb_shape(hbFont, buffer, 0, 0);
                    
                    u32 glyphCount = hb_buffer_get_length(buffer);
                    hb_glyph_info_t *glyph_info    = hb_buffer_get_glyph_infos(buffer, 0);
                    hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buffer, 0);
                    
                    for (u32 idx = 0; idx < glyphCount; ++idx)
                    {
                        u32 glyphIndex = glyph_info[idx].codepoint;
                        if (glyph_info[idx].cluster < text.size &&
                            text.data[glyph_info[idx].cluster] == '\n')
                        {
                            x = startX;
                            y += s32_from_f32_round(ftState->font.lineAdvance);
                        }
                        else
                        {
                            s32 x_offset = (glyph_pos[idx].x_offset + 0x20) >> 6;
                            s32 y_offset = (glyph_pos[idx].y_offset + 0x20) >> 6;
                            s32 x_advance = (glyph_pos[idx].x_advance + 0x20) >> 6;
                            s32 y_advance = (glyph_pos[idx].y_advance + 0x20) >> 6;
                            
                            FT_GlyphSlot slot = get_glyph_from_index(useRegular ? 
                                                                     ftState->font.fonts[0].regular.face :
                                                                     ftState->font.fonts[0].bold.face,
                                                                     glyphIndex);
                            
                            draw_glyph(image, x + x_offset + slot->bitmap_left, y + y_offset - slot->bitmap_top, &slot->bitmap, V4(1, 1, 0, 1));
                            x += x_advance;
                            y += y_advance;
                        }
                    }
                    
                    hb_buffer_reset(buffer);
                }
                else
                {
                    u32 prevGlyph = 0;
                    for (u32 idx = 0; idx < text.size;)
                    {
                        u32 codepoint = 0;
                        u32 advance = get_code_point_from_utf8(text.data + idx, &codepoint);
                        if (!advance) {
                            fprintf(stderr, "Invalid utf character 0x%02X\n", text.data[idx]);
                            advance = 1;
                        }
                        idx += advance;
                        
                        u32 glyphIndex = 0;
                        if (codepoint == '\n') {
                            x = startX;
                            y += s32_from_f32_round(ftState->font.lineAdvance);
                        } else {
                            FT_GlyphSlot slot = get_glyph_from_codepoint(&ftState->font, codepoint, !useRegular, &glyphIndex);
                            
#if 0                            
                            if (ftState->font.hasKerning && prevGlyph && glyphIndex)
                            {
                                FT_Vector kerning = {};
                                error = FT_Get_Kerning(face, prevGlyph, glyphIndex, FT_KERNING_DEFAULT, &kerning);
                                if (error) {
                                    fprintf(stderr, "Could not get the kerning info needed between %u and %u\n",
                                            prevGlyph, glyphIndex);
                                }
                                
                                x += kerning.x >> 6;
                            }
#endif
                            
                            draw_glyph(image, x + slot->bitmap_left, y - slot->bitmap_top,
                                       &slot->bitmap, V4(1, 1, 0, 1));
                            x += slot->advance.x >> 6;
                        }
                        
                        prevGlyph = glyphIndex;
                    }
                }
                
                testStr.data += text.size;
                testStr.size -= text.size;
            }
        }
    }
    
    ftState->prevMouseDown = mouse.mouseDowns;
    ftState->seconds += dt;
    ++ftState->ticks;
    if (ftState->seconds > 1.0f)
    {
        ftState->seconds -= 1.0f;
        fprintf(stdout, "Ticks: %4u | Time: %fms\n", ftState->ticks,
                1000.0f / (f32)ftState->ticks);
        ftState->ticks = 0;
    }
}
