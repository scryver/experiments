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

struct FTState
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    u32 prevMouseDown;
    
    u32 fontSize;
    b32 hasKerning;
    
    b32 useHarfbuzz;
    
    FT_Library  ftLibrary;
    FT_Face     ftFace;
    
    hb_buffer_t *hbBuffer;
    hb_font_t   *hbFont;
};

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
            //Buffer fontFile = read_entire_file(string("/usr/share/fonts/truetype/freefont/FreeSans.ttf"));
            //Buffer fontFile = read_entire_file(string("/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf"));
            Buffer fontFile = read_entire_file(string("/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"));
            //Buffer fontFile = read_entire_file(string("/usr/share/fonts/truetype/malayalam/Suruma.ttf"));
            //Buffer fontFile = read_entire_file(string("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"));
            //Buffer fontFile = read_entire_file(string("/usr/share/fonts/truetype/nanum/NanumMyeongjo.ttf"));
            //Buffer fontFile = read_entire_file(string("/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf"));
            //Buffer fontFile = read_entire_file(string("/usr/share/fonts/truetype/noto/NotoSans-SemiCondensedBlack.ttf"));
            i_expect(fontFile.size);
            error = FT_New_Memory_Face(ftState->ftLibrary, fontFile.data, fontFile.size,
                                       0, &ftState->ftFace);
            if (error == FT_Err_Unknown_File_Format)
            {
                fprintf(stderr, "Unsupported font format!\n");
            }
            else if (error)
            {
                fprintf(stderr, "Could not open font file!\n");
            }
#if 0
            error = FT_New_Face(ftState->ftLibrary, "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
                                0, &ftState->ftFace);
            if (error == FT_Err_Unknown_File_Format)
            {
                fprintf(stderr, "Unsupported font format!\n");
            }
            else if (error)
            {
                fprintf(stderr, "Could not open font file!\n");
            }
