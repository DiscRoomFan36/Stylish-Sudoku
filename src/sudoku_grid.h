
#ifndef SUDOKU_H_
#define SUDOKU_H_



#include "Bested.h"



////////////////////////////////////////////////////////////////////
//                            Defines
////////////////////////////////////////////////////////////////////


#define SUDOKU_SIZE                         9

#define SUDOKU_MAX_MARKINGS                 (SUDOKU_SIZE + 1) // Account for 0





////////////////////////////////////////////////////////////////////
// Sudoku Struct Definitions
////////////////////////////////////////////////////////////////////


// typedef s8 Sudoku_Digit;

#define NO_DIGIT_PLACED     -1





// NOTE lower = higher priority
typedef enum {
    SUL_DIGIT       = 0,
    SUL_CERTAIN     = 1,
    SUL_UNCERTAIN   = 2,
    SUL_COLOR       = 3,
} Sudoku_UI_Layer;



typedef struct {
    // NO_DIGIT_PLACED means no digit
    s8 digit;

    // real digits are Black, players digits are marking color
    bool digit_placed_in_solve_mode;

    u16 uncertain; // bitfield
    u16   certain; // bitfield

    u32 color_bitfield;
    // TODO maybe also lines, but they wouldn't be in this struct.

    // UI stuff
    bool is_selected;
    bool is_hovering_over;
    // all of the digits that are invalid by sudoku logic.
    Int_Array invalid_digits_array;

} Sudoku_Cell;

typedef struct {
    // TODO call this cell, or something
    // bitfields describing witch markings are there.
    Sudoku_Cell cells[SUDOKU_SIZE][SUDOKU_SIZE];

} Sudoku_Grid;

typedef struct {
    _Array_Header_;
    Sudoku_Grid *items;
} Sudoku_Grid_Array;



typedef struct {
    // how far along in the animation it is.
    // [0..1]
    f64 t_animation;

    Sudoku_Grid prev_grid_state;
    Sudoku_Grid curr_grid_state;

} Selected_Animation;

typedef struct {
    _Array_Header_;
    Selected_Animation *items;
} Selected_Animation_Array;



// SOA style, probably a bit overkill.
typedef struct {
    Sudoku_Grid grid;


    String name;
    #define SUDOKU_MAX_NAME_LENGTH 128
    char name_buf[SUDOKU_MAX_NAME_LENGTH+1];
    u32 name_buf_count;



    // Undo
    //
    // the last item in this buffer is always the current grid.
    // 
    // TODO how dose the invalid digits work with this?
    Sudoku_Grid_Array undo_buffer;
    u32 redo_count; // how many times you can redo.


    Selected_Animation_Array selection_animation_array;
} Sudoku;




// helper macro
#define ASSERT_VALID_SUDOKU_ADDRESS(i, j)       ASSERT(Is_Between((i), 0, SUDOKU_SIZE) && Is_Between((j), 0, SUDOKU_SIZE))


//
// helper macro to iterate over all cells in sudoku, might change the
// signature if we change the size of the sudoku, but thats not a
// problem for now.
//
#define FOREACH_IJ_OF_SUDOKU(i, j)      \
    for (u32 j = 0; j < SUDOKU_SIZE; j++) for (u32 i = 0; i < SUDOKU_SIZE; i++)


internal Sudoku_Cell *get_cell(Sudoku_Grid *grid, s8 i, s8 j);


typedef struct {
    bool in_solve_mode;

    bool dont_play_sound;
} Place_Digit_Opt;

// places a digit, also plays a sound when doing it.
internal void _Place_Digit(Sudoku_Grid *grid, s8 i, s8 j, s8 digit_to_place, Place_Digit_Opt opt);

#define Place_Digit(grid, i, j, digit_to_place, ...) _Place_Digit(grid, i, j, digit_to_place, (Place_Digit_Opt){ __VA_ARGS__ })

bool Sudoku_Grid_Is_The_Same_As_The_Last_Element_In_The_Undo_Buffer(Sudoku *sudoku);


// the big one, dose probably to much stuff.
//
// still uses the context for some stuff,
// like in_solve_mode, TODO remove that.
internal void handle_and_draw_sudoku(Sudoku *sudoku, s32 x, s32 y, s32 width, s32 height);


// returns if the action took place
internal bool undo_sudoku(Sudoku *sudoku);
internal bool redo_sudoku(Sudoku *sudoku);

// checks if the grid is the same as the last element
// in the undo buffer, and adds it if it is not.
//
// returns if the grid was added.
internal bool sudoku_maybe_add_grid_into_undo_buffer(Sudoku *sudoku);





///////////////////////////////////////////////////////////////////////////
//                          Save / Load Sudoku
///////////////////////////////////////////////////////////////////////////

// returns NULL on success, otherwise returns an error string.
internal const char *load_sudoku(const char *filename, Sudoku *result);
// returns whether or not the file was saved successfully.
internal bool save_sudoku(const char *filename, Sudoku *to_save);





#endif //  SUDOKU_H_























////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//                      SUDOKU_IMPLEMENTATION
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

#ifdef SUDOKU_IMPLEMENTATION





Sudoku_Cell *get_cell(Sudoku_Grid *grid, s8 i, s8 j) {
    ASSERT_VALID_SUDOKU_ADDRESS(i, j);
    ASSERT(grid);
    return &grid->cells[j][i];
}


void _Place_Digit(Sudoku_Grid *grid, s8 i, s8 j, s8 digit_to_place, Place_Digit_Opt opt) {
    ASSERT_VALID_SUDOKU_ADDRESS(i, j);
    ASSERT(Is_Between(digit_to_place, 0, 9) || digit_to_place == NO_DIGIT_PLACED);

    Sudoku_Cell *cell = get_cell(grid, i, j);

    // just dont do anything
    if (cell->digit == digit_to_place) return;

    // sound stuff
    if (!opt.dont_play_sound) {
        if (cell->digit == NO_DIGIT_PLACED) {
            play_sound("digit_placed_on_empty");
        } else {
            if (digit_to_place == NO_DIGIT_PLACED) {
                play_sound("digit_placed_to_erase");
            } else {
                play_sound("digit_placed_to_overwrite");
            }
        }
    }


    cell->digit = digit_to_place;

    if (digit_to_place == NO_DIGIT_PLACED) {
        ASSERT(opt.in_solve_mode == false);
        cell->digit_placed_in_solve_mode = false;
    } else {
        cell->digit_placed_in_solve_mode = opt.in_solve_mode;
    }
}


bool Sudoku_Grid_Is_The_Same_As_The_Last_Element_In_The_Undo_Buffer(Sudoku *sudoku) {
    ASSERT(sudoku);
    ASSERT(sudoku->undo_buffer.count > 0);

    // could easily be Grid_Cmp or something.
    Sudoku_Grid * this_grid = &sudoku->grid;

    // make sure the sudoku still has this field,
    // in case i end up switching this for undo_index or something.
    (void) sudoku->redo_count;

    Sudoku_Grid *other_grid = &sudoku->undo_buffer.items[sudoku->undo_buffer.count-1];

    FOREACH_IJ_OF_SUDOKU(i, j) {
        Sudoku_Cell * this_cell = get_cell( this_grid, i, j);
        Sudoku_Cell *other_cell = get_cell(other_grid, i, j);

        if (this_cell->digit                      != other_cell->digit)                        return false;
        if (this_cell->digit_placed_in_solve_mode != other_cell->digit_placed_in_solve_mode)   return false;
        if (this_cell->uncertain                  != other_cell->uncertain)                    return false;
        if (this_cell->certain                    != other_cell->certain)                      return false;
        if (this_cell->color_bitfield             != other_cell->color_bitfield)               return false;
    }
    return true;
}



bool undo_sudoku(Sudoku *sudoku) {
    play_sound("sudoku_undo");
    if (sudoku->undo_buffer.count > 1) {
        sudoku->undo_buffer.count   -= 1;
        sudoku->redo_count          += 1;
        sudoku->grid = sudoku->undo_buffer.items[sudoku->undo_buffer.count - 1];
        return true;
    }

    log("no sudoku to undo!");
    return false;
}
bool redo_sudoku(Sudoku *sudoku) {
    play_sound("sudoku_redo");
    if (sudoku->redo_count > 0) {
        sudoku->undo_buffer.count   += 1;
        sudoku->redo_count          -= 1;
        sudoku->grid = sudoku->undo_buffer.items[sudoku->undo_buffer.count - 1];
        return true;
    }

    log("no sudoku to redo!");
    return false;
}

bool sudoku_maybe_add_grid_into_undo_buffer(Sudoku *sudoku) {
    bool same = Sudoku_Grid_Is_The_Same_As_The_Last_Element_In_The_Undo_Buffer(sudoku);
    if (same) return false;

    // should i clean up this grid or something?
    Array_Append(&sudoku->undo_buffer, sudoku->grid);
    sudoku->redo_count = 0;

    return true;
}





////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
//                      Handle And Draw Sudoku
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////


typedef struct Draw_Sudoku_Boundary {
    // integer rectangle
    s32 x, y, width, height;

    // min of width and height
    s32 size; // = Min(width, height);

    f64 cell_size; // = size / 9;

    // these should be at least 1 pixel thick.
    f64 cell_inner_line_thickness; // = cell_size * theme->sudoku.cell_inner_line_thickness_factor;
    f64 boarder_line_thickness;    // = cell_size * theme->sudoku.boarder_line_thickness_factor;


    // TODO add all the other factors here, use the Min()
    // trick to make sure they are not zero.

} Draw_Sudoku_Boundary;

internal Draw_Sudoku_Boundary init_draw_sudoku_boundary_with_x_y_width_and_height(s32 x, s32 y, s32 width, s32 height) {
    Theme *theme = get_theme();

    s32 size = Min(width, height);
    f64 cell_size = size / 9.0;

    Draw_Sudoku_Boundary result = {
        .x = x,
        .y = y,
        .width  = width,
        .height = height,

        .size      = size,
        .cell_size = cell_size,

        // these should be at least 1 pixel thick
        .cell_inner_line_thickness = Max(cell_size * theme->sudoku.cell_inner_line_thickness_factor, 1),
        .boarder_line_thickness    = Max(cell_size * theme->sudoku.boarder_line_thickness_factor   , 1),
    };

    return result;
}

