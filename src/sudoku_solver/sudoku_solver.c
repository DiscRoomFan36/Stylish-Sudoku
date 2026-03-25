
#include "Bested.h"


#define NUM_DIGITS 9
static_assert(NUM_DIGITS == 9, "a lot of this would have to change to allow for different board sizes");



typedef struct Sudoku_Digit_Grid {
    u8 digits[NUM_DIGITS][NUM_DIGITS];
} Sudoku_Digit_Grid;

// the API is that you fill out this struct, then pass it into the solver, and presto!
typedef struct {
    Sudoku_Digit_Grid grid;

    // TODO variant sudoku's stuff.
} Input_Sudoku_Puzzle;

typedef struct {
    bool sudoku_is_possible;

    Sudoku_Digit_Grid correct_grid;

    // TODO maybe something about a path.
} Sudoku_Solver_Result;


Sudoku_Solver_Result Solve_Sudoku(Input_Sudoku_Puzzle input_sudoku);





///////////////////////////////////////////////////////////////////////////////////////
//                              Internal Structures
///////////////////////////////////////////////////////////////////////////////////////

typedef struct Cell {
    // 0 means no digit placed.
    u8 placed_digit;
    // this is a bitfield.
    // all possible digits are [1 -> 9]
    u16 possible_digits;
} Cell;

typedef struct {
    Cell Cells[NUM_DIGITS][NUM_DIGITS];

    u32 number_of_digits_left_to_place;

    // a quick check to see if something is invalid.
    // is set in mark_and_place_digit()
    bool is_invalid;

} Sudoku_Solver_Struct;



typedef enum {
    RESULT_FAIL,
    RESULT_SUCCESS,
} Result_Status;

typedef struct {
    Result_Status status;
    Sudoku_Solver_Struct correct_result;
} Recur_Result;



///////////////////////////////////////////////////////////////////////////////////////
//                                  Helper Macros
///////////////////////////////////////////////////////////////////////////////////////

#define SOLVER_IS_SOLVED(solver) ((solver).number_of_digits_left_to_place == 0)

// maybe we should subtract 1 from this? the 0'th bit is not used.
#define DIGIT_BIT(digit) (1 << digit)

#define ALL_DIGIT_BITS  \
    (                   \
        DIGIT_BIT(1) |  \
        DIGIT_BIT(2) |  \
        DIGIT_BIT(3) |  \
        DIGIT_BIT(4) |  \
        DIGIT_BIT(5) |  \
        DIGIT_BIT(6) |  \
        DIGIT_BIT(7) |  \
        DIGIT_BIT(8) |  \
        DIGIT_BIT(9)    \
    )




///////////////////////////////////////////////////////////////////////////////////////
//                                FOREACH Macro's
///////////////////////////////////////////////////////////////////////////////////////

#define FOREACH_CELL(solver)                                        \
    for (                                                           \
        Cell *cell  = &(solver)->Cells[           0][           0]; \
        cell       <= &(solver)->Cells[NUM_DIGITS-1][NUM_DIGITS-1]; \
        cell += 1                                                   \
    )

#define FOREACH_CELL_IN_ROW(solver, row_number)                         \
    for (                                                               \
        Cell *cell  = &(solver)->Cells[(row_number)][           0];     \
        cell       <= &(solver)->Cells[(row_number)][NUM_DIGITS-1];     \
        cell       += 1                                                 \
    )

#define FOREACH_CELL_IN_COL(solver, col_number)                         \
    for (                                                               \
        Cell *cell  = &(solver)->Cells[           0][(col_number)];     \
        cell       <= &(solver)->Cells[NUM_DIGITS-1][(col_number)];     \
        cell       += Array_Len((solver)->Cells[0]) /* should be 9 */   \
    )

#define FOREACH_CELL_IN_BOX(solver, box_number)                                         \
    for (                                                                               \
        Cell *cell  = &(solver)->Cells[(box_number)/3 * 3    ][(box_number)%3 * 3];     \
        cell       <= &(solver)->Cells[(box_number)/3 * 3 + 2][(box_number)%3 * 3 + 2]; \
        cell = next_in_box_by_pointer((solver), (box_number), cell)                     \
    )


