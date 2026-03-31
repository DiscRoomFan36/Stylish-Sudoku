
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





#include "sudoku_grid.h"
#include "sound.h"
#include "input.h"
#include "logging.h"

#include "sudoku_solver/sudoku_solver.h"








// funny functions, display the color they represent in VSCode.
//
// is the reason I use VSCode to edit colors,
//
// even though this feature is because of CSS, (i presume,)
// its super helpful to have a color picker in your editor.
#define rgba(_r, _g, _b, _a) ( (Color){.r = (_r), .g = (_g), .b = (_b), .a = (_a*255) + 0.5} )
#define rgb(r, g, b) rgba((r), (g), (b), 1)




////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//                              Defines
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


// sqaure for now, change when adding a control pannel.
#define INITAL_WINDOW_WIDTH         (9*80)
#define INITAL_WINDOW_HEIGHT        (9*80)


// in pixels, 60 is highly divisible
#define SUDOKU_CELL_SIZE                    60

#define SUDOKU_CELL_INNER_LINE_THICKNESS    1.0           // (SUDOKU_CELL_SIZE / 24)       // is it cleaner when its in terms of SUDOKU_CELL_SIZE?
#define SUDOKU_CELL_BOARDER_LINE_THICKNESS  3.0

static_assert((s32)SUDOKU_CELL_INNER_LINE_THICKNESS <= (s32)SUDOKU_CELL_BOARDER_LINE_THICKNESS, "must be true");

#define SUDOKU_CELL_SMALLER_HITBOX_SIZE     (SUDOKU_CELL_SIZE / 8)        // is it cleaner when its in terms of SUDOKU_CELL_SIZE?


// the Theme or Pallet.
//
// this makes it easy to change the theme in the future.
// just update this struct and the rest of the program will follow suit.
typedef struct {
    Color background;

    // things relating to the sudoku. withc is most things.
    struct {
        Color cell_background;
        Color cell_lines;
        Color box_lines;

        struct {
            Color   valid_builder;
            Color   valid_solver;
            Color invalid_builder;
            Color invalid_solver;

            Color   valid_marking_certain;
            Color   valid_marking_uncertain;
            Color invalid_marking_certain;
            Color invalid_marking_uncertain;
        } cell_text;

        // turn "nth bit set" into a color
        Color cell_color_bitfield[32];
    } sudoku;

    Color select_highlight;

    struct {
        Color text_color;
        Color error_text_color;

        Color box_background;
        Color box_frame_color;
    } logger;

} Theme;



#define SELECT_LINE_THICKNESS               (SUDOKU_CELL_SIZE / 12)


#define FONT_SIZE                           (SUDOKU_CELL_SIZE / 1)


// TODO maybe the markings use a different font? maybe the bold version.

#define FONT_SIZE_UNCERTAIN                 (FONT_SIZE / 3)

#define FONT_SIZE_MARKING_CERTAIN_MAX_SIZE          (SUDOKU_CELL_SIZE / 2.5)
#define FONT_SIZE_MARKING_CERTAIN_MIN_SIZE          (SUDOKU_CELL_SIZE / 6)

#define MARKING_ALLOWED_CERTAIN_UPTO_BEFORE_SHRINKING  4




#define MARKING_UNCERTAIN_PADDING_UD                        5
#define MARKING_UNCERTAIN_PADDING_LR                        12
#define MARKING_UNCERTAIN_PADDING_9_AND_10_EXTRA            10

