#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "display.h"
#include "clock.h"
#include "sound.h"

#define I2C_BAUDRATE            100 * 1000
#define DS3231_ADDR             0x68
#define DS3231_REG_SECONDS      0x00
#define DS3231_REG_MINUTES      0x01
#define DS3231_REG_HOURS        0x02
#define DS3231_REG_DAY          0x03
#define DS3231_REG_DATE         0x04
#define DS3231_REG_MONTH        0x05
#define DS3231_REG_YEAR         0x06
#define DS3231_REG_A1M1         0x07
#define DS3231_REG_A1M2         0x08
#define DS3231_REG_A1M3         0x09
#define DS3231_REG_A1M4         0x0a
#define DS3231_REG_A2M2         0x0b
#define DS3231_REG_A2M3         0x0c
#define DS3231_REG_A2M4         0x0d
#define DS3231_REG_CONTROL      0x0e
#define DS3231_REG_STATUS       0x0f
#define DS3231_REG_AGING_OFFSET 0x10
#define DS3231_REG_MSB_TEMP     0x11
#define DS3231_REG_LSB_TEMP     0x12

#define BUTTON_SET              14
#define BUTTON_UP               15
#define ALARM_PIN               13 
#define POMODORO_BTN            12
#define POMODORO_TIMER_MS       60000

struct repeating_timer RTClock, RTPomodoro;
clk_t clock;

clk_t *get_clock_inst(void)
{
    return (clk_t *)&clock;
}

void ds3231_update_alarm(struct clock *clock)
{
    uint8_t cmd[2];

    cmd[0] = DS3231_REG_A1M1;
    cmd[1] = 0x00 | (clock->alarm.seconds % 10) | ((clock->alarm.seconds / 10) << 4);
    i2c_write_blocking(i2c_default, DS3231_ADDR, (uint8_t *)cmd, 2, false);

    cmd[0] = DS3231_REG_A1M2;
    cmd[1] = 0x00 | (clock->alarm.minutes % 10) | ((clock->alarm.minutes / 10) << 4);
    i2c_write_blocking(i2c_default, DS3231_ADDR, (uint8_t *)cmd, 2, false);

    cmd[0] = DS3231_REG_A1M3;
    cmd[1] = 0x00 | (clock->alarm.hours % 10) | ((clock->alarm.hours / 10) << 4);
    i2c_write_blocking(i2c_default, DS3231_ADDR, (uint8_t *)cmd, 2, false);

    cmd[0] = DS3231_REG_A1M4;
    cmd[1] = 0x00 | (1U << 7);
    i2c_write_blocking(i2c_default, DS3231_ADDR, (uint8_t *)cmd, 2, false);
}

void ds3231_update_time(struct clock *clock)
{
    uint8_t seconds = 0x00, minutes = 0x00, hours = 0x00;
    uint8_t cmd[2];
    

    cmd[0] = DS3231_REG_MINUTES;
    cmd[1] = 0x00 | clock->time.minutes % 10;
    cmd[1] |= (clock->time.minutes / 10) << 4;
    i2c_write_blocking(i2c_default, DS3231_ADDR, (uint8_t *)cmd, 2, false);

    cmd[0] = DS3231_REG_HOURS;
    cmd[1] = 0x00 | clock->time.hours % 10;
    cmd[1] |= (clock->time.hours / 10) << 4;
    i2c_write_blocking(i2c_default, DS3231_ADDR, (uint8_t *)cmd, 2, false);

    cmd[0] = DS3231_REG_SECONDS;
    cmd[1] = 0x00 | clock->time.seconds % 10;
    cmd[1] |= (clock->time.seconds / 10) << 4;
    i2c_write_blocking(i2c_default, DS3231_ADDR, (uint8_t *)cmd, 2, false);

    cmd[0] = DS3231_REG_CONTROL;
    cmd[1] = clock->control & ~(1U << 7);
    i2c_write_blocking(i2c_default, DS3231_ADDR, (uint8_t *)cmd, 2, false);
}