Cell *next_in_box_by_pointer(Sudoku_Solver_Struct *solver, size_t box_number, Cell *current_cell) {
    // [0..9)
    size_t box_base_row = (box_number / 3) * 3;
    size_t box_base_col = (box_number % 3) * 3;

    assert(Is_Between(box_number,     0, NUM_DIGITS-1));
    assert(Is_Between(box_base_row,   0, NUM_DIGITS-1));
    assert(Is_Between(box_base_col,   0, NUM_DIGITS-1));

    // assumes Cells is a contiguous array;
    assert(&solver->Cells[NUM_DIGITS-1][NUM_DIGITS-1] - &solver->Cells[0][0] == NUM_DIGITS*NUM_DIGITS - 1); // it doesn't contain the last one. 0 index'd

    // [0..45)
    size_t current_cell_index = current_cell - &solver->Cells[0][0];
    // [0..9)
    size_t current_cell_row   = current_cell_index / 9;
    size_t current_cell_col   = current_cell_index % 9;
    assert(current_cell_row/3*3 + current_cell_col/3 == box_number);

    // mapped into the box.
    // [0..3)
    size_t mapped_row   = current_cell_row - box_base_row;
    size_t mapped_col   = current_cell_col - box_base_col;
    // [0..9)
    size_t mapped_index = mapped_row * 3 + mapped_col;

    assert(Is_Between(mapped_row,   0, (NUM_DIGITS/3)-1));
    assert(Is_Between(mapped_col,   0, (NUM_DIGITS/3)-1));
    assert(Is_Between(mapped_index, 0,  NUM_DIGITS   -1));

    size_t next_index_in_box = mapped_index + 1;

    Cell *next_cell = &solver->Cells[box_base_row + next_index_in_box/3][box_base_col + next_index_in_box%3];
    return next_cell;
}



///////////////////////////////////////////////////////////////////////////////////////
//                                 Helper Functions
///////////////////////////////////////////////////////////////////////////////////////

Sudoku_Solver_Struct init_solver(void) {
    Sudoku_Solver_Struct solver = {
        .Cells = ZEROED,

        .is_invalid = false,

        .number_of_digits_left_to_place = NUM_DIGITS*NUM_DIGITS,
    };

    {
        // gotta set all possible digits to "all possible digits",
        // this cannot be zero initalized.
        u16 all_possible_digits = 0;
        for (size_t digit = 1; digit <= 9; digit++) {
            all_possible_digits |= DIGIT_BIT(digit);
        }

        FOREACH_CELL(&solver) {
            cell->placed_digit    = 0;
            cell->possible_digits = all_possible_digits;
        }
    }

    return solver;
}



// is this needed?
size_t cell_to_index(Sudoku_Solver_Struct *solver, Cell *cell) {
    size_t index = cell - &solver->Cells[0][0];
    return index;
}

typedef struct {
    s32 x, y;
} Vec2i;

Vec2i cell_to_xy(Sudoku_Solver_Struct *solver, Cell *cell) {
    size_t index = cell_to_index(solver, cell);
    Vec2i result = {
        .x = index % NUM_DIGITS,
        .y = index / NUM_DIGITS,
    };
    return result;
}

size_t xy_to_index(s32 x, s32 y) {
    return y*NUM_DIGITS + x;
}

size_t xy_to_box_index(s32 x, s32 y) {
    return y/3*3 + x/3;
}
#define xy_to_box_index_v2(pos) xy_to_box_index((pos).x, (pos).y)

Vec2i box_and_index_to_xy(size_t box_index, size_t index_in_box) {
    Vec2i result = {
        .x = box_index%3*3 + index_in_box%3,
        .y = box_index/3*3 + index_in_box/3,
    };
    return result;
}



