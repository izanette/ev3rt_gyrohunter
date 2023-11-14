// ev3eyes.h
#pragma once
#include "ev3api.h"

#define EV3EYE_ANGRY            0
#define EV3EYE_AWAKE            1
#define EV3EYE_BOTTOM_LEFT      2
#define EV3EYE_BOTTOM_RIGHT     3
#define EV3EYE_CRAZY_1          4
#define EV3EYE_CRAZY_2          5
#define EV3EYE_DIZZY            6
#define EV3EYE_DOWN             7
#define EV3EYE_EVIL             8
#define EV3EYE_KNOCKED_OUT      9
#define EV3EYE_MIDDLE_LEFT     10
#define EV3EYE_MIDDLE_RIGHT    11
#define EV3EYE_NEUTRAL         12
#define EV3EYE_PINCHED_LEFT    13
#define EV3EYE_PINCHED_MIDDLE  14
#define EV3EYE_PINCHED_RIGHT   15
#define EV3EYE_SLEEPING        16
#define EV3EYE_TIRED_LEFT      17
#define EV3EYE_TIRED_MIDDLE    18
#define EV3EYE_TIRED_RIGHT     19
#define EV3EYE_UP              20
#define EV3EYE_WINKING         21


extern image_t* eyes_imgs;
extern int eyes_imgs_num;
extern SYSTIM time_last_eyes_drawn;

void draw_eyes(int number);
int get_number_of_eyes_images();
const char* get_eyes_image_name(int n);
    
void load_eyes_images();
