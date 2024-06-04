#ifndef SOUND_H
#define SOUND_H

#define SOUND_PIN       2

uint32_t sound_set_freq_duty(uint32_t f, int d);
void sound_play_music(void);
void sound_init(void);

#endif