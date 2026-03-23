
#include "./sudoku_solver.c"

#include "TEST_MA.h"



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

void test_check_for_naked_singles(void) {
    Sudoku_Solver_Struct solver = init_solver();

    Sudoku_Solver_Struct before = solver;
    bool should_be_false = check_for_naked_singles(&solver);
    TEST_EXPECT(should_be_false == false);
    TEST_EXPECT(Mem_Eq(&solver, &before, sizeof(solver)));


    int row   = 6;
    int col   = 3;
    int digit = 4;

    Cell *cell = &solver.Cells[row][col];

    // give this cell a naked single.
    cell->possible_digits = DIGIT_BIT(digit);
    bool should_be_true = check_for_naked_singles(&solver);
    TEST_EXPECT(should_be_true == true);
    TEST_EXPECT(!Mem_Eq(&solver, &before, sizeof(solver)));
    TEST_EXPECT_EQ(cell->placed_digit, digit);
    TEST_EXPECT_EQ(cell->possible_digits, 0);

    // TODO maybe check that naked singles gets only one digit at a time.
}

void test_check_for_single_in_row_and_columns(void) {
    Sudoku_Solver_Struct solver = init_solver();

    bool should_be_false = check_for_single_in_row_col_and_box(&solver);
    TEST_EXPECT(should_be_false == false);

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

        bool should_be_true = check_for_single_in_row_col_and_box(&solver);
        TEST_EXPECT(should_be_true);
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

        bool should_be_true = check_for_single_in_row_col_and_box(&solver);
        TEST_EXPECT(should_be_true);
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

        FOREACH_CELL_IN_BOX(&solver, row/3*3 + col/3) {
            if (cell == naked_cell) continue;
            cell->possible_digits &= ~DIGIT_BIT(digit);
        }

        bool should_be_true = check_for_single_in_row_col_and_box(&solver);
        TEST_EXPECT(should_be_true);
        TEST_EXPECT_EQ(naked_cell->placed_digit, digit);
    }
}


int main(void) {
    ADD_TEST(test_foreach_cell);
    ADD_TEST(test_foreach_cell_in_row_col_and_box);

    ADD_TEST(test_Mark_and_Place_Digit);
    ADD_TEST(test_check_for_naked_singles);
    ADD_TEST(test_check_for_single_in_row_and_columns);

    int number_of_tests_failed = RUN_TESTS();
    printf("number of tests failed: %d\n\n", number_of_tests_failed);

    return (number_of_tests_failed == 0) ? 0 : 1;
}


#define BESTED_IMPLEMENTATION
#include "Bested.h"
