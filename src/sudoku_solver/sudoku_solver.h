
#ifndef SUDOKU_SOLVER_H
#define SUDOKU_SOLVER_H


#include "Bested.h"


#define NUM_DIGITS 9
static_assert(NUM_DIGITS == 9, "a lot of this would have to change to allow for different board sizes");



typedef struct Sudoku_Digit_Grid Sudoku_Digit_Grid;
struct Sudoku_Digit_Grid {
    u8 digits[NUM_DIGITS][NUM_DIGITS];
};

// the API is that you fill out this struct, then pass it into the solver, and presto!
typedef struct Input_Sudoku_Puzzle Input_Sudoku_Puzzle;
struct Input_Sudoku_Puzzle {
    Sudoku_Digit_Grid grid;

    // TODO variant sudoku stuff.
};


// enum to roughly explain why a sudoku is impossible.
typedef enum {
    RFSI_NONE = 0, // not an error, just the zero value.

    RFSI_BAD_USER_INPUT,         // this means the user gave an obviously bad solution.
    RFSI_NO_POSSIBLE_SOLUTION,   // we checked, and there is not way to make a complete sudoku with this.
    RFSI_MULTIPLE_SOLUTIONS,     // there is more than one solution.
} Reason_For_Sudoku_Impossibility;


//
// the result from the Solve_Sudoku() function
//
typedef struct Sudoku_Solver_Result Sudoku_Solver_Result;
struct Sudoku_Solver_Result {
    bool sudoku_is_possible;

    // is filled out when the sudoku is possible, and contains the answer.
    Sudoku_Digit_Grid correct_grid;


    // only filled out when sudoku_is_possible == false
    Reason_For_Sudoku_Impossibility reason_not_possible;
    // these are set when your input has more than one valid solution.
    Sudoku_Digit_Grid two_different_solutions[2];


    // TODO maybe something about a path the solver took.
};

//
// simply construct an input sudoku, and after a small amount of time.
// (sudoku solvers are pretty fast), receive a result,
//
// if it was able to be completed, you are given the completed grid.
//
// this can also tell when a puzzle has more than one solution.
// this counts as a not possible sudoku. (and is one of the reasons for failure.)
//
Sudoku_Solver_Result Solve_Sudoku(Input_Sudoku_Puzzle input_sudoku);


//
// will generate a random sudoku grid with the num_digits_in_puzzle. (or more)
//
// sudoku is guaranteed to have a unique solution.
//
// 0 == empty cell.
//
// sudoku may not be able to generate sudoku's with few digits,
// but will try its best to get the number pretty low.
//
// fun fact: the minimum amount of digits in a standard
// sudoku is 17, any less will have multiple solutions.
//
// will     assert(num_digits_in_puzzle <= 9*9);
//
// this uses rand(). TODO make Bested.h random number generator.
//
Sudoku_Digit_Grid Generate_Random_Sudoku(u32 num_digits_in_puzzle);



#endif // SUDOKU_SOLVER_H








#ifdef SUDOKU_SOLVER_IMPLEMENTATION

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


