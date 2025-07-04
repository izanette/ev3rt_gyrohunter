/**
 * This sample program balances a two-wheeled Segway type robot such as Gyroboy in EV3 core set.
 *
 * References:
 * http://www.hitechnic.com/blog/gyro-sensor/htway/
 * http://www.cs.bgu.ac.il/~ami/teaching/Lejos-2013/classes/src/lejos/robotics/navigation/Segoway.java
 */

#include "ev3api.h"
#include "app.h"
#include "utils.h"
#include "ev3eyes.h"

#define USE_FACES
#define FIRE_TURNS 15

#define DEBUG

#ifdef DEBUG
#define _debug(x) (x)
#else
#define _debug(x)
#endif

#ifdef USE_FACES
#define DRAW_EYES(idx)               draw_eyes(idx)
#define DRAW_EYES_AFTER_MS(idx, ms)  draw_eyes_after_ms(idx, ms)
#else
#define DRAW_EYES(idx)
#define DRAW_EYES_AFTER_MS(idx, ms)
#endif


typedef enum {
    IDLE_STATUS,
    CALIB_STATUS,
    RUNNING_STATUS,
    KNOCK_OUT_STATUS,
    TNUM_STATUS
} gyrohunter_status_t;

gyrohunter_status_t gyrohunter_status = IDLE_STATUS;

/**
 * Define the connection ports of the gyro sensor and motors.
 * By default, the Gyro Boy robot uses the following ports:
 * Gyro sensor: Port 2
 * Left motor:  Port A
 * Right motor: Port D
 */
const int gyro_sensor = EV3_PORT_2;
const int ir_sensor = EV3_PORT_4;
const int left_motor = EV3_PORT_A;
const int right_motor = EV3_PORT_D;
const int gun_motor = EV3_PORT_C;

/**
 * Constants for the self-balance control algorithm. (Gyrohunter version)
 */
const float KSTEER=-0.25;
const float EMAOFFSET = 0.0005f;
      float KGYROANGLE = 6.0f; // 7.5f
      float KGYROSPEED = 1.4f; // 1.15f
      float KPOS = 0.035f; // 0.07f;
      float KSPEED = 0.1f;
const float KDRIVE = -0.02f;
const float WHEEL_DIAMETER = 5.6;
const uint32_t WAIT_TIME_MS = 5;
const uint32_t FALL_TIME_MS = 1000;
const float INIT_GYROANGLE = -0.25;
const float INIT_INTERVAL_TIME = 0.014;

/**
 * Constants for the self-balance control algorithm. (Original)
 */
//const float KSTEER=-0.25;
//const float EMAOFFSET = 0.0005f;
//const float KGYROANGLE = 7.5f;
//const float KGYROSPEED = 1.15f;
//const float KPOS = 0.07f;
//const float KSPEED = 0.1f;
//const float KDRIVE = -0.02f;
//const float WHEEL_DIAMETER = 5.6;
//const uint32_t WAIT_TIME_MS = 5;
//const uint32_t FALL_TIME_MS = 1000;
//const float INIT_GYROANGLE = -0.25;
//const float INIT_INTERVAL_TIME = 0.014;

/**
 * Constants for the self-balance control algorithm. (Gyroboy Version)
 */
//const float EMAOFFSET = 0.0005f;
//const float KGYROANGLE = 15.0f;
//const float KGYROSPEED = 0.8f;
//const float KPOS = 0.12f;
//const float KSPEED = 0.08f;
//const float KDRIVE = -0.01f;
//const float WHEEL_DIAMETER = 5.6;
//const uint32_t WAIT_TIME_MS = 1;
//const uint32_t FALL_TIME_MS = 1000;
//const float INIT_GYROANGLE = -0.25;
//const float INIT_INTERVAL_TIME = 0.014;

/**
 * Global variables used by the self-balance control algorithm.
 */
