
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
    Sound original_raylib_sound;

    // a circular buffer of duplicate sounds.
    u64 duplicate_sound_index;
    // an array of Alias sounds, array length is
    // how many of the same sound you can play at once.
    //
    // older sounds will be "overwritten" (aka restarting from the begining).
    Sound duplicate_sounds_array[8];
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
        // doing raylib sound interface stuff.
        for (u64 i = 0; i < Array_Len(sound->duplicate_sounds_array); i++) {
            UnloadSoundAlias(sound->duplicate_sounds_array[i]);
        }
        UnloadSound(sound->original_raylib_sound);
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
        found_sound->original_raylib_sound = LoadSound(sound_path);
        // the original_raylib_sound could be invalid at this point.
        // but raylib is merciful to us, and will just not do anything.
        for (u64 i = 0; i < Array_Len(found_sound->duplicate_sounds_array); i++) {
            found_sound->duplicate_sounds_array[i] = LoadSoundAlias(found_sound->original_raylib_sound);
        }

        if (!IsSoundValid(found_sound->original_raylib_sound)) {
            log_error("When trying to load sound '"S_Fmt"', at path '%s', sound is not valid", S_Arg(found_sound->name), sound_path);

            // this prints out all files in a directory,
            // for debugging purposes.
            //
            // FilePathList files = LoadDirectoryFiles(SOUND_FOLDER_PATH);
            // printf("All files in directory:\n");
            // for (u64 i = 0; i < files.count; i++) {
            //     printf(    "%zu: %s\n", i, files.paths[i]);
            // }
        }
    }

    { // play the sound
        // were being super careful here, no array out of bounds for us.
        u64 sound_index = found_sound->duplicate_sound_index % Array_Len(found_sound->duplicate_sounds_array);
        Sound sound = found_sound->duplicate_sounds_array[sound_index];
        PlaySound(sound);
        found_sound->duplicate_sound_index = (found_sound->duplicate_sound_index + 1) % Array_Len(found_sound->duplicate_sounds_array);
    }
}







#endif // SOUND_IMPLEMENTATION


