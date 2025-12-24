
#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "thirdparty/nob.h"

#define SRC_FOLDER          "./src/"
#define BUILD_FOLDER        "./build/"
#define THIRDPARTY_FOLDER   "./thirdparty/"

#define RAYLIB_FOLDER       THIRDPARTY_FOLDER"raylib-5.5/"



static bool build_debug(void);
static bool build_release(void);
static bool build_wasm(void);


static Cmd cmd = {0};



#define ARGUMENTS                                               \
    X(help,         "prints this help message and quits")       \
    X(all,          "build all targets. [debug, release, wasm]")        \
    X(clean,        "clean up all build artifacts, happens before all other commands, so 'all clean' will clean everything, then build everything.")        \
    X(debug,        "build   debug native version")             \
    X(release,      "build release native version")             \
    X(wasm,         "build web page with Emscripten")           \
    X(host_web_with_python3,        "build and use python3 to serve web build on port 8080")        \



void print_usage(const char *program_name, bool to_stderr) {
    FILE *out = to_stderr ? stderr : stdout;

    fprintf(out, "USAGE %s: [...ARGUMENTS]\n", program_name);
    fprintf(out, "[ARGUMENTS]:\n");
    #define X(name, description)    fprintf(out, "%11s: %s\n", #name, description);
        ARGUMENTS
    #undef X
}

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    const char *program_name = argv[0];

    // if no arguments provided, print help
    if (argc == 1) {
        print_usage(program_name, false);
        exit(EXIT_SUCCESS);
    }


    // parse arguments

    struct Flags {
        // make flags for all arguments
        #define X(name, ...) bool name;
            ARGUMENTS
        #undef X
    } flags = {};

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        bool valid_flag = false;
        bool duplicate_flag = false;

        // check if the argument is this flag.
        #define X(name, ...)                    \
            if (strcmp(arg, #name) == 0) {      \
                if (flags.name) duplicate_flag = true;     \
                flags.name = true;             \
                valid_flag  = true;             \
            }
            ARGUMENTS
        #undef X

        if (!valid_flag) {
            fprintf(stderr, "ERROR: INVALID ARGUMENT - '%s' in ", arg);

            fprintf(stderr, "[%s", program_name);
            for (int j = 1; j < argc; j++) fprintf(stderr, " %s", argv[j]);
            fprintf(stderr, "]\n");

            print_usage(program_name, true);
            exit(EXIT_FAILURE);
        }
        if (duplicate_flag) {
            fprintf(stderr, "ERROR: DUPLICATE FLAG - '%s' in ", arg);

            fprintf(stderr, "[%s", program_name);
            for (int j = 1; j < argc; j++) fprintf(stderr, " %s", argv[j]);
            fprintf(stderr, "]\n");

            print_usage(program_name, true);
            exit(EXIT_FAILURE);
        }
    }

    if (flags.help) {
        print_usage(program_name, false);
        exit(EXIT_SUCCESS);
    }


    if (flags.all) {
        flags.debug      = true;
        flags.release    = true;
        flags.wasm       = true;
    }

    if (flags.host_web_with_python3) {
        flags.wasm = true;
    }


    if (flags.clean) {
        cmd_append(&cmd, "rm", "-fr", BUILD_FOLDER);
        if (!cmd_run(&cmd))             exit(EXIT_FAILURE);

        // its ok to ignore the result, if nob.old did not exist, no reason to delete it.
        delete_file("./nob.old");

        // cleans the stuff in raylib (build artifacts)
        cmd_append(&cmd, "make", "clean");
        cmd_append(&cmd, "--directory="RAYLIB_FOLDER"src/");
        if (!cmd_run(&cmd))             exit(EXIT_FAILURE);
    }



    // TODO make this a command line arg
    //
    // this gets the latest version of Bested.h, I know that I might have to
    // refactor stuff when i do this, but its worth it to me.
    #define BESTED_PATH "/home/fletcher/Programming/C-things/Bested.h/Bested.h"
    if (file_exists(BESTED_PATH) == 1) {
        bool result = copy_file(BESTED_PATH, THIRDPARTY_FOLDER"Bested.h");
        if (!result) return 1;
    }


    // dont make a build folder if your not building anything.
    bool building_anything = flags.debug || flags.release || flags.wasm;
    if (building_anything) {
        mkdir_if_not_exists(BUILD_FOLDER);
    }

    if (flags.debug) {
        if (!build_debug())         exit(EXIT_FAILURE);
    }
    if (flags.release) {
        if (!build_release())       exit(EXIT_FAILURE);
    }
    if (flags.wasm) {
        if (!build_wasm())          exit(EXIT_FAILURE);
    }

    if (flags.host_web_with_python3) {
        printf("==============================================\n");
        printf("Running Python HTTP Server, Use Ctl-C to Quit.\n");
        printf("==============================================\n");
        cmd_append(&cmd, "python3");
        cmd_append(&cmd, "-m", "http.server");
        cmd_append(&cmd, "8080");
        cmd_append(&cmd, "--directory", "./build/web/");
        if (!cmd_run(&cmd)) return 1;
    }


    exit(EXIT_SUCCESS);
}








#define BUILD_LIBRARIES_FOLDER  BUILD_FOLDER"libraries/"

