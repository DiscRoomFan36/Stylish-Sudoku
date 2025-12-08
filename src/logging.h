
#ifndef LOGGING_H_
#define LOGGING_H_


typedef enum {
    LOG_LEVEL_NORMAL,
    LOG_LEVEL_ERROR,
} Log_Level;



typedef struct {
    Log_Level level;

    String message;
    const char *message_c_str;

    const char *file;
    s32 line;

    f64 time_of_log;
    // for animation
    f64 t;

    // for the message.
    Arena *allocator;

} Logged_Message;

typedef struct {
    _Array_Header_;
    Logged_Message *items;
} Logged_Message_Array;





#define log(message, ...)           \
    log_impl(LOG_LEVEL_NORMAL, temp_sprintf(message, ##__VA_ARGS__), __FILE__, __LINE__)


#define log_error(message, ...)     \
    log_impl(LOG_LEVEL_ERROR, temp_sprintf(message, ##__VA_ARGS__), __FILE__, __LINE__)


internal void log_impl(Log_Level level, const char *message, const char *file, s32 line);



internal void draw_logger_frame(s32 x, s32 y);

#endif // LOGGING_H_










#ifdef LOGGING_IMPLEMENTATION


internal void print_logged_message(Logged_Message log) {
    FILE *fd_for_fprintf;
    const char *formatted_message;

    switch (log.level) {
        case LOG_LEVEL_NORMAL: {
            fd_for_fprintf = stdout;
            formatted_message = temp_sprintf("SUDOKU LOG: %s", log.message_c_str);
        } break;
        case LOG_LEVEL_ERROR: {
            fd_for_fprintf = stderr;
            const char *bar = "===================================";
            formatted_message = temp_sprintf("%s\n%s:%d: ERROR: %s\n%s", bar, log.file, log.line, log.message_c_str, bar);
        } break;
    }

    fprintf(fd_for_fprintf, "%s\n", formatted_message);
}



void log_impl(Log_Level level, const char *message, const char *file, s32 line) {
    // no newlines at the end please.
    ASSERT(message[strlen(message)-1] != '\n');



    Logged_Message log;

    log.allocator     = Scratch_Get(),

    log.level         = level;
    log.message       = String_Duplicate(log.allocator, S(message), .null_terminate = true);
    log.message_c_str = log.message.data;
    log.file          = file;
    log.line          = line;

    log.time_of_log   = get_input()->time.now;
    log.t             = 0;

    print_logged_message(log);

    Array_Append(&context.logged_messages_to_display, log);
}




void draw_logger_frame(s32 x, s32 y) {
    (void) (x + y);
    // TODO("draw_logger_frame");

    const f64 time_until_message_fades_away_in_seconds = 5;

    f64 dt = get_input()->time.dt / time_until_message_fades_away_in_seconds;

    // update
    for (u64 i = 0; i < context.logged_messages_to_display.count; i++) {
        Logged_Message *log = &context.logged_messages_to_display.items[i];

        log->t += dt;
        if (log->t >= 1) {
            // remove it.
            Scratch_Release(log->allocator);
            Array_Remove(&context.logged_messages_to_display, i, 1);
            i -= 1;
        }
    }

    #define LOGGER_FONT_SIZE    FONT_SIZE/2
    #define LOGGER_FONT_COLOR   FONT_COLOR

    Font_And_Size font_and_size = GetFontWithSize(LOGGER_FONT_SIZE);

    // draw
    s32 yy = 0;
    for (u64 i = 0; i < context.logged_messages_to_display.count; i++) {
        Logged_Message log = context.logged_messages_to_display.items[i];

        Vector2 position = ZEROED;

        position.y += yy;

        position.x += x;
        position.y += y;

        Color color = LOGGER_FONT_COLOR;
        #define PERCENT_BEFORE_START_FADE_OUT   90
        #define BEFORE_START_FADE_OUT           (PERCENT_BEFORE_START_FADE_OUT / 100.0)
        if (log.t > BEFORE_START_FADE_OUT) {
            f64 factor = Remap(log.t, BEFORE_START_FADE_OUT, 1, 0, 1);
            // slow at the start, then moves fast
            color = Fade(color, 1-(factor*factor*factor));
        }

        DrawTextEx(font_and_size.font, log.message_c_str, position, font_and_size.size, 0, color);

        yy += LOGGER_FONT_SIZE + 10; // TODO LOGGER_PADDING
    }
}



#endif // LOGGING_IMPLEMENTATION
