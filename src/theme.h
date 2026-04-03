
#ifndef THEME_H
#define THEME_H


#include "Bested.h"

#include <raylib.h>




// funny functions, display the color they represent in VSCode.
//
// is the reason I use VSCode to edit colors,
//
// even though this feature is because of CSS, (i presume,)
// its super helpful to have a color picker in your editor.
#define rgba(_r, _g, _b, _a) ( (Color){.r = (_r), .g = (_g), .b = (_b), .a = (_a*255) + 0.5} )
#define rgb(r, g, b) rgba((r), (g), (b), 1)



typedef struct {
    Color background_color;
    // bool has_border; // maybe?
    Color boarder_color;

    Color text_color;
} Button_Ui_Theme;

// the Theme or Pallet.
//
// this makes it easy to change the theme in the future.
// just update this struct and the rest of the program will follow suit.
typedef struct {
    Color background_color;

    // things relating to the sudoku. witch is most things.
    struct {
        Color cell_background_color;
        Color cell_lines_color;
        Color box_lines_color;

        struct {
            Color   valid_builder_digit_color;
            Color   valid_solver_digit_color;
            Color invalid_builder_digit_color;
            Color invalid_solver_digit_color;

            Color   valid_marking_certain_digit_color;
            Color   valid_marking_uncertain_digit_color;
            Color invalid_marking_certain_digit_color;
            Color invalid_marking_uncertain_digit_color;
        } cell_text;

        // turn "nth bit set" into a color
        Color cell_color_bitfield[32];

        //
        // factors hare are in terms of cell size
        //
        struct {
            Color highlight_color;
            f64 line_thickness_factor;
        } select;

        struct {
            f64 cell_inner_line_thickness_factor;
            f64 boarder_line_thickness_factor;
        };

        struct {
            f64 digit_size_factor;
            // TODO maybe the markings use a different font? maybe the bold version.
            f64 uncertain_digit_size_factor;

            f64 certain_digit_max_size_factor;
            f64 certain_digit_min_size_factor;

            u64 allowed_certain_digits_before_shrinking;
        } text;
    } sudoku;


    struct {
        Color text_color;
        Color error_text_color;

        Color box_background_color;
        Color box_frame_color;

        f64 font_size;
        // f64 percent_before_fade_out; // TODO maybe?
    } logger;

    struct {
        struct {
            s32 font_size;
            f32 text_padding;
            f32 boarder_size;

            Button_Ui_Theme base;
            Button_Ui_Theme hovered;
            Button_Ui_Theme clicked;
        } button;
    } ui;

} Theme;

//
// so we can separate calls to get_context(),
//    - one for doing stuff with the context (spooky action at a distance.)
//    - other for just drawing stuff
// two very different actions.
//
internal Theme *get_theme(void);

//
// maybe later this can accept a path to a theme file or something.
//
// for now just inits the base theme and returns it.
//
internal Theme init_theme(void);



#endif // THEME_H





#ifdef THEME_IMPLEMENTATION


Theme *get_theme(void) { return &get_context()->theme_base; }


