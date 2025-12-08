
#ifndef INPUT_H_
#define INPUT_H_



typedef struct {
    struct {
        f64 now; // in seconds
        f64 dt;  // in seconds
    } time;

    struct {
        Vector2 pos;
        struct {
            bool clicked;
            bool down;

            f64 last_click_time;
            Vector2 last_click_location;
            bool double_clicked;
        } left;
    } mouse;


    struct {
        bool shift_down;
        bool control_down;
        bool delete_pressed;

        struct {
            bool up_pressed;
            bool down_pressed;
            bool left_pressed;
            bool right_pressed;
        } direction;

        struct {
            bool z_pressed;
            bool x_pressed;
        } key;

        bool shift_or_control_down;
        bool any_direction_pressed;

    } keyboard;

} Input;



internal        void   update_input(void);
internal inline Input *get_input(void);


#endif // INPUT_H_




#ifdef INPUT_IMPLEMENTATION



Input *get_input(void) {
    return &context.input;
}


void update_input(void) {
    Input *input = get_input();

    input->time.now = GetTime();
    input->time.dt  = GetFrameTime();

    input->mouse.pos                           = GetMousePosition();
    input->mouse.left.clicked                  = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    input->mouse.left.down                     = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    input->mouse.left.double_clicked           = false;

    if (input->mouse.left.clicked) {

        const f64 double_click_time_threshold_in_seconds = 0.500; // 500 ms

        f64 last_click_time         = input->mouse.left.last_click_time;
        f64 time_diff               = input->time.now - last_click_time;

        bool is_soon_enough = (time_diff <= double_click_time_threshold_in_seconds);


        // in pixels because thats what GetMousePosition returns
        const f32 max_distange_away_from_last_click_in_pixels = 5; // 5 pixels

        Vector2 last_click_location = input->mouse.left.last_click_location;
        f32 mouse_dist_sqr = Vector2DistanceSqr(last_click_location, input->mouse.pos);

        bool is_close_enough = (mouse_dist_sqr <= (max_distange_away_from_last_click_in_pixels * max_distange_away_from_last_click_in_pixels));


        // TODO think about triple clicks?
        if (is_soon_enough && is_close_enough) {
            input->mouse.left.double_clicked = true;
        }

        input->mouse.left.last_click_time = input->time.now;
        input->mouse.left.last_click_location = input->mouse.pos;
    }

    input->keyboard.shift_down                 = IsKeyDown(KEY_LEFT_SHIFT)   || IsKeyDown(KEY_RIGHT_SHIFT);
    input->keyboard.control_down               = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    input->keyboard.delete_pressed             = IsKeyPressed(KEY_DELETE)    || IsKeyPressed(KEY_BACKSPACE);

    input->keyboard.direction.up_pressed       = IsKeyPressed(KEY_UP)        || IsKeyPressed(KEY_W);
    input->keyboard.direction.down_pressed     = IsKeyPressed(KEY_DOWN)      || IsKeyPressed(KEY_S);
    input->keyboard.direction.left_pressed     = IsKeyPressed(KEY_LEFT)      || IsKeyPressed(KEY_A);
    input->keyboard.direction.right_pressed    = IsKeyPressed(KEY_RIGHT)     || IsKeyPressed(KEY_D);

    input->keyboard.key.z_pressed              = IsKeyPressed(KEY_Z);
    input->keyboard.key.x_pressed              = IsKeyPressed(KEY_X);


    input->keyboard.shift_or_control_down      = input->keyboard.shift_down || input->keyboard.control_down;
    input->keyboard.any_direction_pressed      = input->keyboard.direction.up_pressed || input->keyboard.direction.down_pressed || input->keyboard.direction.left_pressed || input->keyboard.direction.right_pressed;
}



#endif // INPUT_IMPLEMENTATION

