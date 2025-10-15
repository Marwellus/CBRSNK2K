#include <dos.h>
#include <mem.h>
#include <i86.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include "inc/types.h"
#include "inc/logger.h"
#include "inc/opl.h"

bool Opl3_Initialized = false;

// operator to channel map (YMF262 / YMF715x configuration)
static byte op_map[] = { 0, 1, 2, 8, 9, 10, 16, 17, 18 };

// OPL3 detection and init
static void opl3_write(word reg, byte value) {
	int i;
	outp(OPL3_REGISTER, reg);
	for (i = 0; i < 6; i++) inp(OPL3_STATUS);
	outp(OPL3_DATA, value);
	for (i = 0; i < 35; i++) inp(OPL3_STATUS);
}

static bool opl3_detect() {
	unsigned char status1, status2;

	// Reset timers
	opl3_write(OPL3_TIMER_CTRL, 0x60);
	opl3_write(OPL3_TIMER_CTRL, 0x80);

	status1 = inp(OPL3_STATUS);

	opl3_write(OPL3_TIMER1, 0xFF);
	opl3_write(OPL3_TIMER_CTRL, 0x21);

	delay(1); // Wait a bit

	status2 = inp(OPL3_STATUS);

	opl3_write(OPL3_TIMER_CTRL, 0x60);
	opl3_write(OPL3_TIMER_CTRL, 0x80);

	// Check if status changed correctly
	return ((status1 & 0xE0) == 0x00) && ((status2 & 0xE0) == 0xC0);
}

bool Opl3_Init() {
   int offset, idx; word chan, i = 0;
	if (Opl3_Initialized) return true;
	if (!opl3_detect())   return false;

   // reset
   for (i = 0; i < 256; i++) opl3_write(i, 0);
	// enable OPL3 mode
	opl3_write(OPL3_MODE_REG, 0x01);
   opl3_write(0x04, 0x00);

   // set up all 18 chanels, simple sine wave
   for (chan = 0; chan < 18; chan++) {
      // channel-specific register offsets
      offset = (chan >= 9) ? 0x100 : 0x00;
      idx = chan % 9;

      opl3_write(0x20 + offset + op_map[idx], 0x01); // modulator multiple = 1
      opl3_write(0x23 + offset + op_map[idx], 0x01); // carrier multiple = 1  
      opl3_write(0x40 + offset + op_map[idx], 0x10); // mod volume
      opl3_write(0x43 + offset + op_map[idx], 0x00); // car volume (max)
      opl3_write(0x60 + offset + op_map[idx], 0xC0); // mod Attack/Decay
      opl3_write(0x63 + offset + op_map[idx], 0xC0); // car Attack/Decay
      opl3_write(0x80 + offset + op_map[idx], 0x10); // mod Sustain/Release
      opl3_write(0x83 + offset + op_map[idx], 0x10); // car Sustain/Release      
      opl3_write(0xE0 + offset + op_map[idx], 0x03); // wave-form (pulse-sine)
      opl3_write(0xC0 + offset + idx, 0x0A); // feedback/Connection

      delay(1); // Wait a bit
   }

	Opl3_Initialized = true;
   return true;
}

void Opl3_Note_On(int freq, int channel) {
   uint fnum; int offset, chan, block = 4;
   if (!Opl3_Initialized) { sound(freq); return; }

   // calculate frequency and block
   while (freq > 1023) { freq >>= 1; block++; }
   while (freq < 256 && block > 0) { freq <<= 1; block--; }
   fnum = freq;

   // channel-specific register offsets
   offset = (channel >= 9) ? 0x100 : 0x00;
   chan = channel % 9;
   
   // set frequency (register 0xA0 + channel)
   opl3_write(0xA0 + offset + chan, fnum & 0xFF);
   // key on + block + high frequency bits (register 0xB0 + channel)  
   opl3_write(0xB0 + offset + chan, 0x20 | (block << 2) | ((fnum >> 8) & 0x03));
}

void Opl3_Note_Off(int channel) {
   int offset, chan;
   if (!Opl3_Initialized) { nosound(); return; }

   offset = (channel >= 9) ? 0x100 : 0x00;
   chan = channel % 9;
   // key off
   opl3_write(0xA0 + offset + chan, 0x00);
   opl3_write(0xB0 + offset + chan, 0x00);
}

void Opl3_Shutdown() {
   int offset, chan, ch, reg;
	if (!Opl3_Initialized) return;

   // key off every channel
	for (ch = 0; ch < 18; ch++) {
      offset = (ch >= 9) ? 0x100 : 0x00;
      chan = ch % 9;
      opl3_write(0xA0 + offset + chan, 0x00);
		opl3_write(0xB0 + offset + chan, 0x00);
	}

	// reset
	for (reg = 0; reg < 256; reg++) {
		opl3_write(reg, 0x00);
	}

	Opl3_Initialized = false;
}

void Opl3_Test() {
   word chan, i; 
   word freqs[] = {500, 700, 900, 1100};

   for (chan = 0; chan < 18; chan++) {
      printf("Snd: TEST channel %d, map 0x%02X\n", chan, op_map[chan%9]);
      for (i = 0; i < 4; i++) {
         Opl3_Note_On(freqs[i], chan); delay(20);
         Opl3_Note_Off(chan);
      }
      getch();
   }
}
