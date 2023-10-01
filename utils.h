#ifndef __UTILS_H__
#define __UTILS_H__

#include "ev3api.h"

#define MINVAL(X, Y)  ((X) < (Y) ? (X) : (Y))
#define MAXVAL(X, Y)  ((X) > (Y) ? (X) : (Y))

void waitEnterButtonPressed();
button_t waitButtonPressed();
void waitButtonRelease(button_t button);
int hasAnyButtonPressed();
void waitNoButtonPressed();

void clearScreen();
void print(int line, const char* msg);

float getTimeMillis();

void motor(motor_port_t m, int power);
void draw_image(const char* path, int x, int y);

#endif // __UTILS_H__