static int motor_diff, motor_diff_target;
static int loop_count, motor_control_drive, motor_control_steer;
static float gyro_offset, gyro_speed, gyro_angle, interval_time;
static float motor_pos, motor_speed;

/**
 * Calculate the initial gyro offset for calibration.
 */
static ER calibrate_gyro_sensor() {
    int min_rate = 1000, max_rate = -100, gSum = 0;
    for (int i = 0; i < 200; ++i) {
        int gyro = ev3_gyro_sensor_get_rate(gyro_sensor);
        gSum += gyro;
        if (gyro > max_rate)
            max_rate = gyro;
        if (gyro < min_rate)
            min_rate = gyro;
        tslp_tsk(4);
    }
    if(max_rate - min_rate < 2) {
        gyro_offset = gSum / 200.0f;
        return E_OK;
    } else {
        return E_OBJ;
    }
}

/**
 * Calculate the average interval time of the main loop for the self-balance control algorithm.
 * Units: seconds
 */
static void update_interval_time() {
    static SYSTIM start_time;

    if(loop_count++ == 0) { // Interval time for the first iteration (use INIT_INTERVAL_TIME)
        interval_time = INIT_INTERVAL_TIME;
        ER ercd = get_tim(&start_time);
        assert(ercd == E_OK);
    } else {
        SYSTIM now;
        ER ercd = get_tim(&now);
        assert(ercd == E_OK);
        interval_time = ((float)(now - start_time)) / loop_count / 1000;
    }
}

/**
 * Update data of the gyro sensor.
 * gyro_offset: the offset for calibration.
 * gyro_speed: the speed of the gyro sensor after calibration.
 * gyro_angle: the angle of the robot.
 */
static void update_gyro_data() {
    int gyro = ev3_gyro_sensor_get_rate(gyro_sensor);
    gyro_offset = EMAOFFSET * gyro + (1 - EMAOFFSET) * gyro_offset;
    gyro_speed = gyro - gyro_offset;
    gyro_angle += gyro_speed * interval_time;
}

/**
 * Update data of the motors
 */
static void update_motor_data() {
    static int32_t prev_motor_cnt_sum, motor_cnt_deltas[4];

    if(loop_count == 1) { // Reset
        motor_pos = 0;
        prev_motor_cnt_sum = 0;
        motor_cnt_deltas[0] = motor_cnt_deltas[1] = motor_cnt_deltas[2] = motor_cnt_deltas[3] = 0;
    }

    int32_t left_cnt = ev3_motor_get_counts(left_motor);
    int32_t right_cnt = ev3_motor_get_counts(right_motor);
    int32_t motor_cnt_sum = left_cnt + right_cnt;
    motor_diff = right_cnt - left_cnt; // TODO: with diff
    int32_t motor_cnt_delta = motor_cnt_sum - prev_motor_cnt_sum;

    prev_motor_cnt_sum = motor_cnt_sum;
    motor_pos += motor_cnt_delta;
    motor_cnt_deltas[loop_count % 4] = motor_cnt_delta;
    motor_speed = (motor_cnt_deltas[0] + motor_cnt_deltas[1] + motor_cnt_deltas[2] + motor_cnt_deltas[3]) / 4.0f / interval_time;
}

float calculate_battery_gain() {
    const int kMaxBattery = 8500;
    const int kMinBattery = 6500;
    
    int batt = ev3_battery_voltage_mV();
    return 0.7 + ((1.12 - 0.7) / (kMaxBattery - kMinBattery)) * (kMaxBattery - batt);
}

/**
 * Control the power to keep balance.
 * Return false when the robot has fallen.
 */
