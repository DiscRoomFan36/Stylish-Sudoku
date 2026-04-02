
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




#include "theme.h"

#include "sudoku_grid.h"
#include "sound.h"
#include "input.h"
#include "logging.h"

#include "sudoku_solver/sudoku_solver.h"







////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//                              Defines
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


// square for now, change when adding a control panel.
#define INITAL_WINDOW_WIDTH         (9*80)
#define INITAL_WINDOW_HEIGHT        (9*80)


// in pixels, 60 is highly divisible
#define SUDOKU_CELL_SIZE                    60

#define SUDOKU_CELL_INNER_LINE_THICKNESS    1.0           // (SUDOKU_CELL_SIZE / 24)       // is it cleaner when its in terms of SUDOKU_CELL_SIZE?
#define SUDOKU_CELL_BOARDER_LINE_THICKNESS  3.0

static_assert((s32)SUDOKU_CELL_INNER_LINE_THICKNESS <= (s32)SUDOKU_CELL_BOARDER_LINE_THICKNESS, "must be true");

#define SUDOKU_CELL_SMALLER_HITBOX_SIZE     (SUDOKU_CELL_SIZE / 8)        // is it cleaner when its in terms of SUDOKU_CELL_SIZE?



// TODO put into sudoku file.

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


    context->theme_base = init_theme();
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


    // this is my own font system, the functions mimic raylib's style.
    InitDynamicFonts("./assets/font/iosevka-light.ttf");
    // this is my own sound system, the functions *DO NOT* mimic raylib's style.
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
            log("Successfully loaded save file '%s'", autosave_path);
        }
    }


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



Input_Sudoku_Puzzle sudoku_grid_to_input_sudoku_puzzle(Sudoku_Grid *grid) {
    Input_Sudoku_Puzzle input_sudoku_puzzle = ZEROED;
    static_assert(SUDOKU_SIZE == NUM_DIGITS, "these are the same for now, maybe in the future we will have different sized grids for things.");

    for (size_t j = 0; j < SUDOKU_SIZE; j++) {
        for (size_t i = 0; i < SUDOKU_SIZE; i++) {
            Sudoku_Cell *cell = get_cell(grid, i, j);
            u8 digit = Is_Between(cell->digit, 1, 9) ? cell->digit : 0;
            input_sudoku_puzzle.grid.digits[j][i] = digit;
        }
    }

    return input_sudoku_puzzle;
}

bool ui_button(const char *text, Vector2 position) {
    Input *input = get_input();

    // TODO Font size
    Font_And_Size font_and_size = GetFontWithSize(32);

    Vector2 text_size = MeasureTextEx(font_and_size.font, text, font_and_size.size, 0);

    Rectangle text_bounds = {
        position.x, // TODO think about where to place this. maybe in the center?
        position.y, // TODO think about where to place this. maybe in the center?
        text_size.x,
        text_size.y
    };

    const f32 padding = 10;
    Rectangle button_bounds = GrowRectangle(text_bounds, padding);

    // TODO draw differently if hovered / clicked.
    bool button_is_hovered = CheckCollisionPointRec(input->mouse.pos, button_bounds);
    bool button_is_clicked = button_is_hovered && input->mouse.left.clicked;

    // TODO Color
    DrawRectangleRec(button_bounds, rgb(175, 175, 175));
    DrawRectangleLinesEx(button_bounds, 5, rgb(14, 14, 14));

    DrawTextCentered(font_and_size, text, RectangleCenter(button_bounds), rgb(31, 31, 31));

    return button_is_clicked;
}


void do_one_frame() {
    Context *context = get_context();
    Theme *theme = get_theme();

    ASSERT(Sudoku_Grid_Is_The_Same_As_The_Last_Element_In_The_Undo_Buffer(context->sudoku));


    Arena_Clear(context->scratch);

    BeginDrawing();
    ClearBackground(theme->background_color);

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
    // Input *input = get_input();


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
            text_color = theme->sudoku.cell_text.valid_builder_digit_color; // color is the same as the average text.
        } else {
            text       = "SOLVE";
            text_color = theme->sudoku.cell_text.valid_solver_digit_color;  // color is the same as the average text.
        }

        Vector2 text_pos = { context->window_width/2, 10 + FONT_SIZE/2 };
        DrawTextCentered(GetFontWithSize(FONT_SIZE), text, text_pos, text_color);
    }

    { // button to randomly generate a sudoku
        Vector2 button_position = {context->window_width - 200, 20};

        if (ui_button("Generate Random Sudoku", button_position)) {
            Sudoku_Digit_Grid random_sudoku = Generate_Random_Sudoku(20);

            log_error("TODO: load random grid into sudoku");
            (void) random_sudoku;
        }
    }

    { // button to solve sudoku
        // TODO make autolayout system.
        Vector2 button_position = {context->window_width - 200, 80};

        if (ui_button("Solve Sudoku", button_position)) {
            Input_Sudoku_Puzzle input_sudoku_puzzle = sudoku_grid_to_input_sudoku_puzzle(&context->sudoku->grid);

            // this is on the main thread for now. its fast enough.
            Sudoku_Solver_Result sudoku_solver_result = Solve_Sudoku(input_sudoku_puzzle);

            if (sudoku_solver_result.sudoku_is_possible) {
                log("Sudoku is possible.");

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