Theme init_theme(void) {
    Theme theme = ZEROED;

    // https://coolors.co/visualizer/7f5539-a68a64-ede0d4-656d4a-414833
    // const Color pallet_1 = rgb(127, 85, 57);
    // const Color pallet_2 = rgb(166, 138, 100);
    const Color pallet_3 = rgb(237, 224, 212);
    const Color pallet_4 = rgb(101, 109, 74);
    const Color pallet_5 = rgb(65, 72, 51);

    const Color pallet_select = rgb(0, 162, 255);
    const Color pallet_error = rgb(255, 55, 0);

    theme.background_color = pallet_3;

    { // sudoku
        theme.sudoku.cell_background_color    = pallet_3;
        theme.sudoku.cell_lines_color         = pallet_4;
        theme.sudoku.box_lines_color          = pallet_5;


        theme.sudoku.cell_text.  valid_builder_digit_color            = pallet_5;
        theme.sudoku.cell_text.  valid_solver_digit_color             = pallet_select;
        theme.sudoku.cell_text.  valid_marking_certain_digit_color    = pallet_select;
        theme.sudoku.cell_text.  valid_marking_uncertain_digit_color  = pallet_select;

        theme.sudoku.cell_text.invalid_builder_digit_color            = pallet_5; // TODO make this striped
        theme.sudoku.cell_text.invalid_solver_digit_color             = pallet_error;
        theme.sudoku.cell_text.invalid_marking_certain_digit_color    = pallet_error;
        theme.sudoku.cell_text.invalid_marking_uncertain_digit_color  = pallet_error;

        { // sudoku cell color bitfield
            const Color transparent = rgba(0, 0, 0, 0);

            // Colors stolen from https://sudokupad.app
            const Color cell_colors[Array_Len(theme.sudoku.cell_color_bitfield)] = {
                transparent,          // cell color 0
                rgb(214, 214, 214),  // cell color 1
                rgb(124, 124, 124),  // cell color 2
                rgb( 36,  36,  36),  // cell color 3
                rgb(179, 229, 106),  // cell color 4
                rgb(232, 124, 241),  // cell color 5
                rgb(228, 150,  50),  // cell color 6
                rgb(245,  58,  55),  // cell color 7
                rgb(252, 235,  63),  // cell color 8
                rgb( 61, 153, 245),  // cell color 9
                transparent,          // cell color a
                rgb(204,  51,  17),  // cell color b
                rgb( 17, 119,  51),  // cell color c
                rgb(  0,  68, 196),  // cell color d
                rgb(238, 153, 170),  // cell color e
                rgb(255, 255,  25),  // cell color f
                rgb(240,  70, 240),  // cell color g
                rgb(160,  90,  30),  // cell color h
                rgb( 51, 187, 238),  // cell color i
                rgb(145,  30, 180),  // cell color j
                transparent,          // cell color k
                rgb(245,  58,  55),  // cell color l
                rgb( 76, 175,  80),  // cell color m
                rgb( 61, 153, 245),  // cell color n
                rgb(249, 136, 134),  // cell color o
                rgb(149, 208, 151),  // cell color p
                rgb(158, 204, 250),  // cell color q
                rgb(170,  12,   9),  // cell color r
                rgb( 47, 106,  49),  // cell color s
                rgb(  9,  89, 170),  // cell color t
            };

            static_assert(sizeof(cell_colors) <= sizeof(theme.sudoku.cell_color_bitfield), "just making sure.");
            Mem_Copy(theme.sudoku.cell_color_bitfield, (void*)cell_colors, sizeof(cell_colors));

        }

        {
            theme.sudoku.select.highlight_color = pallet_select;
            theme.sudoku.select.line_thickness_factor  = 5.0 / 60.0;
        }

        {
            const f64 inner_line_thickness_factor = 1.0 / 60.0;
            theme.sudoku.cell_inner_line_thickness_factor = inner_line_thickness_factor;
            theme.sudoku.boarder_line_thickness_factor    = inner_line_thickness_factor * 3; // about 3 times as big as inner line

            // there was once an assert that inner line thickness was
            // smaller than boarder line thickness...
            // wonder what that was all about...
        }
        {
            const f64 font_size_factor = 1.0; // as big as the cell size.
            theme.sudoku.text.digit_size_factor           = font_size_factor;
            theme.sudoku.text.uncertain_digit_size_factor = font_size_factor / 3; // 3 times as small.

            theme.sudoku.text.certain_digit_max_size_factor = 1.0 / 2.5;
            theme.sudoku.text.certain_digit_min_size_factor = 1.0 / 6;

            theme.sudoku.text.allowed_certain_digits_before_shrinking = 4;
        }
    }


    { // logger
        theme.logger.text_color         = pallet_4;
        theme.logger.error_text_color   = pallet_error;

        theme.logger.box_background_color     = pallet_3;
        theme.logger.box_frame_color    = pallet_5;

        // this is arbitrary,
        theme.logger.font_size = 60.0 / 3.0;
    }

    { // ui
        { // button
            // TODO think about these values.
            theme.ui.button.font_size    = 32;
            theme.ui.button.text_padding = 10;
            theme.ui.button.boarder_size =  5;

            // TODO Colors
            theme.ui.button.base    = (Button_Ui_Theme){
                .background_color = rgb(175, 175, 175),
                .boarder_color    = rgb(14, 14, 14),
                .text_color       = rgb(31, 31, 31),
            };
            theme.ui.button.hovered = (Button_Ui_Theme){
                .background_color = rgb(214, 84, 84),
                .boarder_color    = rgb(14, 14, 14),
                .text_color       = rgb(31, 31, 31),
            };
            theme.ui.button.clicked = (Button_Ui_Theme){
                .background_color = rgb(61, 72, 221),
                .boarder_color    = rgb(14, 14, 14),
                .text_color       = rgb(31, 31, 31),
            };
        }
    }

    return theme;
}



#endif // THEME_IMPLEMENTATION