static bool_t keep_balance() {
    static SYSTIM ok_time;

    if(loop_count == 1) // Reset ok_time
        get_tim(&ok_time);

    float ratio_wheel = WHEEL_DIAMETER / 5.6;

    // Apply the drive control value to the motor position to get robot to move.
    motor_pos -= motor_control_drive * interval_time;

    // This is the main balancing equation
    int power = (int)(((KGYROSPEED * gyro_speed +                // Deg/Sec from Gyro sensor
                        KGYROANGLE * gyro_angle) / ratio_wheel + // Deg from integral of gyro
                        KPOS       * motor_pos +                 // From MotorRotationCount of both motors
                        KSPEED     * motor_speed  +              // Motor speed in Deg/Sec
                        KDRIVE     * motor_control_drive)        // To improve start/stop performance
                      * calculate_battery_gain());               // To have a more reliable motor output across diff battery voltages

    // Check fallen
    SYSTIM time;
    get_tim(&time);
    if(power > -100 && power < 100)
        ok_time = time;
    else if(time - ok_time >= FALL_TIME_MS)
        return false;

    // Steering control
    motor_diff_target += motor_control_steer * interval_time;

    int left_power, right_power;

    // TODO: support steering and motor_control_drive
    int power_steer = (int)(KSTEER * (motor_diff_target - motor_diff));
    left_power = power + power_steer;
    right_power = power - power_steer;
    if(left_power > 100)
        left_power = 100;
    if(left_power < -100)
        left_power = -100;
    if(right_power > 100)
        right_power = 100;
    if(right_power < -100)
        right_power = -100;

    ev3_motor_set_power(left_motor, (int)left_power);
    ev3_motor_set_power(right_motor, (int)right_power);

    return true;
}

void balance_task(intptr_t unused) {
    ER ercd;

    /**
     * Reset
     */
    loop_count = 0;
    motor_control_drive = 0;
    ev3_motor_reset_counts(left_motor);
    ev3_motor_reset_counts(right_motor);
    //TODO: reset the gyro sensor
    ev3_gyro_sensor_reset(gyro_sensor);

    gyrohunter_status = CALIB_STATUS;
    
    /**
     * Calibrate the gyro sensor and set the led to green if succeeded.
     */
    _debug(syslog(LOG_NOTICE, "Start calibration of the gyro sensor."));
    for(int i = 10; i > 0; --i) { // Max retries: 10 times.
        ercd = calibrate_gyro_sensor();
        if(ercd == E_OK) break;
        if(i != 1) {
            syslog(LOG_ERROR, "Calibration failed, retry.");
            ev3_led_set_color(LED_ORANGE);
            tslp_tsk(100);
        }
        else {
            syslog(LOG_ERROR, "Max retries for calibration exceeded, exit.");
            ev3_led_set_color(LED_RED);
            gyrohunter_status = KNOCK_OUT_STATUS;
            return;
        }
    }
    _debug(syslog(LOG_INFO, "Calibration succeed, offset is %de-3.", (int)(gyro_offset * 1000)));
    gyro_angle = INIT_GYROANGLE;
    ev3_led_set_color(LED_GREEN);

    gyrohunter_status = RUNNING_STATUS;
    
    /**
     * Main loop for the self-balance control algorithm
     */
    while(1) {
        // Update the interval time
        update_interval_time();

        // Update data of the gyro sensor
        update_gyro_data();

        // Update data of the motors
        update_motor_data();

        // Keep balance
        if(!keep_balance()) {
            ev3_motor_stop(left_motor, false);
            ev3_motor_stop(right_motor, false);
            ev3_led_set_color(LED_RED); // TODO: knock out
            syslog(LOG_NOTICE, "Knock out!");
            gyrohunter_status = KNOCK_OUT_STATUS;
            return;
        }

        tslp_tsk(WAIT_TIME_MS);
    }
}

static void button_clicked_handler(intptr_t button) {
    switch(button) {
    case BACK_BUTTON:
        syslog(LOG_NOTICE, "Back button clicked.");
        break;
    case LEFT_BUTTON:
        syslog(LOG_NOTICE, "Left button clicked.");
        break;
    case ENTER_BUTTON:
        syslog(LOG_NOTICE, "Enter button clicked.");
        if (gyrohunter_status == KNOCK_OUT_STATUS) {
            syslog(LOG_NOTICE, "Restarting balance task.");
            act_tsk(BALANCE_TASK);
        }
        break;
    }
}

