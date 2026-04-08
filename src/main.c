
#include <time.h>

#include "Bested.h"
#undef Clamp // TODO we gotta do something about this...




#ifdef PLATFORM_WEB
    #include <emscripten/emscripten.h>
#endif


#include <raylib.h>
#include <raymath.h>

#include "raylib_helpers.c"





typedef struct {
    _Array_Header_;
    s64 *items;
} Int_Array;

// returns -1 if its not in the array
internal s64 index_in_array(Int_Array *array, s64 to_find) {
    for (u64 i = 0; i < array->count; i++) {
        if (array->items[i] == to_find)    return i;
    }
    return -1;
}



typedef struct {
    _Array_Header_;
    Vector2 *items;
} Vector2_Array;



typedef struct {
    const char *file;
    s32 line;
} Source_Code_Location;

// printf helpers
#define SCL_Fmt         "%s:%d:"
#define SCL_Arg(scl)    scl.file, scl.line

#define Get_Source_Code_Location() ( (Source_Code_Location){ .file = __FILE__, .line = __LINE__ } )

bool source_code_location_eq(Source_Code_Location a, Source_Code_Location b) {
    if (a.line != b.line) return false;
    // this should work, the pointer would be the same.
    if (a.file != b.file) return false;
    return true;
}




// helpers and whatnot
#include "theme.h"
#include "input.h"

#include "sound.h"
#include "logging.h"

// sudoku stuff
#include "sudoku_grid.h"
#include "sudoku_solver/sudoku_solver.h"







////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//                              Defines
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


// square for now, change when adding a control panel.
#define INITAL_WINDOW_WIDTH         (16*80)
#define INITAL_WINDOW_HEIGHT        ( 9*80)

#define TARGET_FPS  60





///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//                              Context
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////


typedef struct {
    bool draw_smaller_cell_hitbox;
    bool draw_cursor_position;
    bool draw_color_points;
    bool draw_fps;

    bool draw_layout_areas;

    RenderTexture2D debug_texture;
} Debug_Struct;


typedef struct {
    // all the memory in this program comes from here.
    Arena_Pool pool;
    // resets every frame.
    Arena *temporary_allocator;

    s32 window_width;
    s32 window_height;

    Debug_Struct debug_struct;

    Input input;

    A_Sound_Array               global_sound_array;
    Logged_Message_Array        logged_messages_to_display;


    Sudoku sudoku_base;
    Sudoku *sudoku;

    // TODO is this the right place for this.
    bool in_solve_mode;

    // call get_theme() instead of using this.
    Theme theme_base;

} Context;


global_variable Context __context_base = ZEROED;

internal void   init_context(void);
internal void uninit_context(void);

//
// doing this to help me know where im getting
// the context from in each function.
//
internal Context *get_context(void) { return &__context_base; }

// returns an allocator that gets free'd every frame
internal Arena *get_temporary_allocator(void) { return get_context()->temporary_allocator; }

// so we can separate calls to context for context reasons, and for debugging reasons.
internal Debug_Struct *get_debug_struct() { return &get_context()->debug_struct; }

//
// NOTE. The difference between saying 'Scratch_Get()' and 'Pool_Get(&context->pool)'
//
// 'Scratch_Get()' implies that you will eventually call 'Scratch_Release()'
//
// 'Pool_Get(&context->pool)' says that you intended to hold this Arena for the entire program.
//
internal Arena *Scratch_Get(void);
internal void   Scratch_Release(Arena *scratch);