internal Rectangle get_cell_bounds_with_bounds(Draw_Sudoku_Boundary draw_bounds, s8 i, s8 j) {
    // while this doesn't necessarily cause problems, i want this to be
    // used wherever get_cell is, and them having the same properties is nice
    ASSERT_VALID_SUDOKU_ADDRESS(i, j);

    Rectangle sudoku_cell_bounds = {
        draw_bounds.x + i * draw_bounds.cell_size,
        draw_bounds.y + j * draw_bounds.cell_size,
        draw_bounds.cell_size,
        draw_bounds.cell_size,
    };

    // maybe round this down, have pixel perfect cells.

    return sudoku_cell_bounds;
}




internal void draw_sudoku_selection(Sudoku *sudoku, Draw_Sudoku_Boundary draw_bounds);


//
// draw a sudoku into these bounds, will be clamped to square shape
//
// (x, y, wight, heigh) -> in pixels
//
void handle_and_draw_sudoku(Sudoku *sudoku, s32 x, s32 y, s32 width, s32 height) {
    Context *context = get_context();
    Theme   *theme   = get_theme();
    Input   *input   = get_input();

    ASSERT(sudoku);

    Draw_Sudoku_Boundary draw_bounds = init_draw_sudoku_boundary_with_x_y_width_and_height(x, y, width, height);


    ////////////////////////////////
    //        Selection
    ////////////////////////////////

    // TODO put this local persist stuff into the sudoku struct.

    local_persist bool when_dragging_to_set_selected_to = true;
    if (!input->mouse.left.down) when_dragging_to_set_selected_to = true;


    // TODO put in context or something.
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
        // if shift is not pressed, and the cursor couldn't move to the cell,
        // but will be able to, it doesn't move.
        if (get_cell(&sudoku->grid, cursor_x, cursor_y)->is_selected) {
            cursor_x = prev_x;
            cursor_y = prev_y;
        }

        if (context->debug.draw_cursor_position) {
            Rectangle rec = get_cell_bounds_with_bounds(draw_bounds, cursor_x, cursor_y);
            DebugDraw(DrawCircle(rec.x + rec.width/2, rec.y + rec.height/2, rec.height/3, ColorAlpha(RED, 0.8)));
        }
    }


    ////////////////////////////////
    //       placing digits
    ////////////////////////////////
    Sudoku_UI_Layer layer_to_place;
    if      (input->keyboard.shift_down && input->keyboard.control_down) layer_to_place = SUL_COLOR;
    else if (input->keyboard.shift_down)                                 layer_to_place = SUL_UNCERTAIN;
    else if (input->keyboard.control_down)                               layer_to_place = SUL_CERTAIN;
    else                                                                 layer_to_place = SUL_DIGIT;

    // determines whether a number was pressed to put it into the grid.
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
        undo_sudoku(sudoku);
    }

    // TODO cntl-x should be cut, but we dont have that yet.
    if (input->keyboard.control_down && input->keyboard.key.x_pressed) {
        redo_sudoku(sudoku);
    }


    ////////////////////////////////////////////////
    //            update sudoku grid
    ////////////////////////////////////////////////

    {
        ////////////////////////////////////////////////
        //              Selection stuff
        ////////////////////////////////////////////////
        Sudoku_Grid previous_grid = sudoku->grid;

        bool clicked_on_box = false;
        s8 click_i, click_j;

        // phase 1
        FOREACH_IJ_OF_SUDOKU(i, j) {
            Sudoku_Cell *cell       = get_cell(&sudoku->grid, i, j);
            Rectangle    cell_bounds = get_cell_bounds_with_bounds(draw_bounds, i, j);


            // selected stuff
            bool mouse_is_over = CheckCollisionPointRec(input->mouse.pos, cell_bounds);
            cell->is_hovering_over = mouse_is_over;

            if (input->mouse.left.clicked) {
                if (mouse_is_over) {
                    clicked_on_box = true;
                    click_i = i; click_j = j;

                    if (input->keyboard.shift_or_control_down) {
                        // start of deselection drag
                        cell->is_selected = !cell->is_selected; // TOGGLE
                    } else {
                        // just this one will be selected
                        cell->is_selected = true;
                    }
                    when_dragging_to_set_selected_to = cell->is_selected;
                } else {
                    // only stay active if shift or cntl is down
                    cell->is_selected = cell->is_selected && input->keyboard.shift_or_control_down;
                }
            }

            if (input->keyboard.any_direction_pressed) {
                bool cursor_is_here = ((s8)i == cursor_x) && ((s8)j == cursor_y);
                cell->is_selected = cursor_is_here || (input->keyboard.shift_or_control_down && cell->is_selected);
            }
        }


        // phase 2

        // NOTE this begin/end TextureMode stuff is because if it was inside
        // this 9*9 loop, it drags the fps down to 30.
        if (context->debug.draw_smaller_cell_hitbox) BeginTextureMode(context->debug.debug_texture);
        FOREACH_IJ_OF_SUDOKU(i, j) {
            Sudoku_Cell *cell       = get_cell(&sudoku->grid, i, j);
            Rectangle    cell_bounds = get_cell_bounds_with_bounds(draw_bounds, i, j);

            // selected stuff, mouse dragging

            // dose this make sense at smaller sizes?
            //
            // TODO check if this works at smaller sizes
            const f64 cell_smaller_hitbox_size = draw_bounds.cell_size / 8;

            Rectangle smaller_hitbox = ShrinkRectangle(cell_bounds, cell_smaller_hitbox_size);
            if (context->debug.draw_smaller_cell_hitbox) DrawRectangleRec(smaller_hitbox, ColorAlpha(YELLOW, 0.5));

            if (input->mouse.left.down && CheckCollisionPointRec(input->mouse.pos, smaller_hitbox)) {
                cell->is_selected = when_dragging_to_set_selected_to;
                cursor_x = i; cursor_y = j; // move the cursor here as well.
            }
        }
        if (context->debug.draw_smaller_cell_hitbox) EndTextureMode();



        if (clicked_on_box) {
            cursor_x = click_i; cursor_y = click_j; // put cursor wherever mouse is.

            if (input->mouse.left.double_clicked) {
                // @Hack, this variable will make the cell de-select right away,
                // fix it here.
                //
                // this code is becoming dangerous.
                when_dragging_to_set_selected_to = true;


                Sudoku_Cell *cell = get_cell(&sudoku->grid, click_i, click_j);

                if (cell->digit != NO_DIGIT_PLACED) {
                    // select every digit that is this digit
                    s8 digit_to_select = cell->digit;
                    FOREACH_IJ_OF_SUDOKU(i, j) {
                        Sudoku_Cell *this_cell = get_cell(&sudoku->grid, i, j);

                        if (this_cell->digit == digit_to_select)   this_cell->is_selected = true;
                    }

                } else {
                    // select every empty cell
                    // TODO be smarter and think about the marking and colors for this
                    FOREACH_IJ_OF_SUDOKU(i, j) {
                        Sudoku_Cell *this_cell = get_cell(&sudoku->grid, i, j);
                        if (this_cell->digit == NO_DIGIT_PLACED)   this_cell->is_selected = true;
                    }
                }
            }
        }


        // all the selection stuff has now happened. now do some animation stuff.
        {
            bool selected_changed = false;
            FOREACH_IJ_OF_SUDOKU(i, j) {
                bool is_selected = get_cell(&sudoku->grid, i, j)->is_selected;
                bool previously_is_selected = get_cell(&previous_grid, i, j)->is_selected;

                if (is_selected != previously_is_selected) {
                    selected_changed = true;
                    break;
                }
            }

            if (selected_changed) {
                play_sound("selection_changed");
                Selected_Animation new_animation = {
                    .t_animation   = 0,
                    .prev_grid_state = previous_grid,
                    .curr_grid_state = sudoku->grid,
                };

                Array_Append(&sudoku->selection_animation_array, new_animation);
            }
        }
    }



    ////////////////////////////////////////////////
    //              Placing Digits
    ////////////////////////////////////////////////
    if (number_pressed != NO_DIGIT_PLACED) {
        // pre pass, to figure out what to do.
        FOREACH_IJ_OF_SUDOKU(i, j) {
            Sudoku_Cell *cell = get_cell(&sudoku->grid, i, j);
            if (!cell->is_selected) continue;


            bool has_digit          = (cell->digit != NO_DIGIT_PLACED);
            bool has_builder_digit  = has_digit && !(cell->digit_placed_in_solve_mode);
            // TODO accept in_solve_mode as an argument, solve / build / view mode.
            bool slot_is_modifiable = (context->in_solve_mode && !has_builder_digit) || !context->in_solve_mode;

            switch (layer_to_place) {
                case SUL_DIGIT: {
                    if (slot_is_modifiable) remove_number_this_press = remove_number_this_press && (cell->digit == number_pressed);
                } break;
                case SUL_CERTAIN: {
                    // TODO use DIGIT_BIT macro?
                    if (!has_digit) remove_number_this_press = remove_number_this_press && (cell->  certain & (1 << (number_pressed)));
                } break;
                case SUL_UNCERTAIN: {
                    if (!has_digit) remove_number_this_press = remove_number_this_press && (cell->uncertain & (1 << (number_pressed)));
                } break;
                case SUL_COLOR: {
                    remove_number_this_press = remove_number_this_press && (cell->color_bitfield & (1 << (number_pressed)));
                } break;
            }
        }

        // post pass, to actually do something based on remove_number_this_press.
        FOREACH_IJ_OF_SUDOKU(i, j) {
            Sudoku_Cell *cell = get_cell(&sudoku->grid, i, j);
            if (!cell->is_selected) continue;


            bool has_digit          = (cell->digit != NO_DIGIT_PLACED);
            bool has_builder_digit  = has_digit && !cell->digit_placed_in_solve_mode;
            bool slot_is_modifiable = (context->in_solve_mode && !has_builder_digit) || !context->in_solve_mode;

            switch (layer_to_place) {
            case SUL_DIGIT: {
                if (slot_is_modifiable) {
                    if (remove_number_this_press) {
                        Place_Digit(&sudoku->grid, i, j, NO_DIGIT_PLACED);
                    } else {
                        Place_Digit(&sudoku->grid, i, j, number_pressed, .in_solve_mode = context->in_solve_mode);
                    }
                }
            } break;

            case SUL_CERTAIN: {
                if (!has_digit) {
                    if (remove_number_this_press)   cell->  certain &= ~(1 << number_pressed);
                    else                            cell->  certain |=  (1 << number_pressed);
                }
            } break;

            case SUL_UNCERTAIN: {
                if (!has_digit) {
                    if (remove_number_this_press)   cell->uncertain &= ~(1 << number_pressed);
                    else                            cell->uncertain |=  (1 << number_pressed);
                }
            } break;

            case SUL_COLOR: {
                if (remove_number_this_press)       cell->color_bitfield &= ~(1 << number_pressed);
                else                                cell->color_bitfield |=  (1 << number_pressed);
            } break;
            }
        }
    }


    ////////////////////////////////////////////////
    //             Removeing Digits
    ////////////////////////////////////////////////
    if (input->keyboard.delete_pressed) {
        // pre pass, to figure out what to remove
        FOREACH_IJ_OF_SUDOKU(i, j) {
            Sudoku_Cell *cell = get_cell(&sudoku->grid, i, j);
            if (!cell->is_selected) continue;

            bool has_digit          = (cell->digit != NO_DIGIT_PLACED);
            bool has_builder_digit  = has_digit && !cell->digit_placed_in_solve_mode;
            bool slot_is_modifiable = (context->in_solve_mode && !has_builder_digit) || !context->in_solve_mode;

            // TODO maybe if you have cntl press or something it dose something different?

            if (has_digit && slot_is_modifiable)    layer_to_delete = Min(layer_to_delete, SUL_DIGIT);
            if (cell->  certain)            layer_to_delete = Min(layer_to_delete, SUL_CERTAIN);
            if (cell->uncertain)            layer_to_delete = Min(layer_to_delete, SUL_UNCERTAIN);
            if (cell->color_bitfield)       layer_to_delete = Min(layer_to_delete, SUL_COLOR);
        }

        // actually remove the layer we decided upon,
        FOREACH_IJ_OF_SUDOKU(i, j) {
            Sudoku_Cell *cell = get_cell(&sudoku->grid, i, j);
            if (!cell->is_selected) continue;

            bool has_digit          = (cell->digit != NO_DIGIT_PLACED);
            bool has_builder_digit  = has_digit && !(cell->digit_placed_in_solve_mode);
            bool slot_is_modifiable = (context->in_solve_mode && !has_builder_digit) || !context->in_solve_mode;

            switch (layer_to_delete) {
            case SUL_DIGIT: {
                if (slot_is_modifiable) {
                    Place_Digit(&sudoku->grid, i, j, NO_DIGIT_PLACED);
                }
            } break;
            case SUL_CERTAIN:   { cell->  certain      = 0; } break;
            case SUL_UNCERTAIN: { cell->uncertain      = 0; } break;
            case SUL_COLOR:     { cell->color_bitfield = 0; } break;
            }
        }
    }


    ////////////////////////////////////////////////
    // Add to undo buffer if something just changed
    ////////////////////////////////////////////////
    sudoku_maybe_add_grid_into_undo_buffer(sudoku);


    ////////////////////////////////////////////////
    //        Sudoku Logic - for highlighting
    ////////////////////////////////////////////////
    {
        // here we search for and check if the current digits/markings are
        // invalid by sudoku logic, and if they are, draw them red or something.
        FOREACH_IJ_OF_SUDOKU(i, j) {
            Sudoku_Cell *cell = get_cell(&sudoku->grid, i, j);

            // these arrays hold all the invalid digits for that cell.
            //
            // NOTE this array only needs to be as long as the max number of digits
            // that can be placed in a sudoku grid, however, this is C, it dose
            // not have slices. no further explanation needed.
            Int_Array *invalid_digits_array = &cell->invalid_digits_array;

            Mem_Zero(invalid_digits_array, sizeof(*invalid_digits_array));
            // use the scratch allocator.
            invalid_digits_array->allocator = context->scratch;


            // TODO compress these searches.

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

                        Sudoku_Cell *other_cell = get_cell(&sudoku->grid, group_index_x, group_index_y);
                        // @Copypasta, but thats just C, being C
                        if (other_cell->digit != NO_DIGIT_PLACED) {
                            if (index_in_array(invalid_digits_array, other_cell->digit) == -1) {
                                // if its not NO_DIGIT_PLACED, and its not in the
                                // array allready, add it to the invalid_digits_array
                                Array_Append(invalid_digits_array, other_cell->digit);
                            }
                        }
                    }
                }
            }

            // (search in the Column) && (search in the row)
            //
            // note. this double searches the things in its box.
            // but thats ok because we de-duplicate.
            for (u32 k = 0; k < SUDOKU_SIZE; k++) {
                // row
                if (k != i) {
                    Sudoku_Cell *other_cell = get_cell(&sudoku->grid, k, j);
                    // @Copypasta, but thats just C, being C
                    if (other_cell->digit != NO_DIGIT_PLACED) {
                        if (index_in_array(invalid_digits_array, other_cell->digit) == -1) {
                            // if its not NO_DIGIT_PLACED, and its not in the
                            // array allready, add it to the invalid_digits_array
                            Array_Append(invalid_digits_array, other_cell->digit);
                        }
                    }
                }
                // column
                if (k != j) {
                    Sudoku_Cell *other_cell = get_cell(&sudoku->grid, i, k);
                    // @Copypasta, but thats just C, being C
                    if (other_cell->digit != NO_DIGIT_PLACED) {
                        if (index_in_array(invalid_digits_array, other_cell->digit) == -1) {
                            // if its not NO_DIGIT_PLACED, and its not in the
                            // array allready, add it to the invalid_digits_array
                            Array_Append(invalid_digits_array, other_cell->digit);
                        }
                    }
                }
            }

        }
    }

    ////////////////////////////////////////////////
    //             draw sudoku grid
    ////////////////////////////////////////////////
    FOREACH_IJ_OF_SUDOKU(i, j) {
        Sudoku_Cell *cell        = get_cell(&sudoku->grid, i, j);
        Rectangle    cell_bounds = get_cell_bounds_with_bounds(draw_bounds, i, j);


        { // draw color shading / cell background
            Int_Array color_bits = ZEROED;
            color_bits.allocator = context->scratch;

            #define MAX_BITS_SET    (sizeof(cell->color_bitfield)*8)
            static_assert(Array_Len(theme->sudoku.cell_color_bitfield) == MAX_BITS_SET, "no more than 32 colors please.");

            // loop over all bits, and get the index's of the colors.
            for (u32 k = 0; k < MAX_BITS_SET; k++) {
                if (cell->color_bitfield & (1 << k)) Array_Append(&color_bits, k);
            }


            // were always gonna draw the cell background, because some cell colors are transparent.
            DrawRectangleRec(cell_bounds, theme->sudoku.cell_background_color);

            if (color_bits.count == 0) {
                // do nothing.
            } else if (color_bits.count == 1) {
                // a very obvious special case. only one color
                DrawRectangleRec(cell_bounds, theme->sudoku.cell_color_bitfield[color_bits.items[0]]);
            } else {

                //
                // We're going to split the cell into sections, and paint them with
                // the colors the user selected in the bit mask,
                //
                // first generate points around a circle, place them in the middle of the cell,
                // then extend them until they're past the edge of the rectangle,
                // create a line between the middle and that point,
                //
                // find the point and line on the rectangle that intersects it,
                // fint the intersection with the next point, in the list,
                // turns that section into a bunch of triangles and render it.
                //
                Vector2 points[MAX_BITS_SET];
                u32 points_count = color_bits.count;

                // NOTE this debug option will always case lag, because its
                // turning off and on again in such quick succession,
                //
                // these guards will help, but if every cell has colors will still slow down.
                if (context->debug.draw_color_points) BeginTextureMode(context->debug.debug_texture);

                for (u32 point_index = 0; point_index < points_count; point_index++) {
                    f32 offset = TAU * -0.03; // this is gonna help make the lines have a little slant. looks cooler.
                    f32 percent = (f32)point_index / (f32)points_count;

                    // -percent makes the points be done in clockwise order.
                    //
                    // * SUDOKU_CELL_SIZE means that the points are guaranteed to be outside the cell.
                    // (but they could be slightly smaller, SUDOKU_CELL_SIZE*sqrt(2)/2 is the real minimum size)
                    points[point_index].x = sinf(-percent * TAU + PI + offset) * draw_bounds.cell_size + draw_bounds.cell_size/2;
                    points[point_index].y = cosf(-percent * TAU + PI + offset) * draw_bounds.cell_size + draw_bounds.cell_size/2;

                    if (context->debug.draw_color_points) {
                        Color color = theme->sudoku.cell_color_bitfield[color_bits.items[point_index]];
                        DrawCircleV(Vector2Add(points[point_index], RectangleTopLeft(cell_bounds)), 3, color);
                    }
                }

                if (context->debug.draw_color_points) EndTextureMode();


                Vector2 middle = {draw_bounds.cell_size/2, draw_bounds.cell_size/2};

                Line *rectangle_lines = RectangleToLines((Rectangle){0, 0, draw_bounds.cell_size, draw_bounds.cell_size});
                u32 rectangle_line_index = 0;
                for (u32 point_index = 0; point_index < points_count; point_index++) {
                    Vector2 fan_points[8] = {middle};
                    u32 fan_points_count = 1;

                    Vector2 point = points[point_index];
                    Line line_checking = {.start = middle, .end = point};

                    bool seen_start = false;
                    bool seen_end   = false;


                    // 4 + 1 means we have to check the top edge of the rec again, could probably
                    // restructure this loop so we dont have to check every edge every time but what ever.
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

                    ASSERT(fan_points_count <= 5);

                    // this will crash if the rectangle is <= 0 pixels wide.
                    // ASSERT(seen_start && seen_end);
                    if (!(seen_start && seen_end)) {
                        // TODO de-dup errors.
                        log_error("when drawing backing color fan, did not find both start and end, was probably to small");
                        continue;
                    }

                    Color color = theme->sudoku.cell_color_bitfield[color_bits.items[point_index]];
                    for (u32 fan_index = 0; fan_index < fan_points_count; fan_index++) {
                        fan_points[fan_index].x += cell_bounds.x;
                        fan_points[fan_index].y += cell_bounds.y;
                    }
                    DrawTrianglesFromStartPoint(fan_points, fan_points_count, color);
                }

            }
        }



        { // draw cell surrounding frame
            Rectangle bit_bigger = GrowRectangle(cell_bounds, draw_bounds.cell_inner_line_thickness/2);
            DrawRectangleFrameRec(bit_bigger, draw_bounds.cell_inner_line_thickness, theme->sudoku.cell_lines_color);
        }


        // moved this down here for binding energy.
        Int_Array *invalid_digits_array = &cell->invalid_digits_array;

        if (cell->digit != NO_DIGIT_PLACED) {
            // draw placed digit

            // TODO do it smart
            // char buf[2] = ZEROED;
            // DrawTextCodepoint();
            const char *text = TextFormat("%d", cell->digit);

            Vector2 text_position = { cell_bounds.x + draw_bounds.cell_size/2, cell_bounds.y + draw_bounds.cell_size/2 };

            bool is_an_invalid_digit  = (index_in_array(invalid_digits_array, cell->digit) != -1);
            bool placed_in_solve_mode = cell->digit_placed_in_solve_mode;

            Color text_color;
            if        (!is_an_invalid_digit && !placed_in_solve_mode) {
                // normal builder digit
                text_color = theme->sudoku.cell_text.  valid_builder_digit_color;

            } else if (!is_an_invalid_digit &&  placed_in_solve_mode) {
                // normal solver digit
                text_color = theme->sudoku.cell_text.  valid_solver_digit_color;

            } else if ( is_an_invalid_digit && !placed_in_solve_mode) {
                // invalid builder digit
                text_color = theme->sudoku.cell_text.invalid_builder_digit_color;

            } else if ( is_an_invalid_digit &&  placed_in_solve_mode) {
                // invalid solver digit
                text_color = theme->sudoku.cell_text.invalid_solver_digit_color;

            } else {
                UNREACHABLE();
            }


            DrawTextCentered(GetFontWithSize(draw_bounds.cell_size * theme->sudoku.text.digit_size_factor), text, text_position, text_color);
        } else {
            // draw uncertain and certain digits
            Int_Array uncertain_numbers = { .allocator = context->scratch };
            Int_Array certain_numbers   = { .allocator = context->scratch };

            for (u8 k = 0; k <= 9; k++) {
                if (cell->uncertain & (1 << k)) Array_Append(&uncertain_numbers, k);
                if (cell->  certain & (1 << k)) Array_Append(&  certain_numbers, k);
            }


            // draw uncertain numbers around the edge of the box
            if (uncertain_numbers.count) {
                ASSERT(uncertain_numbers.count <= SUDOKU_MAX_MARKINGS);
                // what matters more? speed or binding energy?
                const f64 font_size = draw_bounds.cell_size * theme->sudoku.text.uncertain_digit_size_factor; 
                Font_And_Size font_and_size = GetFontWithSize(font_size);

                //
                // MARKING_LOCATIONS is the positions that each uncertain number
                // will be placed into the cell. there are in a range from [0..1],
                //
                // takes into account x position, but not y,
                // make sure to add font_size / 2 to y.
                //

                // a shortend name, MARKING_LOCATIONS is super long allready.
                const f64 factor = theme->sudoku.text.uncertain_digit_size_factor;
                //
                // NOTE i dont know how these #defines are influenced by
                // theme->sudoku.text.uncertain_digit_size_factor, could
                // be that when we change that number, the formatting looks
                // wack. might have to look into it when the theme gets
                // the possibility of changing.
                //
                #define MARKING_LR_PAD      (12.0 / 60.0)
                #define MARKING_UD_PAD      ( 5.0 / 60.0)
                #define MARKING_EXTRA_PAD   (10.0 / 60.0)
                const Vector2 MARKING_LOCATIONS[SUDOKU_MAX_MARKINGS] = {
                    /*  1 */ {        MARKING_LR_PAD                    ,         MARKING_UD_PAD           },   // left      top
                    /*  2 */ {1.0   - MARKING_LR_PAD                    ,         MARKING_UD_PAD           },   // right     top
                    /*  3 */ {        MARKING_LR_PAD                    , 1.0   - MARKING_UD_PAD - factor  },   // left      bottom
                    /*  4 */ {1.0   - MARKING_LR_PAD                    , 1.0   - MARKING_UD_PAD - factor  },   // right     bottom
                    /*  5 */ {1.0/2                                     ,         MARKING_UD_PAD           },   // middle    top
                    /*  6 */ {1.0/2                                     , 1.0   - MARKING_UD_PAD - factor  },   // middle    bottom
                    /*  7 */ {        MARKING_LR_PAD                    , 1.0/2 -                  factor/2},   // left      middle
                    /*  8 */ {1.0   - MARKING_LR_PAD                    , 1.0/2 -                  factor/2},   // right     middle
                    /*  9 */ {        MARKING_LR_PAD + MARKING_EXTRA_PAD, 1.0/2 -                  factor/2},   // left -ish middle
                    /* 10 */ {1.0   - MARKING_LR_PAD - MARKING_EXTRA_PAD, 1.0/2 -                  factor/2},   // right-ish middle
                };

                for (u32 k = 0; k < uncertain_numbers.count; k++) {
                    // TODO this will fail when introducing non-char markings
                    // char text[2] = {uncertain_numbers.items[k] + '0', 0};
                    const char *text = TextFormat("%d", uncertain_numbers.items[k]);
                    // use EVIL color when digit is ont valid.
                    Color text_color = theme->sudoku.cell_text.  valid_marking_uncertain_digit_color;
                    if (index_in_array(invalid_digits_array, uncertain_numbers.items[k]) != -1) {
                        text_color = theme->sudoku.cell_text.invalid_marking_uncertain_digit_color;
                    }

                    // kinda annoying that we have to do '+ font_size/2', but
                    // the MARKING_LOCATIONS is allready long enough, and i
                    // dont care to do more factorizing.
                    Vector2 text_base_pos = { cell_bounds.x, cell_bounds.y + font_size/2 };
                    // Lerp here is pretty much a '* draw_bounds.cell_size'
                    Vector2 lerp_position = {
                        Lerp(0, draw_bounds.cell_size, MARKING_LOCATIONS[k].x),
                        Lerp(0, draw_bounds.cell_size, MARKING_LOCATIONS[k].y),
                    };

                    Vector2 final_text_pos = Vector2Add(text_base_pos, lerp_position);
                    DrawTextCentered(font_and_size, text, final_text_pos, text_color);
                }
            }


            // draw small certain numbers in the middle of the box
            if (certain_numbers.count) {
                ASSERT(certain_numbers.count <= SUDOKU_MAX_MARKINGS);

                // TODO move into draw_bounds?
                f64 certain_digit_max_size = draw_bounds.cell_size * theme->sudoku.text.certain_digit_max_size_factor;
                f64 certain_digit_min_size = draw_bounds.cell_size * theme->sudoku.text.certain_digit_min_size_factor;

                f64 font_size = certain_digit_max_size;
                // the the number of certain numbers is two high, shrink the font size.
                if (certain_numbers.count > theme->sudoku.text.allowed_certain_digits_before_shrinking) {
                    font_size = Remap(
                        certain_numbers.count,
                        theme->sudoku.text.allowed_certain_digits_before_shrinking, SUDOKU_MAX_MARKINGS,
                        certain_digit_max_size, certain_digit_min_size
                    );
                }
                Font_And_Size font_and_size = GetFontWithSize(font_size);

                // get the length of the whole text.
                char buf[SUDOKU_MAX_MARKINGS+1] = ZEROED;
                for (u32 k = 0; k < certain_numbers.count; k++) buf[k] = '0' + certain_numbers.items[k];
                Vector2 total_text_size = MeasureTextEx(font_and_size.font, buf, font_and_size.size, 0);
                s32 total_text_length   = total_text_size.x;


                // draw the text one by one, so we can color
                // the text differently for sudoku logic reasons.
                Vector2 char_pos = { cell_bounds.x + draw_bounds.cell_size/2 - total_text_length/2, cell_bounds.y + draw_bounds.cell_size/2 - total_text_size.y/2 };

                for (u32 k = 0; k < certain_numbers.count; k++) {
                    s64 n = certain_numbers.items[k];
                    char c = '0' + n;


                    Color text_color;
                    if (index_in_array(invalid_digits_array, n) == -1) {
                        text_color = theme->sudoku.cell_text.  valid_marking_certain_digit_color;
                    } else {
                        text_color = theme->sudoku.cell_text.invalid_marking_certain_digit_color;
                    }

                    DrawTextCodepoint(font_and_size.font, c, char_pos, font_and_size.size, text_color);


                    // find the corresponding glyph for this char.
                    //
                    // were drawing this one chat at a time so we can color each char differently.
                    GlyphInfo *g = get_glyph_info_for_char(font_and_size.font, c);
                    ASSERT(g != NULL);

                    char_pos.x += g->advanceX;
                }
            }
        }


        // hovering
        if (cell->is_hovering_over) {
            // TODO make this a theme thing?
            f32 factor = !cell->is_selected ? 0.2 : 0.05; // make it lighter when it is selected.
            Color color = Fade(BLACK, factor); // cool trick // @Color?

            // NOTE this code is not with the selection code because
            // it feels better when it acts immediately.
            DrawRectangleRec(ShrinkRectangle(cell_bounds, draw_bounds.cell_inner_line_thickness/2), color);
        }
    }


    { // do selected animation
        // how long it takes to go though one animation
        const f64 animation_speed_in_seconds = 0.2;

        f64 animation_speed = 1 / animation_speed_in_seconds;


        local_persist bool debug_slow_down_animation = false;
        toggle_when_pressed(&debug_slow_down_animation, KEY_F5);
        if (debug_slow_down_animation) animation_speed = 0.1;


        // square macro is helpful, but it shouldn't be here, maybe up top?
        #define Square(x) ((x)*(x))

        // speed up the animation the more elements there are.
        // to try and catch back up.
        animation_speed *= Square(sudoku->selection_animation_array.count);
        f64 dt = input->time.dt * animation_speed;

        while (sudoku->selection_animation_array.count > 0) {
            Selected_Animation *animation = &sudoku->selection_animation_array.items[0];
            animation->t_animation += dt;

            if (animation->t_animation >= 1) {
                dt = animation->t_animation - 1;
                Array_Remove(&sudoku->selection_animation_array, 0, 1);
            } else {
                break;
            }
        }


        draw_sudoku_selection(sudoku, draw_bounds);
    }



    {
        // Lines that separate the Boxes
        for (u32 j = 0; j < SUDOKU_SIZE/3; j++) {
            for (u32 i = 0; i < SUDOKU_SIZE/3; i++) {
                Rectangle ul_box = get_cell_bounds_with_bounds(draw_bounds, i*3,     j*3    );
                Rectangle dr_box = get_cell_bounds_with_bounds(draw_bounds, i*3 + 2, j*3 + 2);

                Rectangle region_box = {
                    ul_box.x,
                    ul_box.y,
                    (dr_box.x + dr_box.width ) - ul_box.x,
                    (dr_box.y + dr_box.height) - ul_box.y,
                };

                DrawRectangleFrameRec(
                    GrowRectangle(region_box, draw_bounds.boarder_line_thickness - draw_bounds.cell_inner_line_thickness/2),
                    draw_bounds.boarder_line_thickness,
                    theme->sudoku.box_lines_color
                );
            }
        }

        { // this outer box hits 1 extra pixel.
            Rectangle ul_box = get_cell_bounds_with_bounds(draw_bounds, 0, 0);
            Rectangle dr_box = get_cell_bounds_with_bounds(draw_bounds, SUDOKU_SIZE-1, SUDOKU_SIZE-1);

            Rectangle region_box = {
                ul_box.x,
                ul_box.y,
                (dr_box.x + dr_box.width ) - ul_box.x,
                (dr_box.y + dr_box.height) - ul_box.y,
            };

            DrawRectangleFrameRec(
                GrowRectangle(region_box, draw_bounds.boarder_line_thickness),
                draw_bounds.boarder_line_thickness,
                theme->sudoku.box_lines_color
            );
        }
    }
}




