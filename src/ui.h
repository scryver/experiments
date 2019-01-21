struct UIID
{
    void *value[2];
};
global UIID gNullUIID;

enum VariableKind
{
    Var_None,
    Var_Boolean,
    Var_Integer,
    Var_Unsigned,
    Var_Float,
    Var_String,
    Var_CString,
};
  struct Variable
{
    VariableKind kind;
    union
    {
        b32 b;
        s64 i;
        u64 u;
        f64 f;
        String s;
        char *c;
};
    };

struct UIButton
{
    UIID   id;
    String label;
};

struct UICheckbox
{
    UIID id;
    b32  checked;
};

struct UISlider
{
    UIID     id;
     Variable value;
    Variable maxValue;
};

struct UIItem;

enum LayoutKind
{
    Layout_None,
    Layout_Vertical,
Layout_Horizontal,
    //Layout_Grid,
    };
struct UILayout
{
    LayoutKind kind;
    
    v2u origin;
    v2u maxSize;
    
    u32 padding;
    
    u32 itemCount;
    UIItem *firstItem;
    UIItem *lastItem;
};

enum UIItemKind
{
    UIItem_None,
    UIItem_Layout,
    UIItem_Button,
    UIItem_Checkbox,
    UIItem_Slider,
    };
struct UIItem
{
    UIItemKind kind;
    UIItem    *next;
    
    union
    {
        UILayout layout;
    UIButton button;
        UICheckbox checkbox;
        UISlider slider;
        };
};

struct UIState
{
    Arena uiMem;
    b32 layedOut;
    
    Image *screen;
    BitmapFont font;
    v2 mouse;
    b32 clicked;
    
    UIID activeItem;
    UIID hotItem;
    
    u32 maxItemCount;
    u32 itemCount;
    UIItem *items;
};

internal inline UIID
create_uiid(void *a, void *b)
{
    UIID result = {0};
    result.value[0] = a;
    result.value[1] = b;
    return result;
}

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

internal inline b32
ui_match_id(UIState *state, UIID id, Rectangle2u rect)
{
    b32 result = false;
    if ((rect.min.x <= state->mouse.x) && (state->mouse.x < rect.max.x) &&
        (rect.min.y <= state->mouse.y) && (state->mouse.y < rect.max.y))
    {
        result = true;
        state->hotItem = id;
    }
    else if (state->hotItem == id) 
    {
        state->hotItem = gNullUIID;
    }
    
    if (result && state->clicked)
    {
        state->activeItem = id;
}
    #if 1
    // NOTE(michiel): Disable this if you want the single end update
    else if (state->activeItem == id)
    {
        state->activeItem = gNullUIID;
    }
#endif
    
    return result;
}

internal void
init_ui(UIState *state, Image *screen, u32 maxItems, char *fontFilename)
{
    state->screen = screen;
    
    ApiFile fontFile = read_entire_file(fontFilename);
    i_expect(fontFile.content.size);
    unpack_font(fontFile.content.data, &state->font);
    
    state->maxItemCount = maxItems;
    state->itemCount = 0;
    state->items = arena_allocate_array(&state->uiMem, UIItem, maxItems);
}

internal void
ui_layout_button(UIState *state, UIButton *button, Rectangle2u rect,
                 v4 background = {1, 1, 1, 1}, v4 textColour = {0, 0, 0, 1})
{
       ui_match_id(state, button->id, rect);
    
    if (state->hotItem == button->id)
    {
        background.r *= 1.2f;
        background.g *= 1.2f;
        background.b *= 1.2f;
    }
    
    if (state->activeItem == button->id)
    {
        rect.min.x += 2;
        rect.min.y += 2;
        rect.max.x -= 1;
        rect.max.y -= 1;
        background.r *= 1.3f;
        background.g *= 1.3f;
        background.b *= 1.1f;
    }
    
    v2u dim = get_dim(rect);
    fill_rectangle(state->screen, rect.min.x, rect.min.y, dim.x, dim.y, background);
    outline_rectangle(state->screen, rect.min.x, rect.min.y, dim.x, dim.y, V4(0, 0, 0, 1));
    draw_text(&state->font, state->screen, rect.min.x + 8, rect.min.y + 3, 
              button->label, textColour);
}

