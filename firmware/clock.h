#ifndef CLOCK_H
#define CLOCK_H

typedef enum {
    TIME_FORMAT_24HRS,
    TIME_FORMAT_12HRS,
} time_format_t;

typedef enum {
    OPR_MODE_UPDATE,
    OPR_MODE_NORMAL,
    OPR_MODE_SET_SECONDS,
    OPR_MODE_SET_MINUTES,
    OPR_MODE_SET_HOURS,
} clk_opr_mode_t;

typedef struct clock {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    time_format_t timeFormat;
    clk_opr_mode_t oprMode;
} clk_t;

void clock_init(void);
void clock_run(void);
void ds1307_read_raw(struct clock *clock);

#endif