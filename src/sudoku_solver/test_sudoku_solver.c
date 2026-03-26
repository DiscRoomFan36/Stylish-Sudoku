
#include "./sudoku_solver.c"

#include "TEST_MA.h"


// helper to construct sudoku grids.
Input_Sudoku_Puzzle sudoku_string_to_input_puzzle(const char *sudoku_string);



void test_foreach_cell(void) {
    Sudoku_Solver_Struct solver = init_solver();

    int cells_checked = 0;
    FOREACH_CELL(&solver) {
        int index_in_solver_array = cell - &solver.Cells[0][0];
        // reduce TEST_MA.h pollution
        if (index_in_solver_array != cells_checked) {
            TEST_EXPECT_EQ(index_in_solver_array, cells_checked);
        }
        cells_checked += 1;
    }

    TEST_EXPECT_EQ(cells_checked, 9*9);
}

void test_foreach_cell_in_row_col_and_box(void) {
    Sudoku_Solver_Struct solver = init_solver();

    for (int i = 0; i < 9; i++) {
        {
            int cells_checked = 0;
            FOREACH_CELL_IN_ROW(&solver, i) {
                Cell *expected_cell = &solver.Cells[i][cells_checked];
                // reducing TEST_MA.h pollution.
                if (expected_cell != cell) { TEST_EXPECT_EQ(expected_cell, cell); }

                cells_checked += 1;
            }
            TEST_EXPECT_EQ(cells_checked, 9);
        }

        {
            int cells_checked = 0;
            FOREACH_CELL_IN_COL(&solver, i) {
                Cell *expected_cell = &solver.Cells[cells_checked][i];
                // reducing TEST_MA.h pollution.
                if (expected_cell != cell) { TEST_EXPECT_EQ(expected_cell, cell); }

                cells_checked += 1;
            }
            TEST_EXPECT_EQ(cells_checked, 9);
        }

        {
            int base_row = (i/3) * 3, base_col = (i%3) * 3;
            int cells_checked = 0;
            FOREACH_CELL_IN_BOX(&solver, i) {
                Cell *expected_cell = &solver.Cells[base_row + cells_checked/3][base_col + cells_checked%3];
                // reducing TEST_MA.h pollution.
                if (expected_cell != cell) { TEST_EXPECT_EQ(expected_cell, cell); }

                cells_checked += 1;
            }
            TEST_EXPECT_EQ(cells_checked, 9);
        }
    }
}


void test_Mark_and_Place_Digit(void) {
    Sudoku_Solver_Struct solver = init_solver();

    {

        int row   = 1;
        int col   = 2;
        int digit = 6;

        TEST_EXPECT_EQ(solver.Cells[row][col].placed_digit, 0);
        TEST_EXPECT(solver.is_invalid == false);

        mark_and_place_digit(&solver, col, row, digit);
        TEST_EXPECT_EQ(solver.Cells[row][col].placed_digit, digit);
        TEST_EXPECT_EQ(solver.Cells[row][col].possible_digits, 0);
        TEST_EXPECT(solver.is_invalid == false);

        solver.number_of_digits_left_to_place = NUM_DIGITS*NUM_DIGITS-1;

        FOREACH_CELL_IN_BOX(&solver, row/3*3 + col/3) {
            TEST_EXPECT((cell->possible_digits & DIGIT_BIT(digit)) == 0);
        }
        FOREACH_CELL_IN_ROW(&solver, row) {
            TEST_EXPECT((cell->possible_digits & DIGIT_BIT(digit)) == 0);
        }
        FOREACH_CELL_IN_COL(&solver, col) {
            TEST_EXPECT((cell->possible_digits & DIGIT_BIT(digit)) == 0);
        }
    }

    { // see if it properly gets set invalid.
        Sudoku_Solver_Struct solver = init_solver();

        // this cell can only be 5
        solver.Cells[0][0].possible_digits = DIGIT_BIT(5);

        // other cell was marked 5
        mark_and_place_digit(&solver, 1, 0, 5);

        // this should be invalid
        TEST_EXPECT(solver.is_invalid);
    }

    // dont know why im doing this, but its fun.
    for (int i = 0; i < 15; i++) {
        int row   = rand() % 9;
        int col   = rand() % 9;
        int digit = rand() % 9;

        Cell *cell = &solver.Cells[row][col];

        if ((cell->possible_digits & DIGIT_BIT(digit)) == 0) continue;

        mark_and_place_digit(&solver, col, row, digit);
    }
}