void ds3231_read_raw(struct clock *clock)
{
    uint8_t startReg = DS3231_REG_SECONDS, rawData[19];
    i2c_write_blocking(i2c_default, DS3231_ADDR, (uint8_t *)&startReg, 1, true);
    i2c_read_blocking(i2c_default, DS3231_ADDR, rawData, 19, false);

    clock->time.seconds = ((rawData[0] >> 4) & 0x07) * 10 + (rawData[0] & 0x0f);
    clock->time.minutes = ((rawData[1] >> 4) & 0x07) * 10 + (rawData[1] & 0x0f);
    if (clock->timeFormat == TIME_FORMAT_24HRS)
        clock->time.hours = ((rawData[2] >> 4) & 0x03) * 10 + (rawData[2] & 0x0f);
    else if (clock->timeFormat == TIME_FORMAT_12HRS)
        clock->time.hours = ((rawData[2] >> 4) & 0x01) * 10 + (rawData[2] & 0x0f);
    clock->control = rawData[0x0e];
    clock->status = rawData[0x0f];
    clock->agingOffset = rawData[0x10];
    clock->tempMSB = rawData[0x11];
    clock->tempLSB = rawData[0x12];
}

bool pomodoro_callback(struct repeating_timer *t)
{
    struct clock *clock = (struct clock *)t->user_data;

    printf("pomodoro callback! pomodoro: %d\n", clock->pomodoroTimer);
    if (clock->pomodoroTimer > 0)
        clock->pomodoroTimer--;
    display_show_time(0, clock->pomodoroTimer, 0);
    return true;
}

bool timer_regular_callback(struct repeating_timer *t)
{
    struct clock *clock = (struct clock *)t->user_data;

    if (clock->oprMode != OPR_MODE_POMODORO_SETUP && clock->oprMode != OPR_MODE_POMODORO_WORK && clock->oprMode != OPR_MODE_POMODORO_REST) {
        if (((clock->oprMode == OPR_MODE_NORMAL) || (clock->oprMode == OPR_MODE_ALARM_TRIGGER))  && !clock->timeOrAlarm) {
            ds3231_read_raw(clock);
        }
        if (!clock->timeOrAlarm)
            display_show_time(clock->time.seconds, clock->time.minutes, clock->time.hours);
        else if (clock->timeOrAlarm)
            display_show_time(clock->alarm.seconds, clock->alarm.minutes, clock->alarm.hours);
    }
    return true;
}

int64_t btn_set_callback(alarm_id_t id, void *user_data)
{
    struct clock *clock = (struct clock *)user_data;

    if (!gpio_get(BUTTON_SET)) {
        clock->oprMode = OPR_MODE_PREPARE_TIME_SET;
        clock->timeOrAlarm = false;

        /* make timekeeper data static; DS3231's oscillator can't stop
           once powered */
        uint8_t cmd[2];
        cmd[0] = DS3231_REG_CONTROL;
        cmd[1] = clock->control | (1U << 7);
        i2c_write_blocking(i2c_default, DS3231_ADDR, (uint8_t *)cmd, 2, false);
    }
    return 0;
}

int64_t btn_up_callback(alarm_id_t id, void *user_data)
{
    struct clock *clock = (struct clock *)user_data;
    
    if (!gpio_get(BUTTON_UP)) {
        clock->oprMode = OPR_MODE_ALARM_SET;
        clock->timeOrAlarm = true;
    }
}