// this dose cashing
bool compile_raylib_library(const char *out_name, const char *platform) {
    const char *file_path = temp_sprintf("%s%s", BUILD_LIBRARIES_FOLDER, out_name);

    // do some caching.
    if (file_exists(file_path) == 1) {
        return true;
    }

    cmd_append(&cmd, "make");
    cmd_append(&cmd, "--directory="RAYLIB_FOLDER"src/");
    cmd_append(&cmd, temp_sprintf("PLATFORM=%s", platform));
    cmd_append(&cmd, "-B"); // rebuild all
    if (!cmd_run(&cmd)) return false;

    mkdir_if_not_exists(BUILD_LIBRARIES_FOLDER);
    // There might not be a raylib.a, it might have been made a shared library...
    // this project is not going to use a .so, so its file. 
    bool result = copy_file(
        RAYLIB_FOLDER"src/libraylib.a",
        file_path
    );
    if (!result) return false;

    return true;
}



void cmd_cc(void) {
    cmd_append(&cmd, "clang");
    cmd_append(&cmd, "-std=gnu11");
}

void cmd_c_flags(void) {
    cmd_append(&cmd, "-Wall", "-Wextra");
    // cmd_append(&cmd, "-Werror");
    cmd_append(&cmd, "-Wno-initializer-overrides");

    cmd_append(&cmd, "-I"THIRDPARTY_FOLDER);
}

void cmd_link_with_raylib(void) {
    cmd_append(&cmd, "-I"RAYLIB_FOLDER"src");
    cmd_append(&cmd, "-L"BUILD_LIBRARIES_FOLDER);
    cmd_append(&cmd, "-lraylib_static", "-lm");
}



bool build_debug(void) {
    if (!compile_raylib_library("libraylib_static.a", "PLATFORM_DESKTOP")) return false;

    cmd_cc();
    cmd_c_flags();
    cmd_append(&cmd, "-ggdb"); // debug flag

    cmd_append(&cmd, "-o", BUILD_FOLDER"main_debug");
    cmd_append(&cmd, SRC_FOLDER"main.c");

    cmd_link_with_raylib();

    if (!cmd_run(&cmd)) return false;
    return true;
}

bool build_release(void) {
    if (!compile_raylib_library("libraylib_static.a", "PLATFORM_DESKTOP")) return false;

    cmd_cc();
    cmd_c_flags();
    cmd_append(&cmd, "-O2");

    cmd_append(&cmd, "-o", BUILD_FOLDER"main_release");
    cmd_append(&cmd, SRC_FOLDER"main.c");

    cmd_link_with_raylib();

    if (!cmd_run(&cmd)) return false;
    return true;
}


bool build_wasm(void) {
    if (!compile_raylib_library("libraylib_web.a", "PLATFORM_WEB")) return false;

    // focus wasn't useing my .bashrc, so i had to provide this directly.
    // #define EMCC_PATH "/home/fletcher/Thirdparty/emsdk/upstream/emscripten/emcc"

    mkdir_if_not_exists(BUILD_FOLDER"web/");

    cmd_append(&cmd, "emcc");

    cmd_append(&cmd, "-o", BUILD_FOLDER"web/sudoku.html");      // Output file, the .html extension determines the files that need to be generated: `.wasm`, `.js` (glue code) and `.html` (optional: `.data`). All files are already configured to just work.
    cmd_append(&cmd, SRC_FOLDER"main.c");                       // The input files for compilation, in this case just one but it could be multiple code files: `game.c screen_logo.c screen_title.c screen_gameplay.c`

    cmd_append(&cmd, "-Os", "-Wall", "-Wextra");                // Some config parameters for the compiler, optimize code for small size and show all warnings generated
    cmd_append(&cmd, "-Wno-initializer-overrides");


    cmd_append(&cmd, BUILD_LIBRARIES_FOLDER"libraylib_web.a");  // This is the libraylib.a generated, it's recommended to provide it directly, with the path to it: i.e. `./raylib/src/libraylib.a`

    cmd_append(&cmd, "-I.", "-I"THIRDPARTY_FOLDER, "-I"RAYLIB_FOLDER"src/");    // Include path to look for additional #include .h files (if required)
    // cmd_append(&cmd, "-L.", RAYLIB_FOLDER"src/libraylib.a");    // Library path to look for additional library .a files (if required)

    cmd_append(&cmd, "-s", "USE_GLFW=3");                       // We tell the linker that the game/library uses GLFW3 library internally, it must be linked automatically (emscripten provides the implementation)

    cmd_append(&cmd, "--shell-file", THIRDPARTY_FOLDER"shell.html");            // All webs need a "shell" structure to load and run the game, by default emscripten has a `shell.html` but we can provide our own

    cmd_append(&cmd, "-DPLATFORM_WEB");


    cmd_append(&cmd, "--preload-file", "./assets/");        // Specify a resources directory for data compilation (it will generate a .data file)
    cmd_append(&cmd, "-s", "TOTAL_MEMORY=67108864");        // Specify a heap memory size in bytes (default = 16MB) (67108864 = 64MB)
    cmd_append(&cmd, "-s", "ALLOW_MEMORY_GROWTH=1");        // Allow automatic heap memory resizing -> NOT RECOMMENDED!
    cmd_append(&cmd, "-s", "FORCE_FILESYSTEM=1");           // Force filesystem creation to load/save files data (for example if you need to support save-game or drag&drop files)
    cmd_append(&cmd, "-s", "ASSERTIONS=1");                 // Enable runtime checks for common memory allocation errors (-O1 and above turn it off)


    if (!cmd_run(&cmd)) return false;

    return true;
}


