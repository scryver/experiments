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

    Var_Constant = 0x100,
};
struct Variable
{
    enum32(VariableKind) kind;
    union
    {
        void *var;
        b32 b;
        u32 u;
        s32 i;
        f32 f;
        String s;
        char *c;
    };
};

internal inline Variable
var(VariableKind kind, void *value)
{
    Variable result = {kind, value};
    return result;
}

internal inline Variable
var_bool(b32 *value)
{
    Variable result = var(Var_Boolean, value);
    return result;
}

internal inline Variable
var_bool(b32 value)
{
    Variable result = {0};
    result.kind = Var_Boolean | Var_Constant;
    result.b = value;
    return result;
}

internal inline Variable
var_unsigned(u32 *value)
{
    Variable result = var(Var_Unsigned, value);
    return result;
}

internal inline Variable
var_unsigned(u32 value)
{
    Variable result = {0};
    result.kind = Var_Unsigned | Var_Constant;
    result.u = value;
    return result;
}

internal inline Variable
var_signed(s32 *value)
{
    Variable result = var(Var_Integer, value);
    return result;
}

internal inline Variable
var_signed(s32 value)
{
    Variable result = {0};
    result.kind = Var_Integer | Var_Constant;
    result.i = value;
    return result;
}

internal inline Variable
var_float(f32 *value)
{
    Variable result = var(Var_Float, value);
    return result;
}

internal inline Variable
var_float(f32 value)
{
    Variable result = {0};
    result.kind = Var_Float | Var_Constant;
    result.f = value;
    return result;
}

internal inline Variable
var_string(String *value)
{
    Variable result = var(Var_String, value);
    return result;
}

internal inline Variable
var_string(String value)
{
    Variable result = {0};
    result.kind = Var_String | Var_Constant;
    result.s = value;
    return result;
}

internal inline Variable
var_cstring(char **value)
{
    Variable result = var(Var_CString, value);
    return result;
}

internal inline Variable
var_cstring(char *value)
{
    Variable result = {0};
    result.kind = Var_CString | Var_Constant;
    result.c = value;
    return result;
}

internal inline f32
map01(Variable val, Variable max)
{
    f32 result = 0.0f;
    i_expect(max.kind & Var_Constant);
    i_expect(val.kind != max.kind);
    i_expect((Var_Constant | val.kind) == max.kind);
    switch (val.kind)
    {
        case Var_Integer:
        {
            s32 value = *(s32 *)val.var;
            result = (((f32)value / (f32)max.i) + 1.0f) / 2.0f;
        } break;

        case Var_Unsigned:
        {
            u32 value = *(u32 *)val.var;
            result = (f32)value / (f32)max.u;
        } break;

        case Var_Float:
        {
            f32 value = *(f32 *)val.var;
            result = value / max.f;
        } break;

        INVALID_DEFAULT_CASE;
    }
    return clamp01(result);
}

internal inline void
unmap01(f32 val01, Variable *val, Variable max)
{
    i_expect(max.kind & Var_Constant);
    i_expect(val->kind != max.kind);
    i_expect((Var_Constant | val->kind) == max.kind);
    switch (val->kind)
    {
        case Var_Integer:
        {
            *(s32 *)val->var = (s32)((val01 * 2.0f - 1.0f) * (f32)max.i);
        } break;

        case Var_Unsigned:
        {
            *(u32 *)val->var = (u32)(val01 * (f32)max.u);
        } break;

        case Var_Float:
        {
            *(f32 *)val->var = val01 * max.f;
        } break;

        INVALID_DEFAULT_CASE;
    }
}

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

struct UIText
{
    UIID   id;
    String text;
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

    f32 itemWeight;
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
    UIItem_Text,
};
struct UIItem
{
    UIItemKind kind;
    UIItem    *next;
    f32 weight;

    union
    {
        UILayout layout;
        UIButton button;
        UICheckbox checkbox;
        UISlider slider;
        UIText text;
    };
};