//static FILE *bt = NULL;

void idle_task(intptr_t unused) {
    while(1) {
        //fprintf(bt, "Press 'h' for usage instructions.\n");
        tslp_tsk(1000);
    }
}

// KGYROANGLE = 7.5f;   .1
// KGYROSPEED = 1.15f;  .01
// KPOS       = 0.07f;  .005
// KSPEED     = 0.1f;   .01
void update_kparameters() {
    const float KGYROANGLE_INC = .1;
    const float KGYROSPEED_INC = .01;
    const float KPOS_INC = .005;
    const float KSPEED_INC = .01;
    
    const int k1_chn = 1;
    const int k2_chn = 2;

    static SYSTIM last_ir_time = 0;
    ER ercd;
    
    SYSTIM now;
    ercd = get_tim(&now);
    assert(ercd == E_OK);
    if (now - last_ir_time < 250) return; // don't overflow with lots of cmds
    
    ir_remote_t val = ev3_infrared_sensor_get_remote(ir_sensor);
    if (val.channel[k1_chn] & IR_RED_UP_BUTTON   ) { // inc KGYROANGLE
        KGYROANGLE += KGYROANGLE_INC;
    }
    if (val.channel[k1_chn] & IR_RED_DOWN_BUTTON ) { // dec KGYROANGLE
        KGYROANGLE -= KGYROANGLE_INC;
    }
    if (val.channel[k1_chn] & IR_BLUE_UP_BUTTON  ) { // inc KGYROSPEED
        KGYROSPEED += KGYROSPEED_INC;
    }
    if (val.channel[k1_chn] & IR_BLUE_DOWN_BUTTON) { // dec KGYROSPEED
        KGYROSPEED -= KGYROSPEED_INC;
    }
    if (val.channel[k2_chn] & IR_RED_UP_BUTTON   ) { // inc KPOS
        KPOS += KPOS_INC;
    }
    if (val.channel[k2_chn] & IR_RED_DOWN_BUTTON ) { // dec KPOS
        KPOS -= KPOS_INC;
    }
    if (val.channel[k2_chn] & IR_BLUE_UP_BUTTON  ) { // inc KSPEED
        KSPEED += KSPEED_INC;
    }
    if (val.channel[k2_chn] & IR_BLUE_DOWN_BUTTON) { // dec KSPEED
        KSPEED -= KSPEED_INC;
    }
    
    if (last_ir_time == 0 ||
        val.channel[k1_chn] || val.channel[k2_chn]) {
        ercd = get_tim(&last_ir_time);
        assert(ercd == E_OK);
    
        char lcdstr[100];
        sprintf(lcdstr, "GYANG: %1.3f", KGYROANGLE);
        print(1, lcdstr);
        sprintf(lcdstr, "GYSPD: %1.4f", KGYROSPEED);
        print(2, lcdstr);
        sprintf(lcdstr, "KPOS : %1.5f", KPOS);
        print(3, lcdstr);
        sprintf(lcdstr, "KSPD : %1.4f", KSPEED);
        print(4, lcdstr);
    }
}

