
#include <time.h>


#define SUDOKU_SOLVER_IMPLEMENTATION
#include "./sudoku_solver.h"



// this is a pretty cool function. i should put it into Bested.h
internal void print_running(size_t i, size_t total) {
    bool do_the_print = false;

    if (i ==     0) do_the_print = true;
    if (i == total) do_the_print = true;

    size_t prev_percent = ((i-1) * 100) / total;
    size_t curr_percent = ( i    * 100) / total;
    if (prev_percent != curr_percent) do_the_print = true;

    if (do_the_print) {
        printf("\rRunning: %6zu / %zu, %3zu%%", i, total, curr_percent);
        fflush(stdout);
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
            print_running(i, TOTAL_LOOPS);

            // we want to make difficult sudoku's, to stress the solver.
            //
            // most of the time, the grid will not reach 17 digits,
            // and will be around 20-25
            Sudoku_Digit_Grid grid = Generate_Random_Sudoku(17);
            Array_Append(&sudoku_array, grid);
        }
        print_running(TOTAL_LOOPS, TOTAL_LOOPS); printf("\n");

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
            print_running(k, sudoku_array.count);

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
        print_running(sudoku_array.count, sudoku_array.count); printf("\n");

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
