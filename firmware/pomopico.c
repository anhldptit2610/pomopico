#include <stdio.h>
#include "pico/stdlib.h"
#include "display.h"
#include "clock.h"

#define LED_PIN 25



int main(void)
{
    stdio_init_all();
    display_init();
    clock_init();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    while (1) {
        clock_run();
        // gpio_put(LED_PIN, 1);
        // sleep_ms(500);
        // gpio_put(LED_PIN, 0);
        // sleep_ms(500);
    }
}