////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
//                         Draw Sudoku Selection
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////
//             Directions
//////////////////////////////////////////

typedef enum {
    DIR_UP    = 0,
    DIR_RIGHT = 1,
    DIR_DOWN  = 2,
    DIR_LEFT  = 3,
} Direction;

internal Direction Opposite_Direction(Direction dir) {
    switch (dir) {
        case DIR_UP:    return DIR_DOWN;
        case DIR_DOWN:  return DIR_UP;
        case DIR_LEFT:  return DIR_RIGHT;
        case DIR_RIGHT: return DIR_LEFT;
    }
    UNREACHABLE();
}

internal Direction Next_Direction_Clockwise(Direction dir)         { return (dir + 1) % 4; }
internal Direction Next_Direction_Counter_Clockwise(Direction dir) { return (dir + 3) % 4; }



//////////////////////////////////////////
//          Rectangle Helpers
//////////////////////////////////////////

internal Rectangle Grow_Rectangle_In_Direction(Rectangle rec, Direction direction, f32 factor) {
    switch (direction) {
        case DIR_UP:    {
            rec.y += rec.height * (1-factor);
            rec.height *= factor;
        } break;

        case DIR_LEFT:  {
            rec.x += rec.width  * (1-factor);
            rec.width  *= factor;
        } break;

        case DIR_DOWN:  { rec.height *= factor; } break;
        case DIR_RIGHT: { rec.width  *= factor; } break;
    }

    return rec;
}


