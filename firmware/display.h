#ifndef DISPLAY_H
#define DISPLAY_H

typedef enum {
    CHAR_0,
    CHAR_1,
    CHAR_2,
    CHAR_3,
    CHAR_4,
    CHAR_5,
    CHAR_6,
    CHAR_7,
    CHAR_8,
    CHAR_9,
    CHAR_CROSS,
    CHAR_E,
    CHAR_H,
    CHAR_L,
    CHAR_P,
    CHAR_BLANK,
} characterFont;

void display_init(void);
void display_enter_test_mode(void);
void display_show_number(uint8_t pos, uint8_t num, bool dp);
void display_show_time(uint8_t seconds, uint8_t minutes, uint8_t hours);

#endif