struct UIState
{
    MemoryArena uiMem;
    b32 layedOut;

    Image *screen;
    BitmapFont font;
    v2 mouse;
    s32 mouseScroll;
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
    if (((f32)rect.min.x <= state->mouse.x) && (state->mouse.x < (f32)rect.max.x) &&
        ((f32)rect.min.y <= state->mouse.y) && (state->mouse.y < (f32)rect.max.y))
    {
        result = true;
        state->hotItem = id;
    }
    else if (state->hotItem == id)
    {
        state->hotItem = gNullUIID;
    }

    if ((state->activeItem == gNullUIID) && result && state->clicked)
    {
        state->activeItem = id;
    }

    return result;
}

internal void
init_ui(UIState *state, Image *screen, u32 maxItems, String fontFilename)
{
    state->screen = screen;

    MemoryAllocator tempAlloc = {};
    initialize_arena_allocator(gMemoryArena, &tempAlloc);
    Buffer fontFile = gFileApi->read_entire_file(&tempAlloc, fontFilename);
    i_expect(fontFile.size);
    unpack_font(&tempAlloc, fontFile.data, &state->font);

    state->maxItemCount = maxItems;
    state->itemCount = 0;
    state->items = arena_allocate_array(&state->uiMem, UIItem, maxItems, default_memory_alloc());
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
    //outline_rectangle(state->screen, rect.min.x, rect.min.y, dim.x, dim.y, V4(0, 0, 0, 1));
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
    //outline_rectangle(state->screen, rect.min.x, rect.min.y, dim.x, dim.y, V4(0, 0, 0, 1));
    if (checkbox->checked)
    {
        fill_rectangle(state->screen, rect.min.x + 5, rect.min.y + 5,
                       dim.x - 10, dim.y - 10, checkColour);
    }
}

internal void
ui_layout_slider(UIState *state, UISlider *slider, Rectangle2u rect, LayoutKind dir,
                 v4 colour = {1, 1, 1, 1})
{
    ui_match_id(state, slider->id, rect);

    b32 hot = false;
    if (state->hotItem == slider->id)
    {
        hot = true;
        colour.r *= 1.2f;
        colour.g *= 1.2f;
        colour.b *= 1.2f;
    }

    b32 active = false;
    if (state->activeItem == slider->id)
    {
        active = true;
        colour.r *= 1.3f;
        colour.g *= 1.3f;
        colour.b *= 1.1f;
    }

    v2u dim = get_dim(rect);

    b32 horizontal = dir == Layout_Horizontal;
    f32 handleSize = (f32)(horizontal ? dim.y : dim.x) * 0.5f;
    v2u handle = V2U((u32)handleSize, (u32)handleSize);

    f32 trackLength = (f32)(horizontal ? dim.x : dim.y) - (f32)(horizontal ? dim.y : dim.x);

    f32 value01 = map01(slider->value, slider->maxValue);
    u32 dimModVal = (u32)(value01 * trackLength);
    v2u dimMod = (horizontal ? V2U(rect.min.x + dimModVal, rect.min.y)
                  : V2U(rect.min.x, rect.min.y + (trackLength - dimModVal)));
    dimMod += handle.x / 2;

    v4 backColour = clamp01(colour - V4(0.3f, 0.3f, 0.3f, 0.0f));
    v4 trackColour = clamp01(colour - V4(0.5f, 0.5f, 0.5f, 0.0f));

    fill_rectangle(state->screen, rect.min.x, rect.min.y, dim.x, dim.y, backColour);
    fill_rectangle(state->screen, rect.min.x + handle.x / 2, rect.min.y + handle.y / 2,
                   dim.x - handle.x, dim.y - handle.y, trackColour);
    fill_rectangle(state->screen, dimMod.x, dimMod.y, (u32)handleSize, (u32)handleSize, colour);

    if (active)
    {
        if (horizontal)
        {
            value01 = (state->mouse.x - rect.min.x - handle.x / 2) / (f32)(dim.x - handle.x);
        }
        else
        {
            value01 = 1.0f - ((state->mouse.y - rect.min.y - handle.y / 2) / (f32)(dim.y - handle.y));
        }
    }
    else if (hot)
    {
        value01 += (f32)state->mouseScroll * 0.01f;
    }
    value01 = clamp01(value01);

    unmap01(value01, &slider->value, slider->maxValue);
}