internal Rectangle Rectangle_Shrink_Both_Sides(Rectangle rec, Direction dir, f32 factor) {
    switch (dir) {
        case DIR_UP:
        case DIR_DOWN: {
            Rectangle final_rec = Grow_Rectangle_In_Direction(rec, DIR_UP, factor);
            final_rec.y -= rec.height/2 - final_rec.height/2;
            return final_rec;
        }

        case DIR_LEFT:
        case DIR_RIGHT: {
            Rectangle final_rec = Grow_Rectangle_In_Direction(rec, DIR_LEFT, factor);
            final_rec.x -= rec.width/2 - final_rec.width/2;
            return final_rec;
        }
    }

    UNREACHABLE();
}




//////////////////////////////////////////
//        Surrounding Helpers
//////////////////////////////////////////

// this struct is 8 bytes, probably doesn't matter for my purposes to pack these.
//
// starts at up and moves clockwise.
typedef union {
    struct {
        bool up;
        bool up_right;

        bool right;

        bool down_right;
        bool down;
        bool down_left;

        bool left;
        bool up_left;
    };

    bool as_array[8];
} Surrounding_Bools;

static_assert(sizeof(Surrounding_Bools) == Member_Size(Surrounding_Bools, as_array), "union must be correctly sized.");


internal Surrounding_Bools surrounding_bools_all_as(bool this) {
    Surrounding_Bools result;
    for (u32 i = 0; i < Array_Len(result.as_array); i++)   result.as_array[i] = this;
    return result;
}





