
#include <time.h>

#include "Bested.h"
#undef Clamp // TODO we gotta do something about this...




#ifdef PLATFORM_WEB
    #include <emscripten/emscripten.h>
#endif


#include <raylib.h>
#include <raymath.h>

#include "raylib_helpers.c"


#include "sudoku_grid.h"
#include "sound.h"
#include "input.h"
#include "logging.h"

// for 'draw_sudoku_selection()'
#include "sudoku_draw_selection.h"




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



#define rgba(_r, _g, _b, _a) ( (Color){.r = (_r), .g = (_g), .b = (_b), .a = (_a*255) + 0.5} )
// Color rgba(u8 r, u8 g, u8 b, f32 a) {
//     s32 alpha = (Clamp(a, 0, 1) * 255) + 0.5;
//     return (Color){ .r = r, .g = g, .b = b, .a = alpha };
// }
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


// could also be known as theme.
typedef struct {
    Color background;

    // things relating to the sudoku. withc is most things.
    struct {
        Color cell_background;
        Color cell_lines;
        Color box_lines;

        Color cell_digit_font_color;
        // for the placed_in_solve_mode digits
        Color cell_font_color_for_marking;
        // for certen markings
        Color cell_font_color_for_certen;
        // for uncerten markings.
        Color cell_font_color_for_uncerten;

        // for placed in solve mode,
        Color cell_digit_is_invalid;


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

    Selected_Animation_Array    selection_animation_array;
    A_Sound_Array               global_sound_array;
    Logged_Message_Array        logged_messages_to_display;


    Sudoku sudoku_base;
    Sudoku *sudoku;


    RenderTexture2D debug_texture;


    Theme theme;

} Context;


global_variable Context context = ZEROED;

internal void   init_context(void);
internal void uninit_context(void);

//
// NOTE. The difference between saying 'Scratch_Get()' and 'Pool_Get(&context.pool)'
//
// 'Scratch_Get()' implies that you will eventually call 'Scratch_Release()'
//
// 'Pool_Get(&context.pool)' says that you indend to hold this Arena for the entire program.
//
internal Arena *Scratch_Get(void);
internal void   Scratch_Release(Arena *scratch);



void init_context(void) {
    context.window_width  = GetScreenWidth();
    context.window_height = GetScreenHeight();


    context.debug_draw_smaller_cell_hitbox  = false;
    context.debug_draw_cursor_position      = false;
    context.debug_draw_color_points         = false;
    context.debug_draw_fps                  = false;


    Mem_Zero_Struct(&context.pool);

    // SDL_RenderCoordinatesFromWindow

    // here we preallocate some buffers, i was having some trouble trying to
    // get emscripten to allocate memory with my arena allocators, this is a
    // hack to give me a bit of room to work with.
    //
    // this allocates 32 MB sort of unessessarily.
    // still less than 1 javascript object.
    local_persist u8 buffers[NUM_POOL_ARENAS][1 * MEGABYTE];
    for (u32 i = 0; i < NUM_POOL_ARENAS; i++) {
        Arena_Add_Buffer_As_Storeage_Space(&context.pool.arena_pool[i], buffers[i], sizeof(buffers[i]));
    }


    context.scratch = Pool_Get(&context.pool);

    Mem_Zero_Struct(&context.input);

    Mem_Zero_Struct(&context.selection_animation_array      );
    Mem_Zero_Struct(&context.global_sound_array             );
    Mem_Zero_Struct(&context.logged_messages_to_display     );


    context.selection_animation_array   .allocator = Pool_Get(&context.pool);
    context.global_sound_array          .allocator = Pool_Get(&context.pool);
    context.logged_messages_to_display  .allocator = Pool_Get(&context.pool);



    // make sure that load_sudoku(), respects this alloctor
    context.sudoku = &context.sudoku_base;
    context.sudoku->undo_buffer.allocator = Pool_Get(&context.pool);
    Array_Reserve(&context.sudoku->undo_buffer, 512); // just give it some room.


    // the perhaps better option would be to make DebugDraw## versions of the draw
    // functions that buffer the commands until later, but that would require
    // making a bunch of extra functions...
    //
    // another option would be to somehow draw rectangles with depth,
    // and make debug stuff the most important.
    context.debug_texture = LoadRenderTexture(context.window_width, context.window_height);

#define DebugDraw(draw_call) do { BeginTextureMode(context.debug_texture); (draw_call);  EndTextureMode(); } while (0)




    { // theme stuff
        Mem_Zero_Struct(&context.theme);


        // https://coolors.co/visualizer/7f5539-a68a64-ede0d4-656d4a-414833
        // const Color pallet_1 = rgb(127, 85, 57);
        // const Color pallet_2 = rgb(166, 138, 100);
        const Color pallet_3 = rgb(237, 224, 212);
        const Color pallet_4 = rgb(101, 109, 74);
        const Color pallet_5 = rgb(65, 72, 51);

        const Color pallet_select = rgb(0, 162, 255);
        const Color pallet_error = rgb(255, 55, 0);

        context.theme.background = pallet_3;

        context.theme.sudoku.cell_background    = pallet_3;
        context.theme.sudoku.cell_lines         = pallet_4;
        context.theme.sudoku.box_lines          = pallet_5;

        context.theme.sudoku.cell_digit_font_color          = pallet_5;
        context.theme.sudoku.cell_font_color_for_marking    = pallet_select;
        context.theme.sudoku.cell_font_color_for_certen     = pallet_select;
        context.theme.sudoku.cell_font_color_for_uncerten   = pallet_select;

        context.theme.sudoku.cell_digit_is_invalid          = pallet_error;
        // context.theme.sudoku.cell_digit_is_invalid          = pallet_error;


        {
            const Color transparent = rgba(0, 0, 0, 0);

            // Colors stolen from https://sudokupad.app
            const Color cell_colors[Array_Len(context.theme.sudoku.cell_color_bitfield)] = {
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

            static_assert(sizeof(cell_colors) <= sizeof(context.theme.sudoku.cell_color_bitfield), "just making sure.");
            Mem_Copy(context.theme.sudoku.cell_color_bitfield, (void*)cell_colors, sizeof(cell_colors));
        }

        context.theme.select_highlight = pallet_select;

        context.theme.logger.text_color         = pallet_4;
        context.theme.logger.error_text_color   = pallet_error;

        context.theme.logger.box_background     = pallet_3;
        context.theme.logger.box_frame_color    = pallet_5;
    }

}
void uninit_context(void) {
    Pool_Free_Arenas(&context.pool);
    UnloadRenderTexture(context.debug_texture);
}


Arena *Scratch_Get(void)               {
    return Pool_Get(&context.pool);
}
void   Scratch_Release(Arena *scratch) {
    Pool_Release(&context.pool, scratch);
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

    // this is my own font system, the functions mimic raylibs style.
    InitDynamicFonts("./assets/font/iosevka-light.ttf");
    // this is my own sound system, the functions *DO NOT* mimic raylibs style.
    init_sounds();


    { // load the autosave.
        const char *error = load_sudoku(autosave_path, context.sudoku);
        if (error) {
            log_error("Failed To Load Save '%s': %s", autosave_path, error);
            // TODO make a "clear grid" function.
            for (u32 j = 0; j < SUDOKU_SIZE; j++) {
                for (u32 i = 0; i < SUDOKU_SIZE; i++) {
                    Place_Digit(&context.sudoku->grid, i, j, NO_DIGIT_PLACED, .dont_play_sound = true);
                }
            }
        } else {
            log("Succsessfully loaded save file '%s'", autosave_path);
        }
    }


#define Sudoku_Grid_Is_The_Same_As_The_Last_Element_In_The_Undo_Buffer(sudoku) Mem_Eq(&(sudoku)->grid, &(sudoku)->undo_buffer.items[(sudoku)->undo_buffer.count-1], sizeof((sudoku)->grid))

    if (context.sudoku->undo_buffer.count == 0) {
        Array_Append(&context.sudoku->undo_buffer, context.sudoku->grid);
        context.sudoku->redo_count = 0;
    }
    ASSERT(Sudoku_Grid_Is_The_Same_As_The_Last_Element_In_The_Undo_Buffer(context.sudoku));



#ifdef PLATFORM_WEB

    emscripten_set_main_loop(do_one_frame, 0, 1);

#else

    // probably could hit 144fps, but maybe later.
    //
    // also only run this when building for desktop.
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        do_one_frame();
    }
#endif



    bool result = save_sudoku(autosave_path, context.sudoku);
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
    ASSERT(Sudoku_Grid_Is_The_Same_As_The_Last_Element_In_The_Undo_Buffer(context.sudoku));


    Arena_Clear(context.scratch);

    BeginDrawing();
    ClearBackground(context.theme.background);

    {
        s32 prev_window_width   = context.window_width;
        s32 prev_window_height  = context.window_height;

        context.window_width    = GetScreenWidth();
        context.window_height   = GetScreenHeight();

        if (prev_window_width != context.window_width || prev_window_height != context.window_height) {
            UnloadRenderTexture(context.debug_texture);
            context.debug_texture = LoadRenderTexture(context.window_width, context.window_height);
        }
    }


    // Clear debug to all zero's
    DebugDraw(ClearBackground((Color){0, 0, 0, 0}));




    ////////////////////////////////
    //        User Input
    ////////////////////////////////
    update_input();
    Input *input = get_input();


    toggle_when_pressed(&context.debug_draw_smaller_cell_hitbox,    KEY_F1);
    toggle_when_pressed(&context.debug_draw_cursor_position,        KEY_F2);
    toggle_when_pressed(&context.debug_draw_color_points,           KEY_F3);


    toggle_when_pressed(&context.debug_draw_fps,                    KEY_F4);
    if (context.debug_draw_fps) DebugDraw(DrawFPS(context.window_width - 100, 10));


    local_persist bool in_solve_mode = true;
    {
        toggle_when_pressed(&in_solve_mode, KEY_B);

        const char *text = in_solve_mode ? "SOLVE"              : "BUILD";
        Color text_color = in_solve_mode ? context.theme.sudoku.cell_font_color_for_marking : context.theme.sudoku.cell_digit_font_color;
        Vector2 text_pos = { context.window_width/2, 10 + FONT_SIZE/2 };
        DrawTextCentered(GetFontWithSize(FONT_SIZE), text, text_pos, text_color);
    }


    ////////////////////////////////
    //        Selection
    ////////////////////////////////
    local_persist bool when_dragging_to_set_selected_to = true;
    if (!input->mouse.left.down) when_dragging_to_set_selected_to = true;


    local_persist s8 cursor_x = SUDOKU_SIZE / 2; // should be 4 (the middle)
    local_persist s8 cursor_y = SUDOKU_SIZE / 2; // should be 4 (the middle)
    {
        s8 prev_x = cursor_x;
        s8 prev_y = cursor_y;

        if (input->keyboard.direction.up_pressed   ) cursor_y -= 1;
        if (input->keyboard.direction.down_pressed ) cursor_y += 1;
        if (input->keyboard.direction.left_pressed ) cursor_x -= 1;
        if (input->keyboard.direction.right_pressed) cursor_x += 1;

        cursor_x = Proper_Mod(cursor_x, SUDOKU_SIZE);
        cursor_y = Proper_Mod(cursor_y, SUDOKU_SIZE);

        // cursor cannot go back over a selected cell.
        //
        // if shift is not pressed, and the cursor couldnt move to the cell,
        // but will be able to, it dosn't move.
        if (get_cell(context.sudoku, cursor_x, cursor_y).ui->is_selected) {
            cursor_x = prev_x;
            cursor_y = prev_y;
        }

        if (context.debug_draw_cursor_position) {
            Rectangle rec = get_cell_bounds(context.sudoku, cursor_x, cursor_y);
            DebugDraw(DrawCircle(rec.x + rec.width/2, rec.y + rec.height/2, rec.height/3, ColorAlpha(RED, 0.8)));
        }
    }


    ////////////////////////////////
    //       placeing digits
    ////////////////////////////////
    Sudoku_UI_Layer layer_to_place;
    if      (input->keyboard.shift_down && input->keyboard.control_down) layer_to_place = SUL_COLOR;
    else if (input->keyboard.shift_down)                                 layer_to_place = SUL_UNCERTAIN;
    else if (input->keyboard.control_down)                               layer_to_place = SUL_CERTAIN;
    else                                                                 layer_to_place = SUL_DIGIT;

    // determines wheather a number was pressed to put it into the grid.
    s8 number_pressed = NO_DIGIT_PLACED;
    {
        u8 number_keys[] = {KEY_ZERO, KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_FIVE, KEY_SIX, KEY_SEVEN, KEY_EIGHT, KEY_NINE};
        for (u32 i = 0; i < Array_Len(number_keys); i++) {
            if (IsKeyPressed(number_keys[i]))   number_pressed = i;
        }
    }


    ////////////////////////////////
    //       removing digits
    ////////////////////////////////
    // make sure that all boxes get the same result after pressing a key (all add or all remove)
    bool remove_number_this_press = true;

    Sudoku_UI_Layer layer_to_delete = SUL_COLOR; // lowest priority



    ////////////////////////////////
    //         Undo / Redo
    ////////////////////////////////

    if (input->keyboard.control_down && input->keyboard.key.z_pressed) {
        play_sound("undo_undo");
        if (context.sudoku->undo_buffer.count > 1) {
            context.sudoku->undo_buffer.count   -= 1;
            context.sudoku->redo_count          += 1;
            context.sudoku->grid = context.sudoku->undo_buffer.items[context.sudoku->undo_buffer.count - 1];
        }
    }

    // TODO cntl-x should be cut, but we dont have that yet.
    if (input->keyboard.control_down && input->keyboard.key.x_pressed) {
        play_sound("undo_redo");
        if (context.sudoku->redo_count > 0) {
            context.sudoku->undo_buffer.count   += 1;
            context.sudoku->redo_count          -= 1;
            context.sudoku->grid = context.sudoku->undo_buffer.items[context.sudoku->undo_buffer.count - 1];
        }
    }



    ////////////////////////////////////////////////
    //            update sudoku grid
    ////////////////////////////////////////////////

    {
        ////////////////////////////////////////////////
        //              Selection stuff
        ////////////////////////////////////////////////
        Sudoku_UI_Grid previous_ui = context.sudoku->ui;

        bool clicked_on_box = false;
        s8 click_i, click_j;

        // phase 1
        for (u32 j = 0; j < SUDOKU_SIZE; j++) {
            for (u32 i = 0; i < SUDOKU_SIZE; i++) {
                Sudoku_Cell cell        = get_cell(context.sudoku, i, j);
                Rectangle   cell_bounds = get_cell_bounds(context.sudoku, i, j);


                // selected stuff
                bool mouse_is_over = CheckCollisionPointRec(input->mouse.pos, cell_bounds);
                cell.ui->is_hovering_over = mouse_is_over;

                if (input->mouse.left.clicked) {
                    if (mouse_is_over) {
                        clicked_on_box = true;
                        click_i = i; click_j = j;

                        if (input->keyboard.shift_or_control_down) {
                            // start of deselection drag
                            cell.ui->is_selected = !cell.ui->is_selected; // TOGGLE
                        } else {
                            // just this one will be selected
                            cell.ui->is_selected = true;
                        }
                        when_dragging_to_set_selected_to = cell.ui->is_selected;
                    } else {
                        // only stay active if shift or cntl is down
                        cell.ui->is_selected = cell.ui->is_selected && input->keyboard.shift_or_control_down;
                    }
                }

                if (input->keyboard.any_direction_pressed) {
                    bool cursor_is_here = ((s8)i == cursor_x) && ((s8)j == cursor_y);
                    cell.ui->is_selected = cursor_is_here || (input->keyboard.shift_or_control_down && cell.ui->is_selected);
                }

            }
        }


        // phase 2

        // NOTE this begin/end TextureMode stuff is because if it was inside
        // this 9*9 loop, it drags the fps down to 30.
        if (context.debug_draw_smaller_cell_hitbox) BeginTextureMode(context.debug_texture);
        for (u32 j = 0; j < SUDOKU_SIZE; j++) {
            for (u32 i = 0; i < SUDOKU_SIZE; i++) {
                Sudoku_Cell cell        = get_cell(context.sudoku, i, j);
                Rectangle   cell_bounds = get_cell_bounds(context.sudoku, i, j);

                // selected stuff, mouse dragging
                Rectangle smaller_hitbox = ShrinkRectangle(cell_bounds, SUDOKU_CELL_SMALLER_HITBOX_SIZE);
                if (context.debug_draw_smaller_cell_hitbox) DrawRectangleRec(smaller_hitbox, ColorAlpha(YELLOW, 0.5));

                if (input->mouse.left.down && CheckCollisionPointRec(input->mouse.pos, smaller_hitbox)) {
                    cell.ui->is_selected = when_dragging_to_set_selected_to;
                    cursor_x = i; cursor_y = j; // move the cursor here as well.
                }
            }
        }
        if (context.debug_draw_smaller_cell_hitbox) EndTextureMode();



        if (clicked_on_box) {
            cursor_x = click_i; cursor_y = click_j; // put cursor wherever mouse is.

            if (input->mouse.left.double_clicked) {
                // @Hack, this variable will make the cell de-select right away,
                // fix it here.
                //
                // this code is becomeing dangerous.
                when_dragging_to_set_selected_to = true;


                Sudoku_Cell cell = get_cell(context.sudoku, click_i, click_j);

                if (*cell.digit != NO_DIGIT_PLACED) {
                    // select every digit that is this digit
                    s8 digit_to_select = *cell.digit;
                    for (u32 j = 0; j < SUDOKU_SIZE; j++) {
                        for (u32 i = 0; i < SUDOKU_SIZE; i++) {
                            Sudoku_Cell this_cell = get_cell(context.sudoku, i, j);

                            if (*this_cell.digit == digit_to_select)    this_cell.ui->is_selected = true;
                        }
                    }

                } else {
                    // select every empty cell
                    // TODO be smarter and think about the marking and colors for this
                    for (u32 j = 0; j < SUDOKU_SIZE; j++) {
                        for (u32 i = 0; i < SUDOKU_SIZE; i++) {
                            Sudoku_Cell this_cell = get_cell(context.sudoku, i, j);
                            if (*this_cell.digit == NO_DIGIT_PLACED)    this_cell.ui->is_selected = true;
                        }
                    }
                }
            }
        }


        // all the selection stuff has now happened. now do some animation stuff.
        {
            bool selected_changed = false;
            for (u32 j = 0; j < SUDOKU_SIZE; j++) {
                for (u32 i = 0; i < SUDOKU_SIZE; i++) {
                    bool is_selected = cell_is_selected(&context.sudoku->ui, i, j);

                    if (is_selected != previous_ui.grid[j][i].is_selected) {
                        selected_changed = true;
                        break;
                    }
                }
            }

            if (selected_changed) {
                play_sound("selection_changed");
                Selected_Animation new_animation = {
                    .t_animation   = 0,
                    .prev_ui_state = previous_ui,
                    .curr_ui_state = context.sudoku->ui,
                };

                Array_Append(&context.selection_animation_array, new_animation);
            }
        }
    }



    ////////////////////////////////////////////////
    //              Placeing Digits
    ////////////////////////////////////////////////
    if (number_pressed != NO_DIGIT_PLACED) {
        for (u32 j = 0; j < SUDOKU_SIZE; j++) {
            for (u32 i = 0; i < SUDOKU_SIZE; i++) {
                Sudoku_Cell cell = get_cell(context.sudoku, i, j);
                if (!cell.ui->is_selected) continue;


                bool has_digit = (*cell.digit != NO_DIGIT_PLACED);
                bool has_builder_digit = has_digit && !(*cell.digit_placed_in_solve_mode);
                bool slot_is_modifiable = (in_solve_mode && !has_builder_digit) || !in_solve_mode;

                switch (layer_to_place) {
                    case SUL_DIGIT: {
                        if (slot_is_modifiable) remove_number_this_press = remove_number_this_press && (*cell.digit == number_pressed);
                    } break;
                    case SUL_CERTAIN: {
                        if (!has_digit) remove_number_this_press = remove_number_this_press && ((*cell.  certain) & (1 << (number_pressed)));
                    } break;
                    case SUL_UNCERTAIN: {
                        if (!has_digit) remove_number_this_press = remove_number_this_press && ((*cell.uncertain) & (1 << (number_pressed)));
                    } break;
                    case SUL_COLOR: {
                        remove_number_this_press = remove_number_this_press && ((*cell.color_bitfield) & (1 << (number_pressed)));
                    } break;
                }
            }
        }

        for (u32 j = 0; j < SUDOKU_SIZE; j++) {
            for (u32 i = 0; i < SUDOKU_SIZE; i++) {
                Sudoku_Cell cell = get_cell(context.sudoku, i, j);
                if (!cell.ui->is_selected) continue;


                bool has_digit = (*cell.digit != NO_DIGIT_PLACED);
                bool has_builder_digit = has_digit && !(*cell.digit_placed_in_solve_mode);
                bool slot_is_modifiable = (in_solve_mode && !has_builder_digit) || !in_solve_mode;

                switch (layer_to_place) {
                case SUL_DIGIT: {
                    if (slot_is_modifiable) {
                        if (remove_number_this_press) {
                            Place_Digit(&context.sudoku->grid, i, j, NO_DIGIT_PLACED);
                        } else {
                            Place_Digit(&context.sudoku->grid, i, j, number_pressed, .in_solve_mode = in_solve_mode);
                        }
                    }
                } break;

                case SUL_CERTAIN: {
                    if (!has_digit) {
                        if (remove_number_this_press)   *cell.  certain &= ~(1 << number_pressed);
                        else                            *cell.  certain |=  (1 << number_pressed);
                    }
                } break;

                case SUL_UNCERTAIN: {
                    if (!has_digit) {
                        if (remove_number_this_press)   *cell.uncertain &= ~(1 << number_pressed);
                        else                            *cell.uncertain |=  (1 << number_pressed);
                    }
                } break;

                case SUL_COLOR: {
                    if (remove_number_this_press)   *cell.color_bitfield &= ~(1 << number_pressed);
                    else                            *cell.color_bitfield |=  (1 << number_pressed);
                } break;
                }
            }
        }
    }


    ////////////////////////////////////////////////
    //             Removeing Digits
    ////////////////////////////////////////////////
    if (input->keyboard.delete_pressed) {
        for (u32 j = 0; j < SUDOKU_SIZE; j++) {
            for (u32 i = 0; i < SUDOKU_SIZE; i++) {
                Sudoku_Cell cell = get_cell(context.sudoku, i, j);
                if (!cell.ui->is_selected) continue;

                bool has_digit = (*cell.digit != NO_DIGIT_PLACED);
                bool has_builder_digit = has_digit && !(*cell.digit_placed_in_solve_mode);
                bool slot_is_modifiable = (in_solve_mode && !has_builder_digit) || !in_solve_mode;

                // TODO maybe if you have cntl press or something it dose something different?

                if (has_digit && slot_is_modifiable)    layer_to_delete = Min(layer_to_delete, SUL_DIGIT);
                if (*cell.  certain)            layer_to_delete = Min(layer_to_delete, SUL_CERTAIN);
                if (*cell.uncertain)            layer_to_delete = Min(layer_to_delete, SUL_UNCERTAIN);
                if (*cell.color_bitfield)       layer_to_delete = Min(layer_to_delete, SUL_COLOR);
            }
        }


        for (u32 j = 0; j < SUDOKU_SIZE; j++) {
            for (u32 i = 0; i < SUDOKU_SIZE; i++) {
                Sudoku_Cell cell = get_cell(context.sudoku, i, j);
                if (!cell.ui->is_selected) continue;

                bool has_digit = (*cell.digit != NO_DIGIT_PLACED);
                bool has_builder_digit = has_digit && !(*cell.digit_placed_in_solve_mode);
                bool slot_is_modifiable = (in_solve_mode && !has_builder_digit) || !in_solve_mode;

                switch (layer_to_delete) {
                case SUL_DIGIT: {
                    if (slot_is_modifiable) {
                        Place_Digit(&context.sudoku->grid, i, j, NO_DIGIT_PLACED);
                    }
                } break;
                case SUL_CERTAIN:   { *cell.  certain      = 0; } break;
                case SUL_UNCERTAIN: { *cell.uncertain      = 0; } break;
                case SUL_COLOR:     { *cell.color_bitfield = 0; } break;
                }
            }
        }
    }


    ////////////////////////////////////////////////
    //                Undo / Redo
    ////////////////////////////////////////////////
    if (!Sudoku_Grid_Is_The_Same_As_The_Last_Element_In_The_Undo_Buffer(context.sudoku)) {
        Array_Append(&context.sudoku->undo_buffer, context.sudoku->grid);
        context.sudoku->redo_count = 0;
    }


    ////////////////////////////////////////////////
    //               Sudoku Logic
    ////////////////////////////////////////////////
    // here we search for and check if the current digits/markings are
    // invalid by sudoku logic, and if they are, draw them red or something.

    // these arrays hold all the invalid digits for that cell.
    //
    // NOTE this array only needs to be as long as the max number of digits
    // that can be placed in a sudoku grid, however, this is C, it dose
    // not have slices. no further explantion needed.
    //
    // TODO where do we put this?
    Int_Array invalid_digits_array_grid[SUDOKU_SIZE][SUDOKU_SIZE] = ZEROED;
    {
        for (u32 j = 0; j < SUDOKU_SIZE; j++) {
            for (u32 i = 0; i < SUDOKU_SIZE; i++) {
                Int_Array *invalid_digits_array = &invalid_digits_array_grid[j][i];

                // use the scratch allocator.
                invalid_digits_array->allocator = context.scratch;

                { // search in the current box
                    static_assert(SUDOKU_SIZE == 9, "3 is 1/3 of SUDOKU_SIZE (witch is 9)");
                    u32 group_start_x = (i / 3) * 3;
                    u32 group_start_y = (j / 3) * 3;
                    for (u32 k = 0; k < 3; k++) {
                        for (u32 l = 0; l < 3; l++) {
                            u32 group_index_x = group_start_x + l;
                            u32 group_index_y = group_start_y + k;
                            // dont check yourself.
                            if (group_index_x == i && group_index_y == j) continue;

                            Sudoku_Cell cell = get_cell(context.sudoku, group_index_x, group_index_y);
                            // @Copypasta, but thats just C, being C
                            if (*cell.digit != NO_DIGIT_PLACED) {
                                if (index_in_array(invalid_digits_array, *cell.digit) == -1) {
                                    // if its not NO_DIGIT_PLACED, and its not in the
                                    // array allready, add it to the invalid_digits_array
                                    Array_Append(invalid_digits_array, *cell.digit);
                                }
                            }
                        }
                    }
                }

                // (search in the collum) && (search in the row)
                //
                // note. this double searches the things in its box.
                // but thats ok because we de-duplicate.
                for (u32 k = 0; k < SUDOKU_SIZE; k++) {
                    // row
                    if (k != i) {
                        Sudoku_Cell cell = get_cell(context.sudoku, k, j);
                        // @Copypasta, but thats just C, being C
                        if (*cell.digit != NO_DIGIT_PLACED) {
                            if (index_in_array(invalid_digits_array, *cell.digit) == -1) {
                                // if its not NO_DIGIT_PLACED, and its not in the
                                // array allready, add it to the invalid_digits_array
                                Array_Append(invalid_digits_array, *cell.digit);
                            }
                        }
                    }
                    // collum
                    if (k != j) {
                        Sudoku_Cell cell = get_cell(context.sudoku, i, k);
                        // @Copypasta, but thats just C, being C
                        if (*cell.digit != NO_DIGIT_PLACED) {
                            if (index_in_array(invalid_digits_array, *cell.digit) == -1) {
                                // if its not NO_DIGIT_PLACED, and its not in the
                                // array allready, add it to the invalid_digits_array
                                Array_Append(invalid_digits_array, *cell.digit);
                            }
                        }
                    }
                }

            }
        }
    }

    ////////////////////////////////////////////////
    //             draw sudoku grid
    ////////////////////////////////////////////////
    for (u32 j = 0; j < SUDOKU_SIZE; j++) {
        for (u32 i = 0; i < SUDOKU_SIZE; i++) {
            Sudoku_Cell cell        = get_cell(context.sudoku, i, j);
            Rectangle   cell_bounds = get_cell_bounds(context.sudoku, i, j);
            Int_Array *invalid_digits_array = &invalid_digits_array_grid[j][i];


            { // draw color shading / cell background
                Int_Array color_bits = ZEROED;
                color_bits.allocator = context.scratch;

                #define MAX_BITS_SET    (sizeof(*cell.color_bitfield)*8)
                static_assert(Array_Len(context.theme.sudoku.cell_color_bitfield) == MAX_BITS_SET, "no more than 32 colors please.");

                // loop over all bits, and get the index's of the colors.
                for (u32 k = 0; k < MAX_BITS_SET; k++) {
                    if (*cell.color_bitfield & (1 << k)) Array_Append(&color_bits, k);
                }


                // were always gonna draw the cell background, because some cell colors are transparent.
                DrawRectangleRec(cell_bounds, context.theme.sudoku.cell_background);

                if (color_bits.count == 0) {
                    // do nothing.
                } else if (color_bits.count == 1) {
                    // a very obvious special case. only one color
                    DrawRectangleRec(cell_bounds, context.theme.sudoku.cell_color_bitfield[color_bits.items[0]]);
                } else {

                    //
                    // We're going to split the cell into sections, and paint them with
                    // the colors the user selected in the bit mask,
                    //
                    // first generate points arond a circle, place them in the middle of the cell,
                    // then extend them until there past the edge of the rectangle,
                    // create a line between the middle and that point,
                    //
                    // find the point and line on the rectangle that intersects it,
                    // fint the intersection with the next point, in the list,
                    // turns that section into a bunch of triangles and render it.
                    //
                    Vector2 points[MAX_BITS_SET];
                    u32 points_count = color_bits.count;

                    // NOTE this option will always case lag, because its
                    // turning off and on again in such quick succsesstion,
                    //
                    // these gards will help, but if every cell has colors will still slow down.
                    if (context.debug_draw_color_points) BeginTextureMode(context.debug_texture);
                    for (u32 point_index = 0; point_index < points_count; point_index++) {
                        f32 offset = TAU * -0.03; // this is gonna help make the lines have a little slant. looks cooler.
                        f32 percent = (f32)point_index / (f32)points_count;

                        // -percent makes the points be done in clockwise order.
                        //
                        // * SUDOKU_CELL_SIZE means that the points are garenteed to be outside the cell.
                        // (but thay could be slightly smaller, SUDOKU_CELL_SIZE*sqrt(2)/2 is the real minimum size)
                        points[point_index].x = sinf(-percent * TAU + PI + offset) * SUDOKU_CELL_SIZE + SUDOKU_CELL_SIZE/2;
                        points[point_index].y = cosf(-percent * TAU + PI + offset) * SUDOKU_CELL_SIZE + SUDOKU_CELL_SIZE/2;

                        if (context.debug_draw_color_points) {
                            Color color = context.theme.sudoku.cell_color_bitfield[color_bits.items[point_index]];
                            DrawCircleV(Vector2Add(points[point_index], RectangleTopLeft(cell_bounds)), 3, color);
                        }
                    }
                    if (context.debug_draw_color_points) EndTextureMode();


                    Vector2 middle = {SUDOKU_CELL_SIZE/2, SUDOKU_CELL_SIZE/2};

                    Line *rectangle_lines = RectangleToLines((Rectangle){0, 0, SUDOKU_CELL_SIZE, SUDOKU_CELL_SIZE});
                    u32 rectangle_line_index = 0;
                    for (u32 point_index = 0; point_index < points_count; point_index++) {
                        Vector2 fan_points[8] = {middle};
                        u32 fan_points_count = 1;

                        Vector2 point = points[point_index];
                        Line line_checking = {.start = middle, .end = point};

                        bool seen_start = false;
                        bool seen_end   = false;


                        // 4 + 1 means we have to check the top edge of the rec again, could probablty
                        // restructure this loop so we dont have to check everty edge every time but what ever.
                        while (rectangle_line_index < 4 + 1) {
                            Line rec_line = rectangle_lines[rectangle_line_index % 4];

                            Vector2 hit_loc;
                            bool hit = CheckCollisionLinesL(line_checking, rec_line, &hit_loc);
                            if (!hit) {
                                if (seen_start) fan_points[fan_points_count++] = rec_line.end; // get the corner of the rectangle.
                                rectangle_line_index += 1;
                                continue;
                            }

                            if (!seen_start) {
                                seen_start = true;
                                fan_points[fan_points_count++] = hit_loc;
                                line_checking = (Line){.start = middle, .end = points[(point_index+1) % points_count]};
                                continue;
                            } else {
                                seen_end = true;
                                fan_points[fan_points_count++] = hit_loc;
                                break;
                            }

                        }

                        ASSERT(seen_start && seen_end);
                        ASSERT(fan_points_count <= 5);

                        Color color = context.theme.sudoku.cell_color_bitfield[color_bits.items[point_index]];
                        for (u32 fan_index = 0; fan_index < fan_points_count; fan_index++) {
                            fan_points[fan_index].x += cell_bounds.x;
                            fan_points[fan_index].y += cell_bounds.y;
                        }
                        DrawTrianglesFromStartPoint(fan_points, fan_points_count, color);
                    }

                }
            }




            { // draw cell surrounding frame
                Rectangle bit_bigger = GrowRectangle(cell_bounds, SUDOKU_CELL_INNER_LINE_THICKNESS/2);
                DrawRectangleFrameRec(bit_bigger, SUDOKU_CELL_INNER_LINE_THICKNESS, context.theme.sudoku.cell_lines);
            }


            if (*cell.digit != NO_DIGIT_PLACED) {
                // draw placed digit

                // TODO do it smart
                // char buf[2] = ZEROED;
                // DrawTextCodepoint();
                const char *text = TextFormat("%d", *cell.digit);

                Vector2 text_position = { cell_bounds.x + SUDOKU_CELL_SIZE/2, cell_bounds.y + SUDOKU_CELL_SIZE/2 };

                Color text_color = *cell.digit_placed_in_solve_mode ? context.theme.sudoku.cell_font_color_for_marking : context.theme.sudoku.cell_digit_font_color;
                if (*cell.digit_placed_in_solve_mode) {
                    text_color = context.theme.sudoku.cell_font_color_for_marking;
                }
                // if the digit is invalid.
                if (index_in_array(invalid_digits_array, *cell.digit) != -1) {
                    if (*cell.digit_placed_in_solve_mode) {
                        text_color = context.theme.sudoku.cell_digit_is_invalid;
                    } else {
                        // TODO
                    }
                }


                DrawTextCentered(GetFontWithSize(FONT_SIZE), text, text_position, text_color);
            } else {
                // draw uncertain and certain digits
                Int_Array uncertain_numbers = { .allocator = context.scratch };
                Int_Array certain_numbers   = { .allocator = context.scratch };

                for (u8 k = 0; k <= 9; k++) {
                    if (*cell.uncertain & (1 << k)) Array_Append(&uncertain_numbers, k);
                    if (*cell.  certain & (1 << k)) Array_Append(&  certain_numbers, k);
                }


                // draw uncertain numbers around the edge of the box
                if (uncertain_numbers.count) {
                    ASSERT(uncertain_numbers.count <= Array_Len(MARKING_LOCATIONS));
                    Font_And_Size font_and_size = GetFontWithSize(FONT_SIZE_UNCERTAIN); // what matters more? speed or binding energy?
                    for (u32 k = 0; k < uncertain_numbers.count; k++) {
                        // TODO do it smarter like above.
                        const char *text = TextFormat("%d", uncertain_numbers.items[k]);

                        Vector2 text_pos = { cell_bounds.x, cell_bounds.y + font_and_size.size/2 };
                        text_pos = Vector2Add(text_pos, MARKING_LOCATIONS[k]);
                        DrawTextCentered(font_and_size, text, text_pos, context.theme.sudoku.cell_font_color_for_uncerten);
                    }
                }


                // draw small certain numbers in the middle of the box
                if (certain_numbers.count) {
                    ASSERT(certain_numbers.count <= SUDOKU_MAX_MARKINGS);

                    char buf[SUDOKU_MAX_MARKINGS+1] = ZEROED;
                    for (u32 k = 0; k < certain_numbers.count; k++) buf[k] = '0' + certain_numbers.items[k];

                    s32 font_size = FONT_SIZE_MARKING_CERTAIN_MAX_SIZE;
                    if (certain_numbers.count > MARKING_ALLOWED_CERTAIN_UPTO_BEFORE_SHRINKING) {
                        font_size = Remap(
                            certain_numbers.count,
                            MARKING_ALLOWED_CERTAIN_UPTO_BEFORE_SHRINKING, SUDOKU_MAX_MARKINGS,
                            FONT_SIZE_MARKING_CERTAIN_MAX_SIZE, FONT_SIZE_MARKING_CERTAIN_MIN_SIZE
                        );
                    }

                    Vector2 text_pos = { cell_bounds.x + SUDOKU_CELL_SIZE/2, cell_bounds.y + SUDOKU_CELL_SIZE/2 };
                    DrawTextCentered(GetFontWithSize(font_size), buf, text_pos, context.theme.sudoku.cell_font_color_for_certen);
                }
            }


            // hovering
            if (cell.ui->is_hovering_over) {
                f32 factor = !cell.ui->is_selected ? 0.2 : 0.05; // make it lighter when it is selected.
                Color color = Fade(BLACK, factor); // cool trick // @Color

                // NOTE this code is not with the selection code because
                // it feels better when it acts immediately.
                DrawRectangleRec(ShrinkRectangle(cell_bounds, SUDOKU_CELL_INNER_LINE_THICKNESS/2), color);
            }
        }
    }


    { // do selected animation
        // how long it takes to go though one animation
        const f64 animation_speed_in_seconds = 0.2;

        f64 animation_speed = 1 / animation_speed_in_seconds;


        local_persist bool debug_slow_down_animation = false;
        toggle_when_pressed(&debug_slow_down_animation, KEY_F5);
        if (debug_slow_down_animation) animation_speed = 0.1;


        #define Square(x) ((x)*(x))

        // speed up the animation the more elements tere, are. to try and catch up
        animation_speed *= Square(context.selection_animation_array.count);
        f64 dt = input->time.dt * animation_speed;

        while (context.selection_animation_array.count > 0) {
            Selected_Animation *animation = &context.selection_animation_array.items[0];
            animation->t_animation += dt;

            if (animation->t_animation >= 1) {
                dt = animation->t_animation - 1;
                Array_Remove(&context.selection_animation_array, 0, 1);
            } else {
                break;
            }
        }


        Selected_Animation *animation = NULL;
        if (context.selection_animation_array.count > 0) {
            animation = &context.selection_animation_array.items[0];
        }

        draw_sudoku_selection(context.sudoku, animation);
    }



    {
        // Lines that seperate the Boxes
        for (u32 j = 0; j < SUDOKU_SIZE/3; j++) {
            for (u32 i = 0; i < SUDOKU_SIZE/3; i++) {
                Rectangle ul_box = get_cell_bounds(context.sudoku, i*3,     j*3    );
                Rectangle dr_box = get_cell_bounds(context.sudoku, i*3 + 2, j*3 + 2);

                Rectangle region_box = {
                    ul_box.x,
                    ul_box.y,
                    (dr_box.x + dr_box.width ) - ul_box.x,
                    (dr_box.y + dr_box.height) - ul_box.y,
                };

                DrawRectangleFrameRec(
                    GrowRectangle(region_box, SUDOKU_CELL_BOARDER_LINE_THICKNESS - SUDOKU_CELL_INNER_LINE_THICKNESS/2),
                    SUDOKU_CELL_BOARDER_LINE_THICKNESS,
                    context.theme.sudoku.box_lines
                );
            }
        }

        { // this outer box hits 1 extra pixel.
            Rectangle ul_box = get_cell_bounds(context.sudoku, 0, 0);
            Rectangle dr_box = get_cell_bounds(context.sudoku, SUDOKU_SIZE-1, SUDOKU_SIZE-1);

            Rectangle region_box = {
                ul_box.x,
                ul_box.y,
                (dr_box.x + dr_box.width ) - ul_box.x,
                (dr_box.y + dr_box.height) - ul_box.y,
            };

            DrawRectangleFrameRec(
                GrowRectangle(region_box, SUDOKU_CELL_BOARDER_LINE_THICKNESS),
                SUDOKU_CELL_BOARDER_LINE_THICKNESS,
                context.theme.sudoku.box_lines
            );
        }
    }



    // draw logged messages.
    draw_logger_frame(10, 40);

    // draw the debug stuff.
    DrawTextureRightsideUp(context.debug_texture.texture, 0, 0);

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


#define SUDOKU_DRAW_SELECTION_IMPLEMENTATION
#include "sudoku_draw_selection.h"

