#ifndef CLOCK_H
#define CLOCK_H

typedef enum {
    TIME_FORMAT_24HRS,
    TIME_FORMAT_12HRS,
} time_format_t;

struct clock {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    time_format_t timeFormat;
};

void clock_init(struct clock *clock);
void ds1307_read_raw(struct clock *clock);

#endif