internal Surrounding_Bools get_surrounding_is_selected(Sudoku_Grid *grid, u8 i, u8 j) {
    ASSERT(grid);
    ASSERT_VALID_SUDOKU_ADDRESS(i, j);
    Surrounding_Bools result = {
        .up         =                         j == 0                ? false : get_cell(grid, i  , j-1)->is_selected,
        .down       =                         j == SUDOKU_SIZE - 1  ? false : get_cell(grid, i  , j+1)->is_selected,
        .left       = i == 0                                        ? false : get_cell(grid, i-1, j  )->is_selected,
        .right      = i == SUDOKU_SIZE - 1                          ? false : get_cell(grid, i+1, j  )->is_selected,

        .up_left    = i == 0               || j == 0                ? false : get_cell(grid, i-1, j-1)->is_selected,
        .up_right   = i == SUDOKU_SIZE - 1 || j == 0                ? false : get_cell(grid, i+1, j-1)->is_selected,
        .down_left  = i == 0               || j == SUDOKU_SIZE-1    ? false : get_cell(grid, i-1, j+1)->is_selected,
        .down_right = i == SUDOKU_SIZE - 1 || j == SUDOKU_SIZE-1    ? false : get_cell(grid, i+1, j+1)->is_selected,
    };
    return result;
}

internal Surrounding_Bools surrounding_is_selected_to_draw_lines(Surrounding_Bools is_selected) {
    Surrounding_Bools result = {
        .up           = !is_selected.up,
        .down         = !is_selected.down,
        .left         = !is_selected.left,
        .right        = !is_selected.right,

        .up_left      = !is_selected.up   || !is_selected.left  || (!is_selected.up_left    && is_selected.up   && is_selected.left ),
        .up_right     = !is_selected.up   || !is_selected.right || (!is_selected.up_right   && is_selected.up   && is_selected.right),
        .down_left    = !is_selected.down || !is_selected.left  || (!is_selected.down_left  && is_selected.down && is_selected.left ),
        .down_right   = !is_selected.down || !is_selected.right || (!is_selected.down_right && is_selected.down && is_selected.right),
    };
    return result;
}



//////////////////////////////////////////
//     Surrounding + Direction
//////////////////////////////////////////


internal bool *get_surrounding_bool_direction(Surrounding_Bools *bools, Direction dir) {
    switch (dir) {
        case DIR_UP:    return &bools->up;
        case DIR_DOWN:  return &bools->down;
        case DIR_LEFT:  return &bools->left;
        case DIR_RIGHT: return &bools->right;
    }
    UNREACHABLE();
}

// from [0..7]
// internal s32 get_direction_to_surrounding_bool_index(Direction dir) {
//     Surrounding_Bools bools;

//     bool *ptr = get_surrounding_bool_direction(&bools, dir);
//     s32 index = (s32)(ptr - &bools.up);
//     ASSERT(Is_Between(index, 0, (s32)Array_Len(bools.as_array)-1));
//     return index;
// }

internal bool *get_corner_between(Surrounding_Bools *bools, Direction dir_1, Direction dir_2) {
    ASSERT(bools);
    ASSERT(dir_1 != dir_2);
    ASSERT(Opposite_Direction(dir_1) != dir_2);

    if (dir_1 > dir_2) Swap(dir_1, dir_2);

    if (dir_1 == DIR_UP) {
        if      (dir_2 == DIR_RIGHT)    return &bools->up_right;
        else if (dir_2 == DIR_LEFT)     return &bools->up_left;
    } else if (dir_1 == DIR_RIGHT) {
        if      (dir_2 == DIR_DOWN)     return &bools->down_right;
    } else if (dir_1 == DIR_DOWN) {
        if      (dir_2 == DIR_LEFT)     return &bools->down_left;
    }
    UNREACHABLE();
}

internal s32 get_corner_between_index(Direction dir_1, Direction dir_2) {
    Surrounding_Bools bools;

    bool *corner = get_corner_between(&bools, dir_1, dir_2);
    s32 index = (s32)(corner - &bools.up);
    ASSERT(Is_Between(index, 0, (s32)Array_Len(bools.as_array)-1));
    return index;
}





//////////////////////////////////////////
//           Drawing Helpers
//////////////////////////////////////////


internal void draw_selected_lines(Rectangle bounds, f32 thickness, Surrounding_Bools draw_lines, Color color) {
    // orthogonal
    Rectangle line_up           = { bounds.x + thickness,                   bounds.y,                               bounds.width - thickness*2, thickness,                   };
    Rectangle line_down         = { bounds.x + thickness,                   bounds.y + bounds.height - thickness,   bounds.width - thickness*2, thickness,                   };
    Rectangle line_left         = { bounds.x,                               bounds.y + thickness,                   thickness,                  bounds.height - thickness*2, };
    Rectangle line_right        = { bounds.x + bounds.width - thickness,    bounds.y + thickness,                   thickness,                  bounds.height - thickness*2, };
    // diagonal
    Rectangle line_up_left      = { bounds.x,                               bounds.y,                               thickness,                  thickness,                   };
    Rectangle line_up_right     = { bounds.x + bounds.width - thickness,    bounds.y,                               thickness,                  thickness,                   };
    Rectangle line_down_left    = { bounds.x,                               bounds.y + bounds.height - thickness,   thickness,                  thickness,                   };
    Rectangle line_down_right   = { bounds.x + bounds.width - thickness,    bounds.y + bounds.height - thickness,   thickness,                  thickness,                   };


    RectangleRemoveNegatives(&line_up);
    RectangleRemoveNegatives(&line_down);
    RectangleRemoveNegatives(&line_left);
    RectangleRemoveNegatives(&line_right);

    RectangleRemoveNegatives(&line_up_left);
    RectangleRemoveNegatives(&line_up_right);
    RectangleRemoveNegatives(&line_down_left);
    RectangleRemoveNegatives(&line_down_right);


    ClipRectangleAIntoRectangleB(bounds, &line_up);
    ClipRectangleAIntoRectangleB(bounds, &line_down);
    ClipRectangleAIntoRectangleB(bounds, &line_left);
    ClipRectangleAIntoRectangleB(bounds, &line_right);

    ClipRectangleAIntoRectangleB(bounds, &line_up_left);
    ClipRectangleAIntoRectangleB(bounds, &line_up_right);
    ClipRectangleAIntoRectangleB(bounds, &line_down_left);
    ClipRectangleAIntoRectangleB(bounds, &line_down_right);


    if (draw_lines.up        )   DrawRectangleRec(line_up,         color); //YELLOW);
    if (draw_lines.down      )   DrawRectangleRec(line_down,       color); //RED);
    if (draw_lines.left      )   DrawRectangleRec(line_left,       color); //PURPLE);
    if (draw_lines.right     )   DrawRectangleRec(line_right,      color); //GREEN);

    if (draw_lines.up_left   )   DrawRectangleRec(line_up_left,    color); //MAROON);
    if (draw_lines.up_right  )   DrawRectangleRec(line_up_right,   color); //ORANGE);
    if (draw_lines.down_left )   DrawRectangleRec(line_down_left,  color); //GOLD);
    if (draw_lines.down_right)   DrawRectangleRec(line_down_right, color); //PINK);
}