void test_check_sudoku_is_invalid_by_no_way_to_put_digit_in_row(void) {
    struct Test {
        const char *test_name;
        const char *sudoku_string;
        bool        is_possible;
    } tests[] = {
        {
            .test_name = "handcrafted 1",
            .sudoku_string =    ".....1..."
                                "........."
                                "........."
                                ".......1."
                                ".1......."
                                "...2....."
                                "1........"
                                "........."
                                "....1....",
            .is_possible = false,
        },
        {
            .test_name = "puzzle that has this behaviour 1",
            .sudoku_string = "302000000000067000600004100563000480004080030900000000001006070050800040837002009",
            .is_possible = true,
        },
        {
            .test_name = "puzzle that has this behaviour 2",
            .sudoku_string = "050604000070010630010000004700000003091308000000040970000900460960007000400005020",
            .is_possible = true,
        },
    };

    for (size_t i = 0; i < Array_Len(tests); i++) {
        struct Test test = tests[i];

        Input_Sudoku_Puzzle input = sudoku_string_to_input_puzzle(test.sudoku_string);

        Sudoku_Solver_Result result = Solve_Sudoku(input);
        TEST_EXPECT_WITH_REASON(result.sudoku_is_possible == test.is_possible, "%s", test.test_name);

        // TODO it would be good if i had a way to detect the reason it was invalid.
        // probably want the path of the solve soon.
    }
}

void test_check_for_naked_singles(void) {
    Sudoku_Solver_Struct solver = init_solver();

    Sudoku_Solver_Struct before = solver;
    bool should_be_false = check_for_naked_singles(&solver);
    TEST_EXPECT(should_be_false == false);
    TEST_EXPECT(Mem_Eq(&solver, &before, sizeof(solver)));


    int row   = rand() % 9;
    int col   = rand() % 9;
    int digit = 4;

    Cell *cell = &solver.Cells[row][col];

    // give this cell a naked single.
    cell->possible_digits = DIGIT_BIT(digit);
    bool should_be_true = check_for_naked_singles(&solver);
    TEST_EXPECT(should_be_true == true);
    TEST_EXPECT(!Mem_Eq(&solver, &before, sizeof(solver)));
    TEST_EXPECT_EQ(cell->placed_digit, digit);
    TEST_EXPECT_EQ(cell->possible_digits, 0);

}

void test_check_for_single_in_row_and_columns(void) {
    Sudoku_Solver_Struct solver = init_solver();

    bool check_for_no_singles_at_start = check_for_single_in_row_col_and_box(&solver);
    TEST_EXPECT(check_for_no_singles_at_start == false);

    { // test rows
        int digit = 1;
        int row, col;
        Cell *naked_cell;

        do {
            row   = rand() % 9;
            col   = rand() % 9;
            naked_cell = &solver.Cells[row][col];
        } while (naked_cell->placed_digit != 0);

        FOREACH_CELL_IN_ROW(&solver, row) {
            if (cell == naked_cell) continue;
            cell->possible_digits &= ~DIGIT_BIT(digit);
        }

        bool should_be_naked_in_row = check_for_single_in_row_col_and_box(&solver);
        TEST_EXPECT(should_be_naked_in_row);
        TEST_EXPECT_EQ(naked_cell->placed_digit, digit);
    }

    { // test cols
        int digit = 2;
        int row, col;
        Cell *naked_cell;

        do {
            row   = rand() % 9;
            col   = rand() % 9;
            naked_cell = &solver.Cells[row][col];
        } while (naked_cell->placed_digit != 0);

        FOREACH_CELL_IN_COL(&solver, col) {
            if (cell == naked_cell) continue;
            cell->possible_digits &= ~DIGIT_BIT(digit);
        }

        bool should_be_naked_in_col = check_for_single_in_row_col_and_box(&solver);
        TEST_EXPECT(should_be_naked_in_col);
        TEST_EXPECT_EQ(naked_cell->placed_digit, digit);
    }

    { // test boxs
        int digit = 3;
        int row, col;
        Cell *naked_cell;

        do {
            row   = rand() % 9;
            col   = rand() % 9;
            naked_cell = &solver.Cells[row][col];
        } while (naked_cell->placed_digit != 0);

        bool seen_naked = false;

        size_t box_index = xy_to_box_index(col, row);

        FOREACH_CELL_IN_BOX(&solver, box_index) {
            if (cell == naked_cell) { seen_naked = true; continue; }
            cell->possible_digits &= ~DIGIT_BIT(digit);
        }

        TEST_EXPECT(seen_naked);

        bool should_be_naked_in_box = check_for_single_in_row_col_and_box(&solver);
        TEST_EXPECT(should_be_naked_in_box);
        TEST_EXPECT_EQ(naked_cell->placed_digit, digit);
    }
}