internal void
ui_layout_checkbox(UIState *state, UICheckbox *checkbox, Rectangle2u rect,
                 v4 background = {1, 1, 1, 1}, v4 checkColour = {0, 0, 0, 1})
{
       ui_match_id(state, checkbox->id, rect);
    
    if (state->hotItem == checkbox->id)
    {
        background.r *= 1.2f;
        background.g *= 1.2f;
        background.b *= 1.2f;
    }
    
    if (state->activeItem == checkbox->id)
    {
        rect.min.x += 2;
        rect.min.y += 2;
        rect.max.x -= 1;
        rect.max.y -= 1;
        background.r *= 1.3f;
        background.g *= 1.3f;
        background.b *= 1.1f;
    }
    
    v2u dim = get_dim(rect);
    fill_rectangle(state->screen, rect.min.x, rect.min.y, dim.x, dim.y, background);
    outline_rectangle(state->screen, rect.min.x, rect.min.y, dim.x, dim.y, V4(0, 0, 0, 1));
        if (checkbox->checked)
        {
        fill_rectangle(state->screen, rect.min.x + 5, rect.min.y + 5, 
                       dim.x - 10, dim.y - 10, checkColour);
        }
}

internal void
ui_layout_slider(UIState *state, UISlider *slider, Rectangle2u rect, LayoutKind dir)
{
    /*
    s32 id;
    v2 pos;
    v2 dim;
    s32 maxValue;
    s32 *value;
    v4 colour;
    
    if (0)
    {
        b32 changed = false;
        
        b32 horizontal = dir == Layout_Horizontal;
        f32 handleSize = (f32)(horizontal ? h : w) * 0.5f;
        f32 trackLength = (f32)(horizontal ? w : h) - (f32)(horizontal ? h : w);
        
        f32 one_over_max = 1.0f / (f32)maxValue;
        f32 value01 = clamp01((f32)(*value) * one_over_max);
        f32 dimMod = (value01 - 0.5f) * trackLength;
        v4 backColour = clamp01(colour - V4(0.3f, 0.3f, 0.3f, 0.0f));
        
        mouse_selection(state, id, rect_center_dim(pos, dim));
        keyboard_selection(state, id, pos, dim);
        sync_keyboard_and_mouse_selection(state, id);
        
        draw_shadow(state, pos, dim);
        
        v4 drawColour = ui_state_colour(state, id, colour);
        push_rect(state->renderer, state->transform, V3(pos, -12.0f), dim, backColour);
        if (horizontal)
        {
            push_rect(state->renderer, state->transform, V3(pos.x + dimMod, pos.y, -10.0f), V2(handleSize, handleSize), drawColour);
        }
        else
        {
            push_rect(state->renderer, state->transform, V3(pos.x, pos.y + dimMod, -10.0f), V2(handleSize, handleSize), drawColour);
        }
        
        if (state->kbItem == id)
        {
            if (select_next(state))
            {
                // NOTE(michiel): Do nothing
            }
            else if (is_down(state, KB_Up, false))
            {
                if (*value < maxValue)
                {
                    ++(*value);
                    changed = true;
                }
            }
            else if (is_down(state, KB_Down, false))
            {
                if (*value > 0)
                {
                    --(*value);
                    changed = true;
                }
            }
        }
        
        if (!changed)
        {
            state->lastWidget = id;
            
            f32 mouseSetter = (f32)(*value) * one_over_max;
            if (state->activeItem == id)
            {
                if (horizontal)
                {
                    mouseSetter = clamp01_map_to_range(0.0f, (state->mousePos.x - pos.x) + 0.5f * trackLength, trackLength);
                }
                else
                {
                    mouseSetter = clamp01_map_to_range(0.0f, (state->mousePos.y - pos.y) + 0.5f * trackLength, trackLength);
                }
            }
            else if ((state->hotItem == id) && (state->inFocus))
            {
                mouseSetter += (f32)state->mouseScroll * one_over_max;
            }
            if (mouseSetter < 0.0f)
            {
                mouseSetter = 0.0f;
            }
            if (mouseSetter > 1.0f)
            {
                mouseSetter = 1.0f;
            }
            s32 v = (s32)(mouseSetter * (f32)maxValue);
            if (v != *value)
            {
                *value = v;
                changed = true;
            }
        }
        
        return changed;
    }
    */
}

