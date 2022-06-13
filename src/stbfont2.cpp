#include "interface.h"
DRAW_IMAGE(draw_image);

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

#define STBTT_ifloor(x)    ((s32)floor32(x))
#define STBTT_iceil(x)     ((s32)ceil32(x))
#define STBTT_sqrt(x)      square_root(x)
#define STBTT_pow(x, y)    pow32(x, y)
#define STBTT_fmod(x, y)   modulus(x, y)
#define STBTT_cos(x)       cos_pi(x)
#define STBTT_acos(x)      acos_pi(x)
#define STBTT_fabs(x)      absolute(x)

//#define STBTT_malloc(x, u) sub_alloc(u, x, no_clear_memory_alloc())
//#define STBTT_free(x, u)   sub_dealloc(u, x)

#define STBTT_assert(x)    i_expect_simple(x)

#define STBTT_strlen(x)    string_length(x)

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "../libberdip/stb_truetype.h"

#include "main.cpp"

#include "../libberdip/std_file.c"
#include "../libberdip/drawing.cpp"
//#include "../libberdip/suballoc.cpp"

#define FONT_REGULAR_TOKEN   0x06
#define FONT_BOLD_TOKEN      0x15
#define FONT_START_REGULAR   "\x06"
#define FONT_START_BOLD      "\x15"

struct StbFont
{
    Buffer memory;
    stbtt_fontinfo info;

    s32 ascender;
    s32 descender;
    s32 lineGap;
};

struct StbState
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    u32 prevMouseDown;

    //SubAllocator allocator;

    StbFont font;
};

internal s32
get_line_advance(StbFont *font)
{
    s32 result = font->ascender - font->descender + font->lineGap;
    return result;
}

internal void
init_stb_font(StbFont *font, String filename)
{
    i_expect(font->memory.size == 0);
    i_expect(font->memory.data == 0);

    MemoryAllocator tempAlloc = {};
    initialize_arena_allocator(gMemoryArena, &tempAlloc);
    font->memory = gFileApi->read_entire_file(&tempAlloc, filename);
    if (font->memory.size)
    {
        s32 numFonts = stbtt_GetNumberOfFonts(font->memory.data);
        if (numFonts > 0)
        {
            int offset = stbtt_GetFontOffsetForIndex(font->memory.data, 0);
            if (stbtt_InitFont(&font->info, font->memory.data, offset) != 0)
            {
                //font->stbFont.userdata = font->allocator;
                fprintf(stdout, "Found %d font%s in '%.*s'\n", numFonts, numFonts == 1 ? "" : "s", STR_FMT(filename));

                stbtt_GetFontVMetrics(&font->info, &font->ascender, &font->descender, &font->lineGap);
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
}

internal void
blow_up(Image *image)
{
    // NOTE(michiel): Move backwards through the rows to not overwrite the source
    u32 *src = image->pixels + (image->height / 2 - 1) * image->rowStride;
    u32 *dst = image->pixels + (image->height - 2) * image->rowStride;
    for (s32 y = 0; y < image->height / 2; ++y)
    {
        // NOTE(michiel): Move backwards through the row for the same reason
        u32 *srcRow = src + image->width / 2 - 1;
        u32 *dstRow0 = dst + image->width - 1;
        u32 *dstRow1 = dstRow0 + image->rowStride;
        for (s32 x = 0; x < image->width / 2; ++x)
        {
            *dstRow0-- = *srcRow;
            *dstRow0-- = *srcRow;
            *dstRow1-- = *srcRow;
            *dstRow1-- = *srcRow--;
        }
        src -= image->rowStride;
        dst -= image->rowStride * 2;
    }
}

internal Image
load_glyph_bitmap(StbFont *font, u32 codePoint, s32 pixelHeight)
{
    Image result = {};

    s32 width, height;
    s32 xOffset, yOffset;
    f32 scale = stbtt_ScaleForPixelHeight(&font->info, pixelHeight);
    u8 *monoBitmap = stbtt_GetCodepointBitmap(&font->info, scale, scale, (s32)codePoint,
                                              &width, &height, &xOffset, &yOffset);

    result.width = width;
    result.height = height;
    result.rowStride = width;

    fprintf(stderr, "Bitmap '%c' size: %d x %d\n", codePoint, result.width, result.height);

    result.pixels = arena_allocate_array(gMemoryArena, u32, result.height * result.width, default_memory_alloc());

    u8 *source = monoBitmap;
    // NOTE(michiel): Account for border
    u32 *destRow = result.pixels;

    for (s32 y = 0; y < height; ++y)
    {
        u32 *dest = destRow;
        for (s32 x = 0; x < width; ++x)
        {
            f32 gray = (f32)(*source++ & 0xFF);
            v4 texel = {255.0f, 255.0f, 255.0f, gray};
            texel = linear1_from_sRGB255(texel);
            texel.rgb *= texel.a;
            texel = sRGB255_from_linear1(texel);

            *dest++ = (((u32)(texel.a + 0.5f) << 24) |
                       ((u32)(texel.r + 0.5f) << 16) |
                       ((u32)(texel.g + 0.5f) << 8) |
                       ((u32)(texel.b + 0.5f) << 0));
        }

        destRow += result.rowStride;
    }

    stbtt_FreeBitmap(monoBitmap, 0);

    return result;
}

global Image testLetters[26];

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(StbState) <= state->memorySize);

    //v2 size = V2((f32)image->width, (f32)image->height);

    s32 pixelScale = 32;

    StbState *stbState = (StbState *)state->memory;
    if (!state->initialized)
    {
        stbState->randomizer = random_seed_pcg(time(0), 192658612912ULL);

        String regularFont = static_string("/usr/share/fonts/truetype/freefont/FreeSans.ttf");
        init_stb_font(&stbState->font, regularFont);

        for (u32 index = 0; index < 26; ++index)
        {
            testLetters[index] = load_glyph_bitmap(&stbState->font, 'a' + index, pixelScale);
        }

        state->initialized = true;
    }

#if 0
    // TODO(michiel): Rename
    Image renderImg = {0};
    renderImg.width  = image->width  / 2;
    renderImg.height = image->height / 2;
    renderImg.rowStride = image->rowStride;
    renderImg.pixels   = image->pixels;
#endif

    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));

    f32 scale = stbtt_ScaleForPixelHeight(&stbState->font.info, pixelScale);

    s32 x = 0;
    s32 y = 0;
    for (u32 index = 0; index < 26; ++index)
    {
        Image *letter = testLetters + index;
        if (x + letter->width > image->width / 2)
        {
            y = s32_from_f32_ceil(scale * (f32)get_line_advance(&stbState->font) + (f32)y);
            x = 0;
        }
        draw_image(image, x, y, letter, V4(1, 1, 0, 1));
        x += letter->width;
    }

    blow_up(image);

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
