#include <stdio.h>
#include "pico/stdlib.h"
#include "display.h"
#include "clock.h"

#define LED_PIN 25

bool timer_callback(struct repeating_timer *t)
{
    struct clock *clock = (struct clock *)t->user_data;

    ds1307_read_raw(clock);
    display_show_time(clock->seconds, clock->minutes, clock->hours);
}

int main(void)
{
    struct clock clock;
    struct repeating_timer timer;

    stdio_init_all();
    display_init();
    clock_init(&clock);
    add_repeating_timer_ms(1000, timer_callback, (void *)&clock, &timer);
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    while (1) {
        gpio_put(LED_PIN, 1);
        sleep_ms(500);
        gpio_put(LED_PIN, 0);
        sleep_ms(500);
    }
}