internal void
ui_layout_text(UIState *state, UIText *text, Rectangle2u rect,
               v4 background = {1, 1, 1, 1}, v4 textColour = {0, 0, 0, 1})
{
    ui_match_id(state, text->id, rect);

    if (state->hotItem == text->id)
    {
        background.r *= 1.2f;
        background.g *= 1.2f;
        background.b *= 1.2f;
    }

    if (state->activeItem == text->id)
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
    outline_rectangle(state->screen, rect.min.x, rect.min.y, dim.x, dim.y, V4(0, 0, 0, 0.5f));
    draw_text(&state->font, state->screen, rect.min.x + 8, rect.min.y + 3,
              text->text, textColour);
}

internal void
ui_layout_layout(UIState *state, UILayout *layout)
{
    u32 itemCount = layout->itemCount;
    if (itemCount)
    {
#if 0
        s32 itemXSize = layout->maxSize.x - 2 * layout->padding;
        s32 itemYSize = layout->maxSize.y - 2 * layout->padding;
        if (layout->kind == Layout_Horizontal) {
            itemXSize -= (itemCount - 1) * layout->padding;
            itemXSize /= itemCount;
        } else {
            i_expect(layout->kind == Layout_Vertical);
            itemYSize -= (itemCount - 1) * layout->padding;
            itemYSize /= itemCount;
        }
        s32 xOffset = layout->origin.x + layout->padding;
        s32 yOffset = layout->origin.y + layout->padding;
#endif

        // TODO(michiel): Make everything more rect based (get_dim is redundant for now)
        Rectangle2u rect = {};
        rect.min.x = layout->origin.x;
        rect.min.y = layout->origin.y;
        rect.max.x = layout->origin.x + layout->maxSize.x;
        rect.max.y = layout->origin.y + layout->maxSize.y;

        //Rectangle2u rect = rect_from_dim(xOffset, yOffset, itemXSize, itemYSize);
        //v2s rectDim = get_dim(rect);

        fill_rectangle(state->screen, layout->origin.x, layout->origin.y, layout->maxSize.x, layout->maxSize.y, V4(0.1f, 0.1f, 0.1f, 0.5f));

        //fprintf(stdout, "Layout: (%d, %d) %dx%d\n", rect.min.x, rect.min.y, rect.max.x - rect.min.x, rect.max.y - rect.min.y);

        f32 oneOverWeight = 1.0f / layout->itemWeight;
        for (UIItem *item = layout->firstItem; item; item = item->next)
        {
            f32 ratio = item->weight * oneOverWeight;
            if (layout->kind == Layout_Horizontal)
            {
                rect.max.x = rect.min.x + s32_from_f32_round(ratio * (f32)layout->maxSize.x);
            }
            else
            {
                rect.max.y = rect.min.y + s32_from_f32_round(ratio * (f32)layout->maxSize.y);
            }

            switch (item->kind)
            {
                case UIItem_Layout:
                {
                    UILayout *innerLayout = &item->layout;
                    v2u rectDim = get_dim(rect);

                    if ((innerLayout->maxSize.x > rectDim.x) ||
                        (innerLayout->maxSize.x < 0)) {
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
                    //fprintf(stdout, "Button: (%d, %d) %dx%d\n", rect.min.x, rect.min.y, rect.max.x - rect.min.x, rect.max.y - rect.min.y);
                    ui_layout_button(state, &item->button, rect, V4(0.5f, 0.5f, 0.1f, 1));
                } break;

                case UIItem_Checkbox:
                {
                    //fprintf(stdout, "Checkbox: (%d, %d) %dx%d\n", rect.min.x, rect.min.y, rect.max.x - rect.min.x, rect.max.y - rect.min.y);
                    ui_layout_checkbox(state, &item->checkbox, rect, V4(0.5f, 0.5f, 0.1f, 1));
                } break;

                case UIItem_Slider:
                {
                    //fprintf(stdout, "Slider: (%d, %d) %dx%d\n", rect.min.x, rect.min.y, rect.max.x - rect.min.x, rect.max.y - rect.min.y);
                    ui_layout_slider(state, &item->slider, rect,
                                     layout->kind == Layout_Horizontal ?
                                     Layout_Vertical : Layout_Horizontal,
                                     V4(0.5f, 0.5f, 0.1f, 1));
                } break;

                case UIItem_Text:
                {
                    //fprintf(stdout, "Text: (%d, %d) %dx%d\n", rect.min.x, rect.min.y, rect.max.x - rect.min.x, rect.max.y - rect.min.y);
                    ui_layout_text(state, &item->text, rect, V4(0.5f, 0.5f, 0.1f, 1));
                } break;

                INVALID_DEFAULT_CASE;
            }

            if (layout->kind == Layout_Horizontal) {
                rect.min.x = rect.max.x;
            } else {
                rect.min.y = rect.max.y;
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
    layout->itemWeight = 0.0f;
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

    if (!state->clicked)
    {
        state->activeItem = gNullUIID;
    }

    if (!state->layedOut)
    {
        ui_layout_layout(state, &baseLayout->layout);
        state->layedOut = true;
    }
}

internal void
add_item_to_layout(UIState *state, UILayout *layout, UIItem *uiItem,
                   f32 weight = 1.0f)
{
    if (!layout->firstItem) {
        layout->firstItem = uiItem;
    }
    if (layout->lastItem) {
        layout->lastItem->next = uiItem;
    }
    i_expect(weight != 0.0f);
    uiItem->weight = weight;
    layout->itemWeight += uiItem->weight;
    layout->lastItem = uiItem;
    ++layout->itemCount;
}

internal UILayout *
ui_layout(UIState *state, UILayout *layout, LayoutKind kind, u32 padding = 0, f32 weight = 1.0f,
          v2u maxSize = V2U(U32_MAX, U32_MAX))
{
    UIItem *uiItem = get_next_item(state, UIItem_Layout);
    uiItem->layout.kind = kind;
    uiItem->layout.maxSize = maxSize;
    uiItem->layout.padding = padding;
    uiItem->layout.itemWeight = 0.0f;
    uiItem->layout.itemCount = 0;
    uiItem->layout.firstItem = 0;
    uiItem->layout.lastItem = 0;
    add_item_to_layout(state, layout, uiItem, weight);
    return &uiItem->layout;
}

internal UIButton *
ui_button(UIState *state, UILayout *layout, char *label, f32 weight = 1.0f)
{
    UIItem *uiItem = get_next_item(state, UIItem_Button);
    uiItem->button.id = create_uiid(layout, &uiItem->button);
    uiItem->button.label = string(label);
    add_item_to_layout(state, layout, uiItem, weight);
    return &uiItem->button;
}

internal UICheckbox *
ui_checkbox(UIState *state, UILayout *layout, b32 checked, f32 weight = 1.0f)
{
    UIItem *uiItem = get_next_item(state, UIItem_Checkbox);
    uiItem->checkbox.id = create_uiid(layout, &uiItem->checkbox);
    uiItem->checkbox.checked = checked;
    add_item_to_layout(state, layout, uiItem, weight);
    return &uiItem->checkbox;
}

internal UISlider *
ui_slider(UIState *state, UILayout *layout, Variable value, Variable max,
          f32 weight = 1.0f)
{
    UIItem *uiItem = get_next_item(state, UIItem_Slider);
    uiItem->slider.id = create_uiid(layout, &uiItem->slider);
    uiItem->slider.value = value;
    uiItem->slider.maxValue = max;
    add_item_to_layout(state, layout, uiItem, weight);
    return &uiItem->slider;
}

internal UISlider *
ui_slider(UIState *state, UILayout *layout, u32 *value, u32 maxValue, f32 weight = 1.0f)
{
    return ui_slider(state, layout, var_unsigned(value), var_unsigned(maxValue), weight);
}

internal UISlider *
ui_slider(UIState *state, UILayout *layout, s32 *value, s32 maxValue, f32 weight = 1.0f)
{
    return ui_slider(state, layout, var_signed(value), var_signed(maxValue), weight);
}

internal UISlider *
ui_slider(UIState *state, UILayout *layout, f32 *value, f32 maxValue, f32 weight = 1.0f)
{
    return ui_slider(state, layout, var_float(value), var_float(maxValue), weight);
}

internal UIText *
ui_text(UIState *state, UILayout *layout, String text, f32 weight = 1.0f)
{
    UIItem *uiItem = get_next_item(state, UIItem_Text);
    uiItem->text.id = create_uiid(layout, &uiItem->text);
    uiItem->text.text = text;
    add_item_to_layout(state, layout, uiItem, weight);
    return &uiItem->text;
}

internal UIText *
ui_text(UIState *state, UILayout *layout, char *text, f32 weight = 1.0f)
{
    UIText *result = ui_text(state, layout, string(text), weight);
    return result;
}

//
internal b32
ui_button_is_pressed(UIState *state, UIButton *button)
{
    // NOTE(michiel): Return true if the current button is pressed
    return (state->clicked && (state->activeItem == button->id) && (state->hotItem == button->id));
}

internal b32
ui_checkbox_is_clicked(UIState *state, UICheckbox *checkbox)
{
    // NOTE(michiel): Only return true on mouse release
    return (!state->clicked && (state->activeItem == checkbox->id) && (state->hotItem == checkbox->id));
}

internal b32
ui_slider_is_set(UIState *state, UISlider *slider)
{
    return ((!state->clicked && (state->activeItem == slider->id)) ||
            ((state->mouseScroll != 0) && (state->hotItem == slider->id)));
}

internal b32
ui_text_is_clicked(UIState *state, UIText *text)
{
    // NOTE(michiel): Only return true on mouse release
    return (!state->clicked && (state->activeItem == text->id) && (state->hotItem == text->id));
}


internal b32
ui_button_imm(UIState *state, UILayout *layout, char *label, f32 weight = 1.0f)
{
    return ui_button_is_pressed(state, ui_button(state, layout, label, weight));
}

internal b32
ui_checkbox_imm(UIState *state, UILayout *layout, b32 checked, f32 weight = 1.0f)
{
    return ui_checkbox_is_clicked(state, ui_checkbox(state, layout, checked, weight));
}

internal b32
ui_slider_imm(UIState *state, UILayout *layout, u32 *value, u32 maxValue, f32 weight = 1.0f)
{
    return ui_slider_is_set(state, ui_slider(state, layout, value, maxValue, weight));
}

internal b32
ui_slider_imm(UIState *state, UILayout *layout, s32 *value, s32 maxValue, f32 weight = 1.0f)
{
    return ui_slider_is_set(state, ui_slider(state, layout, value, maxValue, weight));
}

internal b32
ui_slider_imm(UIState *state, UILayout *layout, f32 *value, f32 maxValue, f32 weight = 1.0f)
{
    return ui_slider_is_set(state, ui_slider(state, layout, value, maxValue, weight));
}

internal b32
ui_text_imm(UIState *state, UILayout *layout, String text, f32 weight = 1.0f)
{
    return ui_text_is_clicked(state, ui_text(state, layout, text, weight));
}

internal b32
ui_text_imm(UIState *state, UILayout *layout, char *text, f32 weight = 1.0f)
{
    b32 result = ui_text_imm(state, layout, string(text), weight);
    return result;
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