void test_check_for_puzzles_with_multiple_solutions(void) {
    struct Test {
        const char *test_name;
        const char *sudoku_string;
    } tests[] = {
        {
            .test_name = "puzzle is impossible, there is no way to disambiguate 8 and 9",
            .sudoku_string =    ".1.743562"
                                "426895317"
                                "375612894"
                                "794538621"
                                "561274938"
                                "283169475"
                                "137486259"
                                ".5.321746"
                                "642957183",
        },
        {
            .test_name = "empty grid, basically inf solutions",
            .sudoku_string =    "........."
                                "........."
                                "........."
                                "........."
                                "........."
                                "........."
                                "........."
                                "........."
                                ".........",
        },
    };

    for (size_t i = 0; i < Array_Len(tests); i++) {
        struct Test test = tests[i];

        Input_Sudoku_Puzzle input = sudoku_string_to_input_puzzle(test.sudoku_string);

        Sudoku_Solver_Result result = Solve_Sudoku(input);

        TEST_EXPECT_WITH_REASON(result.sudoku_is_possible == false, "%s", test.test_name);
        TEST_EXPECT_EQ(result.reason_not_possible, RFSI_MULTIPLE_SOLUTIONS);
    }
}


void test_solver_sudoku_bad_input(void) {
    struct Test {
        const char *test_name;
        const char *sudoku_string;
    } tests[] = {
        {
            .test_name = "row check",
            .sudoku_string =    "........."
                                "........."
                                "........."
                                "........."
                                "........."
                                "........."
                                "........."
                                "3......3."
                                ".........",
        },
        {
            .test_name = "col check",
            .sudoku_string =    "1........"
                                "........."
                                "........."
                                "........."
                                "........."
                                "........."
                                "........."
                                "1........"
                                ".........",
        },
        {
            .test_name = "box check",
            .sudoku_string =    "........."
                                "......9.."
                                "........9"
                                "........."
                                "........."
                                "........."
                                "........."
                                "........."
                                ".........",
        },
        {
            .test_name = "no possible answer in box",
            .sudoku_string =    "123......"
                                "456......"
                                "78......9"
                                "........."
                                "........."
                                "........."
                                "........."
                                "........."
                                ".........",
        },
    };

    for (size_t i = 0; i < Array_Len(tests); i++) {
        struct Test test = tests[i];

        Input_Sudoku_Puzzle input = sudoku_string_to_input_puzzle(test.sudoku_string);

        Sudoku_Solver_Result result = Solve_Sudoku(input);

        TEST_EXPECT_WITH_REASON(result.sudoku_is_possible == false, "%s", test.test_name);
        TEST_EXPECT_EQ(result.reason_not_possible, RFSI_BAD_USER_INPUT);
    }
}

void test_solve_sudoku_easy(void) {
    struct Test {
        const char *test_name;
        const char *sudoku_string;
        const char *correct_string;
    } tests[] = {
        {
            .test_name = "already complete sudoku",
            .sudoku_string =    "913568427"
                                "687342915"
                                "254197683"
                                "479685132"
                                "162734598"
                                "538219764"
                                "345926871"
                                "726851349"
                                "891473256",

            .correct_string =   "913568427"
                                "687342915"
                                "254197683"
                                "479685132"
                                "162734598"
                                "538219764"
                                "345926871"
                                "726851349"
                                "891473256",
        },
        {
            .test_name = "1 digit away",
            .sudoku_string =    "913568427"
                                "687342915"
                                "254197683"
                                "4796.5132"
                                "162734598"
                                "538219764"
                                "345926871"
                                "726851349"
                                "891473256",

            .correct_string =   "913568427"
                                "687342915"
                                "254197683"
                                "479685132"
                                "162734598"
                                "538219764"
                                "345926871"
                                "726851349"
                                "891473256",
        },
        {
            .test_name = "an easy (but non trivial) sudoku",
            .sudoku_string =    "9..5.8..7"
                                ".8.3.29.5"
                                ".54....8."
                                "...68..32"
                                "1....4..8"
                                "5..219.6."
                                "...9.6..1"
                                "726..1.4."
                                "..147..56",

            .correct_string =   "913568427"
                                "687342915"
                                "254197683"
                                "479685132"
                                "162734598"
                                "538219764"
                                "345926871"
                                "726851349"
                                "891473256",
        },
    };


    for (size_t i = 0; i < Array_Len(tests); i++) {
        struct Test test = tests[i];

        Input_Sudoku_Puzzle input   = sudoku_string_to_input_puzzle(test.sudoku_string);
        Input_Sudoku_Puzzle correct = sudoku_string_to_input_puzzle(test.correct_string);

        Sudoku_Solver_Result result = Solve_Sudoku(input);

        TEST_EXPECT_WITH_REASON(result.sudoku_is_possible, "%s", test.test_name);
        TEST_EXPECT_WITH_REASON(memcmp(&correct.grid, &result.correct_grid, sizeof(correct.grid)) == 0, "%s", test.test_name);
    }
}

