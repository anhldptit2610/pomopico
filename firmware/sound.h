#ifndef SOUND_H
#define SOUND_H

#define SOUND_PIN       2

typedef enum {
    SOUND_PACMAN,
    SOUND_BEEP_TIME,
    SOUND_BEEP_ALARM,
} sound_list_t;

uint32_t sound_set_freq_duty(uint32_t f, int d);
void sound_play_music(sound_list_t sound, bool delay);
void sound_init(void);
void disable_pwm(void);

#endif