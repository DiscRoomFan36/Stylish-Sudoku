
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define SUDOKU_SOLVER_IMPLEMENTATION
#include "./sudoku_solver.h"

// helper to construct sudoku grids.
Input_Sudoku_Puzzle sudoku_string_to_input_puzzle(const char *sudoku_string);


typedef struct {
    _Array_Header_;
    Input_Sudoku_Puzzle *items;
} Input_Sudoku_Puzzle_Array;



// TODO
// TODO
// TODO move into bested.h
// TODO
// TODO
internal void print_duration(u64 time_start, u64 time_end) {
    u64 duration = time_end - time_start;

    u64 time_in_ns = (duration                             ) % 1000;
    u64 time_in_us = (duration / (                  1000UL)) % 1000;
    u64 time_in_ms = (duration / (         1000UL * 1000UL)) % 1000;
    u64 time_in_s  = (duration / (1000UL * 1000UL * 1000UL)) % 1000;

    // printf("time took: ");
    printf("%4lds, %4ldms, %4ldus, %4ldns", time_in_s, time_in_ms, time_in_us, time_in_ns);
    // printf("\n");
}


#define SUDOKU_DIRECTORY "./build/randomly_generated_sudoku/"


void bench_test_many_sudoku(void) {
    // dont really need any memory management for this.
    // but Read_Entire_File() expects a valid Arena.
    Arena a = ZEROED;


    u64 total_time_start = nanoseconds_since_unspecified_epoch();
    u64 total_sudoku_solved = 0;

    printf("\n");
    printf("Bench testing Sudoku_Solver() function. will try to open the directory %s and will try to solve ALL sudoku within.\n", SUDOKU_DIRECTORY);
    printf("\n");


    DIR *dir = opendir(SUDOKU_DIRECTORY);

    if (!dir) {
        perror("Could not open directory " SUDOKU_DIRECTORY " with error");
        fprintf(stderr, "Have you tried generating some random sudoku's?\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "   $ ./nob generate_lots_of_random_sudoku && ./build/generate_lots_of_random_sudoku\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "this command will probably take some time. (it takes a lot more work to generate a sudoku then to solve it.)\n");
        exit(1);
    }

    while (true) {
        errno = 0;
        struct dirent *entry = readdir(dir);
        if (!entry) break;

        String filepath = temp_String_sprintf("%s%s", SUDOKU_DIRECTORY, entry->d_name);
        // TODO read entry.

        // check if the end is what we expect, should we warn the user about bad strings?
        if (!String_Ends_With(filepath, S(".rgstf"))) continue;


        Arena_Clear(&a);

        String entire_file = Read_Entire_File(&a, filepath);
        assert(entire_file.length >= 9*9); // expect at least some data

        Input_Sudoku_Puzzle_Array input_array = ZEROED;
        input_array.allocator = &a;

        while (true) {
            String line = String_Get_Next_Line(&entire_file, NULL, SGNL_Trim);
            if (line.length == 0) break;

            assert(line.length == 9*9);

            Input_Sudoku_Puzzle input = sudoku_string_to_input_puzzle(temp_String_To_C_Str(line));

            Array_Append(&input_array, input);

        }

        printf("    Solving  %zu sudoku puzzles in file "S_Fmt"\n", input_array.count, S_Arg(filepath));

        u64 time_start = nanoseconds_since_unspecified_epoch();

        for (size_t i = 0; i < input_array.count; i++) {
            // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
            // print_progress(i, input_array.count);

            Input_Sudoku_Puzzle input = input_array.items[i];

            Sudoku_Solver_Result result = Solve_Sudoku(input);
            assert(result.sudoku_is_possible);
        }

        u64 time_end = nanoseconds_since_unspecified_epoch();
        printf("    Finished %zu sudoku's in: ", input_array.count); print_duration(time_start, time_end); printf("\n");
        printf("\n");

        total_sudoku_solved += input_array.count;
    }

    if (errno) {
        perror("got error when trying to read directory entries");
    }

    closedir(dir); // no error check here, who cares.

    u64 total_time_end = nanoseconds_since_unspecified_epoch();
    printf("solved %zu total sudoku in: ", total_sudoku_solved); print_duration(total_time_start, total_time_end); printf(". (includes parsing time)\n");
}



int main(void) {
    // dont think this actually dose anything...
    srand(time(NULL));

    bench_test_many_sudoku();

    return 0;
}



Input_Sudoku_Puzzle sudoku_string_to_input_puzzle(const char *sudoku_string) {
    if (strlen(sudoku_string) != 9*9) PANIC("sudoku_string is not 9*9");

    char empty_chars[] = {'.', '0'};

    Input_Sudoku_Puzzle input = ZEROED;
    for (size_t j = 0; j < 9; j++) {
        for (size_t i = 0; i < 9; i++) {
            u8 digit = 0;

            // this is literally the only time i use xy_to_index(),
            size_t index = j*9 + i; // xy_to_index(i, j);
            char c = sudoku_string[index];
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
