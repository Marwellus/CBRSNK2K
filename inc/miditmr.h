#ifndef MIDITMR_H
#define MIDITMR_H

#ifdef __cplusplus
extern "C" {
#endif

#define TIMERS_PER_SECOND     960 // Allegro: 1193181L
#define MIDI_TIMER_FREQUENCY  60
#define BPS_TO_TIMER(x)       (TIMERS_PER_SECOND / (long)(x))

extern volatile unsigned int MIDI_Interval;

void midi_timer_tick(void);
void install_int(void (*func)(void), int speed);
void install_int_ex(void (*func)(void), long speed);
void remove_int(void (*func)(void));

#ifdef __cplusplus
}
#endif

#endif