internal Cell *next_in_box_by_pointer(Sudoku_Solver_Struct *solver, size_t box_number, Cell *current_cell) {
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

internal Sudoku_Solver_Struct init_solver(void) {
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
internal size_t cell_to_index(Sudoku_Solver_Struct *solver, Cell *cell) {
    size_t index = cell - &solver->Cells[0][0];
    return index;
}

typedef struct {
    s32 x, y;
} Vec2i;

internal Vec2i cell_to_xy(Sudoku_Solver_Struct *solver, Cell *cell) {
    size_t index = cell_to_index(solver, cell);
    Vec2i result = {
        .x = index % NUM_DIGITS,
        .y = index / NUM_DIGITS,
    };
    return result;
}

internal size_t xy_to_box_index(s32 x, s32 y) {
    return y/3*3 + x/3;
}
#define xy_to_box_index_v2(pos) xy_to_box_index((pos).x, (pos).y)

internal Vec2i box_and_index_to_xy(size_t box_index, size_t index_in_box) {
    Vec2i result = {
        .x = box_index%3*3 + index_in_box%3,
        .y = box_index/3*3 + index_in_box/3,
    };
    return result;
}



internal Sudoku_Digit_Grid solver_grid_to_digit_grid(Sudoku_Solver_Struct *solver) {
    Sudoku_Digit_Grid result = ZEROED;
    FOREACH_CELL(solver) {
        Vec2i pos = cell_to_xy(solver, cell);

        assert(Is_Between(cell->placed_digit, 1, 9));
        result.digits[pos.y][pos.x] = cell->placed_digit;
    }
    return result;
}



internal void mark_and_place_digit(Sudoku_Solver_Struct *solver, size_t x, size_t y, u8 digit) {
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
internal u32 pop_count(u32 x) {
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

internal u32 count_trailing_zeros(u32 x) {
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


// inclusive.
internal size_t random_number_in_range(size_t low, size_t high) {
    // Source - https://stackoverflow.com/a/18386648
    // Posted by Victor, modified by community. See post 'Timeline' for change history
    // Retrieved 2026-03-30, License - CC BY-SA 4.0
    return low + rand() / (RAND_MAX / (high - low + 1) + 1);
}

typedef struct {
    // contains the digits 1 -> 9 in a random order
    u8 permutation[NUM_DIGITS];
} Digit_Permutation;

internal Digit_Permutation random_digit_permutation(void) {
    Digit_Permutation result = ZEROED;
    static_assert(NUM_DIGITS == 9, "this would do bad things to memory");
    for (size_t i = 0; i < Array_Len(result.permutation); i++) {
        size_t digit = i+1;
        assert(Is_Between(digit, 0, 9));
        result.permutation[i] = digit;
    }

    // Fisher-Yates shuffle
    for (size_t i = Array_Len(result.permutation)-1; i > 0; i--) {
        size_t other_index = random_number_in_range(0, i);

        u8 tmp                          = result.permutation[i];
        result.permutation[i]           = result.permutation[other_index];
        result.permutation[other_index] = tmp;
    }

    return result;
}




///////////////////////////////////////////////////////////////////////////////////////
//                                Solver Functions
///////////////////////////////////////////////////////////////////////////////////////

//
// when iterating in recur_and_solve_sudoku, the order you iterate may decide
// the eventual outcome of the sudoku (if there are more than one solution.)
// random is used for Generate_Random_Sudoku().
//
typedef enum {
    ITER_FORWARDS,
    ITER_BACKWARDS,
    ITER_RANDOM,
} Iteration_Direction;

internal Recur_Result recur_and_solve_sudoku(Sudoku_Solver_Struct solver, Iteration_Direction iteration_direction);

internal bool check_sudoku_is_invalid_by_no_way_to_put_digit_in_row_col_or_box(Sudoku_Solver_Struct *solver);

internal bool check_for_naked_singles(Sudoku_Solver_Struct *solver);
internal bool check_for_single_in_row_col_and_box(Sudoku_Solver_Struct *solver);



Sudoku_Solver_Result Solve_Sudoku(Input_Sudoku_Puzzle input_sudoku) {
    Sudoku_Solver_Result sudoku_solver_result = ZEROED;

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
            if ((cell->possible_digits & DIGIT_BIT(digit)) == 0) {
                sudoku_solver_result.sudoku_is_possible  = false;
                sudoku_solver_result.reason_not_possible = RFSI_BAD_USER_INPUT;
                return sudoku_solver_result;
            }

            // the above check prevents this from panic'ing.
            mark_and_place_digit(&solver, i, j, digit);
            if (solver.is_invalid) {
                sudoku_solver_result.sudoku_is_possible  = false;
                sudoku_solver_result.reason_not_possible = RFSI_BAD_USER_INPUT;
                return sudoku_solver_result;
            }
        }
    }


    // iterating forwards here should be the best option
    Recur_Result result = recur_and_solve_sudoku(solver, ITER_FORWARDS);
    if (result.status == RESULT_FAIL) {
        sudoku_solver_result.sudoku_is_possible  = false;
        sudoku_solver_result.reason_not_possible = RFSI_NO_POSSIBLE_SOLUTION;
        return sudoku_solver_result;
    }

    // i dont make this decision lightly.
    //
    // this will literally double the time it takes to solve any sudoku.
    // but as long as the time it takes to solve a single sudoku is low, this wont matter.
    // why would you even need to solve 1000 sudoku's per second anyway?
    //
    // "oh no! i can only solve 500 sudoku's per second, whatever shall I do???"
    //
    // just run multiple solvers at once at that point
    #define CHECK_FOR_MULTIPLE_SOLUTIONS (true)
    if (CHECK_FOR_MULTIPLE_SOLUTIONS) {
        // we do the entire solve again, but this time, when we start to
        // recursively check options, we iterate in reverse. this guarantees
        // that if there is more than 1 solution, the solution generated
        // here will be different from the first one.
        Recur_Result second_result = recur_and_solve_sudoku(solver, ITER_BACKWARDS);
        assert(second_result.status == RESULT_SUCCESS);

        bool result_is_the_same = Mem_Eq(&result.correct_result, &second_result.correct_result, sizeof(result.correct_result));
        if (!result_is_the_same) {
            // there were multiple different solutions
            sudoku_solver_result.sudoku_is_possible  = false;
            sudoku_solver_result.reason_not_possible = RFSI_MULTIPLE_SOLUTIONS;
            sudoku_solver_result.two_different_solutions[0] = solver_grid_to_digit_grid(&result.correct_result);
            sudoku_solver_result.two_different_solutions[1] = solver_grid_to_digit_grid(&second_result.correct_result);
            return sudoku_solver_result;
        }
    }

    sudoku_solver_result.sudoku_is_possible = true;
    sudoku_solver_result.correct_grid = solver_grid_to_digit_grid(&result.correct_result);
    return sudoku_solver_result;
}



internal void do_all_deductions(Sudoku_Solver_Struct *solver) {
    // check for deductions we can make.
    //
    // after a deduction we continue the while loop, hoping to make more.
    // while also checking to see if the sudoku suddenly became unsolvable.
    //
    // performance wise its a little weird to start
    // the loop from the start every time, but i think
    // its a good way to structure this.
    while (true) {
        if (solver->is_invalid)                             break;
        if (solver->number_of_digits_left_to_place == 0)    break;

        if (check_sudoku_is_invalid_by_no_way_to_put_digit_in_row_col_or_box(solver)) continue;

        if (check_for_naked_singles(solver))                continue;
        if (check_for_single_in_row_col_and_box(solver))    continue;

        // no deductions were made, nothing left to do here.
        break;
    }
}


internal Recur_Result recur_and_solve_sudoku(Sudoku_Solver_Struct solver, Iteration_Direction iteration_direction) {
    do_all_deductions(&solver);

    // recursion base cases
    {
        if (solver.is_invalid) {
            Recur_Result result; // dont bother zero initializing this.
            result.status = RESULT_FAIL;
            return result;
        }

        if (solver.number_of_digits_left_to_place == 0) {
            Recur_Result result = ZEROED;
            result.status = RESULT_SUCCESS;
            result.correct_result = solver;
            return result;
        }
    }

    Vec2i pos = {-1, -1};
    {
        u32 min_possible_results = NUM_DIGITS + 1;
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
        assert(pos.x != -1 && pos.y != -1);

        assert(min_possible_results != 1); // this should have allready been done in the deductions.
        assert(min_possible_results != 0); // this should have set the sudoku to invalid, unreachable()
    }

    Cell *cell = &solver.Cells[pos.y][pos.x];

    switch (iteration_direction) {
    case ITER_FORWARDS:
    case ITER_BACKWARDS: {
        for (size_t digit = 1; digit <= 9; digit++) {
            // using this bool, we can decide witch way we iterate over the options,
            // if there are multiple solutions, this will produce 2 different solutions
            size_t real_digit = (iteration_direction == ITER_FORWARDS) ? digit : 9 - digit + 1;

            // make sure this digit is possible.
            if (!(cell->possible_digits & DIGIT_BIT(real_digit))) continue;

            Sudoku_Solver_Struct new_solver = solver;
            mark_and_place_digit(&new_solver, pos.x, pos.y, real_digit);

            Recur_Result result_of_recur = recur_and_solve_sudoku(new_solver, iteration_direction);

            if (result_of_recur.status == RESULT_SUCCESS) {
                // TODO keep track of path.
                return result_of_recur;
            }
        }
    } break;

    case ITER_RANDOM: {
        Digit_Permutation digit_permutation = random_digit_permutation();
        for (size_t i = 0; i < Array_Len(digit_permutation.permutation); i++) {
            int digit = digit_permutation.permutation[i];
            if (!(cell->possible_digits & DIGIT_BIT(digit))) continue;

            Sudoku_Solver_Struct new_solver = solver;
            mark_and_place_digit(&new_solver, pos.x, pos.y, digit);

            Recur_Result result_of_recur = recur_and_solve_sudoku(new_solver, iteration_direction);
            if (result_of_recur.status == RESULT_SUCCESS) {
                // TODO? keep track of path.
                return result_of_recur;
            }
        }
    } break;

    default: UNREACHABLE();
    }


    // this is not initalized.
    Recur_Result result;
    result.status = RESULT_FAIL;
    return result;
}



internal bool check_sudoku_is_invalid_by_no_way_to_put_digit_in_row_col_or_box(Sudoku_Solver_Struct *solver) {
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


internal bool check_for_naked_singles(Sudoku_Solver_Struct *solver) {
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

internal bool check_for_single_in_row_col_and_box(Sudoku_Solver_Struct *solver) {
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





Sudoku_Digit_Grid Generate_Random_Sudoku(u32 num_digits_in_puzzle) {
    assert(num_digits_in_puzzle <= 9*9);

    // by iterating randomly on an empty grid,
    // the resulting sudoku should be random.
    Recur_Result recur_result = recur_and_solve_sudoku(init_solver(), ITER_RANDOM);
    // there is no possible way this could fail.
    assert(recur_result.status == RESULT_SUCCESS);

    Sudoku_Digit_Grid resulting_grid = solver_grid_to_digit_grid(&recur_result.correct_result);

    // now we remove digits randomly
    u32 digits_currently_in_puzzle = NUM_DIGITS*NUM_DIGITS;
    while (digits_currently_in_puzzle > num_digits_in_puzzle) {
        bool removed_a_digit = false;

        // generate 2 permutations, should be fine to only
        // generate 1 x positions, should be random enough.
        Digit_Permutation pos_x = random_digit_permutation();
        Digit_Permutation pos_y = random_digit_permutation();

        for (size_t j = 0; j < Array_Len(pos_y.permutation); j++) {
            s32 y = pos_y.permutation[j] - 1;
            // assert(y != 0);
            assert(Is_Between(y, 0, NUM_DIGITS-1));
            for (size_t i = 0; i < Array_Len(pos_x.permutation); i++) {
                s32 x = pos_x.permutation[i] - 1;

                if (resulting_grid.digits[y][x] == 0) continue;

                u8 digit = resulting_grid.digits[y][x];
                // remove digit
                resulting_grid.digits[y][x] = 0;
                Input_Sudoku_Puzzle input = { .grid = resulting_grid };

                Sudoku_Solver_Result solver_result = Solve_Sudoku(input);

                // we removed another digit!
                if (solver_result.sudoku_is_possible) {
                    removed_a_digit = true;
                    break;
                }

                // retry
                resulting_grid.digits[y][x] = digit;
            }
            if (removed_a_digit) break;
        }

        // we couldn't find a digit to remove.
        if (!removed_a_digit) break;

        digits_currently_in_puzzle -= 1;
    }

    return resulting_grid;
}



#endif // SUDOKU_SOLVER_IMPLEMENTATION