void mark_and_place_digit(Sudoku_Solver_Struct *solver, size_t x, size_t y, u8 digit) {
    // zero index'd
    assert(Is_Between(x, 0, NUM_DIGITS-1));
    assert(Is_Between(y, 0, NUM_DIGITS-1));

    Cell *cell = &solver->Cells[y][x];

    // make sure nothing was placed here before.
    assert(cell->placed_digit == 0);

    // must have been possible to place this digit in the first place.
    assert(cell->possible_digits & DIGIT_BIT(digit));

    cell->placed_digit = digit;
    // set to zero, no-one can place anything here anymore.
    cell->possible_digits = 0;
    solver->number_of_digits_left_to_place -= 1;

    //
    // remove this possibility from things that can see this cell.
    //
    // first check if it was the only possibility for a cell, if so mark as a invalid sudoku.
    //
    // then remove the possibility from the cell.
    //
    // seen_in_xxx is used to detect when a digit is no longer able
    // to be placed in a row col or box. this check dose not guarantee
    // that no such digit has appeared, but it shouldn't take to much
    // extra time.
    //
    {
        u16 seen_in_xxx = 0;
        FOREACH_CELL_IN_ROW(solver, y) {
            if (cell->possible_digits == DIGIT_BIT(digit))   solver->is_invalid = true;
            cell->possible_digits &= ~DIGIT_BIT(digit);

            if (cell->placed_digit != 0)   seen_in_xxx |= DIGIT_BIT(cell->placed_digit);
            seen_in_xxx |= cell->possible_digits;
        }
        if (seen_in_xxx != ALL_DIGIT_BITS) solver->is_invalid = true;
    }
    {
        u16 seen_in_xxx = 0;
        FOREACH_CELL_IN_COL(solver, x) {
            if (cell->possible_digits == DIGIT_BIT(digit))   solver->is_invalid = true;
            cell->possible_digits &= ~DIGIT_BIT(digit);

            if (cell->placed_digit != 0)   seen_in_xxx |= DIGIT_BIT(cell->placed_digit);
            seen_in_xxx |= cell->possible_digits;
        }
        if (seen_in_xxx != ALL_DIGIT_BITS) solver->is_invalid = true;
    }
    {
        u16 seen_in_xxx = 0;
        // hope this xy_to_box_index(x, y) is not called 5 times
        FOREACH_CELL_IN_BOX(solver, xy_to_box_index(x, y)) {
            if (cell->possible_digits == DIGIT_BIT(digit))   solver->is_invalid = true;
            cell->possible_digits &= ~DIGIT_BIT(digit);

            if (cell->placed_digit != 0)   seen_in_xxx |= DIGIT_BIT(cell->placed_digit);
            seen_in_xxx |= cell->possible_digits;
        }
        if (seen_in_xxx != ALL_DIGIT_BITS) solver->is_invalid = true;
    }
}


//
// counts the number of 1 bits
//
// when all you have is a hammer.
//
u32 pop_count(u32 x) {
    #if __has_builtin(__builtin_popcount)
        return __builtin_popcount(x);
    #else
        #error "Your Environment Is Bad";
        u32 count = 0;
        while (x != 0) {
            x &= x - 1;
            count += 1;
        }
        return count;
    #endif
}

u32 count_trailing_zeros(u32 x) {
    #if __has_builtin(__builtin_ctz)
        return __builtin_ctz(x);
    #else
        #error "Your Environment Is Bad";
        assert(x != 0);
        for (size_t i = 0; i < sizeof(x)*8; i++) {
            if (x & (1 << i)) return i;
        }
        UNREACHABLE();
    #endif
}




///////////////////////////////////////////////////////////////////////////////////////
//                                Solver Functions
///////////////////////////////////////////////////////////////////////////////////////


Recur_Result recur_and_solve_sudoku(Sudoku_Solver_Struct solver);

bool check_sudoku_is_invalid_by_no_way_to_put_digit_in_row_col_or_box(Sudoku_Solver_Struct *solver);

bool check_for_naked_singles(Sudoku_Solver_Struct *solver);
bool check_for_single_in_row_col_and_box(Sudoku_Solver_Struct *solver);