#endif
            i_expect(ftState->ftFace->face_flags & FT_FACE_FLAG_SCALABLE);
            
            ftState->hasKerning = FT_HAS_KERNING(ftState->ftFace);
            fprintf(stdout, "This font has%s got kerning support!\n",
                    ftState->hasKerning ? "" : "n't");
            
            // NOTE(michiel): Horizontal = 0 (same as vertical), Vertical = x pixels
            ftState->fontSize = 32;
            error = FT_Set_Pixel_Sizes(ftState->ftFace, 0, ftState->fontSize);
            if (error)
            {
                fprintf(stderr, "Could not set pixel size!\n");
            }
        }
        
        fprintf(stdout, "Got %ld font face%s\n", ftState->ftFace->num_faces,
                ftState->ftFace->num_faces == 1 ? "" : "s");
        
        ftState->hbBuffer = hb_buffer_create();
        ftState->hbFont = hb_ft_font_create(ftState->ftFace, NULL);
        //hb_font_set_scale(ftState->hbFont, ftState->fontSize, ftState->fontSize);
        
        state->initialized = true;
    }
    
    if (keyboard->keys[Key_Space].isPressed) {
        ftState->hasKerning = !ftState->hasKerning;
        fprintf(stdout, "This font has%s got kerning support!\n",
                ftState->hasKerning ? "" : "n't");
    }
    
    if (keyboard->keys[Key_H].isPressed) {
        ftState->useHarfbuzz = !ftState->useHarfbuzz;
        fprintf(stdout, "%sing Harfbuzz to support lygitures\n",
                ftState->useHarfbuzz ? "U" : "Not u");
    }
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    {
        FT_GlyphSlot slot = ftState->ftFace->glyph;
        
        // NOTE(michiel): Loading glyphs
        u32 startX = 20;
        s32 x = startX;
        s32 y = startX + (ftState->ftFace->size->metrics.height >> 6);
        
        String testStr = static_string("Hello, world!\n"
                                       "\xEB\xB8\xA0\xEB\xB8\xA1\xEB\xB8\xA2\xEB\xB8\xA3\n"
                                       "And some more text for\nVAWA jij and fij ffi");
        
        
        if (ftState->useHarfbuzz)
        {
            hb_buffer_add_utf8(ftState->hbBuffer, (char *)testStr.data, testStr.size, 0, testStr.size);
            hb_buffer_guess_segment_properties(ftState->hbBuffer);
            hb_shape(ftState->hbFont, ftState->hbBuffer, 0, 0);
            
            u32 glyphCount = hb_buffer_get_length(ftState->hbBuffer);
            hb_glyph_info_t *glyph_info    = hb_buffer_get_glyph_infos(ftState->hbBuffer, 0);
            hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(ftState->hbBuffer, 0);
            
            for (u32 idx = 0; idx < glyphCount; ++idx)
            {
                u32 glyphIndex = glyph_info[idx].codepoint;
                if (glyph_info[idx].cluster < testStr.size &&
                    testStr.data[glyph_info[idx].cluster] == '\n')
                {
                    x = startX;
                    y += ftState->ftFace->size->metrics.height >> 6;
                }
                else
                {
                    s32 x_offset = (glyph_pos[idx].x_offset + 0x20) >> 6;
                    s32 y_offset = (glyph_pos[idx].y_offset + 0x20) >> 6;
                    s32 x_advance = (glyph_pos[idx].x_advance + 0x20) >> 6;
                    s32 y_advance = (glyph_pos[idx].y_advance + 0x20) >> 6;
                    
                    b32 error = FT_Load_Glyph(ftState->ftFace, glyphIndex, FT_LOAD_DEFAULT);
                    if (error) {
                        fprintf(stderr, "Could not load glyph %u\n", glyphIndex);
                    }
                    
                    error = FT_Render_Glyph(ftState->ftFace->glyph, FT_RENDER_MODE_NORMAL);
                    if (error) {
                        fprintf(stderr, "Could not render glyph %u\n", glyphIndex);
                    }
                    
                    draw_glyph(image, x + x_offset + slot->bitmap_left, y + y_offset - slot->bitmap_top, &slot->bitmap, V4(1, 1, 0, 1));
                    x += x_advance;
                    y += y_advance;
                }
            }
            
            hb_buffer_reset(ftState->hbBuffer);
            
        }
        else
        {
            u32 prevGlyph = 0;
            for (u32 idx = 0; idx < testStr.size;)
            {
                u32 codepoint = 0;
                u32 advance = get_code_point_from_utf8(testStr.data + idx, &codepoint);
                if (!advance) {
                    fprintf(stderr, "Invalid utf character 0x%02X\n", testStr.data[idx]);
                    advance = 1;
                }
                idx += advance;
                
                u32 glyphIndex = 0;
                if (codepoint == '\n') {
                    x = startX;
                    y += ftState->ftFace->size->metrics.height >> 6;
                } else {
                    glyphIndex = FT_Get_Char_Index(ftState->ftFace, codepoint);
                    b32 error = FT_Load_Glyph(ftState->ftFace, glyphIndex, FT_LOAD_DEFAULT);
                    if (error) {
                        fprintf(stderr, "Could not load glyph %u\n", glyphIndex);
                    }
                    
                    error = FT_Render_Glyph(ftState->ftFace->glyph, FT_RENDER_MODE_NORMAL);
                    if (error) {
                        fprintf(stderr, "Could not render glyph %u\n", glyphIndex);
                    }
                    
                    if (ftState->hasKerning && prevGlyph && glyphIndex)
                    {
                        FT_Vector kerning = {};
                        error = FT_Get_Kerning(ftState->ftFace, prevGlyph, glyphIndex, FT_KERNING_DEFAULT, &kerning);
                        if (error) {
                            fprintf(stderr, "Could not get the kerning info needed between %u and %u\n", prevGlyph, glyphIndex);
                        }
                        
                        x += kerning.x >> 6;
                    }
                    
                    draw_glyph(image, x + slot->bitmap_left, y - slot->bitmap_top,
                               &slot->bitmap, V4(1, 1, 0, 1));
                    x += slot->advance.x >> 6;
                }
                
                prevGlyph = glyphIndex;
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