void gpio_irq(uint gpio, uint32_t events) 
{
    uint8_t cmd[2];

    switch (gpio) {
    case BUTTON_SET:
        clock.setBtnPressed = true;
        if (clock.oprMode == OPR_MODE_NORMAL)
            add_alarm_in_ms(3000, btn_set_callback, (void *)&clock, false);
        else if ((clock.oprMode == OPR_MODE_SET_SECONDS) && (clock.timeOrAlarm == false))
            clock.oprMode = OPR_MODE_SET_MINUTES;
        else if ((clock.oprMode == OPR_MODE_SET_MINUTES) && (clock.timeOrAlarm == false))
            clock.oprMode = OPR_MODE_SET_HOURS;
        else if ((clock.oprMode == OPR_MODE_SET_HOURS) && (clock.timeOrAlarm == false))
            clock.oprMode = OPR_MODE_UPDATE;
        else if (clock.oprMode == OPR_MODE_ALARM_TRIGGER) {
            clock.alarmAck = true;
            disable_pwm();
        } else if (clock.oprMode == OPR_MODE_POMODORO_WORK || clock.oprMode == OPR_MODE_POMODORO_REST) {
            clock.oprMode = OPR_MODE_POMODORO_QUIT;
        }
        break;
    case BUTTON_UP:
        clock.upBtnPressed = true;
        if (clock.oprMode == OPR_MODE_NORMAL)
            add_alarm_in_ms(3000, btn_up_callback, (void *)&clock, false);
        else if ((clock.oprMode == OPR_MODE_SET_SECONDS) && clock.timeOrAlarm)
            clock.oprMode = OPR_MODE_SET_MINUTES;
        else if ((clock.oprMode == OPR_MODE_SET_MINUTES) && clock.timeOrAlarm)
            clock.oprMode = OPR_MODE_SET_HOURS;
        else if ((clock.oprMode == OPR_MODE_SET_HOURS) && clock.timeOrAlarm)
            clock.oprMode = OPR_MODE_UPDATE;
        break;
    case ALARM_PIN:
        clock.oprMode = OPR_MODE_ALARM_TRIGGER;
        break;
    case POMODORO_BTN:
        if (!clock.pomodoroSetup) {
            clock.pomodoroSetup = true;
            clock.oprMode = OPR_MODE_POMODORO_SETUP;
        } else if (!clock.pomodoroTimer && !clock.pomodoroSwitchMode)
            clock.pomodoroSwitchMode = true;
        break;
    default:
        break;
    }
    // if (gpio == BUTTON_SET) {
    //     clock.setBtnPressed = true;
    //     if (clock.oprMode == OPR_MODE_NORMAL)
    //         add_alarm_in_ms(3000, btn_set_callback, (void *)&clock, false);
    //     else if ((clock.oprMode == OPR_MODE_SET_SECONDS) && (clock.timeOrAlarm == false))
    //         clock.oprMode = OPR_MODE_SET_MINUTES;
    //     else if ((clock.oprMode == OPR_MODE_SET_MINUTES) && (clock.timeOrAlarm == false))
    //         clock.oprMode = OPR_MODE_SET_HOURS;
    //     else if ((clock.oprMode == OPR_MODE_SET_HOURS) && (clock.timeOrAlarm == false))
    //         clock.oprMode = OPR_MODE_UPDATE;
    //     else if (clock.oprMode == OPR_MODE_ALARM_TRIGGER) {
    //         clock.alarmAck = true;
    //         disable_pwm();
    //     } else if (clock.oprMode == OPR_MODE_POMODORO_WORK || clock.oprMode == OPR_MODE_POMODORO_REST) {
    //         clock.oprMode = OPR_MODE_POMODORO_QUIT;
    //     }
    // } else if (gpio == BUTTON_UP) {
    //     clock.upBtnPressed = true;
    //     if (clock.oprMode == OPR_MODE_NORMAL) {
    //         add_alarm_in_ms(3000, btn_up_callback, (void *)&clock, false);
    //     } else if ((clock.oprMode == OPR_MODE_SET_SECONDS) && clock.timeOrAlarm) {
    //         clock.oprMode = OPR_MODE_SET_MINUTES;
    //     } else if ((clock.oprMode == OPR_MODE_SET_MINUTES) && clock.timeOrAlarm) {
    //         clock.oprMode = OPR_MODE_SET_HOURS;
    //     } else if ((clock.oprMode == OPR_MODE_SET_HOURS) && clock.timeOrAlarm) {
    //         clock.oprMode = OPR_MODE_UPDATE;
    //     }
    // } else if (gpio == ALARM_PIN) {
    //     clock.oprMode = OPR_MODE_ALARM_TRIGGER;
    // } else if (gpio == POMODORO_BTN) {
    //     if (clock.oprMode != OPR_MODE_POMODORO_WORK)
    //         clock.oprMode = OPR_MODE_POMODORO_SETUP;
    //     else if (!clock.pomodoroTimer && !clock.pomodoroSwitchMode)
    //         clock.pomodoroSwitchMode = true;
    // }
}

