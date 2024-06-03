#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "display.h"
#include "clock.h"

#define I2C_BAUDRATE            100 * 1000
#define DS1307_ADDR             0x68
#define DS1307_REG_SECONDS      0x00
#define DS1307_REG_MINUTES      0x01
#define DS1307_REG_HOURS        0x02
#define DS1307_REG_DAY          0x03
#define DS1307_REG_DATE         0x04
#define DS1307_REG_MONTH        0x05
#define DS1307_REG_YEAR         0x06
#define DS1307_REG_CONTROL      0x07
#define DS1307_RAM_START        0x08
#define DS1307_RAM_END          0x3f

#define BUTTON_SET 14
#define BUTTON_UP 15

struct repeating_timer timer;
clk_t clock;
volatile bool upBtnPressed;

clk_t *get_clock_inst(void)
{
    return (clk_t *)&clock;
}

void ds1307_update_time(struct clock *clock)
{
    uint8_t seconds = 0x00, minutes = 0x00, hours = 0x00;
    uint8_t cmd[2];
    

    cmd[0] = DS1307_REG_MINUTES;
    cmd[1] = 0x00 | clock->minutes % 10;
    cmd[1] |= (clock->minutes / 10) << 4;
    i2c_write_blocking(i2c_default, DS1307_ADDR, (uint8_t *)cmd, 2, false);

    cmd[0] = DS1307_REG_HOURS;
    cmd[1] = 0x00 | clock->hours % 10;
    cmd[1] |= (clock->hours / 10) << 4;
    i2c_write_blocking(i2c_default, DS1307_ADDR, (uint8_t *)cmd, 2, false);

    cmd[0] = DS1307_REG_SECONDS;
    cmd[1] = 0x00 | clock->seconds % 10;
    cmd[1] |= (clock->seconds / 10) << 4;
    i2c_write_blocking(i2c_default, DS1307_ADDR, (uint8_t *)cmd, 2, false);
}

void ds1307_read_raw(struct clock *clock)
{
    uint8_t startReg = DS1307_REG_SECONDS, rawData[7];
    i2c_write_blocking(i2c_default, DS1307_ADDR, (uint8_t *)&startReg, 1, true);
    i2c_read_blocking(i2c_default, DS1307_ADDR, rawData, 7, false);

    clock->seconds = ((rawData[0] >> 4) & 0x07) * 10 + (rawData[0] & 0x0f);
    clock->minutes = ((rawData[1] >> 4) & 0x07) * 10 + (rawData[1] & 0x0f);
    if (clock->timeFormat == TIME_FORMAT_24HRS)
        clock->hours = ((rawData[2] >> 4) & 0x03) * 10 + (rawData[2] & 0x0f);
    else if (clock->timeFormat == TIME_FORMAT_12HRS)
        clock->hours = ((rawData[2] >> 4) & 0x01) * 10 + (rawData[2] & 0x0f);
}

bool timer_regular_callback(struct repeating_timer *t)
{
    struct clock *clock = (struct clock *)t->user_data;

    if (clock->oprMode == OPR_MODE_NORMAL)
        ds1307_read_raw(clock);
    display_show_time(clock->seconds, clock->minutes, clock->hours);
    return true;
}

int64_t button_polling_callback(alarm_id_t id, void *user_data)
{
    struct clock *clock = (struct clock *)user_data;

    if (!gpio_get(BUTTON_SET)) {
        clock->oprMode = OPR_MODE_SET_SECONDS;
        display_show_time(clock->seconds, clock->minutes, clock->hours);

        uint8_t cmd[2];
        cmd[0] = DS1307_REG_SECONDS;
        cmd[1] = 1U << 7;
        i2c_write_blocking(i2c_default, DS1307_ADDR, (uint8_t *)cmd, 2, false);
    }
    return 0;
}

void button_irq(uint gpio, uint32_t events) 
{
    if (gpio == BUTTON_SET) {
        if (clock.oprMode == OPR_MODE_NORMAL)
            add_alarm_in_ms(3000, button_polling_callback, (void *)&clock, false);
        else if (clock.oprMode == OPR_MODE_SET_SECONDS)
            clock.oprMode = OPR_MODE_SET_MINUTES;
        else if (clock.oprMode == OPR_MODE_SET_MINUTES)
            clock.oprMode = OPR_MODE_SET_HOURS;
        else if (clock.oprMode == OPR_MODE_SET_HOURS)
            clock.oprMode = OPR_MODE_UPDATE;
    } else if (gpio == BUTTON_UP) {
        if (((clock.oprMode == OPR_MODE_SET_SECONDS) || (clock.oprMode == OPR_MODE_SET_MINUTES)
            || (clock.oprMode == OPR_MODE_SET_HOURS)) && (upBtnPressed == false)) {
            upBtnPressed = true;
        }
    }
}

void clock_run(void)
{
    switch (clock.oprMode) {
    case OPR_MODE_NORMAL:
        break;
    case OPR_MODE_SET_SECONDS:
        if (upBtnPressed) {
            upBtnPressed = false;
            clock.seconds = (clock.seconds + 1) % 60;
        }
        break;
    case OPR_MODE_SET_MINUTES:
        if (upBtnPressed) {
            upBtnPressed = false;
            clock.minutes = (clock.minutes + 1) % 60;
        }
        break;
    case OPR_MODE_SET_HOURS:
        if (upBtnPressed) {
            upBtnPressed = false;
            clock.hours = (clock.hours + 1) % 60;
        }
        break;
    case OPR_MODE_UPDATE:
        ds1307_update_time(&clock);
        clock.oprMode = OPR_MODE_NORMAL;
        break;
    default:
        break;
    }
}

void clock_init(void)
{
    uint8_t oscEnable[2];
    
    i2c_init(i2c_default, I2C_BAUDRATE);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    add_repeating_timer_ms(500, timer_regular_callback, (void *)&clock, &timer);

    clock.timeFormat = TIME_FORMAT_24HRS;
    clock.oprMode = OPR_MODE_NORMAL;
    upBtnPressed = false;

    gpio_init(BUTTON_SET);
    gpio_set_dir(BUTTON_SET, GPIO_IN);
    gpio_set_irq_enabled_with_callback(BUTTON_SET, GPIO_IRQ_EDGE_FALL, true, button_irq);
    gpio_init(BUTTON_UP);
    gpio_set_dir(BUTTON_UP, GPIO_IN);
    gpio_set_irq_enabled_with_callback(BUTTON_UP, GPIO_IRQ_EDGE_FALL, true, button_irq);
}