void init_context(void) {
    Context *context = get_context();

    // just making sure.
    Mem_Zero_Struct(context);

    { // pool
        Mem_Zero_Struct(&context->pool);

        // SDL_RenderCoordinatesFromWindow

        // here we preallocate some buffers, i was having some trouble trying to
        // get emscripten to allocate memory with my arena allocators, this is a
        // hack to give me a bit of room to work with.
        //
        // this allocates 32 MB, might be unecessary.
        //
        // still less than 1 javascript object.
        local_persist u8 buffers[NUM_POOL_ARENAS][1 * MEGABYTE];
        for (u32 i = 0; i < NUM_POOL_ARENAS; i++) {
            Arena_Add_Buffer_As_Storeage_Space(&context->pool.arena_pool[i], buffers[i], sizeof(buffers[i]));
        }

        context->temporary_allocator = Pool_Get(&context->pool);
    }

    context->window_width  = GetScreenWidth();
    context->window_height = GetScreenHeight();

    { // debug
        Debug_Struct *debug_struct = get_debug_struct();
        ASSERT(debug_struct == &context->debug_struct);

        debug_struct->draw_smaller_cell_hitbox  = false;
        debug_struct->draw_cursor_position      = false;
        debug_struct->draw_color_points         = false;
        debug_struct->draw_fps                  = false;
        debug_struct->draw_layout_areas         = false;

        // the perhaps better option would be to make DebugDraw## versions of the draw
        // functions that buffer the commands until later, but that would require
        // making a bunch of extra functions...
        //
        // another option would be to somehow draw rectangles with depth,
        // and make debug stuff the most important.
        debug_struct->debug_texture = LoadRenderTexture(context->window_width, context->window_height);

    #define DebugDraw(draw_call) do { BeginTextureMode(get_debug_struct()->debug_texture); (draw_call);  EndTextureMode(); } while (0)

    }



    Mem_Zero_Struct(&context->input);

    Mem_Zero_Struct(&context->global_sound_array             );
    Mem_Zero_Struct(&context->logged_messages_to_display     );


    context->global_sound_array          .allocator = Pool_Get(&context->pool);
    context->logged_messages_to_display  .allocator = Pool_Get(&context->pool);


    {
        // zero all the things.
        Mem_Zero_Struct(&context->sudoku_base);

        // make sure that load_sudoku(), respects this allocator
        context->sudoku = &context->sudoku_base;
        context->sudoku->undo_buffer.allocator = Pool_Get(&context->pool);
        Array_Reserve(&context->sudoku->undo_buffer, 512); // just give it some room.

        // zero the selection animation array inside the sudoku.
        // also give it an allocator.
        Mem_Zero_Struct(&context->sudoku->selection_animation_array);
        context->sudoku->selection_animation_array.allocator = Pool_Get(&context->pool);
    }


    context->in_solve_mode = true;


    context->theme_base = init_theme();
}
void uninit_context(void) {
    Context *context = get_context();

    Pool_Free_Arenas(&context->pool);
    UnloadRenderTexture(get_debug_struct()->debug_texture);

    // goodbye pointers.
    Mem_Zero_Struct(&context);
}


Arena *Scratch_Get(void)               {
    Context *context = get_context();
    return Pool_Get(&context->pool);
}
void   Scratch_Release(Arena *scratch) {
    Context *context = get_context();
    Pool_Release(&context->pool, scratch);
}



// just a helper function. should go in raylib_helpers.c
internal void toggle_when_pressed(bool *to_toggle, int key) { *to_toggle ^= IsKeyPressed(key); }


// TODO @Bested.h
#define Proper_Mod(x, y) ({ Typeof(y) _y = (y); (((x) % _y) + _y) % _y; })




////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
//                                   Main
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

const char *autosave_path = "./build/autosave.sudoku";

void do_one_frame(void);