void clock_run(void)
{
    static bool playSound = false;
    static bool pomodoroNoti = false;
    static uint8_t cmd[2];

    switch (clock.oprMode) {
    case OPR_MODE_NORMAL:
        if (clock.time.seconds == 0 && clock.time.minutes == 0 && !playSound) {
            playSound = true;
            sound_play_music(SOUND_PACMAN, false);
            playSound = false;
        }
        break;
    case OPR_MODE_PREPARE_TIME_SET:
        clock.oprMode = OPR_MODE_SET_SECONDS;
        sound_play_music(SOUND_BEEP_TIME, false);
        break;
    case OPR_MODE_SET_SECONDS:
        if (clock.timeOrAlarm) {
            clock.alarm.seconds = (clock.setBtnPressed) ? (clock.alarm.seconds + 1) % 60 : clock.alarm.seconds;
            clock.setBtnPressed = (clock.setBtnPressed) ? false : clock.setBtnPressed;
        } else if (clock.timeOrAlarm == false) {
            clock.time.seconds = (clock.upBtnPressed) ? (clock.time.seconds + 1) % 60 : clock.time.seconds;
            if (clock.upBtnPressed)
                clock.upBtnPressed = false;
        }    
        break;
    case OPR_MODE_SET_MINUTES:
        if (clock.timeOrAlarm) {
            clock.alarm.minutes = (clock.setBtnPressed) ? (clock.alarm.minutes + 1) % 60 : clock.alarm.minutes;
            clock.setBtnPressed = (clock.setBtnPressed) ? false : clock.setBtnPressed;
        } else if (clock.timeOrAlarm == false) {
            clock.time.minutes = (clock.upBtnPressed) ? (clock.time.minutes + 1) % 60 : clock.time.minutes;
            if (clock.upBtnPressed)
                clock.upBtnPressed = false;
        }
        break;
    case OPR_MODE_SET_HOURS:
        if (clock.timeOrAlarm) {
            clock.alarm.hours = (clock.setBtnPressed) ? (clock.alarm.hours + 1) % 24 : clock.alarm.hours;
            clock.setBtnPressed = (clock.setBtnPressed) ? false : clock.setBtnPressed;
        } else if (clock.timeOrAlarm == false) {
            clock.time.hours = (clock.upBtnPressed) ? (clock.time.hours + 1) % 24 : clock.time.hours;
            if (clock.upBtnPressed)
                clock.upBtnPressed = false;
        }
        break;
    case OPR_MODE_UPDATE:
        if (clock.timeOrAlarm)
            ds3231_update_alarm(&clock);
        else if (!clock.timeOrAlarm)
            ds3231_update_time(&clock);
        clock.timeOrAlarm = false;
        clock.oprMode = OPR_MODE_NORMAL;
        sound_play_music(SOUND_BEEP_TIME, false);
        break;
    case OPR_MODE_ALARM_SET:
        clock.timeOrAlarm = true;
        clock.alarm.seconds = clock.time.seconds;
        clock.alarm.minutes = clock.time.minutes;
        clock.alarm.hours = clock.time.hours;
        clock.oprMode = OPR_MODE_SET_SECONDS;
        sound_play_music(SOUND_BEEP_ALARM, false);
        break;
    case OPR_MODE_ALARM_TRIGGER:
        sound_play_music(SOUND_PACMAN, true);
        if (clock.alarmAck) {
            cmd[0] = DS3231_REG_STATUS;
            cmd[1] = clock.status & ~(1U << 0);
            i2c_write_blocking(i2c_default, DS3231_ADDR, (uint8_t *)cmd, 2, false);
            clock.oprMode = OPR_MODE_NORMAL;
            clock.alarmAck = false;
        }
        break;
    case OPR_MODE_POMODORO_SETUP:
        clock.oprMode = OPR_MODE_POMODORO_WORK;
        clock.pomodoroTimer = 25;
        display_show_time(0, clock.pomodoroTimer, 0);
        sound_play_music(SOUND_BEEP_ALARM, false);
        add_repeating_timer_ms(POMODORO_TIMER_MS, pomodoro_callback, (void *)&clock, &RTPomodoro);
        break;
    case OPR_MODE_POMODORO_WORK:
        if (!clock.pomodoroTimer && !pomodoroNoti) {
            pomodoroNoti = true;
            sound_play_music(SOUND_PACMAN, false);
            cancel_repeating_timer(&RTPomodoro);
        }
        if (clock.pomodoroSwitchMode) {
            clock.pomodoroSwitchMode = false;
            pomodoroNoti = false;
            clock.pomodoroTimer = 5;
            clock.oprMode = OPR_MODE_POMODORO_REST;
            sound_play_music(SOUND_BEEP_ALARM, false);
            display_show_time(0, clock.pomodoroTimer, 0);
            add_repeating_timer_ms(POMODORO_TIMER_MS, pomodoro_callback, (void *)&clock, &RTPomodoro);
        }
        break;
    case OPR_MODE_POMODORO_REST:
        if (!clock.pomodoroTimer && !pomodoroNoti) {
            pomodoroNoti = true;
            sound_play_music(SOUND_PACMAN, false);
            cancel_repeating_timer(&RTPomodoro);
        }
        if (clock.pomodoroSwitchMode) {
            clock.pomodoroSwitchMode = false;
            pomodoroNoti = false;
            clock.pomodoroTimer = 25;
            clock.oprMode = OPR_MODE_POMODORO_WORK;
            display_show_time(0, clock.pomodoroTimer, 0);
            sound_play_music(SOUND_BEEP_ALARM, false);
            add_repeating_timer_ms(POMODORO_TIMER_MS, pomodoro_callback, (void *)&clock, &RTPomodoro);
        }
        break;
    case OPR_MODE_POMODORO_QUIT:
        clock.oprMode = OPR_MODE_NORMAL;
        sound_play_music(SOUND_BEEP_ALARM, false);
        // acknowledge any alarm
        cmd[0] = DS3231_REG_STATUS;
        cmd[1] = clock.status & ~(1U << 0);
        i2c_write_blocking(i2c_default, DS3231_ADDR, (uint8_t *)cmd, 2, false);
        break;
    default:
        break;
    }
}