internal void draw_selected_lines_based_on_surrounding_is_selected(Rectangle bounds, f32 thickness, Surrounding_Bools is_selected, Color color) {
    Surrounding_Bools draw_lines = surrounding_is_selected_to_draw_lines(is_selected);
    draw_selected_lines(bounds, thickness, draw_lines, color);
}





//////////////////////////////////////////////////////////////////
//        The Big One  /  Main  /  Draw Sudoku Selection
//////////////////////////////////////////////////////////////////

void draw_sudoku_selection(Sudoku *sudoku, Draw_Sudoku_Boundary draw_bounds) {
    // used to be a call to get_context(), now it is much more clear what this function wants.
    Theme *theme = get_theme();

    ASSERT(sudoku);

    Selected_Animation *animation = NULL;
    if (sudoku->selection_animation_array.count > 0) {
        animation = &sudoku->selection_animation_array.items[0];
    }

    const f64 cell_inner_line_thickness = draw_bounds.cell_size * theme->sudoku.cell_inner_line_thickness_factor;
    const f64 select_line_thickness = draw_bounds.cell_size * theme->sudoku.select.line_thickness_factor;

    if (!animation) {
        // what to do if no animation.
        Sudoku_Grid *grid = &sudoku->grid;
        FOREACH_IJ_OF_SUDOKU(i, j) {
            if (!get_cell(grid, i, j)->is_selected) continue;

            Surrounding_Bools surrounding_is_selected = get_surrounding_is_selected(grid, i, j);
            Surrounding_Bools draw_lines = surrounding_is_selected_to_draw_lines(surrounding_is_selected);

            Rectangle cell_bounds       = get_cell_bounds_with_bounds(draw_bounds, i, j);
            Rectangle select_bounds     = ShrinkRectangle(cell_bounds, cell_inner_line_thickness/2);

            Color color = theme->sudoku.select.highlight_color;
            draw_selected_lines(select_bounds, select_line_thickness, draw_lines, color);
        }

        return;
    }

    ASSERT(animation);

    Sudoku_Grid *curr_grid = &animation->curr_grid_state;
    Sudoku_Grid *prev_grid = &animation->prev_grid_state;

    Surrounding_Bools curr_surrounding_is_selected_grid[SUDOKU_SIZE][SUDOKU_SIZE];
    Surrounding_Bools prev_surrounding_is_selected_grid[SUDOKU_SIZE][SUDOKU_SIZE];
    FOREACH_IJ_OF_SUDOKU(i, j) {
        curr_surrounding_is_selected_grid[j][i] = get_surrounding_is_selected(curr_grid, i, j);
        prev_surrounding_is_selected_grid[j][i] = get_surrounding_is_selected(prev_grid, i, j);
    }


    // not selected loop, for animation
    {
        f64 t = animation->t_animation;
        f64 factor = sqrt(t);

        FOREACH_IJ_OF_SUDOKU(i, j) {
            // if it is not selected this ui, and was selected in the previous ui,
            // fade it out.
            if (get_cell(curr_grid, i, j)->is_selected == true ) continue;
            if (get_cell(prev_grid, i, j)->is_selected == false) continue;

            // current_surrounding_is_selected
            Surrounding_Bools csis = curr_surrounding_is_selected_grid[j][i];
            // previous_surrounding_is_selected
            Surrounding_Bools psis = prev_surrounding_is_selected_grid[j][i];

            Rectangle cell_bounds       = get_cell_bounds_with_bounds(draw_bounds, i, j);
            Rectangle select_bounds     = ShrinkRectangle(cell_bounds, cell_inner_line_thickness/2);

            Color color = theme->sudoku.select.highlight_color;

            bool prev_no_surrounding_selected = !psis.up && !psis.right && !psis.down && !psis.left;
            bool curr_no_surrounding_selected = !csis.up && !csis.right && !csis.down && !csis.left;

            bool curr_only_up                =  csis.up && !csis.right && !csis.down && !csis.left;
            bool curr_only_right             = !csis.up &&  csis.right && !csis.down && !csis.left;
            bool curr_only_down              = !csis.up && !csis.right &&  csis.down && !csis.left;
            bool curr_only_left              = !csis.up && !csis.right && !csis.down &&  csis.left;

            if (prev_no_surrounding_selected && curr_no_surrounding_selected) {
                // ideally we would want all massive selections to shrink into themselves,
                // but i have no idea how to do that...
                //
                // grab all lines, make bounding box, shrink bounding box.
                // make all lines be within the box...
                //
                // TODO this could work ^
                select_bounds = ShrinkRectanglePercent(select_bounds, 1-factor);

            } else if (prev_no_surrounding_selected && curr_only_up)    {
                select_bounds.height  *= 1-factor;
                psis.up = true; // remove top edge of the select
                // remove the down edge of the thing up.
                curr_surrounding_is_selected_grid[j-1][i  ].down = true;

            } else if (prev_no_surrounding_selected && curr_only_right) {
                select_bounds.x += select_bounds.width  * factor;
                select_bounds.width  *= 1-factor;
                psis.right = true; // remove right edge of the select
                // remove the left edge of the thing right.
                curr_surrounding_is_selected_grid[j  ][i+1].left = true;

            } else if (prev_no_surrounding_selected && curr_only_down)  {
                select_bounds.y += select_bounds.height * factor;
                select_bounds.height *= 1-factor;
                psis.down = true; // remove down edge of the select
                // remove the up edge of the thing down.
                curr_surrounding_is_selected_grid[j+1][i  ].up = true;

            } else if (prev_no_surrounding_selected && curr_only_left)  {
                select_bounds.width  *= 1-factor;
                psis.left = true; // remove left edge of the select
                // remove the right edge of the thing left.
                curr_surrounding_is_selected_grid[j  ][i-1].right = true;


            } else {
                // else just fade away.
                // the big shrink is coming. run
                color = Fade(color, 1-factor);
            }

            draw_selected_lines_based_on_surrounding_is_selected(select_bounds, select_line_thickness, psis, color);
        }
    }



    // selected loop
    FOREACH_IJ_OF_SUDOKU(i, j) {
        if (!get_cell(curr_grid, i, j)->is_selected) continue;

        Surrounding_Bools csis = curr_surrounding_is_selected_grid[j][i];

        Rectangle cell_bounds       = get_cell_bounds_with_bounds(draw_bounds, i, j);
        Rectangle select_bounds     = ShrinkRectangle(cell_bounds, cell_inner_line_thickness/2);

        Color color = theme->sudoku.select.highlight_color;

        // if it was the same as last time, just render them normally.
        if (get_cell(prev_grid, i, j)->is_selected) {
            draw_selected_lines_based_on_surrounding_is_selected(select_bounds, select_line_thickness, csis, color);
            continue;
        }

        // if it was not selected on the previous ui, do some animation
        ASSERT(!get_cell(prev_grid, i, j)->is_selected);

        // previous_surrounding_is_selected
        Surrounding_Bools psis = prev_surrounding_is_selected_grid[j][i];

        f64 t = animation->t_animation;
        f64 factor = sqrt(t);

        bool draw_selected_bounds_at_the_end = true;

        enum {
            UP    = (1<<0), // 1
            RIGHT = (1<<1), // 2
            DOWN  = (1<<2), // 4
            LEFT  = (1<<3), // 8
        };

        u32 prev_selected = 0;
        if (psis.up)    prev_selected |= UP;
        if (psis.right) prev_selected |= RIGHT;
        if (psis.down)  prev_selected |= DOWN;
        if (psis.left)  prev_selected |= LEFT;

        switch (prev_selected) {
            case 0:     { //  0
                // grow from nothing
                select_bounds = ShrinkRectanglePercent(select_bounds, factor);
                color = Fade(color, factor);
            } break;

            case UP:    /*  1 */ { select_bounds = Grow_Rectangle_In_Direction(select_bounds, DIR_DOWN,  factor); } break;
            case LEFT:  /*  8 */ { select_bounds = Grow_Rectangle_In_Direction(select_bounds, DIR_RIGHT, factor); } break;
            case DOWN:  /*  2 */ { select_bounds = Grow_Rectangle_In_Direction(select_bounds, DIR_UP,    factor); } break;
            case RIGHT: /*  4 */ { select_bounds = Grow_Rectangle_In_Direction(select_bounds, DIR_LEFT,  factor); } break;


            case UP   | DOWN:    // 5
            case LEFT | RIGHT: { // 10
                // draw closeing from both sides
                draw_selected_bounds_at_the_end = false;

                Direction dir     = prev_selected & UP ? DIR_UP : DIR_LEFT;
                Direction opp_dir = Opposite_Direction(dir);

                {
                    // grows up / left
                    Surrounding_Bools draw_lines = surrounding_bools_all_as(true);
                    *get_surrounding_bool_direction(&draw_lines, opp_dir) = false;

                    Rectangle rec = Grow_Rectangle_In_Direction(select_bounds, dir, factor/2);

                    draw_selected_lines(rec, select_line_thickness, draw_lines, color);
                }


                {
                    // grows down / right
                    Surrounding_Bools draw_lines = surrounding_bools_all_as(true);
                    *get_surrounding_bool_direction(&draw_lines, dir) = false;

                    Rectangle rec = Grow_Rectangle_In_Direction(select_bounds, opp_dir, factor/2);

                    draw_selected_lines(rec, select_line_thickness, draw_lines, color);
                }
            } break;


            case UP   | RIGHT:   //  3
            case DOWN | RIGHT:   //  6
            case UP   | LEFT:    //  9
            case DOWN | LEFT:  { // 12

                // draw two sides coming together

                // NOTE
                //
                // this function assumes that (for the up right case)
                // the up and right edges of the box are not drawn.
                //
                // but if you just click, (without shift), those lines will be there,
                // (in the final product)
                //
                // however, in the case where its only up and right (and not up-right),
                // the loop that dose the animation for de-selection will de-select
                // the right and up edge of this animation cell. (makeing it look fine)
                //
                // however^2, if you select up, right AND up-right, this de-select dose
                // not happen, and the animation looks kinda fucked up.
                //
                // the interplay between de-selection and selection animations,
                // are the kind of edge cases programmers have nightmares about.
                //
                // instead of playing wack-a-mole with these edges,
                // the animations will stay messed up, until a later date
                // (aka probably never) wherein I replace this entire system
                // for something more advanced and complicated,
                // but wouldn't have to deal with these bugs by definition.
                // (aka the new system would handle it by definition.)
                //
                // TODO be a better programmer some day

                draw_selected_bounds_at_the_end = false;

                Direction dir;
                switch (prev_selected) {
                    case UP   | RIGHT: { dir = DIR_UP;    } break;
                    case UP   | LEFT:  { dir = DIR_LEFT;  } break;
                    case DOWN | RIGHT: { dir = DIR_RIGHT; } break;
                    case DOWN | LEFT:  { dir = DIR_DOWN;  } break;
                    default: UNREACHABLE();
                }


                Direction next_dir = Next_Direction_Clockwise(dir);

                Direction opp_dir      = Opposite_Direction(dir);
                Direction opp_next_dir = Opposite_Direction(next_dir);


                Surrounding_Bools draw_lines = surrounding_is_selected_to_draw_lines(csis);
                {
                    Surrounding_Bools draw_lines_extra = surrounding_bools_all_as(false);

                    s32 index_corner = get_corner_between_index(dir, next_dir);

                    draw_lines_extra.as_array[index_corner] = draw_lines.as_array[index_corner];

                    draw_selected_lines(select_bounds, select_line_thickness, draw_lines_extra, color);

                    draw_lines.as_array[index_corner] = false;
                }


                // so the lines cross each other, and look nice.
                // also dont want the lines to shrink to zero with 1-factor,
                f64 line_thickness_over_width = select_line_thickness / select_bounds.width;
                f64 inner_side_factor = Remap(factor, 0, 1, 1 + line_thickness_over_width, line_thickness_over_width);

                {
                    Rectangle outer_rec = Grow_Rectangle_In_Direction(select_bounds, opp_dir, factor);
                    Rectangle final_rec = Grow_Rectangle_In_Direction(outer_rec, next_dir, inner_side_factor);

                    draw_selected_lines(final_rec, select_line_thickness, draw_lines, color);
                }

                {
                    Rectangle outer_rec = Grow_Rectangle_In_Direction(select_bounds, opp_next_dir, factor);
                    Rectangle final_rec = Grow_Rectangle_In_Direction(outer_rec, dir, inner_side_factor);

                    draw_selected_lines(final_rec, select_line_thickness, draw_lines, color);
                }

            } break;


            case UP   | DOWN | RIGHT:   //  7
            case UP   | DOWN | LEFT:    // 13
            case UP   | LEFT | RIGHT:   // 11
            case DOWN | LEFT | RIGHT: { // 14
                // close in from 3 sides
                draw_selected_bounds_at_the_end = false;

                Direction dir_middle;
                switch (prev_selected) {
                    case UP   | DOWN | RIGHT: { dir_middle = DIR_RIGHT; } break;
                    case UP   | DOWN | LEFT:  { dir_middle = DIR_LEFT;  } break;
                    case UP   | LEFT | RIGHT: { dir_middle = DIR_UP;    } break;
                    case DOWN | LEFT | RIGHT: { dir_middle = DIR_DOWN;  } break;
                    default: UNREACHABLE();
                }


                Direction dir_left   = Next_Direction_Counter_Clockwise(dir_middle);
                Direction dir_right  = Next_Direction_Clockwise        (dir_middle);
                Direction dir_opp    = Opposite_Direction              (dir_middle);


                Surrounding_Bools draw_lines = surrounding_is_selected_to_draw_lines(csis);
                {
                    Surrounding_Bools draw_lines_the_small_corners = surrounding_bools_all_as(false);

                    s32 index_corner_middle_right = get_corner_between_index(dir_middle, dir_right);
                    s32 index_corner_middle_left  = get_corner_between_index(dir_middle, dir_left);

                    draw_lines_the_small_corners.as_array[index_corner_middle_right] = draw_lines.as_array[index_corner_middle_right];
                    draw_lines_the_small_corners.as_array[index_corner_middle_left]  = draw_lines.as_array[index_corner_middle_left];

                    draw_selected_lines(select_bounds, select_line_thickness, draw_lines_the_small_corners, color);

                    draw_lines.as_array[index_corner_middle_right] = false;
                    draw_lines.as_array[index_corner_middle_left]  = false;
                }


                // so the corners touch.
                f64 line_thickness_over_width = select_line_thickness / select_bounds.width;

                f64 half_factor = Remap(factor, 0, 1, 0, 0.5 + line_thickness_over_width/2);

                f64 inner_side_factor = Remap(factor, 0, 1, 1 + line_thickness_over_width, 0.5 + line_thickness_over_width);

                { // left
                    Surrounding_Bools this_draw_lines = draw_lines;
                    *get_surrounding_bool_direction(&this_draw_lines, dir_left) = true;
                    *get_corner_between(&this_draw_lines, dir_middle, dir_left) = true;

                    Rectangle outer_rec = Grow_Rectangle_In_Direction(select_bounds, dir_left,   half_factor);
                    Rectangle final_rec = Grow_Rectangle_In_Direction(outer_rec,     dir_middle, inner_side_factor);

                    draw_selected_lines(final_rec, select_line_thickness, this_draw_lines, color);
                }

                { // right
                    Surrounding_Bools this_draw_lines = draw_lines;
                    *get_surrounding_bool_direction(&this_draw_lines, dir_right) = true;
                    *get_corner_between(&this_draw_lines, dir_middle, dir_right) = true;

                    Rectangle outer_rec = Grow_Rectangle_In_Direction(select_bounds, dir_right,  half_factor);
                    Rectangle final_rec = Grow_Rectangle_In_Direction(outer_rec,     dir_middle, inner_side_factor);

                    draw_selected_lines(final_rec, select_line_thickness, this_draw_lines, color);
                }


                { // middle
                    // this is to meet in the middle with the others
                    Rectangle outer_rec = Grow_Rectangle_In_Direction(select_bounds, dir_opp, factor/2);
                    // // this is to shrink down to nothing.
                    Rectangle final_rec = Rectangle_Shrink_Both_Sides(outer_rec, dir_left, 1-factor);

                    draw_selected_lines(final_rec, select_line_thickness, draw_lines, color);
                }


            } break;


            case UP | DOWN | RIGHT | LEFT: { // 15
                // draw a shrinking Rect that makes it look like the outside is sinking in.
                Surrounding_Bools draw_lines = surrounding_bools_all_as(true);
                Rectangle shrinking_rectangle = ShrinkRectanglePercent(select_bounds, 1-factor);
                draw_selected_lines(shrinking_rectangle, select_line_thickness, draw_lines, color);
            } break;


            default: { UNREACHABLE(); } break;
        }

        if (draw_selected_bounds_at_the_end) {
            draw_selected_lines_based_on_surrounding_is_selected(select_bounds, select_line_thickness, csis, color);
        }
    }

}















