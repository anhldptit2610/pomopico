#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "display.h"

typedef enum {
    DECODE_MODE_NONE = 0x00,
    DECODE_MODE_DIGIT_0_ONLY = 0x01,
    DECODE_MODE_FIRST_HALF = 0x0f,
    DECODE_MODE_ALL = 0xff,
} decode_mode_t;

struct font {
    uint8_t codeB;
    uint8_t onSegment;
} font[] = {
    [CHAR_0]     = {.codeB = 0b00000000, .onSegment = 0b01111110},
    [CHAR_1]     = {.codeB = 0b00000001, .onSegment = 0b00110000},
    [CHAR_2]     = {.codeB = 0b00000010, .onSegment = 0b01101101},
    [CHAR_3]     = {.codeB = 0b00000011, .onSegment = 0b01111001},
    [CHAR_4]     = {.codeB = 0b00000100, .onSegment = 0b00110011},
    [CHAR_5]     = {.codeB = 0b00000101, .onSegment = 0b01011011},
    [CHAR_6]     = {.codeB = 0b00000110, .onSegment = 0b01011111},
    [CHAR_7]     = {.codeB = 0b00000111, .onSegment = 0b01110000},
    [CHAR_8]     = {.codeB = 0b00001000, .onSegment = 0b01111111},
    [CHAR_9]     = {.codeB = 0b00001001, .onSegment = 0b01111011},
    [CHAR_CROSS] = {.codeB = 0b00001010, .onSegment = 0b00000001},
    [CHAR_E]     = {.codeB = 0b00001011, .onSegment = 0b01001111},
    [CHAR_H]     = {.codeB = 0b00001100, .onSegment = 0b00110111},
    [CHAR_L]     = {.codeB = 0b00001101, .onSegment = 0b00001110},
    [CHAR_P]     = {.codeB = 0b00001110, .onSegment = 0b01100111},
    [CHAR_BLANK] = {.codeB = 0b00001111, .onSegment = 0b00000000}
};

decode_mode_t decodeMode = DECODE_MODE_NONE;
int scanLimit = 1;

#define TO_U16(lsb, msb)            (((uint16_t)(msb) << 8) | (uint16_t)(lsb))
#define SPI_BAUDRATE                10 * 1000 * 1000
#define LED_COUNT                   4
#define MAX7219_DIGIT_0             0x01
#define MAX7219_DIGIT_1             0x02
#define MAX7219_DIGIT_2             0x03
#define MAX7219_DIGIT_3             0x04
#define MAX7219_DECODE_MODE         0x09
#define MAX7219_INTENSITY           0x0a
#define MAX7219_SCAN_LIMIT          0x0b
#define MAX7219_SHUTDOWN            0x0c
#define MAX7219_DISPLAY_TEST        0x0f 

void display_send_cmd(uint8_t addr, uint8_t data)
{
    uint16_t cmd = TO_U16(data, addr);

    spi_write16_blocking(spi_default, (uint16_t *)&cmd, 1);
}

void display_show_number(uint8_t pos, uint8_t num, bool dp)
{
    uint8_t data;

    if (decodeMode == DECODE_MODE_NONE)
        data = font[num].onSegment | ((uint8_t)dp << 7);
    else if (decodeMode == DECODE_MODE_ALL)
        data = font[num].codeB | ((uint8_t)dp << 7);
    display_send_cmd(pos + 1, data);
}

void display_enter_test_mode(void)
{
    display_send_cmd(MAX7219_DISPLAY_TEST, 0x01);
}

void display_set_decode_mode(decode_mode_t decodeMode)
{
    decodeMode = decodeMode;
    display_send_cmd(MAX7219_DECODE_MODE, decodeMode);
}

void display_set_scan_limit(int num)
{
    scanLimit = num;
    display_send_cmd(MAX7219_SCAN_LIMIT, scanLimit - 1);
}

void display_init(void)
{
    spi_init(spi_default, SPI_BAUDRATE);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_CSN_PIN, GPIO_FUNC_SPI);
    spi_set_format(spi_default, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    display_set_scan_limit(LED_COUNT);
    display_send_cmd(MAX7219_INTENSITY, 0x0f);
    display_set_decode_mode(DECODE_MODE_NONE);
    display_send_cmd(MAX7219_DISPLAY_TEST, 0x00);
    display_send_cmd(MAX7219_SHUTDOWN, 0x01);
}