void clock_init(void)
{
    uint8_t cmd[2];
    
    i2c_init(i2c_default, I2C_BAUDRATE);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    clock.oprMode = OPR_MODE_NORMAL;

    clock.timeFormat = TIME_FORMAT_24HRS;
    clock.upBtnPressed = false;
    clock.timeOrAlarm = false;
    clock.alarmAck = false;
    clock.pomodoroEnable = false;
    clock.pomodoroSwitchMode = false;
    clock.pomodoroSetup = false;

    ds3231_read_raw(&clock);

    // enable alarm 1
    cmd[0] = DS3231_REG_CONTROL;
    cmd[1] = clock.control | (1U << 2) | (1U << 0);
    i2c_write_blocking(i2c_default, DS3231_ADDR, (uint8_t *)cmd, 2, false);

    // acknowledge alarm
    cmd[0] = DS3231_REG_STATUS;
    cmd[1] = clock.status & ~(1U << 0);
    i2c_write_blocking(i2c_default, DS3231_ADDR, (uint8_t *)cmd, 2, false);


    gpio_init(BUTTON_SET);
    gpio_set_dir(BUTTON_SET, GPIO_IN);
    gpio_set_irq_enabled_with_callback(BUTTON_SET, GPIO_IRQ_EDGE_FALL, true, gpio_irq);
    gpio_init(BUTTON_UP);
    gpio_set_dir(BUTTON_UP, GPIO_IN);
    gpio_set_irq_enabled_with_callback(BUTTON_UP, GPIO_IRQ_EDGE_FALL, true, gpio_irq);

    gpio_init(ALARM_PIN);
    gpio_set_dir(ALARM_PIN, GPIO_IN);
    gpio_set_irq_enabled_with_callback(ALARM_PIN, GPIO_IRQ_EDGE_FALL, true, gpio_irq);

    gpio_init(POMODORO_BTN);
    gpio_set_dir(POMODORO_BTN, GPIO_IN);
    gpio_set_irq_enabled_with_callback(POMODORO_BTN, GPIO_IRQ_EDGE_FALL, true, gpio_irq);
    
    add_repeating_timer_ms(500, timer_regular_callback, (void *)&clock, &RTClock);
}