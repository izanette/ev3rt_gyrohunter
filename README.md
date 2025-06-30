# EV3RT Gyro Hunter

This repository contains an EV3RT application for LEGO® MINDSTORMS EV3 robots. The program keeps the robot balanced while responding to infrared remote commands and displaying animated "eye" images on the EV3 LCD.

## Repository Layout

- `app.c` – Main application logic, including the balance control algorithm and IR remote handling.
- `app.cfg` – EV3RT configuration file that defines tasks for the real-time kernel.
- `app.h` – Task priorities and function prototypes.
- `ev3eyes.c`/`ev3eyes.h` – Routines for loading and drawing eye images.
- `utils.c`/`utils.h` – Helper utilities for button handling, timing, and LCD output.
- `Makefile.inc` – Build configuration for EV3RT.

## Hardware Setup

The code assumes the following sensors and motors are connected:

- **Gyro sensor** – Port 2
- **Infrared sensor** – Port 4
- **Left motor** – Port A
- **Right motor** – Port D
- **Gun motor** – Port C

These assignments appear near the top of `app.c` and can be changed by
modifying the constants `gyro_sensor`, `ir_sensor`, `left_motor`,
`right_motor`, and `gun_motor`.

## Tasks

`app.cfg` defines three tasks inside the `TDOM_APP` domain:

1. **BALANCE_TASK** &ndash; Runs `balance_task`, which handles sensor calibration and keeps the robot upright by calling `keep_balance()` in a loop.
2. **MAIN_TASK** &ndash; Runs `main_task` at startup. It sets up sensors, starts other tasks, and interprets commands from the infrared remote to drive or steer the robot.
3. **IDLE_TASK** &ndash; Runs `idle_task` with the lowest priority.

## Balance Control

The balancing logic in `app.c` uses gyro and motor feedback. Parameters like `KGYROANGLE`, `KGYROSPEED`, `KPOS`, and `KSPEED` tune the control algorithm. The infrared remote can adjust these values at runtime.

## Eye Animations

`ev3eyes.c` expects BMP images in `/eyes_imgs` on the EV3 filesystem. The functions load these bitmaps and draw them on the LCD, allowing simple facial expressions while the robot is running.

## Recovering from Falls

If the robot tips over, `balance_task` stops and the status becomes `KNOCK_OUT_STATUS`.
Press the center (Enter) button to restart the balancing task. The program will
recalibrate the gyro sensor and resume operation once the status changes to
`RUNNING_STATUS`.

## Getting Started

To build and run the program, install the EV3RT toolchain and follow its standard workflow for compiling and deploying applications to the EV3. The code relies on `ev3api.h` from EV3RT.

## Next Steps

New contributors should become familiar with EV3RT's task system and the balancing algorithm. Experimenting with the tuning constants and extending the set of eye images are good ways to learn how the system works.

