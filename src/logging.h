
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

    Source_Code_Location source_code_location;

    // how many times this message was sent.
    // TODO better name.
    u32 message_duplicate_count;

    f64 time_of_first_log;
    f64 time_of_last_log;
    // for animation
    f64 t;

    // for the message.
    Arena *allocator;

} Logged_Message;

typedef Array(Logged_Message) Logged_Message_Array;





#define log(message, ...)           \
    log_impl(LOG_LEVEL_NORMAL, temp_sprintf(message, ##__VA_ARGS__), Get_Source_Code_Location())


#define log_error(message, ...)     \
    log_impl(LOG_LEVEL_ERROR, temp_sprintf(message, ##__VA_ARGS__), Get_Source_Code_Location())


internal void log_impl(Log_Level level, const char *message, Source_Code_Location source_code_location);



internal void draw_logger_frame(s32 x, s32 y);

#endif // LOGGING_H_










#ifdef LOGGING_IMPLEMENTATION


internal const char *temp_format_message(Logged_Message log) {
    const char *duplicate_message_string = "";
    if (log.message_duplicate_count > 1) {
        duplicate_message_string = temp_sprintf(" (%u)", log.message_duplicate_count);
    }

    switch (log.level) {
    case LOG_LEVEL_NORMAL:
        return temp_sprintf("SUDOKU LOG: %s%s", log.message_c_str, duplicate_message_string);
    case LOG_LEVEL_ERROR:
        return temp_sprintf(SCL_Fmt" ERROR: %s%s", SCL_Arg(log.source_code_location), log.message_c_str, duplicate_message_string);
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



void log_impl(Log_Level level, const char *_message, Source_Code_Location source_code_location) {
    Context *context = get_context();

    String message = S(_message);

    // no newlines at the end please.
    ASSERT(message.data[message.length-1] != '\n');

    // check if this logged message is allready in the messages, to de-dup
    for (u64 i = 0; i < context->logged_messages_to_display.count; i++) {
        Logged_Message *this_log = &context->logged_messages_to_display.items[i];

        if (!source_code_location_eq(source_code_location, this_log->source_code_location)) continue;

        if (level != this_log->level) continue;

        // if the message 
        if (!String_Eq(message, this_log->message)) continue;

        // we got a duplicate message.
        this_log->message_duplicate_count += 1;
        this_log->time_of_last_log = get_input()->time.now;

        // probably dont print this, dont want to flood the console.
        // print_logged_message(*this_log);

        return;
    }


    Logged_Message log = ZEROED;

    log.allocator     = Scratch_Get(),

    log.level         = level;
    log.message       = String_Duplicate(log.allocator, message, .null_terminate = true);
    log.message_c_str = log.message.data;

    log.source_code_location = source_code_location;

    log.message_duplicate_count = 1;

    log.time_of_first_log = get_input()->time.now;
    log.time_of_last_log  = get_input()->time.now;
    log.t                 = 0;

    print_logged_message(log);

    Array_Append(&context->logged_messages_to_display, log);
}



// uses temporary allocator
//
// no strings are harmed, or null terminated,
// so do not put the result into strlen(),
// it will measure the whole of the array.
internal String_Array string_split_by(String input, const char *split_by) {
    String needle = S(split_by);

    String_Array result = { .allocator = get_temporary_allocator() };

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
    String_Array result = { .allocator = get_temporary_allocator() };


    // we'll do something special with these later.
    String_Array new_line_separated_text_array = string_split_by(text, "\n");

    for (u32 k = 0; k < new_line_separated_text_array.count; k++) {
        String text_without_new_lines = new_line_separated_text_array.items[k];

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


        // dont do thing if this is the last new_line_separated_text_array item.
        if (k != new_line_separated_text_array.count-1) {
            // append the empty string, to indicate a newline.
            Array_Append(&result, S(""));
        }
    }

    return result;
}



// TODO maybe this should take a width and height? like the handle_and_draw_sudoku()?
void draw_logger_frame(s32 x, s32 y) {

    Context *context = get_context();
    Theme   *theme   = get_theme();
    Input   *input   = get_input();

    const f64 time_until_message_fades_away_in_seconds = 5;

    f64 dt = input->time.dt / time_until_message_fades_away_in_seconds;

    // update
    for (u64 i = 0; i < context->logged_messages_to_display.count; i++) {
        Logged_Message *log = &context->logged_messages_to_display.items[i];

        // dont update if this is the first time.
        if (log->time_of_last_log == input->time.now) {
            log->t = 0; // will reset when duplicate message shows up
            continue;
        }

        log->t += dt;
        if (log->t >= 1) {
            // remove it.
            Scratch_Release(log->allocator);
            Array_Remove(&context->logged_messages_to_display, i, 1);
            i -= 1;
        }
    }


    #define LOGGER_PERCENT_BEFORE_START_FADE_OUT   90
    #define LOGGER_BEFORE_START_FADE_OUT           (LOGGER_PERCENT_BEFORE_START_FADE_OUT / 100.0)

    Font_And_Size font_and_size = GetFontWithSize(theme->logger.font_size);

    // draw
    s32 yy = 0;
    for (u64 i = 0; i < context->logged_messages_to_display.count; i++) {
        Logged_Message log = context->logged_messages_to_display.items[i];

        // TODO make red flash when t == 0, so its shows duplicates better.
        const char *formatted_message = temp_format_message(log);

        Vector2 position = ZEROED;

        position.y += yy;

        position.x += x;
        position.y += y;


        Color text_color = theme->logger.text_color;
        if (log.level == LOG_LEVEL_ERROR) text_color = theme->logger.error_text_color;

        Color color_background = theme->logger.box_background_color;
        Color color_frame      = theme->logger.box_frame_color;


        if (log.t > LOGGER_BEFORE_START_FADE_OUT) {
            f64 factor = Remap(log.t, LOGGER_BEFORE_START_FADE_OUT, 1, 0, 1);
            // slow at the start, then moves fast
            text_color       = Fade(text_color,       1-(factor*factor*factor));
            color_background = Fade(color_background, 1-(factor*factor*factor));
            color_frame      = Fade(color_frame,      1-(factor*factor*factor));
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

        DrawRectangleRec(box, color_background);
        DrawRectangleFrameRec(box, padding/2, color_frame);

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