void test_solve_sudoku_medium(void) {
    struct Test {
        const char *test_name;
        const char *sudoku_string;
        const char *correct_string;
    } tests[] = {
        {
            .test_name = "medium difficulty sudoku",

            .sudoku_string =    "2.34....5"
                                "8.916.7.4"
                                "..6.3..19"
                                "7.2..3.6."
                                "..825...."
                                "..16.7..2"
                                "..7..5926"
                                "93.72...."
                                "6...9.47.",

            .correct_string =   "213479685"
                                "859162734"
                                "476538219"
                                "742913568"
                                "368254197"
                                "591687342"
                                "187345926"
                                "934726851"
                                "625891473",
        },
    };

    for (size_t i = 0; i < Array_Len(tests); i++) {
        struct Test test = tests[i];

        Input_Sudoku_Puzzle input   = sudoku_string_to_input_puzzle(test.sudoku_string);
        Input_Sudoku_Puzzle correct = sudoku_string_to_input_puzzle(test.correct_string);
        TEST_EXPECT(Solve_Sudoku(correct).sudoku_is_possible);

        Sudoku_Solver_Result result = Solve_Sudoku(input);

        TEST_EXPECT_WITH_REASON(result.sudoku_is_possible, "%s", test.test_name);
        TEST_EXPECT_WITH_REASON(memcmp(&correct.grid, &result.correct_grid, sizeof(correct.grid)) == 0, "%s", test.test_name);
    }
}

void bench_test_many_sudoku(void) {
    #define SUDOKU_FILE_PATH "./src/sudoku_solver/printable_sudoku_puzzle_4.txt"
    // #define SUDOKU_FILE_PATH "./src/sudoku_solver/printable_sudoku_puzzle_5.txt"

    // dont really need any memory management for this.
    // but Read_Entire_File() expects a valid Arena.
    Arena a = ZEROED;
    String many_sudoku_strings = Read_Entire_File(&a, S(SUDOKU_FILE_PATH));
    TEST_EXPECT_WITH_REASON(many_sudoku_strings.length != 0, "if this file dose not exist, this means i dont know what to do with the copyright for now.");

    u64 i = 0;
    while (true) {
        String line = String_Get_Next_Line(&many_sudoku_strings, &i, SGNL_Trim);
        if (line.length == 0) break;

        Input_Sudoku_Puzzle input = sudoku_string_to_input_puzzle(temp_String_To_C_Str(line));
        if (line.length != 9*9) TEST_FAIL("line length was not 9*9 was %ld", line.length);

        Sudoku_Solver_Result result = Solve_Sudoku(input);
        if (!result.sudoku_is_possible)   TEST_FAIL("Sudoku %zu was not possible", i);
    }
    TEST_EXPECT_WITH_REASON(true, "finished %zu sudoku's", i);
}



#define DISABLE_SANDBOX     (false)

int main(void) {
    srand(time(NULL));

    ADD_TEST(test_foreach_cell);
    ADD_TEST(test_foreach_cell_in_row_col_and_box);

    ADD_TEST(test_Mark_and_Place_Digit);
    ADD_TEST(test_check_sudoku_is_invalid_by_no_way_to_put_digit_in_row);
    ADD_TEST(test_check_for_naked_singles);
    ADD_TEST(test_check_for_single_in_row_and_columns);
    ADD_TEST(test_check_for_puzzles_with_multiple_solutions);

    ADD_TEST(test_solver_sudoku_bad_input);
    ADD_TEST(test_solve_sudoku_easy);
    ADD_TEST(test_solve_sudoku_medium);

    ADD_TEST(bench_test_many_sudoku, .timeout_time = 5);

    int number_of_tests_failed = RUN_TESTS(.disable_sandboxing_for_all_tests = DISABLE_SANDBOX);
    printf("number of tests failed: %d\n\n", number_of_tests_failed);
    return (number_of_tests_failed == 0) ? 0 : 1;
}



Input_Sudoku_Puzzle sudoku_string_to_input_puzzle(const char *sudoku_string) {
    if (strlen(sudoku_string) != 9*9) PANIC("sudoku_string is not 9*9");

    char empty_chars[] = {'.', '0'};

    Input_Sudoku_Puzzle input = ZEROED;
    for (size_t j = 0; j < 9; j++) {
        for (size_t i = 0; i < 9; i++) {
            u8 digit = 0;

            char c = sudoku_string[xy_to_index(i, j)];
            bool is_empty_char = false;
            for (size_t k = 0; k < Array_Len(empty_chars); k++) {
                if (c == empty_chars[k]) {
                    is_empty_char = true;
                    break;
                }
            }

            if (!is_empty_char) { digit = c - '0'; }

            assert(Is_Between(digit, 0, 9));
            input.grid.digits[j][i] = digit; 
        }
    }

    return input;
}

#define BESTED_IMPLEMENTATION
#include "Bested.h"
