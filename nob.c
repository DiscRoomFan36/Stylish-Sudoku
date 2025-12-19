
#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "thirdparty/nob.h"

#define SRC_FOLDER          "./src/"
#define BUILD_FOLDER        "./build/"
#define THIRDPARTY_FOLDER   "./thirdparty/"

#define RAYLIB_FOLDER       THIRDPARTY_FOLDER"raylib-5.5/"


bool check_if_file_exists(const char *filepath) {
    return access(filepath, F_OK) == 0;
}


Cmd cmd = {0};


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

void cmd_debug(void) {
    cmd_append(&cmd, "-ggdb");
}

void cmd_link_with_raylib(void) {
    cmd_append(&cmd, "-I"RAYLIB_FOLDER"src");
    cmd_append(&cmd, "-L"RAYLIB_FOLDER"src");
    cmd_append(&cmd, "-l:libraylib.so", "-lm");
}

bool build_debug(void) {
    cmd_cc();
    cmd_c_flags();
    cmd_debug();
    // cmd_append(&cmd, "-O2");
    cmd_append(&cmd, "-o", BUILD_FOLDER"main_debug");
    cmd_append(&cmd, SRC_FOLDER"main.c");
    cmd_link_with_raylib();
    if (!cmd_run(&cmd)) return false;

    return true;
}

bool build_release(void) {
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

    // focus wanst useing my .bashrc, so i had to provide this directly.
    #define EMCC_PATH "/home/fletcher/Thridparty/emsdk/upstream/emscripten/emcc"

    mkdir_if_not_exists(BUILD_FOLDER"web/");

    // emcc -o ./build/web/sudoku.html ./src/main.c -Os -Wall ./thirdparty/raylib-5.5/src/libraylib.a -I. -I./thirdparty/ -I./thirdparty/raylib-5.5/src/ -L. ./thirdparty/raylib-5.5/src/libraylib.a -s USE_GLFW=3 -s ASYNCIFY --shell-file ./thirdparty/shell.html -DPLATFORM_WEB
    cmd_append(&cmd, EMCC_PATH);

    cmd_append(&cmd, "-o", BUILD_FOLDER"web/sudoku.html");      // Output file, the .html extension determines the files that need to be generated: `.wasm`, `.js` (glue code) and `.html` (optional: `.data`). All files are already configured to just work.
    cmd_append(&cmd, SRC_FOLDER"main.c");                       // The input files for compilation, in this case just one but it could be multiple code files: `game.c screen_logo.c screen_title.c screen_gameplay.c`

    cmd_append(&cmd, "-Os", "-Wall", "-Wextra");                // Some config parameters for the compiler, optimize code for small size and show all warnings generated
    cmd_append(&cmd, "-Wno-initializer-overrides");


    cmd_append(&cmd, RAYLIB_FOLDER"src/libraylib.a");           // This is the libraylib.a generated, it's recommended to provide it directly, with the path to it: i.e. `./raylib/src/libraylib.a`

    cmd_append(&cmd, "-I.", "-I"THIRDPARTY_FOLDER, "-I"RAYLIB_FOLDER"src/");    // Include path to look for additional #include .h files (if required)
    cmd_append(&cmd, "-L.", RAYLIB_FOLDER"src/libraylib.a");    // Library path to look for additional library .a files (if required)

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


int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);


    // TODO make this a command line arg
    #define BESTED_PATH "/home/fletcher/Programming/C-things/Bested.h/Bested.h"
    if (check_if_file_exists(BESTED_PATH)) {
        cmd_append(&cmd, "cp");
        cmd_append(&cmd, BESTED_PATH);
        cmd_append(&cmd, THIRDPARTY_FOLDER"Bested.h");
        if (!cmd_run(&cmd)) return 1;
    }

    mkdir_if_not_exists(BUILD_FOLDER);

    if (!build_debug())     return 1;
    // if (!build_release())   return 1;


    // if (!build_wasm()) return 1;

    return 0;
}
