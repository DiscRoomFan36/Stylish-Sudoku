
#ifndef SOUND_H_
#define SOUND_H_



typedef struct {
    String name;
    Sound raylib_sound;
} A_Sound;

typedef struct {
    _Array_Header_;
    A_Sound *items;
} A_Sound_Array;



internal void   init_sounds(void);
internal void uninit_sounds(void);

internal void play_sound(const char *sound_name);



#endif // SOUND_H_










#ifdef SOUND_IMPLEMENTATION



void init_sounds(void) {
    Mem_Zero(&context.global_sound_array, sizeof(context.global_sound_array));
    context.global_sound_array.allocator = Pool_Get(&context.pool);
}
void uninit_sounds(void) {
    for (u64 i = 0; i < context.global_sound_array.count; i++) {
        A_Sound *sound = &context.global_sound_array.items[i];
        UnloadSound(sound->raylib_sound);
    }


    Pool_Release(&context.pool, context.global_sound_array.allocator);

    Mem_Zero(&context.global_sound_array, sizeof(context.global_sound_array));
}



void play_sound(const char *_sound_name) {
    String sound_name = S(_sound_name);

    // check if the sound is in the array
    A_Sound *found_sound = NULL;
    for (u64 i = 0; i < context.global_sound_array.count; i++) {
        A_Sound *sound = &context.global_sound_array.items[i];

        if (String_Eq(sound->name, sound_name)) {
            found_sound = sound;
            break;
        }
    }


    if (!found_sound) {
        // add a new sound
        found_sound = Array_Add_Clear(&context.global_sound_array, 1);
        found_sound->name = String_Duplicate(context.global_sound_array.allocator, sound_name, .null_terminate = true);
        found_sound->raylib_sound = LoadSound(temp_sprintf("assets/sound/"S_Fmt"", S_Arg(sound_name)));
        // LoadSoundAlias() use this to make copies to play at the same time.

        if (!IsSoundValid(found_sound->raylib_sound)) {
            log_error("When trying to load sound '"S_Fmt"', sound is not valid", S_Arg(found_sound->name));
        }
    }

    // play the sound
    PlaySound(found_sound->raylib_sound);
}







#endif // SOUND_IMPLEMENTATION