Sudoku_Solver_Result Solve_Sudoku(Input_Sudoku_Puzzle input_sudoku) {
    // initialize the solver
    Sudoku_Solver_Struct solver = init_solver();

    // place the digits given by the input sudoku.
    // 
    // TODO could this be a FOREACH_CELL?
    for (size_t j = 0; j < NUM_DIGITS; j++) {
        for (size_t i = 0; i < NUM_DIGITS; i++) {
            u8 digit = input_sudoku.grid.digits[j][i];
            if (!Is_Between(digit, 0, 9)) {
                PANIC("Solve_Sudoku: invalid input, got digit that was not in range, expected [0..9], was %d (at cell (%zu,%zu))", digit, i, j);
            }
            if (digit == 0) { continue; }

            Cell *cell = &solver.Cells[j][i];
            // check if the digit is possible to place.
            if ((cell->possible_digits & DIGIT_BIT(digit)) == 0)   goto exit_failure;

            // the above check prevents this from panic'ing.
            mark_and_place_digit(&solver, i, j, digit);
            if (solver.is_invalid)   goto exit_failure;
        }
    }


    Recur_Result result = recur_and_solve_sudoku(solver);
    if (result.status == RESULT_FAIL) {
        goto exit_failure;
    }

    Sudoku_Solver_Result sudoku_solver_result = ZEROED;
    sudoku_solver_result.sudoku_is_possible = true;
    FOREACH_CELL(&result.correct_result) {
        Vec2i pos = cell_to_xy(&result.correct_result, cell);

        assert(Is_Between(cell->placed_digit, 1, 9));
        sudoku_solver_result.correct_grid.digits[pos.y][pos.x] = cell->placed_digit;
    }

    return sudoku_solver_result;



exit_failure:
    // the user handed us an invalid sudoku from the very begining.
    return (Sudoku_Solver_Result){ .sudoku_is_possible = false };
}



Recur_Result recur_and_solve_sudoku(Sudoku_Solver_Struct solver) {
    // TODO do extra steps to reduce the number of possibilities.

    // check for deductions we can make.
    //
    // after a deduction we continue the while loop, hoping to make more.
    // while also checking to see if the sudoku suddenly became unsolvable.
    //
    // performance wise its a little weird to start
    // the loop from the start every time, but i think
    // its a good way to structure this.
    while (true) {
        if (solver.is_invalid) {
            Recur_Result result;
            result.status = RESULT_FAIL;
            return result;
        }

        if (SOLVER_IS_SOLVED(solver)) {
            Recur_Result result = ZEROED;
            result.status = RESULT_SUCCESS;
            result.correct_result = solver;
            return result;
        }

        if (check_sudoku_is_invalid_by_no_way_to_put_digit_in_row_col_or_box(&solver)) continue;

        if (check_for_naked_singles(&solver)) continue;
        if (check_for_single_in_row_col_and_box(&solver)) continue;

        // no deductions were made, nothing left to do here.
        break;
    }


    u32 min_possible_results = NUM_DIGITS+1;
    Vec2i pos = {-1, -1};
    FOREACH_CELL(&solver) {
        Vec2i cell_pos = cell_to_xy(&solver, cell);

        // only check places that dont have a digit.
        if (cell->placed_digit == 0) {
            u32 count = pop_count(cell->possible_digits);
            assert(Is_Between(count, 0, 9));

            if (count < min_possible_results) {
                min_possible_results = count;
                pos = cell_pos;
            }
        }
    }

    // TODO handle when grid is full, or something went wrong.
    assert(min_possible_results != NUM_DIGITS + 1);

    // we will handle this, but not in this loop.
    assert(min_possible_results != 0);
    assert(min_possible_results != 1);
    assert(pos.x != -1 && pos.y != -1);

    Cell *cell = &solver.Cells[pos.y][pos.x];
    for (size_t digit = 1; digit <= 9; digit++) {
        // make sure this digit is possible.
        if (!(cell->possible_digits & DIGIT_BIT(digit))) continue;

        Sudoku_Solver_Struct new_solver = solver;
        mark_and_place_digit(&new_solver, pos.x, pos.y, digit);

        Recur_Result result_of_recur = recur_and_solve_sudoku(new_solver);

        if (result_of_recur.status == RESULT_SUCCESS) {
            // TODO keep track of path.
            return result_of_recur;
        }
    }

    // this is not initalized. be careful.
    Recur_Result result;
    result.status = RESULT_FAIL;
    return result;
}