const Vector2 MARKING_LOCATIONS[SUDOKU_MAX_MARKINGS] = {
    /*  1 */ {                   MARKING_UNCERTAIN_PADDING_LR,                                                                 MARKING_UNCERTAIN_PADDING_UD},                           // left      top
    /*  2 */ {SUDOKU_CELL_SIZE - MARKING_UNCERTAIN_PADDING_LR,                                                                 MARKING_UNCERTAIN_PADDING_UD},                           // right     top
    /*  3 */ {                   MARKING_UNCERTAIN_PADDING_LR,                                              SUDOKU_CELL_SIZE - MARKING_UNCERTAIN_PADDING_UD - FONT_SIZE_UNCERTAIN},     // left      bottom
    /*  4 */ {SUDOKU_CELL_SIZE - MARKING_UNCERTAIN_PADDING_LR,                                              SUDOKU_CELL_SIZE - MARKING_UNCERTAIN_PADDING_UD - FONT_SIZE_UNCERTAIN},     // right     bottom
    /*  5 */ {SUDOKU_CELL_SIZE/2,                                                                                              MARKING_UNCERTAIN_PADDING_UD},                           // middle    top
    /*  6 */ {SUDOKU_CELL_SIZE/2,                                                                           SUDOKU_CELL_SIZE - MARKING_UNCERTAIN_PADDING_UD - FONT_SIZE_UNCERTAIN},     // middle    bottom
    /*  7 */ {                   MARKING_UNCERTAIN_PADDING_LR,                                              SUDOKU_CELL_SIZE/2                              - FONT_SIZE_UNCERTAIN/2},   // left      middle
    /*  8 */ {SUDOKU_CELL_SIZE - MARKING_UNCERTAIN_PADDING_LR,                                              SUDOKU_CELL_SIZE/2                              - FONT_SIZE_UNCERTAIN/2},   // right     middle
    /*  9 */ {                   MARKING_UNCERTAIN_PADDING_LR + MARKING_UNCERTAIN_PADDING_9_AND_10_EXTRA,   SUDOKU_CELL_SIZE/2                              - FONT_SIZE_UNCERTAIN/2},   // left-ish  middle
    /* 10 */ {SUDOKU_CELL_SIZE - MARKING_UNCERTAIN_PADDING_LR - MARKING_UNCERTAIN_PADDING_9_AND_10_EXTRA,   SUDOKU_CELL_SIZE/2                              - FONT_SIZE_UNCERTAIN/2},   // right-ish middle
};










///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//                              Context
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////


typedef struct {

    s32     window_width;
    s32     window_height;


    bool    debug_draw_smaller_cell_hitbox;
    bool    debug_draw_cursor_position;
    bool    debug_draw_color_points;
    bool    debug_draw_fps;


    // all the memory in this program comes from here.
    Arena_Pool pool;
    // resets every frame.
    Arena *scratch;

    Input input;

    A_Sound_Array               global_sound_array;
    Logged_Message_Array        logged_messages_to_display;


    Sudoku sudoku_base;
    Sudoku *sudoku;

    // TODO is this the right place for this.
    bool in_solve_mode;


    RenderTexture2D debug_texture;


    Theme theme;

} Context;


global_variable Context __context_base = ZEROED;

internal void   init_context(void);
internal void uninit_context(void);

//
// doing this to help me know where im getting
// the context from in each function.
//
internal Context *get_context(void) { return &__context_base; }

//
// NOTE. The difference between saying 'Scratch_Get()' and 'Pool_Get(&context->pool)'
//
// 'Scratch_Get()' implies that you will eventually call 'Scratch_Release()'
//
// 'Pool_Get(&context->pool)' says that you indend to hold this Arena for the entire program.
//
internal Arena *Scratch_Get(void);
internal void   Scratch_Release(Arena *scratch);



