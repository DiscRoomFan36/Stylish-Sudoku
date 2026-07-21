
#ifndef LAYOUT_H_
#define LAYOUT_H_

// for UI stuff. TODO make into a .h

#include "raylib.h"
#include "raymath.h"

#include "Bested.h"
#include "raylib_helpers.c"

#include "input.h"
#include "sound.h"
#include "theme.h"


typedef enum {
    LAY_UP,
    LAY_DOWN,
    LAY_LEFT,
    LAY_RIGHT,
} Layout_Direction;

internal const char *layout_direction_to_string(Layout_Direction direction);


typedef enum {
    LAY_IN_PIXELS,
    LAY_IN_FRACTION,
} Layout_Amount_Kind;

typedef struct {
    Layout_Amount_Kind kind;
    // changes use based on kind.
    f64 value;
} Layout_Amount;

internal const char *layout_amount_to_string(Layout_Amount amount);
internal Layout_Amount Amount(Layout_Amount_Kind kind, f64 value);

internal Rectangle layout_take_from(Rectangle *layout, Layout_Direction direction, Layout_Amount amount);


// colors like this are better to lerp / smoothly transition from.
typedef Vector4 Float_Color;

internal Float_Color color_to_float_color(Color color);
internal Color color_from_float_color(Float_Color float_color);

internal void move_float_color_towards_color(Float_Color *float_color, Color color, f64 distance);


typedef struct UI_Button_Data {
    String text;
    const char *text_c_str;

    f64 time_since_last_hovered;
    f64 time_since_last_clicked;

    struct {
        Float_Color background_color;
        Float_Color boarder_color;
        Float_Color text_color;
    } current_theme;

    // useful to check if this was just made this frame.
    bool was_created_this_frame;

    Source_Code_Location source_code_location;

    // probably wasteful to have 1 allocator per button,
    // but where else are we gonna get the memory for the text?
    Arena *allocator;
} UI_Button_Data;

typedef Array(UI_Button_Data) UI_Button_Data_Array;


internal UI_Button_Data *get_ui_button_data(String text, Source_Code_Location source_code_location);

#define ui_button(text, layout_ptr) ui_button_impl(text, layout_ptr, Get_Source_Code_Location())

internal bool ui_button_impl(const char *_text, Rectangle *layout, Source_Code_Location source_code_location);

#endif // LAYOUT_H_




#ifdef LAYOUT_IMPLEMENTATION

const char *layout_direction_to_string(Layout_Direction direction) {
    switch (direction) {
    case LAY_UP   : return "Lay_Up";
    case LAY_DOWN : return "Lay_Down";
    case LAY_LEFT : return "Lay_Left";
    case LAY_RIGHT: return "Lay_Right";

    default: return temp_sprintf("(UNKNOWN{%d})", direction);
    }
}


const char *layout_amount_to_string(Layout_Amount amount) {
    const char *kind_string = temp_sprintf("(UNKNOWN{%d})", amount.kind);
    switch (amount.kind) {
    case LAY_IN_PIXELS   : { kind_string = "Lay_In_Pixels";   } break;
    case LAY_IN_FRACTION : { kind_string = "Lay_In_Fraction"; } break;
    }

    return temp_sprintf("(Layout_Amount){ .kind = %s, .value = %.3f }", kind_string, amount.value);
}

Layout_Amount Amount(Layout_Amount_Kind kind, f64 value) {
    Layout_Amount result = { .kind = kind, .value = value };
    return result;
}

