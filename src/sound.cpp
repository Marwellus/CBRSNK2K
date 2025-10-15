#include <dos.h>
#include <mem.h>
#include <i86.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include "inc/types.h"
#include "inc/logger.h"
#include "inc/opl.h"
#include "inc/dsp.h"
#include "inc/music.hpp"
#include "inc/sound.hpp"

bool Sound::_oplReady  = false;
bool Sound::_dspReady  = false;
bool Sound::_midiReady = false;

int Sound::_activeChannel[MAX_CHANNELS] = { -1 };
AudioEvent Sound::_audioQueue[MAX_AUDIO_EVENTS] = { 0 };

/* init dsp, midi & opl */
void Sound::Init(int hw_type, bool use_midi) {
	if (DSP_Test()) { _dspReady = true; DSP_Set_Volume(90); }
	if (use_midi) _midiReady = Music::Init(hw_type);
	if (hw_type == USE_GUS) return; // no OPL for ya (yet)
   if (Opl3_Init()) _oplReady  = true; else return;

   for (word chan = 0; chan < 18; chan++) {
      _activeChannel[chan] = -1;
   }

   for (word i = 0; i < MAX_AUDIO_EVENTS; i++) { 
      _audioQueue[i].active = false;
      memset(_audioQueue[i].frequencies, 0, PLAY_LIMIT*2); // word array
      memset(_audioQueue[i].durations, 0, PLAY_LIMIT*2);   // word array
   }
}

void Sound::Dispose() {
	DSP_Dispose(); 
   Music::Dispose();
	Opl3_Shutdown();
}

/* OPL sounds */

void Sound::HitWallBeep(uint tick) {
   if (!_oplReady) return;

   word bass_freqs[] = {100, 95, 90, 85, 80, 75, 70, 65};
   word bass_durations[] = {30, 30, 30, 40, 40, 50, 50, 70};
   queueSequence(bass_freqs, bass_durations, 9, tick);

   word crack_freqs[] = {440,220};
   word crack_durations[] = {30,40};
   queueSequence(crack_freqs, crack_durations, 2, tick);
}

void Sound::OuchBeep(uint tick) {
   if (!_oplReady) return;

	word freqs[] = {1200, 800, 400, 200, 600, 1500};
   word durations[] = {30,30,30,30,30,30};
   queueSequence(freqs, durations, 6, tick);
}

void Sound::HealthyBeep(uint tick) {
   if (!_oplReady) return;

	word freqs[] = {500, 700, 900, 1100};
   word durations[] = {30, 30, 30, 30};
   queueSequence(freqs, durations, 4, tick);
}

void Sound::GoldyBeep(uint tick) {
   if (!_oplReady) return;
	
	word freqs[] = {500, 700, 900, 1100, 700, 900, 1100, 700, 900, 1100};
   word durations[] = {30, 30, 30, 30, 30, 30, 30, 30, 30, 30};
   queueSequence(freqs, durations, 10, tick);
}

void Sound::Burp(uint tick) {
	if (!_oplReady) return;
	
   word freqs[] = {250, 350, 450, 550};
   word durations[] = {30,30,30,30};
   queueSequence(freqs, durations, 4, tick);
}

void Sound::SickBeep(uint tick) {
	if (!_oplReady) return;
	
	word freqs[] = {500, 1100, 700, 1100, 900};
   word durations[] = {30,30,30,30,30};
   queueSequence(freqs, durations, 5, tick);
}

void Sound::Beep(word freq, word ms, uint tick) {
	if (!_oplReady) return;
	
   word freqs[1] = { freq };
   word durations[1] = { ms };
   queueSequence(freqs, durations, 1, tick);   
}

void Sound::Booep(word from, word to, uint tick) {
	if (!_oplReady) return;
	
   word n = 0;
   word freqs[PLAY_LIMIT] = {0};
   word durs[PLAY_LIMIT] = {0};

   if (from < to)
      for (word freq = from; freq <= to; freq += 10) {
         freqs[n] = freq; durs[n] = 5;
         n += n < PLAY_LIMIT ? 1 : 0;
      }
   else
      for (word freq = from; freq >= to; freq -= 10) {
         freqs[n] = freq; durs[n] = 5;
         n += n < PLAY_LIMIT ? 1 : 0;
      }
   
	queueSequence(freqs, durs, n, tick);
}