int main(void) {
    // spice up your life.
    //
    // TODO make a better random generator
    srand(time(NULL));

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);  // the context contains the window width/height, and stuff updates dynamically.
    SetTraceLogLevel(LOG_WARNING);          // only show warning or worse logs, the console is being spammed in LOG_INFO mode.
    InitWindow(INITAL_WINDOW_WIDTH, INITAL_WINDOW_HEIGHT, "Sudoku");


    init_context();

    Context *context = get_context();


    // this is my own font system, the functions mimic raylib's style.
    InitDynamicFonts("./assets/font/iosevka-light.ttf");
    // this is my own sound system, the functions *DO NOT* mimic raylib's style.
    init_sounds();


    { // load the autosave.
        const char *error = load_sudoku(autosave_path, context->sudoku);
        if (error) {
            log_error("Failed To Load Save '%s': %s", autosave_path, error);
            // TODO make a "clear grid" function.
            FOREACH_IJ_OF_SUDOKU(i, j) {
                Place_Digit(&context->sudoku->grid, i, j, NO_DIGIT_PLACED, .dont_play_sound = true);
            }
        } else {
            log("Successfully loaded save file '%s'", autosave_path);
        }
    }


    if (context->sudoku->undo_buffer.count == 0) {
        Array_Append(&context->sudoku->undo_buffer, context->sudoku->grid);
        context->sudoku->redo_count = 0;
    }
    ASSERT(Sudoku_Grid_Is_The_Same_As_The_Last_Element_In_The_Undo_Buffer(context->sudoku));



#ifdef PLATFORM_WEB

    //
    // function arguments:
    //   1. the function to call in a loop.
    //   2. requested fps, 0 == use javascripts RequestAnimationFrame() function. (its a good function)
    //   3. simulate infinite loop, set to try will make it so the function never returns.
    //
    // I dont have a way to clean anything up after this, but its probably fine.
    //
    // will have to get create when trying to autosave in web version.
    //
    emscripten_set_main_loop(do_one_frame, 0, true);

    UNREACHABLE();

#else

    // probably could hit 144fps, but maybe later.
    //
    // also only run this when building for desktop.
    SetTargetFPS(TARGET_FPS);

    while (!WindowShouldClose()) {
        do_one_frame();
    }
#endif



    bool result = save_sudoku(autosave_path, context->sudoku);
    if (!result) {
        log_error("something went wrong when saveing");
    }

    uninit_sounds();
    UnloadDynamicFonts();

    uninit_context();

    CloseWindow();

    return result ? 0 : 1;
}



Input_Sudoku_Puzzle sudoku_grid_to_input_sudoku_puzzle(Sudoku_Grid *grid) {
    Input_Sudoku_Puzzle input_sudoku_puzzle = ZEROED;
    static_assert(SUDOKU_SIZE == NUM_DIGITS, "these are the same for now, maybe in the future we will have different sized grids for things.");

    FOREACH_IJ_OF_SUDOKU(i, j) {
        Sudoku_Cell *cell = get_cell(grid, i, j);
        u8 digit = Is_Between(cell->digit, 1, 9) ? cell->digit : 0;
        input_sudoku_puzzle.grid.digits[j][i] = digit;
    }

    return input_sudoku_puzzle;
}

// keeps is_selected and is_hovered, but thats it.
void place_digit_grid_into_sudoku_grid(Sudoku_Digit_Grid digit_grid, Sudoku_Grid *grid, bool keep_markings) {
    FOREACH_IJ_OF_SUDOKU(i, j) {
        Sudoku_Cell *cell = get_cell(grid, i, j);

        { // clear the cell. dont like whats going on here.
            // this always must be cleared
            Place_Digit(grid, i, j, NO_DIGIT_PLACED, .dont_play_sound = true);

            if (!keep_markings) {
                cell->color_bitfield = 0;
                cell->certain = 0;
                cell->uncertain = 0;
                cell->digit_placed_in_solve_mode = false;
            }
        }

        u8 digit = digit_grid.digits[j][i];

        if (digit == 0) {
            // ignore this digit. this is just hove the solver functions, we dont want to place '0' digits.
            // log_error("digit is equal to zero, what should we do here?");
        } else {
            Place_Digit(grid, i, j, digit, .dont_play_sound = true);
        }
    }
}



typedef enum {
    LAY_UP,
    LAY_DOWN,
    LAY_LEFT,
    LAY_RIGHT,
} Layout_Direction;

internal const char *layout_direction_to_string(Layout_Direction direction) {
    switch (direction) {
    case LAY_UP   : return "Lay_Up";
    case LAY_DOWN : return "Lay_Down";
    case LAY_LEFT : return "Lay_Left";
    case LAY_RIGHT: return "Lay_Right";

    default: return temp_sprintf("(UNKNOWN{%d})", direction);
    }
}


