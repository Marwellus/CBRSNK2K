#include <dos.h>
#include <stdio.h>
#include "inc/types.h"
#include "inc/logger.h"
#include "inc/midi.h"
#include "inc/miditmr.h"

volatile uint MIDI_Interval = 0;

// Timer callback management
static void (*midi_timer_callback)(void) = NULL;
static uint midi_timer_interval = 0;
static uint midi_timer_counter = 0;

void midi_timer_tick(void) {
   if (!midi_timer_callback) return;
   midi_timer_callback();
   MIDI_Interval = midi_timer_interval;
}

void install_int(void (*func)(void), int speed) {
   midi_timer_callback = func;
   midi_timer_interval = speed;
   midi_timer_counter = 0;
}

void install_int_ex(void (*func)(void), long speed) {
   midi_timer_callback = func;
   midi_timer_interval = speed;
   midi_timer_counter = 0;
}

void remove_int(void (*func)(void)) {
   midi_timer_callback = NULL;
   midi_timer_interval = 0;
   midi_timer_counter = 0;
}
