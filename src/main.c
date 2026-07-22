
#include <time.h>

#include "Bested.h"
#undef Clamp // TODO we gotta do something about this...



// an easier way to tell if we are in web mode.
// can use WEB_MODE in an `if` statement.
#if PLATFORM_WEB
    #define WEB_MODE true
#else
    #define WEB_MODE false
#endif


#if WEB_MODE
    #include <emscripten/emscripten.h>
#endif


#include <raylib.h>
#include <raymath.h>

#include "raylib_helpers.c"

// helpers and whatnot
#include "theme.h"
#include "input.h"

#include "sound.h"
#include "logging.h"

// sudoku stuff
#include "sudoku_solver/sudoku_solver.h"
#include "sudoku_grid.h"

#include "layout.h"


// i cant put these anywhere else...

// returns -1 if its not in the array
internal s64 index_in_array(Int_Array *array, s64 to_find) {
    for (u64 i = 0; i < array->count; i++) {
        if (array->items[i] == to_find)    return i;
    }
    return -1;
}

#define Square(x) ((x)*(x))



#define TARGET_FPS 60

// inital window size
#define WINDOW_WIDTH_FACTOR         16
#define WINDOW_HEIGHT_FACTOR         9
#define INITAL_WINDOW_SIZE_FACTOR   80

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

    bool draw_mouse_cursor;

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
            Arena_Add_Buffer_As_Storage_Space(&context->pool.arena_pool[i], buffers[i], sizeof(buffers[i]));
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

        debug_struct->draw_cursor_position      = false;

        debug_struct->draw_layout_areas         = false;

        // the perhaps better option would be to make DebugDraw## versions of the draw
        // functions that buffer the commands until later, but that would require
        // making a bunch of extra functions...
        //
        // another option would be to somehow draw rectangles with depth,
        // and make debug stuff the most important.
        debug_struct->debug_texture = LoadRenderTexture(context->window_width, context->window_height);

        #define Debug_Draw()        \
            Defer_Scope(BeginTextureMode(get_debug_struct()->debug_texture), EndTextureMode())



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

    // init the raylib window.
    InitWindow(
        WINDOW_WIDTH_FACTOR  * INITAL_WINDOW_SIZE_FACTOR,
        WINDOW_HEIGHT_FACTOR * INITAL_WINDOW_SIZE_FACTOR,
        "Sudoku"
    );

    // in my application context
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

            if (WEB_MODE) {
                // generate a random sudoku on first appearance.
                Sudoku_Digit_Grid grid = Generate_Random_Sudoku(25);
                place_digit_grid_into_sudoku_grid(grid, &context->sudoku->grid, false);
            } else {
                clear_sudoku_grid(&context->sudoku->grid);
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



#if WEB_MODE
    printf("Hello WASM!\n");

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
    printf("Hello Native Window!\n");

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


void do_one_frame() {
    Context      *context      = get_context();
    Theme        *theme        = get_theme();
    Debug_Struct *debug_struct = get_debug_struct();

    if (WEB_MODE) {
        // on the web, the size of the window is not in alignment with what
        // raylib considers the size of the window. the numbers you supply
        // to InitWindow() are treated as gospel, setting the ratio of the
        // window. this ratio is incorrect in most cases, and even when the
        // browser window size is changed, raylib dose not fix the problem.
        //
        // this leads to the mouse being offset from its true position.
        //
        // the fix for this is to call SetWindowSize() to get raylib to properly
        // figure out the size of the window. we only need to call this once,
        // (as the raylib window size and the browser window size is only
        // de-synced once at the start of the program.)
        //
        // SetWindowSize() actually dose nothing on web platforms, and the
        // browser immediately sets the window back to the correct size, but
        // this one touch is enough to fix the bug.
        //
        // TODO post an issue to the raylib github.
        local_persist bool lock = false;
        if (!lock) {
            lock = true;
            SetWindowSize(context->window_width, context->window_height);
        }
    }

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
            // log("Window size changed! (%d, %d)", context->window_width, context->window_height);
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
        Debug_Draw() { ClearBackground((Color){0, 0, 0, 0}); }

        toggle_when_pressed(&debug_struct->draw_smaller_cell_hitbox,    KEY_F1);
        toggle_when_pressed(&debug_struct->draw_cursor_position,        KEY_F2);
        toggle_when_pressed(&debug_struct->draw_color_points,           KEY_F3);

        toggle_when_pressed(&debug_struct->draw_fps,                    KEY_F4);
        if (debug_struct->draw_fps) Debug_Draw() {
            DrawFPS(context->window_width - 100, 10);
        }

        toggle_when_pressed(&debug_struct->draw_mouse_cursor,           KEY_F5);

        toggle_when_pressed(&debug_struct->draw_layout_areas,           KEY_F6);
    }


    const f64 percent = 0.7;
    // percent = fmod(percent + input->time.dt, 1);

    Rectangle layout_total_area = {0, 0, context->window_width, context->window_height};
    Rectangle layout_sudoku_area = layout_take_from(&layout_total_area, LAY_LEFT, Amount(LAY_IN_FRACTION, percent));
    Rectangle layout_button_area = layout_total_area;

    if (debug_struct->draw_layout_areas) {
        Debug_Draw() { DrawRectangleRec(layout_total_area,  ColorAlpha(BLUE, 0.7)); }
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
            bool do_undo = ui_button("Undo (Ctrl-Z)", &layout_button_area);
            bool do_redo = ui_button("Redo (Ctrl-X)", &layout_button_area);

            do_undo = do_undo || (input->keyboard.control_down && input->keyboard.key.z_pressed);
            // TODO cntl-x should be cut, but we dont have that yet.
            do_redo = do_redo || (input->keyboard.control_down && input->keyboard.key.x_pressed);

            if (do_undo)   undo_sudoku(context->sudoku);
            if (do_redo)   redo_sudoku(context->sudoku);
        }


        if (ui_button("Toggle Solve Mode", &layout_button_area)) {
            context->in_solve_mode = !context->in_solve_mode;
        }

        if (ui_button("Clear Sudoku", &layout_button_area)) {
            clear_sudoku_grid(&context->sudoku->grid);
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

        // button to solve sudoku instantly.
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

        local_persist bool auto_solve_sudoku = false;
        // ui_toggle("Auto Solve Sudoku", &layout_button_area, &auto_solve_sudoku);
        if (ui_button("Auto Solve Sudoku", &layout_button_area)) {
            auto_solve_sudoku = !auto_solve_sudoku;
            sudoku_grid_changed(context->sudoku);
        }

        // would have liked this to happen after the user places the digit, (aka on the same frame)
        //
        // there will be a 1 frame delay for this.
        if (auto_solve_sudoku) {
            if (!context->sudoku->auto_solver.have_solver_result) {
                Input_Sudoku_Puzzle input_sudoku_puzzle = sudoku_grid_to_input_sudoku_puzzle(&context->sudoku->grid);

                // this is on the main thread for now. its fast enough.
                Sudoku_Solver_Result sudoku_solver_result = Solve_Sudoku(input_sudoku_puzzle);

                context->sudoku->auto_solver.result_from_solving_sudoku = sudoku_solver_result;
                context->sudoku->auto_solver.have_solver_result = true;

                // print the result of the solve (once)
                //
                // TODO combine with solve sudoku above.
                if (context->sudoku->auto_solver.have_solver_result) {
                    if (context->sudoku->auto_solver.result_from_solving_sudoku.sudoku_is_possible) {

                        log("sudoku is possible!");

                    } else {

                        const char *reason_not_possible_string = "NO_REASON_GIVEN";
                        switch (context->sudoku->auto_solver.result_from_solving_sudoku.reason_not_possible) {
                        case RFSI_NONE: {
                            log_error("no reason for impossibly given?");
                        } break;

                        case RFSI_BAD_USER_INPUT:{
                            reason_not_possible_string = "bad user input (aka  its obviously wrong.)";
                        } break;
                        case RFSI_NO_POSSIBLE_SOLUTION: {
                            reason_not_possible_string = "not possible with given restraints.";
                        } break;
                        case RFSI_MULTIPLE_SOLUTIONS:{
                            reason_not_possible_string = "multiple solutions";
                        } break;

                        default: UNREACHABLE();
                        }

                        log("Sudoku is not possible, reason: %s", reason_not_possible_string);
                    }
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

        if (debug_struct->draw_layout_areas) Debug_Draw() {
            DrawRectangleRec(layout_sudoku_area, ColorAlpha(RED, 0.7));
            DrawRectangleRec(layout_title_area, ColorAlpha(GREEN, 0.7));
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

    // debug draw mouse cursor.
    if (debug_struct->draw_mouse_cursor) Debug_Draw() {
        Vector2 pos = GetMousePosition();
        // log("Mouse Position: (%.4f, %.4f)", pos.x, pos.y);
        DrawCircleV(pos, 5, RED);
    }

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

#define INPUT_IMPLEMENTATION
#include "input.h"


#define SOUND_IMPLEMENTATION
#include "sound.h"

#define LOGGING_IMPLEMENTATION
#include "logging.h"


#define SUDOKU_SOLVER_IMPLEMENTATION
#include "sudoku_solver/sudoku_solver.h"

#define SUDOKU_IMPLEMENTATION
#include "sudoku_grid.h"


#define LAYOUT_IMPLEMENTATION
#include "layout.h"