internal void
ui_layout_layout(UIState *state, UILayout *layout)
{
    u32 itemCount = layout->itemCount;
    if (itemCount)
    {
        u32 itemXSize = layout->maxSize.x - 2 * layout->padding;
        u32 itemYSize = layout->maxSize.y - 2 * layout->padding;
        if (layout->kind == Layout_Horizontal) {
            itemXSize -= (itemCount - 1) * layout->padding;
            itemXSize /= itemCount;
        } else {
            i_expect(layout->kind == Layout_Vertical);
            itemYSize -= (itemCount - 1) * layout->padding;
            itemYSize /= itemCount;
        }
        u32 xOffset = layout->origin.x + layout->padding;
        u32 yOffset = layout->origin.y + layout->padding;
        
        // TODO(michiel): Make everything more rect based (get_dim is redundant for now)
        Rectangle2u rect = rect_from_dim(xOffset, yOffset, itemXSize, itemYSize);
        v2u rectDim = get_dim(rect);
        
        for (UIItem *item = layout->firstItem; item; item = item->next)
        {
            switch (item->kind)
            {
                case UIItem_Layout:
                {
                    UILayout *innerLayout = &item->layout;
                    if (innerLayout->maxSize.x > rectDim.x) {
                        innerLayout->maxSize.x = rectDim.x;
                    }
                    if (innerLayout->maxSize.y > rectDim.y) {
                        innerLayout->maxSize.y = rectDim.y;
                    }
                    innerLayout->origin.x = rect.min.x;
                    innerLayout->origin.y = rect.min.y;
                    ui_layout_layout(state, innerLayout);
                } break;
                
                case UIItem_Button:
                {
                    ui_layout_button(state, &item->button, rect, V4(0.5f, 0.5f, 0.1f, 1));
                } break;
                
                case UIItem_Checkbox:
                {
                    ui_layout_checkbox(state, &item->checkbox, rect, V4(0.5f, 0.5f, 0.1f, 1));
                } break;
                
                case UIItem_Slider:
                {
                    ui_layout_slider(state, &item->slider, rect,
                                     layout->kind == Layout_Horizontal ?
                                     Layout_Vertical : Layout_Horizontal);
                } break;
                
                INVALID_DEFAULT_CASE;
            }
            
            if (layout->kind == Layout_Horizontal) {
                rect.min.x += itemXSize + layout->padding;
                rect.max.x = rect.min.x + itemXSize;
            } else {
                rect.min.y += itemYSize + layout->padding;
                rect.max.y = rect.min.y + itemYSize;
            }
        }
    }
}

internal UIItem *
get_next_item(UIState *state, UIItemKind kind)
{
    i_expect(state->itemCount < state->maxItemCount);
    UIItem *result = state->items + state->itemCount++;
    result->kind = kind;
    return result;
}

internal UIItem *
get_last_item(UIState *state)
{
    i_expect(state->itemCount);
    UIItem *result = state->items + state->itemCount - 1;
    return result;
}

internal UILayout *
ui_begin(UIState *state, LayoutKind kind, u32 x, u32 y, u32 width, u32 height,
                u32 padding = 0)
{
    state->itemCount = 0;
    state->layedOut = false;
    
    UIItem *baseLayout = get_next_item(state, UIItem_Layout);
    baseLayout->next = 0;
    
    UILayout *layout = &baseLayout->layout;
    
    layout->kind = kind;
    layout->origin.x = x;
    layout->origin.y = y;
    layout->maxSize.x = width;
    layout->maxSize.y = height;
    layout->padding = padding;
    layout->itemCount = 0;
    layout->firstItem = 0;
    layout->lastItem = 0;
    
    return layout;
}

