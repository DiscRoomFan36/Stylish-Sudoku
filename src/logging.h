
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


internal const char *temp_format_message(Logged_Message log) {
    switch (log.level) {
    case LOG_LEVEL_NORMAL:
        return temp_sprintf("SUDOKU LOG: %s", log.message_c_str);
    case LOG_LEVEL_ERROR:
        return temp_sprintf("%s:%d: ERROR: %s", log.file, log.line, log.message_c_str);
    }
}

internal void print_logged_message(Logged_Message log) {
    FILE *fd_for_fprintf;
    const char *formatted_message;

    switch (log.level) {
        case LOG_LEVEL_NORMAL: {
            fd_for_fprintf = stdout;
            formatted_message = temp_format_message(log);
        } break;
        case LOG_LEVEL_ERROR: {
            fd_for_fprintf = stderr;
            const char *bar = "===================================";
            formatted_message = temp_sprintf("%s\n%s\n%s", bar, temp_format_message(log), bar);
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



// uses scratch arena to allocate temp array.
//
// no strings are harmed, or null terminated,
// so do not put the result into strlen(),
// it will measure the whole of the array.
internal String_Array string_split_by(String input, const char *split_by) {
    String needle = S(split_by);

    String_Array result = { .allocator = context.scratch };

    while (true) {
        s64 index = String_Find_Index_Of(input, needle);
        if (index == -1) {
            Array_Append(&result, input);
            break;
        }

        String advanced = String_Advanced(input, index + needle.length);
        input.length = index;
        Array_Append(&result, input);
        input = advanced;
    }

    return result;
}







//
// String_Array wrap_text_with_font_and_size(String text, Font_And_Size font_and_size, s32 max_width, ...)
//
// returns a scratch allocated String_Array
//
// the original string is preserved, and used for the new strings,
// only the array needs to be allocated.
//

typedef struct {
    // split word if its to long to fit onto the line by itself
    //
    // normal behaviour just lets the line stick out
    bool split_word_if_cant_fit;
} wrap_text_with_font_and_size_Opt;

#define wrap_text_with_font_and_size(text, font_and_size, max_width, ...)       \
    _wrap_text_with_font_and_size(text, font_and_size, max_width, (wrap_text_with_font_and_size_Opt){ __VA_ARGS__ })

internal String_Array _wrap_text_with_font_and_size(String text, Font_And_Size font_and_size, s32 max_width, wrap_text_with_font_and_size_Opt opt) {
    String_Array result = { .allocator = context.scratch };


    // we'll do something special with these later.
    String_Array new_line_seperated_text_array = string_split_by(text, "\n");

    for (u32 k = 0; k < new_line_seperated_text_array.count; k++) {
        String text_without_new_lines = new_line_seperated_text_array.items[k];

        // get all the individual words,
        //
        // if two spaces are right next to each other,
        // it dose not matter, we will stick them together anyway
        String_Array words = string_split_by(text_without_new_lines, " ");
        ASSERT(words.count > 0); // TODO handle empty lines. maybe just skip to the end?

        // if (IsKeyPressed(KEY_A)) debug_break();

        String line_accumulator = ZEROED;
        for (u32 i = 0; i < words.count; i++) {
            String word = words.items[i];

            String checking_line_with_added = line_accumulator;
            if (line_accumulator.data == NULL) {
                // just set the current_line to the new word
                checking_line_with_added = word;
            } else {
                checking_line_with_added.length += word.length + 1; // add the extra space between.
            }


            const char *checking_c_str = temp_String_To_C_Str(checking_line_with_added);
            Vector2 text_size = MeasureTextEx(font_and_size.font, checking_c_str, font_and_size.size, 0);

            if (text_size.x > max_width) {
                // we need to break up the text.

                // check if there is only 1 word in the accumulator
                if (line_accumulator.data == NULL) {
                    // one single word has taken the whole line...
                    if (opt.split_word_if_cant_fit) {
                        TODO("opt.split_word_if_cant_fit");

                    } else {
                        // just put the entire word into a single line.
                        Array_Append(&result, word);

                        // we dont need to clear the accumulator, nothing is in it.
                        // Mem_Zero(&line_accumulator, sizeof(line_accumulator));
                        ASSERT((line_accumulator.data == NULL) && (line_accumulator.length == 0));

                        // move on.
                        continue;
                    }
                }

                // the new word tipped us over the edge. reject it.
                Array_Append(&result, line_accumulator); // put the new line into the result.

                // clear the accumulator
                Mem_Zero(&line_accumulator, sizeof(line_accumulator));

                // redo the word.
                //
                // if the word is by itself to big to fit onto
                // the line, it will be handled in the next loop.

                i -= 1; // while loops suck. give me a 'do_that_again;' keyword.
                continue;

            } else {
                // add to the line and keep going.
                line_accumulator = checking_line_with_added;
            }
        }

        // add whats left in the accumulator
        if (line_accumulator.data != NULL) {
            Array_Append(&result, line_accumulator);
        }


        // dont do thing if this is the last new_line_seperated_text_array item.
        if (k != new_line_seperated_text_array.count-1) {
            Array_Append(&result, S("")); // append the empty string, to indecate a newline.
        }
    }

    return result;
}



void draw_logger_frame(s32 x, s32 y) {
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


    #define LOGGER_FONT_SIZE    (FONT_SIZE/3)

    #define LOGGER_PERCENT_BEFORE_START_FADE_OUT   90
    #define LOGGER_BEFORE_START_FADE_OUT           (LOGGER_PERCENT_BEFORE_START_FADE_OUT / 100.0)

    Font_And_Size font_and_size = GetFontWithSize(LOGGER_FONT_SIZE);

    // draw
    s32 yy = 0;
    for (u64 i = 0; i < context.logged_messages_to_display.count; i++) {
        Logged_Message log = context.logged_messages_to_display.items[i];

        const char *formatted_message = temp_format_message(log);

        Vector2 position = ZEROED;

        position.y += yy;

        position.x += x;
        position.y += y;

        Color text_color = context.theme.logger.text_color;
        if (log.level == LOG_LEVEL_ERROR) text_color = context.theme.logger.error_text_color;

        if (log.t > LOGGER_BEFORE_START_FADE_OUT) {
            f64 factor = Remap(log.t, LOGGER_BEFORE_START_FADE_OUT, 1, 0, 1);
            // slow at the start, then moves fast
            text_color = Fade(text_color, 1-(factor*factor*factor));
        }

        s32 max_text_width = 400; // TODO
        String_Array lines = wrap_text_with_font_and_size(S(formatted_message), font_and_size, max_text_width);

        u32 num_empty_lines = 0;
        for (u32 i = 0; i < lines.count; i++) {
            String line = lines.items[i];
            if (line.length == 0)    num_empty_lines += 1;
        }
        if (num_empty_lines > 0) TODO("num_empty_lines > 0");


        // TODO padding
        const f32 padding = 10;
        Rectangle box = {
            .x = position.x,
            .y = position.y,
            .width = max_text_width + padding*2,
            .height = lines.count * font_and_size.size + padding*2,
        };

        position.x += padding;
        position.y += padding;

        DrawRectangleRec(box, context.theme.logger.box_background);
        DrawRectangleFrameRec(box, padding/2, context.theme.logger.box_frame_color);

        for (u32 i = 0; i < lines.count; i++) {
            String line = lines.items[i];

            if (line.length == 0) {
                TODO("handle empty lines");

            } else {
                // not cool, but what can we really do?
                const char *message = temp_String_To_C_Str(line);
                DrawTextEx(font_and_size.font, message, position, font_and_size.size, 0, text_color);
                // TODO do this better
                position.y += font_and_size.size;
                yy += font_and_size.size;
            }

        }

        // yy += box.height;
        yy += padding * 2 + 10; // TODO LOGGER_PADDING
    }
}



#endif // LOGGING_IMPLEMENTATION