///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//                          Save / Load Sudoku
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////





#define MAX_TEMP_FILE_SIZE      (1 * MEGABYTE)
// overwrites temporary buffer every call.
internal String temp_Read_Entire_File(const char *filename) {
    local_persist u8 temp_file_storeage[MAX_TEMP_FILE_SIZE];

    FILE *file = fopen(filename, "rb");
    String result = ZEROED;
    result.data = (void*) temp_file_storeage;

    if (file) {
        fseek(file, 0, SEEK_END);
        s64 size = Min((s64)MAX_TEMP_FILE_SIZE, ftell(file));
        fseek(file, 0, SEEK_SET);

        if (size >= 0) {
            result.length = size;
            fread(result.data, 1, result.length, file);
        }
        fclose(file);
    }

    return result;
}






typedef u32     Magic_Number;
typedef u32     Version_Number;


u8 SUDOKU_MAGIC_NUMBER_ARRAY[sizeof(Version_Number)] = {'S', 'U', 'D', 'K'};
#define SUDOKU_MAGIC_NUMBER         (*(u32*)SUDOKU_MAGIC_NUMBER_ARRAY)

#define CURRENT_SUDOKU_SAVE_VERSION_NUMBER      2



struct Save_Header {
    Magic_Number    magic_number;
    Version_Number  version_number;
};











/////////////////////////////////////////////////////////////////
//                          Load Version 1
/////////////////////////////////////////////////////////////////


typedef struct {
    struct Save_Header header;

    #define SUDOKU_SIZE_VERSION_1           9
    s8  digits_on_the_grid[SUDOKU_SIZE_VERSION_1][SUDOKU_SIZE_VERSION_1];

    struct Markings_V1 {
        u16 uncertain;
        u16   certain;
    } digit_markings_on_the_grid[SUDOKU_SIZE_VERSION_1][SUDOKU_SIZE_VERSION_1];
} Sudoku_Save_Struct_Version_1;



// returns NULL on success, else returns error message
internal const char *load_sudoku_version_1(String file, Sudoku *result) {
    // String file = temp_Read_Entire_File(filename);

    Sudoku_Save_Struct_Version_1 *save_struct = (void*) file.data;
    ASSERT(save_struct->header.version_number == 1);

    // Check if the file is valid.

    if (file.length != sizeof(Sudoku_Save_Struct_Version_1)) return temp_sprintf("incorrect file size for version 1, wanted (%ld), got (%ld)", sizeof(Sudoku_Save_Struct_Version_1), file.length);


    for (u8 j = 0; j < Array_Len(save_struct->digits_on_the_grid); j++) {
        for (u8 i = 0; i < Array_Len(save_struct->digits_on_the_grid[j]); i++) {
            s8 digit = save_struct->digits_on_the_grid[j][i];
            if (!Is_Between(digit, -1, 9)) return temp_sprintf("(%d, %d) was outside the acceptable range [-1, 9], was, %d", i, j, digit);

            struct Markings_V1 markings = save_struct->digit_markings_on_the_grid[j][i];
            (void) markings; // TODO validate this.
        }
    }


    // load into Sudoku_Grid
    if (result) {
        // TODO this assert is still relevent down here,
        // because the new sudoku grid might do something weird.
        //
        // one way to remove it would be have this function produce a
        // different struct that can always be turned into a sudoku grid somewhere else...
        static_assert(CURRENT_SUDOKU_SAVE_VERSION_NUMBER == 2, "to change when new version");
        static_assert(SUDOKU_SIZE_VERSION_1 <= SUDOKU_SIZE, "would be pretty dumb to miss this...");

        for (u8 j = 0; j < Array_Len(save_struct->digits_on_the_grid); j++) {
            for (u8 i = 0; i < Array_Len(save_struct->digits_on_the_grid[j]); i++) {
                Sudoku_Cell *cell = get_cell(&result->grid, i, j);

                // need to think about appropriate times to use this function, aka will this function have more side effects?
                // Place_Digit(&result->grid, i, j, save_struct->digits_on_the_grid[j][i], .dont_play_sound = true);
                cell->digit = save_struct->digits_on_the_grid[j][i];

                cell->digit_placed_in_solve_mode = false;
                cell->uncertain = save_struct->digit_markings_on_the_grid[j][i].uncertain;
                cell->  certain = save_struct->digit_markings_on_the_grid[j][i].  certain;
                cell->color_bitfield = 0;

                // clear these also.
                cell->is_selected = false;
                cell->is_hovering_over = false;
            }
        }
    }

    return NULL;
}