/* process ingame sounds (interrupt controlled) */

void Sound::ProcessQueue(uint current_tick) {
   AudioEvent* event;
   word idx;
   uint elapsed;

   for (word chan = 0; chan < 18; chan++) {
      if (_activeChannel[chan] >= 0) {
         idx = _activeChannel[chan];
         event = &_audioQueue[idx];

         if (event->next_tick <= current_tick) {
            if  (event->played < event->count) {
               if (event->frequencies[event->played] > 0) {
                  Opl3_Note_On(event->frequencies[event->played], event->channel);               
               } else {
                  Opl3_Note_Off(chan);
               }
               event->next_tick = current_tick + event->durations[event->played];
               event->played++;
            } else {
               Opl3_Note_Off(chan);
               _activeChannel[chan] = -1;
               event->active = false;
            }
         }
      }
   }
}

void Sound::ClearQueue() {
   for (int chan = 0; chan < 18; chan++) {
      _activeChannel[chan] = -1;
      Opl3_Note_Off(chan);
   }

   for (int i = 0; i < MAX_AUDIO_EVENTS; i++) {
      _audioQueue[i].active = false;
      memset(_audioQueue[i].frequencies, 0, PLAY_LIMIT);
      memset(_audioQueue[i].durations, 0, PLAY_LIMIT);
   }
}

void Sound::queueSequence(word freqs[], word durations[], word count, uint current_tick) {
   if (count > PLAY_LIMIT) count = PLAY_LIMIT;

   int channel = -1; word n = 3;
   while (channel == -1 && n < 18) {
      if (_activeChannel[n] < 0 ) { channel = n; }
      n++;
   }
   if (channel == -1) { Log_Info("Snd: no free channel found!"); return; };

   for (word i = 0; i < MAX_AUDIO_EVENTS; i++) {
      if (!_audioQueue[i].active) {
         _audioQueue[i].next_tick 	= current_tick;
         _audioQueue[i].channel 		= channel;
         _audioQueue[i].count 		= count;
         _audioQueue[i].played 		= 0;
         _audioQueue[i].active 		= true;
         _activeChannel[channel] 	= i;

         for (word n = 0; n < count; n++) {
            _audioQueue[i].frequencies[n] = freqs[n];
            _audioQueue[i].durations[n]   = durations[n];
         }

         break;
      }      
   }
}

void Sound::Siren(word freq, bool turn_off) {
	if (!_oplReady) return;

	if (!turn_off) 
		Opl3_Note_On(freq, 1);
	else 
		Opl3_Note_Off(1);
}

/* sounds used pre-/after game */

void Sound::Booep(word from, word to) {
	if (!_oplReady) return;

   if (from < to )
      for (word freq = from; freq <= to; freq += 10) {
         Opl3_Note_On(freq, 0);
         delay(5);
      }
   else
      for (word freq = from; freq >= to; freq -= 10) {
         Opl3_Note_On(freq, 0);
         delay(5);
      }
   
	Opl3_Note_Off(0);
}

void Sound::HitWallBeep() {
	if (!_oplReady) return;

	Opl3_Note_On(440, 0); delay(20);
	Opl3_Note_On(220, 0); delay(30);
	Opl3_Note_Off(0);
}

void Sound::OuchBeep() {
	if (!_oplReady) return;

	word freq[] = {1200, 800, 400, 200, 600, 1500};
	for (word i = 0; i < 6; i++) {
		Opl3_Note_On(freq[i], 0); delay(20);
   	Opl3_Note_Off(0);
	}
}

void Sound::Beep(word freq, word ms) {
	if (!_oplReady) return;

	Opl3_Note_On(freq, 0); delay(ms);
	Opl3_Note_Off(0);
}

/* debug/testing */

void Sound::TestBeep() {
	if (!_oplReady) return;
   Opl3_Test();
}

