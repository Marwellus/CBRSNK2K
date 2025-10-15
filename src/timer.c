#include <dos.h>
#include <bios.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <i86.h>
#include "inc/types.h"
#include "inc/miditmr.h"
#include "inc/timer.h"

// timer configuration
#define TIMER_FREQUENCY 960
#define DIV_LSB 0xDB // 0xB6 (480hz)
#define DIV_MSB 0x04 // 0x09 (480hz)
#define DOS_CHAIN_MASK 52 // 480/52 ≈ 18.46 Hz

// global game counters
volatile uint t_snake_tick = 0;
volatile uint t_game_tick  = 0; 
volatile uint t_sound_tick = 0;
volatile uint t_frame_tick = 0;
volatile uint t_measure_tick = 0;

volatile float t_midi_tick = 0;
extern float t_midi_speed = MIDI_DEFAULT_SPEED;

// Internal timer state
static void __interrupt (*old_timer_handler)(void);
static byte dos_chain_counter = 0;
static byte timer_installed = 0;

static void game_timer_shutdown(void);
static uint game_timer_to_ticks(uint ms);
static uint game_timer_get_frequency(void);

static void __interrupt game_timer_handler(void) {
	t_measure_tick++;
	
	t_game_tick++;
	t_snake_tick++;
	t_sound_tick++;
	t_frame_tick++;
	t_midi_tick += t_midi_speed; // adjust general speed

	// chain to original DOS timer to maintain system clock
	dos_chain_counter++;
	if (dos_chain_counter > DOS_CHAIN_MASK) {
		dos_chain_counter = 0;
		_chain_intr(old_timer_handler); // call original DOS handler
	} else {		
		outp(0x20, 0x20); // acknowledge interrupt
	}
}

int Game_Timer_Init(void) {
	if (timer_installed) return 1;
	old_timer_handler = _dos_getvect(0x08);

   Game_Timer_Reset();
	dos_chain_counter = 0;

	_disable();

	_dos_setvect(0x08, game_timer_handler);

	outp(0x43, 0x36);
	outp(0x40, DIV_LSB);
	outp(0x40, DIV_MSB);

	_enable();

	timer_installed = 1;

	atexit(game_timer_shutdown);
	return 1;
}

/* resets all t_* vars */
void Game_Timer_Reset() {
	t_snake_tick  	= 0;
	t_game_tick 	= 0;
	t_sound_tick 	= 0;
	t_midi_tick  	= 0;
	t_frame_tick 	= 0;
}

static void game_timer_shutdown(void) {
	if (!timer_installed) return;

	_disable();

	_dos_setvect(0x08, old_timer_handler);
	outp(0x43, 0x36); // Timer 0, LSB+MSB, mode 3
	outp(0x40, 0x00); // LSB = 0 (65536 = 0)
	outp(0x40, 0x00); // MSB = 0

	_enable();

	timer_installed = 0;
}

static uint game_timer_to_ticks(uint ms) {
	return (ms * TIMER_FREQUENCY) / 1000;
}

static uint game_timer_get_frequency(void) {
	uint divisor;
	byte low_byte, high_byte;

	_disable();
	outp(0x43, 0x00);
	low_byte = inp(0x40);
	high_byte = inp(0x40);
	_enable();

	divisor = (high_byte << 8) | low_byte;
	if (divisor == 0) divisor = 65536;

	return (uint)(1193180L / divisor);
}

void delay_no_midi(uint ms) {
	uint start_ticks, target_ticks;
	if (!timer_installed || ms == 0) return;

	start_ticks = t_snake_tick;
	target_ticks = game_timer_to_ticks(ms);

	while ((t_snake_tick - start_ticks) < target_ticks) {
		_asm { hlt };
	}
}

void delay(uint ms) {
	uint start_ticks, target_ticks;
	if (!timer_installed || ms == 0) return;

	start_ticks = t_snake_tick;
	target_ticks = game_timer_to_ticks(ms);

	while ((t_snake_tick - start_ticks) < target_ticks) {
		// keep MIDI updated
		if (t_midi_tick >= MIDI_Interval) {
			midi_timer_tick();
			t_midi_tick = 0;
		}
		_asm { hlt };
	}
}