internal void
ui_end(UIState *state)
{
    UIItem *baseLayout = state->items;
    i_expect(baseLayout->kind == UIItem_Layout);

#if 0
    // NOTE(michiel): This can be used, but I liked it more when the widget itself does the
    //   clearing, it seems more natural
    if (!state->clicked)
    {
        state->activeItem = gNullUIID;
    }
    #endif

    if (!state->layedOut) 
    {
        ui_layout_layout(state, &baseLayout->layout);
        state->layedOut = true;
    }
    }

internal void
add_item_to_layout(UIState *state, UILayout *layout, UIItem *uiItem)
{
    if (!layout->firstItem) {
        layout->firstItem = uiItem;
    }
    if (layout->lastItem) {
    layout->lastItem->next = uiItem;
    }
layout->lastItem = uiItem;
    ++layout->itemCount;
        }

internal UILayout *
ui_layout(UIState *state, UILayout *layout, LayoutKind kind, u32 padding = 0,
          v2u maxSize = V2U(U32_MAX, U32_MAX))
{
    UIItem *uiItem = get_next_item(state, UIItem_Layout);
    uiItem->layout.kind = kind;
    uiItem->layout.maxSize = maxSize;
    uiItem->layout.padding = padding;
    uiItem->layout.itemCount = 0;
    uiItem->layout.firstItem = 0;
    uiItem->layout.lastItem = 0;
    add_item_to_layout(state, layout, uiItem);
    return &uiItem->layout;
}

internal UIButton *
ui_button(UIState *state, UILayout *layout, char *label)
{
    UIItem *uiItem = get_next_item(state, UIItem_Button);
    uiItem->button.id = create_uiid(layout, &uiItem->button);
    uiItem->button.label = string(label);
    add_item_to_layout(state, layout, uiItem);
    return &uiItem->button;
}

internal UICheckbox *
ui_checkbox(UIState *state, UILayout *layout, b32 checked)
{
    UIItem *uiItem = get_next_item(state, UIItem_Checkbox);
    uiItem->checkbox.id = create_uiid(layout, &uiItem->checkbox);
    uiItem->checkbox.checked = checked;
    add_item_to_layout(state, layout, uiItem);
    return &uiItem->checkbox;
}

internal UISlider *
ui_slider(UIState *state, UILayout *layout, Variable value, Variable max)
{
    UIItem *uiItem = get_next_item(state, UIItem_Slider);
    uiItem->slider.id = create_uiid(layout, &uiItem->slider);
    uiItem->slider.value = value;
    uiItem->slider.maxValue = max;
    add_item_to_layout(state, layout, uiItem);
    return &uiItem->slider;
}

//
internal inline b32
ui_id_is_clicked(UIState *state, UIID id)
{
    return (!state->clicked && (state->activeItem == id) && (state->hotItem == id));
}

internal inline b32
ui_button_is_clicked(UIState *state, UIButton *button)
{
    return ui_id_is_clicked(state, button->id);
}

internal inline b32
ui_checkbox_is_clicked(UIState *state, UICheckbox *checkbox)
{
    return ui_id_is_clicked(state, checkbox->id);
}

internal inline b32
ui_button_imm(UIState *state, UILayout *layout, char *label)
{
    return ui_button_is_clicked(state, ui_button(state, layout, label));
}

internal inline b32
ui_checkbox_imm(UIState *state, UILayout *layout, b32 checked)
{
    return ui_checkbox_is_clicked(state, ui_checkbox(state, layout, checked));
}