void init_context(void) {
    Context *context = get_context();

    // just making sure.
    Mem_Zero_Struct(context);

    context->window_width  = GetScreenWidth();
    context->window_height = GetScreenHeight();


    context->debug_draw_smaller_cell_hitbox  = false;
    context->debug_draw_cursor_position      = false;
    context->debug_draw_color_points         = false;
    context->debug_draw_fps                  = false;


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


    context->scratch = Pool_Get(&context->pool);

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

    // the perhaps better option would be to make DebugDraw## versions of the draw
    // functions that buffer the commands until later, but that would require
    // making a bunch of extra functions...
    //
    // another option would be to somehow draw rectangles with depth,
    // and make debug stuff the most important.
    context->debug_texture = LoadRenderTexture(context->window_width, context->window_height);

#define DebugDraw(draw_call) do { BeginTextureMode(context->debug_texture); (draw_call);  EndTextureMode(); } while (0)




    { // theme stuff
        Mem_Zero_Struct(&context->theme);


        // https://coolors.co/visualizer/7f5539-a68a64-ede0d4-656d4a-414833
        // const Color pallet_1 = rgb(127, 85, 57);
        // const Color pallet_2 = rgb(166, 138, 100);
        const Color pallet_3 = rgb(237, 224, 212);
        const Color pallet_4 = rgb(101, 109, 74);
        const Color pallet_5 = rgb(65, 72, 51);

        const Color pallet_select = rgb(0, 162, 255);
        const Color pallet_error = rgb(255, 55, 0);

        context->theme.background = pallet_3;

        context->theme.sudoku.cell_background    = pallet_3;
        context->theme.sudoku.cell_lines         = pallet_4;
        context->theme.sudoku.box_lines          = pallet_5;


        context->theme.sudoku.cell_text.  valid_builder            = pallet_5;
        context->theme.sudoku.cell_text.  valid_solver             = pallet_select;
        context->theme.sudoku.cell_text.  valid_marking_certain    = pallet_select;
        context->theme.sudoku.cell_text.  valid_marking_uncertain  = pallet_select;

        context->theme.sudoku.cell_text.invalid_builder            = pallet_5; // TODO make this striped
        context->theme.sudoku.cell_text.invalid_solver             = pallet_error;
        context->theme.sudoku.cell_text.invalid_marking_certain    = pallet_error;
        context->theme.sudoku.cell_text.invalid_marking_uncertain  = pallet_error;


        {
            const Color transparent = rgba(0, 0, 0, 0);

            // Colors stolen from https://sudokupad.app
            const Color cell_colors[Array_Len(context->theme.sudoku.cell_color_bitfield)] = {
                transparent,          // cell color 0
                rgb(214, 214, 214),   // cell color 1
                rgb(124, 124, 124),   // cell color 2
                rgb( 36,  36,  36),   // cell color 3
                rgb(179, 229, 106),   // cell color 4
                rgb(232, 124, 241),   // cell color 5
                rgb(228, 150,  50),   // cell color 6
                rgb(245,  58,  55),   // cell color 7
                rgb(252, 235,  63),   // cell color 8
                rgb( 61, 153, 245),   // cell color 9
                transparent,          // cell color a
                rgb(204,  51,  17),   // cell color b
                rgb( 17, 119,  51),   // cell color c
                rgb(  0,  68, 196),   // cell color d
                rgb(238, 153, 170),   // cell color e
                rgb(255, 255,  25),   // cell color f
                rgb(240,  70, 240),   // cell color g
                rgb(160,  90,  30),   // cell color h
                rgb( 51, 187, 238),   // cell color i
                rgb(145,  30, 180),   // cell color j
                transparent,          // cell color k
                rgb(245,  58,  55),   // cell color l
                rgb( 76, 175,  80),   // cell color m
                rgb( 61, 153, 245),   // cell color n
                rgb(249, 136, 134),   // cell color o
                rgb(149, 208, 151),   // cell color p
                rgb(158, 204, 250),   // cell color q
                rgb(170,  12,   9),   // cell color r
                rgb( 47, 106,  49),   // cell color s
                rgb(  9,  89, 170),   // cell color t
            };

            static_assert(sizeof(cell_colors) <= sizeof(context->theme.sudoku.cell_color_bitfield), "just making sure.");
            Mem_Copy(context->theme.sudoku.cell_color_bitfield, (void*)cell_colors, sizeof(cell_colors));
        }

        context->theme.select_highlight = pallet_select;

        context->theme.logger.text_color         = pallet_4;
        context->theme.logger.error_text_color   = pallet_error;

        context->theme.logger.box_background     = pallet_3;
        context->theme.logger.box_frame_color    = pallet_5;
    }

}
void uninit_context(void) {
    Context *context = get_context();

    Pool_Free_Arenas(&context->pool);
    UnloadRenderTexture(context->debug_texture);

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
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);  // the context contains the window width/height, and stuff updates dynamically.
    SetTraceLogLevel(LOG_WARNING);          // only show warning or worse logs, the console is being spammed in LOG_INFO mode.
    InitWindow(INITAL_WINDOW_WIDTH, INITAL_WINDOW_HEIGHT, "Sudoku");


    init_context();

    Context *context = get_context();


    // this is my own font system, the functions mimic raylibs style.
    InitDynamicFonts("./assets/font/iosevka-light.ttf");
    // this is my own sound system, the functions *DO NOT* mimic raylibs style.
    init_sounds();


    { // load the autosave.
        const char *error = load_sudoku(autosave_path, context->sudoku);
        if (error) {
            log_error("Failed To Load Save '%s': %s", autosave_path, error);
            // TODO make a "clear grid" function.
            for (u32 j = 0; j < SUDOKU_SIZE; j++) {
                for (u32 i = 0; i < SUDOKU_SIZE; i++) {
                    Place_Digit(&context->sudoku->grid, i, j, NO_DIGIT_PLACED, .dont_play_sound = true);
                }
            }
        } else {
            log("Succsessfully loaded save file '%s'", autosave_path);
        }
    }