Rectangle layout_take_from(Rectangle *layout, Layout_Direction direction, Layout_Amount amount) {
    f64 real_amount = amount.value;

    f64 width_or_height = (direction == LAY_UP || direction == LAY_DOWN) ? layout->height : layout->width;

    switch (amount.kind) {
    case LAY_IN_PIXELS: {
        real_amount = amount.value;
    } break;
    case LAY_IN_FRACTION: {
        real_amount = width_or_height * amount.value;
    } break;

    default: UNREACHABLE();
    }

    if (real_amount > width_or_height) {
        // TODO maybe provide source code location of caller?
        log_error("layout ran out of space, with direction %s, amount %s", layout_direction_to_string(direction), layout_amount_to_string(amount));

        // clamp this, the next time someone tries to
        // get something it will have 0 width / height.
        //
        // hope you can handle that.
        real_amount = width_or_height;
    }

    switch (direction) {
    case LAY_UP: {
        Rectangle result = *layout;
        result.height = real_amount;

        // shift the layout,
        layout->y      += real_amount;
        layout->height -= real_amount;

        return result;
    }

    case LAY_DOWN: {
        Rectangle result = *layout;
        result.y      = layout->y      + real_amount;
        result.height = layout->height - real_amount;

        // shift the layout,
        layout->height = real_amount;

        return result;
    }

    case LAY_LEFT: {
        Rectangle result = *layout;
        result.width = real_amount;

        // shift the layout,
        layout->x     += real_amount;
        layout->width -= real_amount;

        return result;
    }

    case LAY_RIGHT: {
        Rectangle result = *layout;
        result.x     = layout->x     + real_amount;
        result.width = layout->width - real_amount;

        // shift the layout,
        layout->width = real_amount;

        return result;
    }

    default: UNREACHABLE();
    }
}



Float_Color color_to_float_color(Color color) {
    Float_Color float_color = {
        .x = color.r / 255.0f,
        .y = color.g / 255.0f,
        .z = color.b / 255.0f,
        .w = color.a / 255.0f,
    };
    return float_color;
}

Color color_from_float_color(Float_Color float_color) {
    // clamp values from [0..1]
    Float_Color clamped = {
        .x = Clamp(float_color.x, 0, 1),
        .y = Clamp(float_color.y, 0, 1),
        .z = Clamp(float_color.z, 0, 1),
        .w = Clamp(float_color.w, 0, 1),
    };
    Color color = {
        .r = Round(clamped.x * 255),
        .g = Round(clamped.y * 255),
        .b = Round(clamped.z * 255),
        .a = Round(clamped.w * 255),
    };
    return color;
}

void move_float_color_towards_color(Float_Color *float_color, Color color, f64 distance) {
    Float_Color color_as_float_color = color_to_float_color(color);
    Float_Color new_float_color = Vector4MoveTowards(*float_color, color_as_float_color, distance);
    *float_color = new_float_color;
}


// TODO get an allocator with pool_get()
global_variable UI_Button_Data_Array global_ui_button_data_array = ZEROED;

UI_Button_Data *get_ui_button_data(String text, Source_Code_Location source_code_location) {
    for (u64 i = 0; i < global_ui_button_data_array.count; i++) {
        UI_Button_Data *this = &global_ui_button_data_array.items[i];

        if (!source_code_location_eq(source_code_location, this->source_code_location)) continue;

        // TODO use # parseing to make it so we can give buttons different names,
        if (!String_Eq(text, this->text)) continue;

        // this is the same button
        this->was_created_this_frame = false;
        return this;
    }

    // make something new.
    UI_Button_Data *new_button_data = Array_Add(&global_ui_button_data_array, 1, true);

    new_button_data->allocator = Scratch_Get();

    new_button_data->text = String_Duplicate(text, .allocator = new_button_data->allocator, .null_terminate = true);
    new_button_data->text_c_str = text.data;

    new_button_data->source_code_location = source_code_location;

    // the mirror of `this->was_created_this_frame = false;`
    new_button_data->was_created_this_frame = true;

    printf("New button! [%s]\n", new_button_data->text_c_str);

    return new_button_data;
}

