#include "ev3api.h"
#include "utils.h"

int util_time_start_set = false;
SYSTIM util_time_start;

void waitEnterButtonPressed()
{
    while(!ev3_button_is_pressed(ENTER_BUTTON));
    while(ev3_button_is_pressed(ENTER_BUTTON));
}

button_t waitButtonPressed()
{
    while(1)
    {
        for(int i = 0; i < TNUM_BUTTON; i++)
        {
            if (ev3_button_is_pressed((button_t)i))
                return (button_t)i;
        }
    }
    
    return TNUM_BUTTON;
}

void waitButtonRelease(button_t button)
{
    while(ev3_button_is_pressed(button));
}

int hasAnyButtonPressed()
{
    for(int i = 0; i < TNUM_BUTTON; i++)
    {
        if (ev3_button_is_pressed((button_t)i))
            return true;
    }
    
    return false;
}

void waitNoButtonPressed()
{
    while(1)
    {
        // wait 10 milliseconds
        tslp_tsk(10);
        
        int stop = 1;
        for(int i = 0; i < TNUM_BUTTON; i++)
        {
            if (ev3_button_is_pressed((button_t)i))
                stop = 0;
        }
        if (stop) break;
    }
}

void clearScreen()
{
    ev3_lcd_fill_rect(0, 0, EV3_LCD_WIDTH, EV3_LCD_HEIGHT, EV3_LCD_WHITE);
}

void print(int line, const char* msg)
{
    // Draw information
    lcdfont_t font = EV3_FONT_MEDIUM;
    ev3_lcd_set_font(font);
    int32_t fontw, fonth;
    ev3_font_get_size(font, &fontw, &fonth);
    const char* lcdclean = "                    ";
    
    ev3_lcd_draw_string(lcdclean, 0, fonth * line);
    ev3_lcd_draw_string(msg, 0, fonth * line);
}

float getTimeMillis()
{
    if (!util_time_start_set)
    {
        get_tim(&util_time_start);
        util_time_start_set = true;
    }
    
    //const float TIME_TO_SECONDS = 1000.0;
    SYSTIM now;
    get_tim(&now);
    return (now - util_time_start); // / TIME_TO_SECONDS;
}

void motor(motor_port_t m, int power)
{
    if (power)
        ev3_motor_set_power(m, power);
    else
        ev3_motor_stop(m, false);
}

void draw_image(const char* path, int x, int y)
{
    static memfile_t memfile = { .buffer = NULL };
    static image_t image;
    if (memfile.buffer != NULL) ev3_memfile_free(&memfile);
    if (image.data != NULL) ev3_image_free(&image);
    ev3_memfile_load(path, &memfile);
    ev3_image_load(&memfile, &image);
    ev3_lcd_draw_image(&image, x, y);
}