bool check_sudoku_is_invalid_by_no_way_to_put_digit_in_row_col_or_box(Sudoku_Solver_Struct *solver) {
    for (size_t row = 0; row < NUM_DIGITS; row++) {
        u16 seen_in_xxx = 0;
        FOREACH_CELL_IN_ROW(solver, row) {
            if (cell->placed_digit != 0) {
                seen_in_xxx |= DIGIT_BIT(cell->placed_digit);
            } else {
                seen_in_xxx |= cell->possible_digits;
            }
        }
        if (seen_in_xxx != ALL_DIGIT_BITS) goto exit_failure;
    }

    for (size_t col = 0; col < NUM_DIGITS; col++) {
        u16 seen_in_xxx = 0;
        FOREACH_CELL_IN_COL(solver, col) {
            if (cell->placed_digit != 0) {
                seen_in_xxx |= DIGIT_BIT(cell->placed_digit);
            } else {
                seen_in_xxx |= cell->possible_digits;
            }
        }
        if (seen_in_xxx != ALL_DIGIT_BITS) goto exit_failure;
    }

        for (size_t box = 0; box < NUM_DIGITS; box++) {
        u16 seen_in_xxx = 0;
        FOREACH_CELL_IN_BOX(solver, box) {
            if (cell->placed_digit != 0) {
                seen_in_xxx |= DIGIT_BIT(cell->placed_digit);
            } else {
                seen_in_xxx |= cell->possible_digits;
            }
        }
        if (seen_in_xxx != ALL_DIGIT_BITS) goto exit_failure;
    }
    return false;

exit_failure:
    solver->is_invalid = true;
    return true;
}


bool check_for_naked_singles(Sudoku_Solver_Struct *solver) {
    // naked single in possibilities.
    FOREACH_CELL(solver) {
        u32 count = pop_count(cell->possible_digits);
        // if count == 0, means digit is allready placed here.
        // we dont need to do anything to handle that.
        assert(Is_Between(count, 0, 9));

        if (count == 1) {
            u32 digit = count_trailing_zeros(cell->possible_digits);
            assert(Is_Between(digit, 1, 9));
            assert(cell->possible_digits & DIGIT_BIT(digit));

            Vec2i pos = cell_to_xy(solver, cell);
            mark_and_place_digit(solver, pos.x, pos.y, digit);

            return true;
        }
    }

    return false;
}

