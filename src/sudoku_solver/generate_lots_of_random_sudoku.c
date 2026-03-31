
#include <time.h>


#define SUDOKU_SOLVER_IMPLEMENTATION
#include "./sudoku_solver.h"


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

// something like this function should definitely go into Bested.h, printing the time something took is super important.
internal void print_duration(u64 time_start, u64 time_end) {
    u64 duration = time_end - time_start;

    u64 time_in_ns = (duration                             ) % 1000;
    u64 time_in_us = (duration / (                  1000UL)) % 1000;
    u64 time_in_ms = (duration / (         1000UL * 1000UL)) % 1000;
    u64 time_in_s  = (duration / (1000UL * 1000UL * 1000UL)) % 1000;

    printf("time took: ");
    printf("%4lds, %4ldms, %4ldus, %4ldns", time_in_s, time_in_ms, time_in_us, time_in_ns);
    printf("\n");
}



#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

internal bool file_exists(const char *filepath) {
    struct stat stat_buf;
    if (stat(filepath, &stat_buf) < 0) {
        if (errno == ENOENT) return false;
        assert(false && "could not check if file exists.");
    }
    return true;
}

internal void mkdir_if_not_exists(const char *dirpath) {
    if (file_exists(dirpath)) return;

    if (mkdir(dirpath, 0755) < 0) {
        perror("could not make directory");
        exit(1);
    }
}


int main(void) {
    srand(time(NULL));

    typedef struct {
        _Array_Header_;
        Sudoku_Digit_Grid *items;
    } Sudoku_Digit_Grid_Array;

    Sudoku_Digit_Grid_Array sudoku_array = ZEROED;

    #define TOTAL_LOOPS 10000

    {
        printf("Generating %d sudoku's!\n", TOTAL_LOOPS);

        u64 time_start = nanoseconds_since_unspecified_epoch();

        Array_Reserve(&sudoku_array, TOTAL_LOOPS);

        // TODO this is slow, maybe multithreading?
        for (size_t i = 0; i < TOTAL_LOOPS; i++) {
            print_running("    Generating:", i, TOTAL_LOOPS);

            // we want to make difficult sudoku's, to stress the solver.
            //
            // most of the time, the grid will not reach 17 digits,
            // and will be around 20-25
            Sudoku_Digit_Grid grid = Generate_Random_Sudoku(17);
            Array_Append(&sudoku_array, grid);
        }
        print_running("    Generating:", TOTAL_LOOPS, TOTAL_LOOPS); printf("\n");

        u64 time_end = nanoseconds_since_unspecified_epoch();
        print_duration(time_start, time_end); printf("\n");
    }

    String_Builder sb = ZEROED;
    {

        #define SAVE_FOLDER "./build/randomly_generated_sudoku/"

        // the exe is probably in this file
        assert(file_exists("./build/"));
        mkdir_if_not_exists(SAVE_FOLDER);

        char filename_str_buf[FILENAME_MAX] = ZEROED;
        {
            time_t date_time_now = time(NULL);
            // date time deconstructed
            struct tm dtd = *localtime(&date_time_now);
            // .rgstf -> Randomly Generated Sudoku Text File
            snprintf(filename_str_buf, sizeof(filename_str_buf), SAVE_FOLDER "%04d_%02d_%02d_%02d_%02d_%02d.rgstf", dtd.tm_year + 1900, dtd.tm_mon + 1, dtd.tm_mday, dtd.tm_hour, dtd.tm_min, dtd.tm_sec);
        }

        const char *save_file_path = filename_str_buf;


        printf("Saving to file %s\n", save_file_path);
        u64 time_start = nanoseconds_since_unspecified_epoch();

        for (u64 k = 0; k < sudoku_array.count; k++) {
            print_running("    Formatting:", k, sudoku_array.count);

            Sudoku_Digit_Grid grid = sudoku_array.items[k];

            for (size_t j = 0; j < Array_Len(grid.digits); j++) {
                for (size_t i = 0; i < Array_Len(grid.digits[0]); i++) {
                    u8 cell = grid.digits[j][i];
                    if (cell == 0) {
                        String_Builder_printf(&sb, ".");
                    } else {
                        String_Builder_printf(&sb, "%d", cell);
                    }
                }
            }

            String_Builder_printf(&sb, "\n");
        }
        print_running("    Formatting:", sudoku_array.count, sudoku_array.count); printf("\n");

        String big_sudoku_string = String_Builder_To_String(&sb);


        FILE *f = fopen(save_file_path, "wb");
        if (!f) {
            perror("Could not open file");
            exit(EXIT_FAILURE);
        }

        size_t write_count = fwrite(big_sudoku_string.data, sizeof(char), big_sudoku_string.length, f);
        fclose(f);

        if (write_count != big_sudoku_string.length) {
            perror("write count is not equal to string length");
            exit(EXIT_FAILURE);
        }

        u64 time_end = nanoseconds_since_unspecified_epoch();
        print_duration(time_start, time_end); printf("\n");
    }

    printf("Finished!\n");
    return 0;
}


#define BESTED_IMPLEMENTATION
#include "Bested.h"
