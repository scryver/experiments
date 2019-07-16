#include "../libberdip/platform.h"
#include "../libberdip/random.h"
#include "../libberdip/perlin.h"
#include "interface.h"
DRAW_IMAGE(draw_image);

#undef internal
#include <ft2build.h>
#include FT_FREETYPE_H
#define internal static

#include "main.cpp"

#include "../libberdip/std_file.c"
#include "../libberdip/drawing.cpp"

struct FTState
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    u32 prevMouseDown;
    
    b32 hasKerning;
    
    FT_Library  ftLibrary;
    FT_Face     ftFace;
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
            Buffer fontFile = read_entire_file(string("/usr/share/fonts/truetype/freefont/FreeSans.ttf"));
            //Buffer fontFile = read_entire_file(string("/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf"));
            //Buffer fontFile = read_entire_file(string("/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"));
            //Buffer fontFile = read_entire_file(string("/usr/share/fonts/truetype/malayalam/Suruma.ttf"));
            
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
            
            // NOTE(michiel): Horizontal = 0 (same as vertical), Vertical = 32 pixels
            error = FT_Set_Pixel_Sizes(ftState->ftFace, 0, 64);
            if (error)
            {
                fprintf(stderr, "Could not set pixel size!\n");
            }
        }
        
        fprintf(stdout, "Got %ld font face%s\n", ftState->ftFace->num_faces,
                ftState->ftFace->num_faces == 1 ? "" : "s");
        state->initialized = true;
    }
    
    if (keyboard->keys[Key_Space].isPressed) {
        ftState->hasKerning = !ftState->hasKerning;
        fprintf(stdout, "This font has%s got kerning support!\n",
                ftState->hasKerning ? "" : "n't");
    }
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    {
        // NOTE(michiel): Loading glyphs
        u32 startX = 20;
        u32 x = startX;
        u32 y = 64;
        
        String testStr = static_string("Hello, world!\n\xEB\xB8\xA0\nAnd some more text for\nVAWA jij and fij ffi");
        FT_GlyphSlot slot = ftState->ftFace->glyph;
        
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
