#ifndef TIMER_H
#define TIMER_H
#include "inc/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Timer counter types
typedef enum {
	COUNTER_GAME = 0,		// Main game timing
   COUNTER_MIDI = 1,		// MIDI playback timing  
   COUNTER_EFFECT = 2,	// Visual effects timing
   COUNTER_SOUND = 3		// Sound effects timing
} TimerCounter;

// Global counters
extern volatile uint t_game_tick;
extern volatile uint t_snake_tick;
extern volatile uint t_sound_tick;
extern volatile uint t_frame_tick;
extern volatile uint t_measure_tick;

extern volatile float t_midi_tick;
extern float t_midi_speed;
#define MIDI_DEFAULT_SPEED 0.97

// Convenience macros
#define MS_TO_TICKS(ms) GameTimerMsToTicks(ms)

// Common timing intervals (in ticks)
#define TICKS_PER			960
#define TICKS_10MS		(TICKS_PER / 100)		// 9.6 ticks
#define TICKS_50MS		(TICKS_PER / 20)		// 48 ticks
#define TICKS_100MS		(TICKS_PER / 10)		// 96 ticks
#define TICKS_250MS		(TICKS_PER / 4)		// 240 ticks
#define TICKS_500MS		(TICKS_PER / 2)		// 480 ticks
#define TICKS_1000MS		(TICKS_PER)				// 960 ticks

int Game_Timer_Init();
void Game_Timer_Reset();

void delay_no_midi(uint ms);
void delay(uint ms);

#ifdef __cplusplus
}
#endif

#endif