bool check_for_single_in_row_col_and_box(Sudoku_Solver_Struct *solver) {
    // Extreme levels of @Copypasta in this function.

    // rows
    for (size_t row = 0; row < NUM_DIGITS; row++) {
        u16 bit_field[NUM_DIGITS] = ZEROED;
        u16 is_placed = 0;

        // this is kinda dumb. wish i could get the it_index in FOREACH
        size_t index_in_xxx = 0;
        FOREACH_CELL_IN_ROW(solver, row) {
            if (cell->placed_digit != 0) {
                is_placed |= DIGIT_BIT(cell->placed_digit);
            } else {
                for (size_t digit = 1; digit <= 9; digit++) {
                    if (cell->possible_digits & DIGIT_BIT(digit)) {
                        bit_field[digit-1] |= (1 << index_in_xxx);
                    }
                }
            }

            index_in_xxx += 1;
        }

        for (size_t digit = 1; digit <= NUM_DIGITS; digit++) {
            // could use count_trailing_zeros(~is_placed);
            if (is_placed & DIGIT_BIT(digit)) continue;

            u32 digit_pop_count = pop_count(bit_field[digit-1]);
            assert(digit_pop_count > 0);
            if (digit_pop_count != 1) continue;

            u32 digit_loc = count_trailing_zeros(bit_field[digit-1]);
            assert(Is_Between(digit_loc, 0, NUM_DIGITS-1));
            Vec2i pos = { digit_loc, row };
            assert(solver->Cells[pos.y][pos.x].possible_digits & DIGIT_BIT(digit));

            //
            // TODO being able to say something like "dont check this row" would be super good.
            // but it would have to be a macro.
            //
            // cannot wait for jai to release.
            //
            // mark_and_place_digit(&solver, digit_loc, row, digit, .no_row_check = true);
            mark_and_place_digit(solver, pos.x, pos.y, digit);

            // i was going to try and continue this loop, to finish the row,
            // but too many complications arise when trying to do that.
            // we will visit this row again in due time.
            return true;
        }
    }

    // cols
    for (size_t col = 0; col < NUM_DIGITS; col++) {
        u16 bit_field[NUM_DIGITS] = ZEROED;
        u16 is_placed = 0;

        // this is kinda dumb. wish i could get the it_index in FOREACH
        size_t index_in_xxx = 0;
        FOREACH_CELL_IN_COL(solver, col) {
            if (cell->placed_digit != 0) {
                is_placed |= DIGIT_BIT(cell->placed_digit);
            } else {
                for (size_t digit = 1; digit <= 9; digit++) {
                    if (cell->possible_digits & DIGIT_BIT(digit)) {
                        bit_field[digit-1] |= (1 << index_in_xxx);
                    }
                }
            }

            index_in_xxx += 1;
        }

        for (size_t digit = 1; digit <= NUM_DIGITS; digit++) {
            // could use count_trailing_zeros(~is_placed);
            if (is_placed & DIGIT_BIT(digit)) continue;

            u32 digit_pop_count = pop_count(bit_field[digit-1]);
            assert(digit_pop_count > 0);
            if (digit_pop_count != 1) continue;

            if (pop_count(bit_field[digit-1]) != 1) continue;

            u32 digit_loc = count_trailing_zeros(bit_field[digit-1]);
            assert(Is_Between(digit_loc, 0, NUM_DIGITS-1));
            Vec2i pos = { col, digit_loc };
            assert(solver->Cells[pos.y][pos.x].possible_digits & DIGIT_BIT(digit));

            //
            // TODO being able to say something like "dont check this row" would be super good.
            // but it would have to be a macro.
            //
            // cannot wait for jai to release.
            //
            // mark_and_place_digit(&solver, digit_loc, row, digit, .no_row_check = true);
            mark_and_place_digit(solver, pos.x, pos.y, digit);

            // i was going to try and continue this loop, to finish the row,
            // but too many complications arise when trying to do that.
            // we will visit this row again in due time.
            return true;
        }
    }

    // boxs
    for (size_t box = 0; box < NUM_DIGITS; box++) {
        u16 bit_field[NUM_DIGITS] = ZEROED;
        u16 is_placed = 0;

        // this is kinda dumb. wish i could get the it_index in FOREACH
        size_t index_in_xxx = 0;
        FOREACH_CELL_IN_BOX(solver, box) {
            if (cell->placed_digit != 0) {
                is_placed |= DIGIT_BIT(cell->placed_digit);
            } else {
                for (size_t digit = 1; digit <= 9; digit++) {
                    if (cell->possible_digits & DIGIT_BIT(digit)) {
                        bit_field[digit-1] |= (1 << index_in_xxx);
                    }
                }
            }

            index_in_xxx += 1;
        }

        for (size_t digit = 1; digit <= NUM_DIGITS; digit++) {
            // could use count_trailing_zeros(~is_placed);
            if (is_placed & DIGIT_BIT(digit)) continue;

            u32 digit_pop_count = pop_count(bit_field[digit-1]);
            assert(digit_pop_count > 0);
            if (digit_pop_count != 1) continue;

            u32 digit_loc = count_trailing_zeros(bit_field[digit-1]);
            assert(Is_Between(digit_loc, 0, NUM_DIGITS-1));
            Vec2i pos = box_and_index_to_xy(box, digit_loc);
            assert(solver->Cells[pos.y][pos.x].possible_digits & DIGIT_BIT(digit));

            //
            // TODO being able to say something like "dont check this row" would be super good.
            // but it would have to be a macro.
            //
            // cannot wait for jai to release.
            //
            // mark_and_place_digit(&solver, digit_loc, row, digit, .no_row_check = true);
            mark_and_place_digit(solver, pos.x, pos.y, digit);

            // i was going to try and continue this loop, to finish the row,
            // but too many complications arise when trying to do that.
            // we will visit this row again in due time.
            return true;
        }
    }

    return false;
}

