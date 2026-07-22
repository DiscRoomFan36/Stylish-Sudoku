
#ifndef SOUND_H_
#define SOUND_H_




typedef enum Sound_Event {
    SE_SUDOKU_DIGIT_PLACED_ON_EMPTY,
    SE_SUDOKU_DIGIT_PLACED_TO_ERASE,
    SE_SUDOKU_DIGIT_PLACED_TO_OVERWRITE,

    SE_SUDOKU_UNDO,
    SE_SUDOKU_REDO,

    SE_SUDOKU_CLEAR_GRID,

    SE_SUDOKU_SELECTION_CHANGED,

    // when the user clicks on a button
    SE_UI_BUTTON_CLICKED,
} Sound_Event;


typedef struct {
    String name;
    Sound raylib_sound;
} A_Sound;

typedef Array(A_Sound) A_Sound_Array;


internal void   init_sounds(void);
internal void uninit_sounds(void);

internal void play_sound(Sound_Event event);



#endif // SOUND_H_










#ifdef SOUND_IMPLEMENTATION



void init_sounds(void) {
    Context *context = get_context();

    Mem_Zero(&context->global_sound_array, sizeof(context->global_sound_array));
    context->global_sound_array.allocator = Pool_Get(&context->pool);

    // raylib sound initializer
    InitAudioDevice();
}
void uninit_sounds(void) {
    Context *context = get_context();

    for (u64 i = 0; i < context->global_sound_array.count; i++) {
        A_Sound *sound = &context->global_sound_array.items[i];
        UnloadSound(sound->raylib_sound);
    }

    Pool_Release(&context->pool, context->global_sound_array.allocator);
    // fully clear the pointers.
    Mem_Zero(&context->global_sound_array, sizeof(context->global_sound_array));

    // raylib sound de-initializer
    CloseAudioDevice();
}


internal const char *sound_event_to_name(Sound_Event event) {
    switch (event) {
    case SE_SUDOKU_DIGIT_PLACED_ON_EMPTY:     return "SE_SUDOKU_DIGIT_PLACED_ON_EMPTY";
    case SE_SUDOKU_DIGIT_PLACED_TO_ERASE:     return "SE_SUDOKU_DIGIT_PLACED_TO_ERASE";
    case SE_SUDOKU_DIGIT_PLACED_TO_OVERWRITE: return "SE_SUDOKU_DIGIT_PLACED_TO_OVERWRITE";

    case SE_SUDOKU_UNDO: return "SE_SUDOKU_UNDO";
    case SE_SUDOKU_REDO: return "SE_SUDOKU_REDO";

    case SE_SUDOKU_CLEAR_GRID:          return "SE_SUDOKU_CLEAR_GRID";
    case SE_SUDOKU_SELECTION_CHANGED:   return "SE_SUDOKU_SELECTION_CHANGED";

    case SE_UI_BUTTON_CLICKED:          return "SE_UI_BUTTON_CLICKED";
    }
    UNREACHABLE();
}

// returns the sound associated with that event.
//
// TODO have return an array so it can play multiple
// sounds for the same event.
internal String get_sounds_for_event(Sound_Event event) {
    switch (event) {
    case SE_SUDOKU_DIGIT_PLACED_ON_EMPTY: return S("");
    case SE_SUDOKU_DIGIT_PLACED_TO_ERASE: return S("");
    case SE_SUDOKU_DIGIT_PLACED_TO_OVERWRITE: return S("");

    case SE_SUDOKU_UNDO: return S("");
    case SE_SUDOKU_REDO: return S("");

    case SE_SUDOKU_CLEAR_GRID: return S("");

    case SE_SUDOKU_SELECTION_CHANGED: return S("");

    case SE_UI_BUTTON_CLICKED: return S("button_press.mp3");
    }
    UNREACHABLE();
}


void play_sound(Sound_Event event) {
    Context *context = get_context();

    String sound_name = get_sounds_for_event(event);
    if (String_Eq(sound_name, S(""))) {
        // TODO this is an error every time...
        log_error("no sound for event '%s'", sound_event_to_name(event));
        return;
    }

    // check if the sound is in the array
    A_Sound *found_sound = NULL;
    for (u64 i = 0; i < context->global_sound_array.count; i++) {
        A_Sound *sound = &context->global_sound_array.items[i];

        if (String_Eq(sound->name, sound_name)) {
            found_sound = sound;
            break;
        }
    }


    if (!found_sound) {
        #define SOUND_FOLDER_PATH   "assets/sound"
        const char *sound_path = temp_sprintf(SOUND_FOLDER_PATH "/" S_Fmt, S_Arg(sound_name));

        // add a new sound
        found_sound = Array_Add(&context->global_sound_array, 1, true);
        found_sound->name = String_Duplicate(sound_name, .allocator = context->global_sound_array.allocator, .null_terminate = true);
        found_sound->raylib_sound = LoadSound(sound_path);
        // TODO
        // LoadSoundAlias() use this to make copies to play at the same time.
        // UnloadSoundAlias() use to unload
        //
        // make 10 alias's of each sound, circler buffer technique.

        if (!IsSoundValid(found_sound->raylib_sound)) {
            log_error("When trying to load sound '"S_Fmt"', at path '%s', sound is not valid", S_Arg(found_sound->name), sound_path);

            FilePathList files = LoadDirectoryFiles(SOUND_FOLDER_PATH);
            printf("All files in directory:\n");
            for (u64 i = 0; i < files.count; i++) {
                printf(    "%zu: %s\n", i, files.paths[i]);
            }
        }
    }

    // play the sound
    PlaySound(found_sound->raylib_sound);
}







#endif // SOUND_IMPLEMENTATION


