#include <stdio.h>
#include "pico/stdlib.h"
#include "display.h"
#include "clock.h"
#include "sound.h"

int main(void)
{
    stdio_init_all();
    display_init();
    clock_init();
    sound_init();
    while (1) {
        clock_run();
    }
}