uint8_t get_ir_control() {
    static SYSTIM last_ir_time = 0;
    const int control_chn = 0;
    const int gun_chn = 3;
    ER ercd;
    
    if (last_ir_time == 0) {
        ercd = get_tim(&last_ir_time);
        assert(ercd == E_OK);
    } 
    else {
        SYSTIM now;
        ercd = get_tim(&now);
        assert(ercd == E_OK);
        if (now - last_ir_time < 100) return 1; // don't overflow with lots of cmds
    }
    
    uint8_t result = 0;
    
    ir_remote_t val = ev3_infrared_sensor_get_remote(ir_sensor);
    if      (val.channel[gun_chn] & (IR_BLUE_UP_BUTTON   | IR_RED_UP_BUTTON))   result = 'f'; // fire up!
    else if (val.channel[gun_chn] & (IR_BLUE_DOWN_BUTTON | IR_RED_DOWN_BUTTON)) result = 'g'; // fire straight!
    else if (val.channel[control_chn] & IR_BLUE_UP_BUTTON  ) 
        if      (val.channel[control_chn] & IR_RED_UP_BUTTON   ) result = 'q'; // left forward
        else if (val.channel[control_chn] & IR_RED_DOWN_BUTTON ) result = 'e'; // right forward
        else                                                     result = 'w'; // forward
    else if (val.channel[control_chn] & IR_BLUE_DOWN_BUTTON)
        if      (val.channel[control_chn] & IR_RED_UP_BUTTON   ) result = 'z'; // left backward
        else if (val.channel[control_chn] & IR_RED_DOWN_BUTTON ) result = 'c'; // right backward
        else                                                     result = 's'; // backward
    else if (val.channel[control_chn] & IR_RED_UP_BUTTON   ) result = 'a'; // left
    else if (val.channel[control_chn] & IR_RED_DOWN_BUTTON ) result = 'd'; // right
    
    if (result) {
        ercd = get_tim(&last_ir_time);
        assert(ercd == E_OK);
    }

    return result;
}

#define MAX_SPEED 600
#define MAX_STEER 170
#define SPEED_INC 50
#define STEER_INC 85