#if 0
internal b32
textfield(UIState *state, s32 id, v2 pos, v2 dim, char *text, v4 colour = V4(0.1f, 0.1f, 0.1f, 1.0f))
{
    if ((dim.x == 0) || (dim.y == 0))
    {
        if (dim.x == 0)
        {
            Rectangle2 stringSize = get_text_size(state->renderer, state->transform, state->font, text);
            dim.x = get_width(stringSize) + state->font->scale * 5.0f;
        }
        if (dim.y == 0)
        {
            dim.y = get_text_height(state->font, text);
        }
    }
    
    state->font->baseP = V3(pos, state->font->baseP.z);
    state->font->baseP.x -= 0.5f * dim.x - state->font->scale * 5.0f;
    state->font->baseP.y += 0.5f * dim.y - 0.667f * state->font->scale * get_line_advance_for(state->font->info);
    state->font->offsetAt = state->font->baseP;
    
    u32 length = string_length(text);
    b32 changed = false;
    
    mouse_selection(state, id, rect_center_dim(pos, dim + V2(8, 8)));
    keyboard_selection(state, id, pos, dim);
    sync_keyboard_and_mouse_selection(state, id);
    
    if (state->kbItem == id)
    {
        if (select_next(state))
        {
            // NOTE(michiel): Do nothing
        }
        else if (is_down(state, KB_Backspace, 0.06f))
        {
            if (length > 0)
            {
                --length;
                text[length] = 0;
                changed = true;
            }
        }
        else // if (!state->keysProcessed[0])
        {
            // TODO(michiel): Rewrite key processing in platform layer
            enum KeyIndex letterNums[] =
            {
                KB_A, KB_B, KB_C, KB_D, KB_E, KB_F, KB_G, KB_H, KB_I, KB_J, KB_K, KB_L, KB_M,
                KB_N, KB_O, KB_P, KB_Q, KB_R, KB_S, KB_T, KB_U, KB_V, KB_W, KB_X, KB_Y, KB_Z,
                KB_0, KB_1, KB_2, KB_3, KB_4, KB_5, KB_6, KB_7, KB_8, KB_9,
                KB_Space, KB_Comma, KB_Dot, KB_Slash, KB_Semicolon, KB_Quote,
                KB_LeftBracket, KB_RightBracket, KB_Backslash, KB_Minus, KB_Equal, KB_Enter,
            };
            char letterNumsChar[] = "abcdefghijklmnopqrstuvwxyz0123456789 ,./;'[]\\-=\n";
            char shiftNumsChar[]  = "ABCDEFGHIJKLMNOPQRSTUVWXYZ)!@#$%^&*( <>?:\"{}|_+\n";
            for (u32 letterIndex = 0; letterIndex < ARRAY_COUNT(letterNums); ++letterIndex)
            {
                if (is_down(state, letterNums[letterIndex], 0.3f))
                {
                    if (is_down(state->input, KB_LeftShift) ||
                        is_down(state->input, KB_RightShift))
                    {
                        text[length++] = shiftNumsChar[letterIndex];
                        if (shiftNumsChar[letterIndex] == '%')
                        {
                            // NOTE(michiel): Escape character
                            text[length++] = '%';
                        }
                    }
                    else
                    {
                        text[length++] = letterNumsChar[letterIndex];
                    }
                    text[length] = 0;
                    changed = true;
                }
            }
        }
    }
    
    draw_shadow(state, pos, dim);
    
    v3 position = V3(pos, -10.0f);
    v4 drawColour = ui_state_colour(state, id, colour);
    drawColour.a = 0.5f;
    push_rect(state->renderer, state->transform, position, dim, drawColour);
    
    {
        TransientClipRect t = TransientClipRect(state->renderer, get_clip_rect(state->renderer, state->transform, position, dim));
        draw_text(state->renderer, state->transform, state->font, text);
        
        if ((state->kbItem == id) && ((u32)(state->now * 4.0f) & 0x1))
        {
            f32 advance = 0.0f;
            if (length > 1)
            {
                advance = state->font->scale * get_horizontal_advance_for_pair(state->font->info, state->font->font,
                                                                               (u32)text[length - 1], (u32)'_');
            }
            
            state->font->offsetAt.x += advance;
            draw_text(state->renderer, state->transform, state->font, "_");
        }
    }
    
    state->lastWidget = id;
    
    return changed;
}

internal b32
textfield(UIState *state, s32 id, v2 pos, f32 width, u32 lines, char *text, v4 colour = V4(0.1f, 0.1f, 0.1f, 1.0f))
{
    v2 dim = V2(width, (f32)lines * state->font->scale * get_line_advance_for(state->font->info));
    return textfield(state, id, pos, dim, text, colour);
}
#endif
