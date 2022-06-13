struct MnistSet
{
    u32 count;
    u8 *labels;
    Image *images;
};

internal inline u32
msb_to_lsb32(u32 msb)
{
    u32 result;
    result = (((msb & 0x000000FF) << 24) |
              ((msb & 0x0000FF00) << 8) |
              ((msb & 0x00FF0000) >> 8) |
              ((msb & 0xFF000000) >> 24));
    return result;
}

internal MnistSet
parse_mnist(String labelPath, String imagePath)
{
    MnistSet result = {};

    MemoryAllocator tempAlloc = {};
    initialize_arena_allocator(gMemoryArena, &tempAlloc);
    Buffer labels = gFileApi->read_entire_file(&tempAlloc, labelPath);
    i_expect(labels.size > 0);
    Buffer images = gFileApi->read_entire_file(&tempAlloc, imagePath);
    i_expect(images.size > 0);

    u8 *l = labels.data;
    i_expect(*l++ == 0x00);
    i_expect(*l++ == 0x00);
    i_expect(*l++ == 0x08);
    i_expect(*l++ == 0x01);

    u8 *i = images.data;
    i_expect(*i++ == 0x00);
    i_expect(*i++ == 0x00);
    i_expect(*i++ == 0x08);
    i_expect(*i++ == 0x03);

    u32 labelCount = msb_to_lsb32(*(u32 *)l);
    l += 4;

    u32 imageCount = msb_to_lsb32(*(u32 *)i);
    i += 4;
    u32 imageRows = msb_to_lsb32(*(u32 *)i);
    i += 4;
    u32 imageCols = msb_to_lsb32(*(u32 *)i);
    i += 4;

    i_expect(labelCount == imageCount);
    i_expect(imageRows == 28);
    i_expect(imageCols == 28);

    result.count = labelCount;
    result.labels = l;
    result.images = arena_allocate_array(gMemoryArena, Image, result.count, default_memory_alloc());
    u32 stride = imageCols * imageRows;
    for (u32 imageIdx = 0; imageIdx < result.count; ++imageIdx)
    {
        Image *image = result.images + imageIdx;
        image->width = imageCols;
        image->height = imageRows;
        image->rowStride = imageCols;
        image->pixels = (u32 *)i;
        i += stride;
    }

    return result;
}

internal void
draw_mnist(Image *screen, u32 xStart, u32 yStart, Image *mnist, v4 modColour = V4(1, 1, 1, 1))
{
    draw_image(screen, xStart, yStart, mnist, modColour);

#if 0
    u8 *imageAt = (u8 *)mnist->pixels;
    for (u32 y = yStart; (y < (yStart + mnist->height)) && (y < screen->height); ++y)
    {
        for (u32 x = xStart; (x < (xStart + mnist->width)) && (x < screen->width); ++x)
        {
            f32 gray = 1.0f - (f32)(*imageAt++) / 255.0f;
            v4 pixel = V4(gray, gray, gray, 1.0f);
            pixel.r *= modColour.r;
            pixel.g *= modColour.g;
            pixel.b *= modColour.b;
            pixel.a *= modColour.a;
            draw_pixel(screen, x, y, pixel);
        }
    }
#endif

}

internal MnistSet
split_mnist(MnistSet *base, u32 splitAt)
{
    i_expect(splitAt > 0);
    i_expect(splitAt < base->count);

    MnistSet result = {};
    result.count = base->count - splitAt;
    result.labels = base->labels + splitAt;
    result.images = base->images + splitAt;

    base->count = splitAt;
    return result;
}

internal TrainingSet
mnist_to_training(MnistSet *mnist)
{
    TrainingSet result = {};
    result.count = mnist->count;
    result.set = arena_allocate_array(gMemoryArena, Training, mnist->count, default_memory_alloc());

    for (u32 tIdx = 0; tIdx < mnist->count; ++tIdx)
    {
        Training *item = result.set + tIdx;
        u8 label = mnist->labels[tIdx];
        Image *image = mnist->images + tIdx;

        item->inputCount = image->width * image->height;
        item->inputs = arena_allocate_array(gMemoryArena, f32, item->inputCount, default_memory_alloc());
        item->outputCount = 10;
        item->outputs = arena_allocate_array(gMemoryArena, f32, item->outputCount, default_memory_alloc());

        for (u32 iIdx = 0; iIdx < item->inputCount; ++iIdx)
        {
            item->inputs[iIdx] = (f32)(((u8 *)image->pixels)[iIdx]) / 255.0f;
        }
        item->outputs[(u32)label] = 1.0f;
    }

    return result;
}