#define Sudoku_Grid_Is_The_Same_As_The_Last_Element_In_The_Undo_Buffer(sudoku) Mem_Eq(&(sudoku)->grid, &(sudoku)->undo_buffer.items[(sudoku)->undo_buffer.count-1], sizeof((sudoku)->grid))

    if (context->sudoku->undo_buffer.count == 0) {
        Array_Append(&context->sudoku->undo_buffer, context->sudoku->grid);
        context->sudoku->redo_count = 0;
    }
    ASSERT(Sudoku_Grid_Is_The_Same_As_The_Last_Element_In_The_Undo_Buffer(context->sudoku));



#ifdef PLATFORM_WEB

    emscripten_set_main_loop(do_one_frame, 0, 1);
    #error "do we exit now? or dose this function call kidnap the control flow?"
    return 0;

#else

    // probably could hit 144fps, but maybe later.
    //
    // also only run this when building for desktop.
    SetTargetFPS(60);

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




void do_one_frame() {
    Context *context = get_context();

    ASSERT(Sudoku_Grid_Is_The_Same_As_The_Last_Element_In_The_Undo_Buffer(context->sudoku));


    Arena_Clear(context->scratch);

    BeginDrawing();
    ClearBackground(context->theme.background);

    {
        s32 prev_window_width   = context->window_width;
        s32 prev_window_height  = context->window_height;

        context->window_width    = GetScreenWidth();
        context->window_height   = GetScreenHeight();

        if (prev_window_width != context->window_width || prev_window_height != context->window_height) {
            UnloadRenderTexture(context->debug_texture);
            context->debug_texture = LoadRenderTexture(context->window_width, context->window_height);
        }
    }


    // Clear debug to all zero's
    DebugDraw(ClearBackground((Color){0, 0, 0, 0}));




    ////////////////////////////////
    //        User Input
    ////////////////////////////////
    update_input();


    toggle_when_pressed(&context->debug_draw_smaller_cell_hitbox,    KEY_F1);
    toggle_when_pressed(&context->debug_draw_cursor_position,        KEY_F2);
    toggle_when_pressed(&context->debug_draw_color_points,           KEY_F3);


    toggle_when_pressed(&context->debug_draw_fps,                    KEY_F4);
    if (context->debug_draw_fps) DebugDraw(DrawFPS(context->window_width - 100, 10));


    // TODO maybe it would be better to pass this in as a argument.
    // local_persist bool in_solve_mode = true;
    {
        toggle_when_pressed(&context->in_solve_mode, KEY_B);

        const char *text;
        Color text_color;
        if (!context->in_solve_mode) {
            text       = "BUILD";
            text_color = context->theme.sudoku.cell_text.valid_builder; // color is the same as the average text.
        } else {
            text       = "SOLVE";
            text_color = context->theme.sudoku.cell_text.valid_solver;  // color is the same as the average text.
        }

        Vector2 text_pos = { context->window_width/2, 10 + FONT_SIZE/2 };
        DrawTextCentered(GetFontWithSize(FONT_SIZE), text, text_pos, text_color);
    }



    Vector2 top_left_corner = {
        (context->window_width /2) - (SUDOKU_SIZE*SUDOKU_CELL_SIZE)/2,
        (context->window_height/2) - (SUDOKU_SIZE*SUDOKU_CELL_SIZE)/2,
    };

    handle_and_draw_sudoku(
        context->sudoku,
        top_left_corner.x,
        top_left_corner.y,
        SUDOKU_SIZE * SUDOKU_CELL_SIZE,
        SUDOKU_SIZE * SUDOKU_CELL_SIZE
    );


    // draw logged messages.
    draw_logger_frame(10, 40);

    // draw the debug stuff.
    DrawTextureRightsideUp(context->debug_texture.texture, 0, 0);

    EndDrawing();
}





////////////////////////////////////////////
//             final includes
////////////////////////////////////////////


#define BESTED_IMPLEMENTATION
#include "Bested.h"

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