void main_task(intptr_t unused) {
    static SYSTIM last_gun_time = 0;
    
#ifndef USE_FACES
    // Draw information
    lcdfont_t font = EV3_FONT_MEDIUM;
    ev3_lcd_set_font(font);
    int32_t fontw, fonth;
    ev3_font_get_size(font, &fontw, &fonth);
    char lcdstr[100];
    ev3_lcd_draw_string("App: Gyrohunter", 0, 0);
    sprintf(lcdstr, "Port%c:Gyro sensor", '1' + gyro_sensor);
    ev3_lcd_draw_string(lcdstr, 0, fonth);
    sprintf(lcdstr, "Port%c:IR sensor", '1' + ir_sensor);
    ev3_lcd_draw_string(lcdstr, 0, fonth * 2);
    sprintf(lcdstr, "Port%c:Left motor", 'A' + left_motor);
    ev3_lcd_draw_string(lcdstr, 0, fonth * 3);
    sprintf(lcdstr, "Port%c:Right motor", 'A' + right_motor);
    ev3_lcd_draw_string(lcdstr, 0, fonth * 4);
    sprintf(lcdstr, "Port%c:Gun motor", 'A' + gun_motor);
    ev3_lcd_draw_string(lcdstr, 0, fonth * 5);
#else
    load_eyes_images();
    draw_eyes(EV3EYE_SLEEPING);
#endif
    // Register button handlers
    ev3_button_set_on_clicked(BACK_BUTTON, button_clicked_handler, BACK_BUTTON);
    ev3_button_set_on_clicked(ENTER_BUTTON, button_clicked_handler, ENTER_BUTTON);
    ev3_button_set_on_clicked(LEFT_BUTTON, button_clicked_handler, LEFT_BUTTON);

    // Configure sensors
    ev3_sensor_config(gyro_sensor, GYRO_SENSOR);
    ev3_sensor_config(ir_sensor, INFRARED_SENSOR);

    // Configure motors
    ev3_motor_config(left_motor, LARGE_MOTOR);
    ev3_motor_config(right_motor, LARGE_MOTOR);
    ev3_motor_config(gun_motor, MEDIUM_MOTOR);

    // Start task for self-balancing
    act_tsk(BALANCE_TASK);

    // Open Bluetooth file
    //bt = ev3_serial_open_file(EV3_SERIAL_BT);
    //assert(bt != NULL);

    // Start task for printing message while idle
    act_tsk(IDLE_TASK);
    
    tslp_tsk(1000);
    clearScreen();
#ifndef USE_FACES
    print(0, "App: Gyrohunter");
#else
    draw_eyes(EV3EYE_TIRED_MIDDLE);
#endif
    // wait for the gyro calibration to finish
#ifndef USE_FACES
    print(1, "Calibrating");
#endif
    while(gyrohunter_status != RUNNING_STATUS) {
        tslp_tsk(200);
    }

#ifdef USE_FACES
    draw_eyes(EV3EYE_AWAKE);
#endif
    
    // go forward a bit
    tslp_tsk(1000);
    motor_control_drive = 100;
    tslp_tsk(2000);
    motor_control_drive = 0;

    while(1) {
#ifndef USE_FACES
        update_kparameters();
#else
        if (gyrohunter_status == KNOCK_OUT_STATUS)
        {
            DRAW_EYES(EV3EYE_DIZZY);
            if (ev3_button_is_pressed(ENTER_BUTTON)) {
                waitButtonRelease(ENTER_BUTTON);
                act_tsk(BALANCE_TASK);
                while (gyrohunter_status != RUNNING_STATUS) {
                    tslp_tsk(50);
                }
            }
            tslp_tsk(1000);
            continue;
        }
#endif
        
        char* status = "IDL";
        //while (!ev3_bluetooth_is_connected()) tslp_tsk(100);
        //uint8_t c = fgetc(bt);
        uint8_t c = get_ir_control();
        sus_tsk(IDLE_TASK);
        switch(c) {
        case 0:
            tslp_tsk(10);
            //ev3_lcd_draw_string("IDL", 0, fonth * 5);
            motor_control_drive = 0;
            motor_control_steer = 0;
            status = "IDL";
            DRAW_EYES_AFTER_MS(EV3EYE_AWAKE, 1200);
            break;

        case 1:
            tslp_tsk(10);
            status = "IDL";
            DRAW_EYES_AFTER_MS(EV3EYE_AWAKE, 1200);
            break;

        case 'f':
        case 'g':
            {
                SYSTIM now;
                ER ercd = get_tim(&now);
                assert(ercd == E_OK);
                if (now - last_gun_time > 2000) {
                    DRAW_EYES(EV3EYE_EVIL);
                    ev3_motor_reset_counts(gun_motor);
                    if (c == 'f')
                        ev3_motor_rotate(gun_motor, FIRE_TURNS*360, 100, false);
                    else
                        ev3_motor_rotate(gun_motor, -FIRE_TURNS*360, 100, false);
                    status = "GUN";
                    get_tim(&last_gun_time);
                }
            }
            break;

        case 'w': // forward
            DRAW_EYES(EV3EYE_NEUTRAL);
            if (motor_control_drive < 0)
                motor_control_drive = 0;
            else if (motor_control_drive < MAX_SPEED)
                motor_control_drive += SPEED_INC;
            motor_control_steer = 0;
            status = "FWD";
            break;

        case 's': // backward
            DRAW_EYES(EV3EYE_NEUTRAL);
            if (motor_control_drive > 0)
                motor_control_drive = 0;
            else if (motor_control_drive > -MAX_SPEED)
                motor_control_drive -= SPEED_INC;
            motor_control_steer = 0;
            status = "BCK";
            break;

        case 'a': // left
            DRAW_EYES(EV3EYE_MIDDLE_LEFT);
            if (motor_control_steer < 0)
                motor_control_steer = 0;
            else if (motor_diff >= 0 && motor_control_steer < MAX_STEER)
                motor_control_steer += STEER_INC;
            else if (motor_diff < 0 && motor_control_steer < MAX_STEER/2)
                motor_control_steer += STEER_INC;
            motor_control_drive = 0;
            status = "LFT";
            break;

        case 'd': // right
            DRAW_EYES(EV3EYE_MIDDLE_RIGHT);
            if (motor_control_steer > 0)
                motor_control_steer = 0;
            else if (motor_diff <= 0 && motor_control_steer > -MAX_STEER)
                motor_control_steer -= STEER_INC;
            else if (motor_diff > 0 && motor_control_steer > -MAX_STEER/2)
                motor_control_steer -= STEER_INC;
            motor_control_drive = 0;
            status = "RGT";
            break;

        case 'q': // left forward
            DRAW_EYES(EV3EYE_MIDDLE_LEFT);
            if (motor_control_steer < 0)
                motor_control_steer = 0;
            else if (motor_diff >= 0 && motor_control_steer < MAX_STEER)
                motor_control_steer += STEER_INC;
            else if (motor_diff < 0 && motor_control_steer < MAX_STEER/2)
                motor_control_steer += STEER_INC;
            if (motor_control_drive < 0)
                motor_control_drive = 0;
            else if (motor_control_drive < MAX_SPEED)
                motor_control_drive += SPEED_INC;
            status = "LFW";
            break;

        case 'e': // right forward
            DRAW_EYES(EV3EYE_MIDDLE_RIGHT);
            if (motor_control_steer > 0)
                motor_control_steer = 0;
            else if (motor_diff <= 0 && motor_control_steer > -MAX_STEER)
                motor_control_steer -= STEER_INC;
            else if (motor_diff > 0 && motor_control_steer > -MAX_STEER/2)
                motor_control_steer -= STEER_INC;
            if (motor_control_drive < 0)
                motor_control_drive = 0;
            else if (motor_control_drive < MAX_SPEED)
                motor_control_drive += SPEED_INC;
            status = "RFW";
            break;

        case 'z': // left backward
            DRAW_EYES(EV3EYE_MIDDLE_LEFT);
            if (motor_control_steer < 0)
                motor_control_steer = 0;
            else if (motor_diff >= 0 && motor_control_steer < MAX_STEER)
                motor_control_steer += STEER_INC;
            else if (motor_diff < 0 && motor_control_steer < MAX_STEER/2)
                motor_control_steer += STEER_INC;
            if (motor_control_drive > 0)
                motor_control_drive = 0;
            else if (motor_control_drive > -MAX_SPEED)
                motor_control_drive -= 50;
            status = "LBK";
            break;

        case 'c': // right backward
            DRAW_EYES(EV3EYE_MIDDLE_RIGHT);
            if (motor_control_steer > 0)
                motor_control_steer = 0;
            else if (motor_diff <= 0 && motor_control_steer > -MAX_STEER)
                motor_control_steer -= STEER_INC;
            else if (motor_diff > 0 && motor_control_steer > -MAX_STEER/2)
                motor_control_steer -= STEER_INC;
            if(motor_control_drive > 0)
                motor_control_drive = 0;
            else if (motor_control_drive > -MAX_SPEED)
                motor_control_drive -= SPEED_INC;
            status = "RBK";
            break;

        case 'h':
            //fprintf(bt, "==========================\n");
            //fprintf(bt, "Usage:\n");
            //fprintf(bt, "Press 'w' to speed up\n");
            //fprintf(bt, "Press 's' to speed down\n");
            //fprintf(bt, "Press 'a' to turn left\n");
            //fprintf(bt, "Press 'd' to turn right\n");
            //fprintf(bt, "Press 'i' for idle task\n");
            //fprintf(bt, "Press 'h' for this message\n");
            //fprintf(bt, "==========================\n");
            tslp_tsk(10);
            break;

        case 'i':
            //fprintf(bt, "Idle task started.\n");
            rsm_tsk(IDLE_TASK);
            break;

        default:
            //fprintf(bt, "Unknown key '%c' pressed.\n", c);
            tslp_tsk(10);
        }
        
#ifndef USE_FACES
        sprintf(lcdstr, "%s D:%d S:%d", status, motor_control_drive, motor_control_steer);
        print(5, lcdstr);
        sprintf(lcdstr, "%d mV", ev3_battery_voltage_mV());
        print(6, lcdstr);
        //sprintf(lcdstr, "%d %d %d", motor_diff, motor_diff_target, (motor_diff_target - motor_diff));
        //print(7, lcdstr);
#endif
    }
}
