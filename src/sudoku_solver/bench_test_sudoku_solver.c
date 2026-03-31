
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



// this is a pretty cool function. i should put it into Bested.h
internal void print_running(const char *leading_text, u64 current, u64 total) {
    bool do_the_print = false;

    // if at the start, or the end, do the print.
    if (current ==     0)   do_the_print = true;
    if (current == total)   do_the_print = true;

    // if we advanced a percent, do the print.
    u64 prev_percent = ((current-1) * 100) / total;
    u64 curr_percent = ( current    * 100) / total;
    if (prev_percent != curr_percent)   do_the_print = true;

    // if some time has passed, and we are further than some percent, do the print.
    local_persist u64 last_time_printed   = 0;
    local_persist u64 last_number_printed = 0;
    {
        // in %, so 50% == 0.5
        #define ONCE_EVERY_X_PERCENT            0.01
        #define INVERSE_ONCE_EVERY_X_PERCENT    ((u64)(100 / ONCE_EVERY_X_PERCENT))

        //
        // this check is so the nanoseconds_since_unspecified_epoch()
        // function gets called less often.
        //
        // if for instance, we were iterating 100000000 times,
        // i dont want to call the nanoseconds_since_unspecified_epoch()
        // function that many times.
        //
        u64 prev_greater_percent = (last_number_printed * INVERSE_ONCE_EVERY_X_PERCENT) / total;
        u64 curr_greater_percent = (            current * INVERSE_ONCE_EVERY_X_PERCENT) / total;
        if (prev_greater_percent != curr_greater_percent) {
            // this function should take about 30ns max...
            // makes this print_running() function a bit worse to call in a super hot loop.
            u64 now = nanoseconds_since_unspecified_epoch();

            #define MAX_PRINTS_PER_SECOND        10
            if (now - last_time_printed >= (NANOSECONDS_PER_SECOND / MAX_PRINTS_PER_SECOND))   do_the_print = true;
        }
    }


    if (do_the_print) {
        // use this to add some ending padding to match starting padding.
        int num_leading_spaces = 0;
        {
            String s = S(leading_text);
            while (String_Starts_With(s, S(" "))) {
                num_leading_spaces += 1;
                String_Advance(&s, 1);
            }
        }

        int number_width = 0;
        { // int log 10
            u64 x = total;
            while (x > 0) { number_width += 1; x /= 10; }
        }

        // [leading_statement] [    i] / [total], [ xx]% [maybe_trailing_spaces]
        String to_print = temp_String_sprintf(
            "%s %*zu / %*zu, %3zu%%%*.s",
            leading_text,
            number_width, current,
            number_width, total,
            curr_percent,
            num_leading_spaces, "" // trailing spaces
        );

        // black letters on a white background.
        #define ESC_BLACK_N_WHITE   "\x1b[30;47m"
        #define ESC_RESET           "\x1b[0m"

        assert(Is_Between(curr_percent, 0, 100)); // dont overrun the buffer
        int how_far_along = (to_print.length * curr_percent) / 100;

        // this is also printing a progress bar, thats why this statement is weird
        printf(
            "\r" ESC_BLACK_N_WHITE "%.*s" ESC_RESET "%.*s",
            how_far_along,                          to_print.data,
            (int)(to_print.length - how_far_along), to_print.data + how_far_along
        );

        fflush(stdout);

        // call this now, so we get a longer max period between printf() calls.
        last_time_printed   = nanoseconds_since_unspecified_epoch();
        last_number_printed = current;
    }
}

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
            print_running("        Solving:", i, input_array.count);

            Input_Sudoku_Puzzle input = input_array.items[i];

            Sudoku_Solver_Result result = Solve_Sudoku(input);
            assert(result.sudoku_is_possible);
        }
        print_running("        Solving:", input_array.count, input_array.count); printf("\n");


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
