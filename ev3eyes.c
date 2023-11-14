// ev3eyes.c
#include "ev3eyes.h"
#include <stdlib.h>

const char* filenames[] = {
    "angry",
    "awake",
    "bottom_left",
    "bottom_right",
    "crazy_1",
    "crazy_2",
    "dizzy",
    "down",
    "evil",
    "knocked_out",
    "middle_left",
    "middle_right",
    "neutral",
    "pinched_left",
    "pinched_middle",
    "pinched_right",
    "sleeping",
    "tired_left",
    "tired_middle",
    "tired_right",
    "up",
    "winking"
};

image_t* eyes_imgs;
int eyes_imgs_num;
int last_eyes_drawn = -1;
SYSTIM time_last_eyes_drawn = 0;
int get_number_of_eyes_images()
{
    return eyes_imgs_num;
}

const char* get_eyes_image_name(int n)
{
    if (n < 0 || n >= eyes_imgs_num) n = 0;
    
    return filenames[n];
}

void draw_eyes(int n)
{
    if (n == last_eyes_drawn) return;
    
    ev3_lcd_draw_image(&eyes_imgs[n], 0, 0);
    last_eyes_drawn = n;
    
    ER ercd = get_tim(&time_last_eyes_drawn);
    assert(ercd == E_OK);
}

void draw_eyes_after_ms(int n, int ms)
{
    SYSTIM now;
    ER ercd = get_tim(&now);
    assert(ercd == E_OK);
    
    if (now - time_last_eyes_drawn > ms)
    {
        draw_eyes(n);
    }
}

void load_eyes_images()
{
    const char *input_path = "/eyes_imgs";
    eyes_imgs_num = sizeof(filenames) / sizeof(char*);
    eyes_imgs = (image_t*) calloc(eyes_imgs_num, sizeof(image_t));
    
    char path[50];
    for(int n = 0; n < eyes_imgs_num; n++)
    {
        static memfile_t memfile = { .buffer = NULL };
        if (memfile.buffer != NULL) ev3_memfile_free(&memfile);
        sprintf(path, "%s/%s.bmp", input_path, filenames[n]);
        ev3_memfile_load(path, &memfile);
        ev3_image_load(&memfile, &eyes_imgs[n]);
    }
}