/////////////////////////////////////////////////////////////////
//                          Load Version 2
/////////////////////////////////////////////////////////////////



#define SUDOKU_SIZE_VERSION_2           9

typedef struct {
    union Cell_Version_2 {
        struct {
            s8 digit;
            b8 placed_in_solve_mode;

            u16 uncertain;
            u16 certain;

            u32 color_bitfield;
        };

        u64 padding[2]; // this struct must be 2 pointers long.

    } cells[SUDOKU_SIZE_VERSION_2][SUDOKU_SIZE_VERSION_2];
    static_assert(sizeof(union Cell_Version_2) == sizeof(u64)*2, "cannot be bigger than 2 pointers");
} Sudoku_Grid_Version_2;

// packed so it never changes
typedef struct __attribute__((packed)) {
    struct Save_Header header;


    Sudoku_Grid_Version_2 grid;

    u32 reserved_for_color; // may want a background color;

    u32 reserved_for_array_1; // reserved for use as pointers to extra rules
    u32 reserved_for_array_2; // reserved for use as pointers to extra rules


    u32 name_offset_in_file;
    u32 name_length;

    u32 reserved_for_description_1;
    u32 reserved_for_description_2;


    u32 undo_buffer_offset_in_file;
    u32 undo_buffer_length;

    u32 redo_count;

} Sudoku_Save_Struct_Version_2;
// TODO
static_assert(sizeof(Sudoku_Save_Struct_Version_2) == 1344, "this size never changes");

static_assert(CURRENT_SUDOKU_SAVE_VERSION_NUMBER == 2, "to change when new version");




internal const char *load_sudoku_version_2(String file, Sudoku *result) {
    Sudoku_Save_Struct_Version_2 *save_struct = (void*)file.data;
    ASSERT(save_struct->header.version_number == 2);

    // Check if the file is valid.

    if (file.length < sizeof(Sudoku_Save_Struct_Version_2)) return temp_sprintf("file is smaller that save struct, can only be bigger, wanted (%ld), got (%ld)", sizeof(Sudoku_Save_Struct_Version_2), file.length);

    // TODO more checks


    if (result) {
        for (u32 j = 0; j < SUDOKU_SIZE_VERSION_2; j++) {
            for (u32 i = 0; i < SUDOKU_SIZE_VERSION_2; i++) {
                Sudoku_Cell *cell = get_cell(&result->grid, i, j);

                cell->digit                         = save_struct->grid.cells[j][i].digit;
                cell->digit_placed_in_solve_mode    = save_struct->grid.cells[j][i].placed_in_solve_mode;
                cell->uncertain                     = save_struct->grid.cells[j][i].uncertain;
                cell->  certain                     = save_struct->grid.cells[j][i].  certain;
                cell->color_bitfield                = save_struct->grid.cells[j][i].color_bitfield;
            }
        }


        ASSERT(save_struct->name_length <= SUDOKU_MAX_NAME_LENGTH);                         // TODO move this to validation
        ASSERT(file.length >= save_struct->name_length + save_struct->name_offset_in_file); // TODO move this to validation
        char *name_start = file.data + save_struct->name_offset_in_file;
        Mem_Copy(result->name_buf, name_start, save_struct->name_length);


        // TODO move this to validation
        ASSERT(file.length >= save_struct->name_offset_in_file + (save_struct->undo_buffer_length + save_struct->redo_count) * sizeof(Sudoku_Grid_Version_2));
        Sudoku_Grid_Version_2 *undo_buffer_array = (void*) (file.data + save_struct->name_offset_in_file);

        for (u32 k = 0; k < save_struct->undo_buffer_length + save_struct->redo_count; k++) {
            Sudoku_Grid_Version_2 *undo_grid = &undo_buffer_array[k];
            Sudoku_Grid grid = ZEROED;

            for (u32 j = 0; j < SUDOKU_SIZE_VERSION_2; j++) {
                for (u32 i = 0; i < SUDOKU_SIZE_VERSION_2; i++) {
                    Sudoku_Cell *cell = get_cell(&grid, i, j);

                    cell->digit                         = undo_grid->cells[j][i].digit;
                    cell->digit_placed_in_solve_mode    = undo_grid->cells[j][i].placed_in_solve_mode;
                    cell->uncertain                     = undo_grid->cells[j][i].uncertain;
                    cell->  certain                     = undo_grid->cells[j][i].  certain;
                    cell->color_bitfield                = undo_grid->cells[j][i].color_bitfield;
                }
            }

            Array_Append(&result->undo_buffer, grid);
        }

        result->undo_buffer.count  -= save_struct->redo_count;
        result->redo_count          = save_struct->redo_count;

        // we dont do anything to this guy. its more of a runtime thing.
        // result->selection_animation_array
    }
    return NULL;
}







/////////////////////////////////////////////////////////////////
//                             Load
/////////////////////////////////////////////////////////////////




internal const char *load_sudoku(const char *filename, Sudoku *result) {
    String file = temp_Read_Entire_File(filename);
    if (file.length == 0) return "Could not read file for some reason, or file was empty.";

    if (file.length < sizeof(struct Save_Header)) return "File was to small to contain header and version number";

    struct Save_Header *save_header = (void*) file.data;
    if (save_header->magic_number != SUDOKU_MAGIC_NUMBER) return temp_sprintf("Magic number was incorrect. wanted (0x%X), got (0x%X)", SUDOKU_MAGIC_NUMBER, save_header->magic_number);

    switch (save_header->version_number) {
        case 1: return load_sudoku_version_1(file, result);
        case 2: return load_sudoku_version_2(file, result);
        default: return temp_sprintf("Unknown version number (%d)", save_header->version_number);
    }
}




/////////////////////////////////////////////////////////////////
//                             Save
/////////////////////////////////////////////////////////////////

internal bool save_sudoku(const char *filename, Sudoku *to_save) {
    ASSERT(to_save);



    static_assert(CURRENT_SUDOKU_SAVE_VERSION_NUMBER == 2, "to change when new version");
    log("Saving sudoku version %d", CURRENT_SUDOKU_SAVE_VERSION_NUMBER);

    Sudoku_Save_Struct_Version_2 save_struct = ZEROED;
    save_struct.header.magic_number = SUDOKU_MAGIC_NUMBER;
    save_struct.header.version_number = CURRENT_SUDOKU_SAVE_VERSION_NUMBER;


    static_assert(SUDOKU_SIZE_VERSION_2 <= SUDOKU_SIZE, "Would hit an assert here");
    for (u8 j = 0; j < Array_Len(save_struct.grid.cells); j++) {
        for (u8 i = 0; i < Array_Len(save_struct.grid.cells[j]); i++) {
            Sudoku_Cell *cell = get_cell(&to_save->grid, i, j);

            save_struct.grid.cells[j][i].digit                   = cell->digit;
            save_struct.grid.cells[j][i].placed_in_solve_mode    = cell->digit_placed_in_solve_mode;
            save_struct.grid.cells[j][i].uncertain               = cell->uncertain;
            save_struct.grid.cells[j][i].  certain               = cell->  certain;
            save_struct.grid.cells[j][i].color_bitfield          = cell->color_bitfield;
        }
    }



    save_struct.name_offset_in_file         = sizeof(save_struct);
    save_struct.name_length                 = to_save->name_buf_count;
    save_struct.undo_buffer_offset_in_file  = sizeof(save_struct) + to_save->name_buf_count;
    save_struct.undo_buffer_length          = to_save->undo_buffer.count;
    save_struct.redo_count                  = to_save->redo_count;


    // reserved stuff
    save_struct.reserved_for_color         = 0;
    save_struct.reserved_for_array_1       = 0;
    save_struct.reserved_for_array_2       = 0;
    save_struct.reserved_for_description_1 = 0;
    save_struct.reserved_for_description_2 = 0;


    String_Builder sb = ZEROED;
    sb.allocator = Scratch_Get();
    String_Builder_Ptr_And_Size(&sb, (void*)&save_struct, sizeof(save_struct));

    String_Builder_Ptr_And_Size(&sb, to_save->name_buf, to_save->name_buf_count);


    for (u32 k = 0; k < to_save->undo_buffer.count + to_save->redo_count; k++) {
        Sudoku_Grid *grid = &to_save->undo_buffer.items[k];

        Sudoku_Grid_Version_2 undo_grid = ZEROED;

        for (u32 j = 0; j < SUDOKU_SIZE_VERSION_2; j++) {
            for (u32 i = 0; i < SUDOKU_SIZE_VERSION_2; i++) {
                Sudoku_Cell *cell = get_cell(grid, i, j);
                // TODO use get_cell();
                undo_grid.cells[j][i].digit                = cell->digit;
                undo_grid.cells[j][i].placed_in_solve_mode = cell->digit_placed_in_solve_mode;
                undo_grid.cells[j][i].uncertain            = cell->uncertain;
                undo_grid.cells[j][i].certain              = cell->  certain;
                undo_grid.cells[j][i].color_bitfield       = cell->color_bitfield;
            }
        }

        String_Builder_Ptr_And_Size(&sb, (void*)&undo_grid, sizeof(undo_grid));
    }

    // Would be kinda awkward, we cant load this...
    u64 size_of_file = String_Builder_Count(&sb);
    if (size_of_file > MAX_TEMP_FILE_SIZE) {
        log_error("file is to big to fit into temporary buffer, is %.2fMB (%lu)", (f64)size_of_file / (f64)MEGABYTE, size_of_file);
    }


    bool result = false;
    FILE *f = fopen(filename, "wb");
    if (f) {
        String_Builder_To_File(&sb, f);
        fclose(f);
        result = true;
    }

    Scratch_Release(sb.allocator);
    return result && size_of_file < MAX_TEMP_FILE_SIZE;
}





#endif // SUDOKU_IMPLEMENTATION