typedef enum {
    LAY_IN_PIXELS,
    LAY_IN_FRACTION,
} Layout_Amount_Kind;

typedef struct {
    Layout_Amount_Kind kind;
    // changes use based on kind.
    f64 value;
} Layout_Amount;

internal const char *layout_amount_to_string(Layout_Amount amount) {
    const char *kind_string = temp_sprintf("(UNKNOWN{%d})", amount.kind);
    switch (amount.kind) {
    case LAY_IN_PIXELS   : { kind_string = "Lay_In_Pixels";   } break;
    case LAY_IN_FRACTION : { kind_string = "Lay_In_Fraction"; } break;
    }

    return temp_sprintf("(Layout_Amount){ .kind = %s, .value = %.3f }", kind_string, amount.value);
}

internal Layout_Amount Amount(Layout_Amount_Kind kind, f64 value) {
    Layout_Amount result = { .kind = kind, .value = value };
    return result;
}


internal Rectangle layout_take_from(Rectangle *layout, Layout_Direction direction, Layout_Amount amount) {
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


// colors like this are better to lerp / smoothly transition from.
typedef Vector4 Float_Color;

Float_Color color_to_float_color(Color color) {
    Float_Color float_color = {
        .x = color.r / 255.0f,
        .y = color.g / 255.0f,
        .z = color.b / 255.0f,
        .w = color.a / 255.0f,
    };
    return float_color;
}

Color float_color_to_color(Float_Color float_color) {
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


typedef struct {
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

typedef struct {
    _Array_Header_;
    UI_Button_Data *items;
} UI_Button_Data_Array;

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
    UI_Button_Data *new_button_data = Array_Add_Clear(&global_ui_button_data_array, 1);

    new_button_data->allocator = Scratch_Get();

    new_button_data->text = String_Duplicate(new_button_data->allocator, text, .null_terminate = true);
    new_button_data->text_c_str = text.data;

    new_button_data->source_code_location = source_code_location;

    // the mirror of `this->was_created_this_frame = false;`
    new_button_data->was_created_this_frame = true;

    printf("New button! [%s]\n", new_button_data->text_c_str);

    return new_button_data;
}



#define ui_button(text, layout_ptr) ui_button_impl(text, layout_ptr, Get_Source_Code_Location())

internal bool ui_button_impl(const char *_text, Rectangle *layout, Source_Code_Location source_code_location) {
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

    Font_And_Size font_and_size = GetFontWithSize(theme->ui.button.font_size);

    Vector2 text_size = MeasureTextEx(font_and_size.font, ui_button_data->text_c_str, font_and_size.size, 0);

    f64 text_padding   = theme->ui.button.text_padding;
    f64 button_padding = theme->ui.between_button_padding;
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
    DrawRectangleRounded       (button_bounds, theme->ui.button.roundness, segments,                                float_color_to_color(ui_button_data->current_theme.background_color));
    DrawRectangleRoundedLinesEx(button_bounds, theme->ui.button.roundness, segments, theme->ui.button.boarder_size, float_color_to_color(ui_button_data->current_theme.boarder_color));

    DrawTextCentered(font_and_size, ui_button_data->text_c_str, RectangleCenter(button_bounds), float_color_to_color(ui_button_data->current_theme.text_color));

    return button_is_clicked;
}



void do_one_frame() {
    Context      *context      = get_context();
    Theme        *theme        = get_theme();
    Debug_Struct *debug_struct = get_debug_struct();

    // make sure the sudoku grid did not change between frames.
    ASSERT(Sudoku_Grid_Is_The_Same_As_The_Last_Element_In_The_Undo_Buffer(context->sudoku));

    // lol, i just find this line funny.
    Arena_Clear(get_temporary_allocator());

    BeginDrawing();
    ClearBackground(theme->background_color);

    {
        s32 prev_window_width   = context->window_width;
        s32 prev_window_height  = context->window_height;

        context->window_width    = GetScreenWidth();
        context->window_height   = GetScreenHeight();

        if (prev_window_width != context->window_width || prev_window_height != context->window_height) {
            UnloadRenderTexture(debug_struct->debug_texture);
            debug_struct->debug_texture = LoadRenderTexture(context->window_width, context->window_height);
        }
    }



    ////////////////////////////////
    //        User Input
    ////////////////////////////////
    update_input();
    Input *input = get_input();


    { // debug stuff
        // Clear debug to all zero's
        DebugDraw(ClearBackground((Color){0, 0, 0, 0}));

        toggle_when_pressed(&debug_struct->draw_smaller_cell_hitbox,    KEY_F1);
        toggle_when_pressed(&debug_struct->draw_cursor_position,        KEY_F2);
        toggle_when_pressed(&debug_struct->draw_color_points,           KEY_F3);

        toggle_when_pressed(&debug_struct->draw_fps,                    KEY_F4);
        if (debug_struct->draw_fps) DebugDraw(DrawFPS(context->window_width - 100, 10));

        toggle_when_pressed(&debug_struct->draw_layout_areas, KEY_F5);
    }


    const f64 percent = 0.7;
    // percent = fmod(percent + input->time.dt, 1);

    Rectangle layout_total_area = {0, 0, context->window_width, context->window_height};
    Rectangle layout_sudoku_area = layout_take_from(&layout_total_area, LAY_LEFT, Amount(LAY_IN_FRACTION, percent));
    Rectangle layout_button_area = layout_total_area;

    if (debug_struct->draw_layout_areas) {
        DebugDraw(DrawRectangleRec(layout_total_area,  ColorAlpha(BLUE, 0.7)));
    }

    { // layout_button_area
        // give some padding around the edge.
        layout_button_area = ShrinkRectangle(layout_button_area, theme->ui.button_area_padding);

        // TODO more ui stuff.
        /*
        Rectangle_or_something scroll_able = ui_push_scrollable_area(&layout_button_area); {
            ui_push( ui_dropdown_menu("Debug Controls") )
            if (ui_checkbox("debug cell hitbox's")) {

        }

            ui_pop();
        } ui_pop();
        */


        // TODO make a bunch of debug toggles.
        //
        // but have them in a drop down area


        { // Undo / Redo
            // TODO have these in the same row, side by side.
            bool do_undo = ui_button("Undo Ctrl-Z", &layout_button_area);
            bool do_redo = ui_button("Redo Ctrl-X", &layout_button_area);

            do_undo = do_undo || (input->keyboard.control_down && input->keyboard.key.z_pressed);
            // TODO cntl-x should be cut, but we dont have that yet.
            do_redo = do_redo || (input->keyboard.control_down && input->keyboard.key.x_pressed);

            if (do_undo)   undo_sudoku(context->sudoku);
            if (do_redo)   redo_sudoku(context->sudoku);
        }


        if (ui_button("Toggle Build / Solve Mode", &layout_button_area)) {
            context->in_solve_mode = !context->in_solve_mode;
        }

        if (ui_button("Clear Sudoku", &layout_button_area)) {
            // TODO this is definitely a hack...
            Sudoku_Digit_Grid zero_grid = ZEROED;
            place_digit_grid_into_sudoku_grid(zero_grid, &context->sudoku->grid, false);

            if (!sudoku_maybe_add_grid_into_undo_buffer(context->sudoku)) {
                log("Nothing to clear!");
            }
        }

        // button to randomly generate a sudoku
        if (ui_button("Generate Random Sudoku", &layout_button_area)) {
            Sudoku_Digit_Grid random_sudoku = Generate_Random_Sudoku(20);

            place_digit_grid_into_sudoku_grid(random_sudoku, &context->sudoku->grid, false);

            bool changed = sudoku_maybe_add_grid_into_undo_buffer(context->sudoku);
            if (!changed)   log_error("randomly generated sudoku was the same as the current grid?");
        }

        // button to solve sudoku
        if (ui_button("Solve Sudoku", &layout_button_area)) {
            Input_Sudoku_Puzzle input_sudoku_puzzle = sudoku_grid_to_input_sudoku_puzzle(&context->sudoku->grid);

            // this is on the main thread for now. its fast enough.
            Sudoku_Solver_Result sudoku_solver_result = Solve_Sudoku(input_sudoku_puzzle);

            if (sudoku_solver_result.sudoku_is_possible) {
                log("Sudoku is possible.");

                place_digit_grid_into_sudoku_grid(sudoku_solver_result.correct_grid, &context->sudoku->grid, true);

                bool changed = sudoku_maybe_add_grid_into_undo_buffer(context->sudoku);
                if (!changed)   log_error("solve sudoku didn't do anything?");

            } else {
                // TODO do more.
                log_error("Sudoku is not possible");
                switch (sudoku_solver_result.reason_not_possible) {
                case RFSI_BAD_USER_INPUT:
                case RFSI_NO_POSSIBLE_SOLUTION: {
                    log_error("Reason: No Possible Solution");
                } break;
                case RFSI_MULTIPLE_SOLUTIONS: {
                    log_error("Reason: More than 1 possible solution");
                } break;
                case RFSI_NONE: { UNREACHABLE(); } break;
                }
            }
        }
    }


    { // layout_sudoku_area
        // this is definitely some ui thing. ui_label or something
        //
        // font + some padding.
        f32 padding = 10;

        // take some space to make title.
        Rectangle layout_title_area = layout_take_from(&layout_sudoku_area, LAY_UP, Amount(LAY_IN_PIXELS, theme->title_text_font_size + padding*2));

        if (debug_struct->draw_layout_areas) {
            DebugDraw(DrawRectangleRec(layout_sudoku_area,  ColorAlpha(RED, 0.7)));
            DebugDraw(DrawRectangleRec(layout_title_area, ColorAlpha(GREEN, 0.7)));
        }

        // TODO maybe it would be better to pass this in as a argument
        // to the handle_and_draw_sudoku() function.
        // local_persist bool in_solve_mode = true;
        {
            toggle_when_pressed(&context->in_solve_mode, KEY_B);

            const char *text;
            Color text_color;
            if (!context->in_solve_mode) {
                text       = "BUILD";
                text_color = theme->sudoku.cell_text.valid_builder_digit_color; // color is the same as the average text.
            } else {
                text       = "SOLVE";
                text_color = theme->sudoku.cell_text.valid_solver_digit_color;  // color is the same as the average text.
            }

            // TODO change font size with screen size.
            DrawTextCentered(GetFontWithSize(theme->title_text_font_size), text, RectangleCenter(layout_title_area), text_color);
        }

        { // the sudoku

            // 10 pixels pad,
            f64 sudoku_size = Min(layout_sudoku_area.width, layout_sudoku_area.height) - 10*2;
            sudoku_size = Max(sudoku_size, 0); // dont be negative

            Vector2 center = RectangleCenter(layout_sudoku_area);
            Vector2 top_left_corner = {
                center.x - sudoku_size/2,
                center.y - sudoku_size/2,
            };

            handle_and_draw_sudoku(
                context->sudoku,
                top_left_corner.x,
                top_left_corner.y,
                sudoku_size,
                sudoku_size
            );
        }

    }


    // draw logged messages.
    draw_logger_frame(10, 40);

    // draw the debug stuff.
    DrawTextureRightsideUp(debug_struct->debug_texture.texture, 0, 0);

    EndDrawing();
}





////////////////////////////////////////////
//             final includes
////////////////////////////////////////////


#define BESTED_IMPLEMENTATION
#include "Bested.h"


#define THEME_IMPLEMENTATION
#include "theme.h"


#define SUDOKU_IMPLEMENTATION
#include "sudoku_grid.h"

#define SOUND_IMPLEMENTATION
#include "sound.h"

#define INPUT_IMPLEMENTATION
#include "input.h"

#define LOGGING_IMPLEMENTATION
#include "logging.h"

#define SUDOKU_SOLVER_IMPLEMENTATION
#include "sudoku_solver/sudoku_solver.h"