bool ui_button_impl(const char *_text, Rectangle *layout, Source_Code_Location source_code_location) {
    Input *input = get_input();
    Theme *theme = get_theme();

    UI_Button_Data *ui_button_data = get_ui_button_data(S(_text), source_code_location);

    if (ui_button_data->was_created_this_frame) {
        // do some setup.
        //
        // otherwise these would start from 0,
        ui_button_data->current_theme.background_color = color_to_float_color(theme->ui.button.base.background_color);
        ui_button_data->current_theme.boarder_color    = color_to_float_color(theme->ui.button.base.boarder_color   );
        ui_button_data->current_theme.text_color       = color_to_float_color(theme->ui.button.base.text_color      );
    }

    // something to scale the entire ui by,
    f64 size_factor = 1;
    {
        Context *context = get_context();
        s32 window_size = Min(context->window_width, context->window_height);

        // TODO magic constants
        size_factor = Remap(window_size, 0, 800, 0.1, 1);
    }

    // ever7thing is effected by the theme.
    f64 font_size      = theme->ui.button.font_size       * size_factor;
    f64 text_padding   = theme->ui.button.text_padding    * size_factor;
    f64 button_padding = theme->ui.between_button_padding * size_factor;
    f64 roundness      = theme->ui.button.roundness; // roundness is not effected by size.
    f64 boarder_size   = theme->ui.button.boarder_size    * size_factor;


    Font_And_Size font_and_size = GetFontWithSize(font_size);

    Vector2 text_size = MeasureTextEx(font_and_size.font, ui_button_data->text_c_str, font_and_size.size, 0);

    // only add button padding to the bottom.
    Rectangle button_total_area = layout_take_from(layout, LAY_UP, Amount(LAY_IN_PIXELS, text_size.y + text_padding*2 + button_padding));

    Rectangle button_bounds = button_total_area;
    button_bounds.height -= button_padding;


    // TODO draw differently if hovered / clicked.
    bool button_is_hovered = CheckCollisionPointRec(input->mouse.pos, button_bounds);
    bool button_is_clicked = button_is_hovered && input->mouse.left.clicked;

    if (button_is_clicked) {
        // TODO do we want buttons to capture this?
        //
        // if this button is before the sudoku in processing order, the selection will not go away when this is clicked.
        //
        // input->mouse.left.clicked = false;
        //
        // maybe we want a function?
        // capture_click(MOUSE_LEFT);
    }

    { // move the current theme towards the new theme if they are different.
        Button_Ui_Theme *button_theme = &theme->ui.button.base;
        if (button_is_hovered) button_theme = &theme->ui.button.hovered;
        if (button_is_clicked) button_theme = &theme->ui.button.clicked;

        if (!Mem_Eq(&ui_button_data->current_theme, button_theme, sizeof(ui_button_data->current_theme))) {
            // not the same, we need to move towards the new theme.
            if (button_is_clicked) {
                // click events happen for only 1 frame, so we need to snap towards this theme...
                //
                // maybe we can just turn this speed up super high?
                ui_button_data->current_theme.background_color = color_to_float_color(button_theme->background_color);
                ui_button_data->current_theme.boarder_color    = color_to_float_color(button_theme->boarder_color   );
                ui_button_data->current_theme.text_color       = color_to_float_color(button_theme->text_color      );

            } else {

                // every second, move 0.05 towards the target color.
                //
                // colors are from [0..1] in rgba, so the max distance something could be is 4,
                // and would take 80 seconds to move from transparent black, to full white...
                //
                // a better solution probably exists, but this is good enough for now.
                const f64 factor = 0.05 * (input->time.dt * TARGET_FPS);
                // not the best solution...
                move_float_color_towards_color(&ui_button_data->current_theme.background_color, button_theme->background_color, factor);
                move_float_color_towards_color(&ui_button_data->current_theme.boarder_color,    button_theme->boarder_color,    factor);
                move_float_color_towards_color(&ui_button_data->current_theme.text_color,       button_theme->text_color,       factor);

                //
                // i think there was a talk by casey in the tower, (a ThePrimeagen thing), where he talked about
                // animation systems, i think the code i have now is a dead end. they way he suggested was
                // to make a map? or something that has values from 0 to 1, where every frame they get
                // decreased, except the one you want to go up, want then the output is a blend of
                // the results in that map.
                //
                // so, all that said,
                //
                // TODO re-watch that section and implement something like that.
                //
            }
        }
    }

    // it is important that the segments be both:
    //   a. the same
    //        for obvious reasons.
    //   b. above 4
    //        when below 4, raylib will calculate the number of segments needed
    //        by itself, and these 2 function calls can produce different results
    //        for what the segments need to be.)
    const s32 segments = 5;
    DrawRectangleRounded       (button_bounds, roundness, segments,               color_from_float_color(ui_button_data->current_theme.background_color));
    DrawRectangleRoundedLinesEx(button_bounds, roundness, segments, boarder_size, color_from_float_color(ui_button_data->current_theme.boarder_color));

    DrawTextCentered(font_and_size, ui_button_data->text_c_str, RectangleCenter(button_bounds), color_from_float_color(ui_button_data->current_theme.text_color));

    return button_is_clicked;
}

#endif // LAYOUT_IMPLEMENTATION
