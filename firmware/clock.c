#include "pico/stdlib.h"
#include "hardware/i2c.h"
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

void clock_init(struct clock *clock)
{
    uint8_t oscEnable[2];
    
    i2c_init(i2c_default, I2C_BAUDRATE);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);

    clock->timeFormat = TIME_FORMAT_24HRS;

    oscEnable[0] = DS1307_REG_SECONDS;
    oscEnable[1] = 0x00;
    i2c_write_blocking(i2c_default, DS1307_ADDR, (uint8_t *)oscEnable, 2, false);
}