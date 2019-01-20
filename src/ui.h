struct UIID
{
    void *value[2];
};

struct UIButton
{
    char *label;
};

enum UIItemKind
{
    UIItem_None,
    UIItem_Button,
};
struct UIItem
{
    UIItemKind kind;
    UIID       id;
    
    union
    {
        UIButton button;
    };
};

struct UILayout
{
    v2u origin;
    v2u maxSize;
    
    u32 maxItemCount;
    u32 itemCount;
    UIItem *items;
};

struct UIState
{
    Image *screen;
    Font font;
    v2 mouse;
    b32 clicked;
    
    UIID activeItem;
    UIID hotItem;
};

internal inline b32
operator ==(UIID a, UIID b)
{
    return (a.value[0] == b.value[0]) && (a.value[1] == b.value[1]);
}

internal inline b32
operator !=(UIID a, UIID b)
{
    return (a.value[0] != b.value[0]) || (a.value[1] != b.value[1]);
}

internal void
ui_layout_button(UIState *state, UIItem *item, u32 x, u32 y, u32 w, u32 h,
                 v4 background = {1, 1, 1, 1}, v4 textColour = {0, 0, 0, 1})
{
    b32 mouseInside = false;
    
    if ((x <= state->mouse.x) && (state->mouse.x < (x + w)) &&
        (y <= state->mouse.y) && (state->mouse.y < (y + h)))
    {
        state->hotItem = item->id;
        mouseInside = true;
        background.r *= 1.2f;
        background.g *= 1.2f;
        background.b *= 1.2f;
    }
    
    if (mouseInside && state->clicked)
    {
        state->activeItem = item->id;
    }
    else if ((state->activeItem == item->id) &&
             !state->clicked)
    {
        state->activeItem.value[0] = 0;
        state->activeItem.value[1] = 0;
    }
    
    if (state->activeItem == item->id)
    {
        x += 2;
        y += 2;
        w -= 3;
        h -= 3;
        background.r *= 1.3f;
        background.g *= 1.3f;
        background.b *= 1.1f;
    }
    
    fill_rectangle(state->screen, x, y, w, h, background);
    //draw_text(&uiState->font, uiState->screen, xStart + 8, yStart + 3, label, textColour);
}

internal UILayout
ui_begin_layout(UIState *state, u32 x, u32 y, u32 width, u32 height)
{
    UILayout result = {};
    
    result.origin.x = x;
    result.origin.y = y;
    result.maxSize.x = width;
    result.maxSize.y = height;
    
    result.maxItemCount = 64;
    result.items = allocate_array(UIItem, result.maxItemCount);
    
    return result;
}

internal void
ui_end_layout(UIState *state, UILayout *layout)
{
    u32 itemCount = layout->itemCount;
    if (itemCount)
    {
    u32 itemXSize = layout->maxSize.x;
    u32 itemYSize = layout->maxSize.y / itemCount;
        u32 xOffset = layout->origin.x;
        u32 yOffset = layout->origin.y;
        for (u32 itemIdx = 0; itemIdx < itemCount; ++itemIdx)
        {
            UIItem *item = layout->items + itemIdx;
            switch (item->kind)
            {
                case UIItem_Button:
                {
                    ui_layout_button(state, item, xOffset, yOffset, itemXSize, itemYSize,
                                     V4(0.5f, 0.5f, 0.1f, 1), V4(0, 0, 0, 1));
                    outline_rectangle(state->screen, xOffset, yOffset, itemXSize, itemYSize,
                                      V4(1, 1, 1, 1));
                    fill_rectangle(state->screen, xOffset + 10, yOffset + 10, 
                                   itemXSize - 20, itemYSize - 20,
                                   V4(0.4f, 0.7f, 0, 1));
                } break;
                
                INVALID_DEFAULT_CASE;
            }
            yOffset += itemYSize;
        }
    // TODO(michiel): Call drawing routines
}
    }

internal b32
ui_button(UIState *state, UILayout *layout, char *label)
{
    i_expect(layout->itemCount < layout->maxItemCount);
    UIItem *uiItem = layout->items + layout->itemCount++;
    uiItem->kind = UIItem_Button;
    uiItem->id.value[0] = layout;
    uiItem->id.value[1] = label;
    uiItem->button.label = label;
    return (state->activeItem == uiItem->id);
}
