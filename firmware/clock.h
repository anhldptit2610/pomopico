#ifndef CLOCK_H
#define CLOCK_H

typedef enum {
    TIME_FORMAT_24HRS,
    TIME_FORMAT_12HRS,
} time_format_t;

typedef enum {
    OPR_MODE_UPDATE,
    OPR_MODE_NORMAL,
    OPR_MODE_PREPARE_TIME_SET,
    OPR_MODE_SET_SECONDS,
    OPR_MODE_SET_MINUTES,
    OPR_MODE_SET_HOURS,
    OPR_MODE_ALARM_SET,
    OPR_MODE_ALARM_TRIGGER,
    OPR_MODE_POMODORO_SETUP,
    OPR_MODE_POMODORO_WORK,
    OPR_MODE_POMODORO_QUIT,
    OPR_MODE_POMODORO_REST,
} clk_opr_mode_t;

typedef struct alarm {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
} alarm_t;

typedef struct clk_time {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
} clk_time_t;

typedef struct clock {
    volatile bool timeOrAlarm;   // false = time, true = alarm
    bool pomodoroEnable;
    bool upBtnPressed;
    bool setBtnPressed;
    bool alarmAck;
    int pomodoroTimer;
    bool pomodoroSwitchMode;
    bool pomodoroSetup;
    clk_time_t time;
    alarm_t alarm;
    uint8_t control;
    uint8_t status;
    uint8_t agingOffset;
    uint8_t tempMSB;
    uint8_t tempLSB;
    time_format_t timeFormat;
    clk_opr_mode_t oprMode;
} clk_t;

void clock_init(void);
void clock_run(void);
void ds1307_read_raw(struct clock *clock);

#endif