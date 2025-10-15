#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include "inc/types.h"
#include "inc/logger.h"
#include "awe/ctaweapi.h"
#include "awe/awe32drv.h"

// =============================================================================
// AWE32 MIDI DRIVER
// =============================================================================

static AWE32_VOICE awe32_voices[32];
static int awe32_initialized = 0;

/* SoundFont variables */
SOUND_PACKET sp_sound      = {0};
char* presets[MAXBANKS]    = {0};
long bank_sizes[MAXBANKS]  = {0};
char packet[PACKETSIZE]    = {0};

// driver struct
MIDI_DRIVER awe32_midi_driver = {
	0x415745,		// 'AWE' ID
	"AWE32",		  	// name
	"Creative AWE32", // description
	"awe32",		  	// ascii name
	32,				// voices
	0,				  	// basevoice
	32,				// max_voices
	16,				// def_voices
	-1, -1,			// xmin, xmax (no reserved range)

	awe32_detect, 	// detect
	awe32_init,	  	// init
	awe32_exit,	  	// exit
	NULL,		  		// set_mixer_volume (optional)
	NULL,		  		// get_mixer_volume (optional)

	// MIDI functions
	awe32_raw_midi,		// raw_midi only is used
	awe32_load_patches, 	// load_patches
	NULL,						// adjust_patches (optional)
	NULL,						// key_on
	NULL,						// key_off
	NULL,						// set_volume
	NULL						// set_pitch
};

// =============================================================================
// IMPLEMENTATION DER DRIVER-FUNKTIONEN
// =============================================================================

MIDI_DRIVER* get_awe32_midi_driver(void) {
    return &awe32_midi_driver;
}

bool awe32_detect(int input) {
	if (awe32Detect(0x620)) 
		return false;
	return true;
}

bool awe32_init() {
	int i;
	if (awe32_initialized) return true;

	// AWE32 init
	if (awe32InitHardware()) {
		return false;
	}
   
   delay(5);
   if (!awe32_load_sfb("assets\\4MBGM.SF2")) {
      Log_Info("AWE: Failed to load SoundFont, loading fallback");
      awe32_load_sfb("assets\\synthgm.sf2");
   }

	// MIDI start
	awe32InitMIDI();

	// voice table init
	for (i = 0; i < 32; i++) {
		awe32_voices[i].channel = -1;
		awe32_voices[i].note = -1;
		awe32_voices[i].instrument = -1;
		awe32_voices[i].active = false;
	}

	awe32_initialized = true;
	return true;
}

int awe32_load_sfb(char* filepath) {
   FILE* fp; int i; int file_size;
   fp = fopen(filepath, "rb");
   if (!fp) {
	   Log_Info("AWE: Cannot open %s\n", filepath);
	   return FALSE;
   }

   fseek(fp, 0, SEEK_END);
   file_size = ftell(fp);
   fseek(fp, 0, SEEK_SET);
   
   sp_sound.bank_no = 0;
   sp_sound.total_banks = 1;
   sp_sound.data = packet;
   
   if (awe32TotalPatchRam(&sp_sound) != 0) {
      Log_Info("AWE: Failed to get patch RAM size\n");
      fclose(fp);
      return FALSE;
   }
   
   Log_Info("AWE: Available patch RAM: %ld KB\n", sp_sound.total_patch_ram / 1024);
   
   bank_sizes[0] = sp_sound.total_patch_ram;
   sp_sound.banksizes = bank_sizes;
   
   if (awe32DefineBankSizes(&sp_sound) != 0) {
      Log_Info("AWE: Failed to define bank sizes\n");
      fclose(fp);
      return FALSE;
   }

   fread(packet, 1, PACKETSIZE, fp);
   sp_sound.data = packet;
   
   if (awe32SFontLoadRequest(&sp_sound) != 0) {
      fclose(fp);
      Log_Info("AWE: SFLoadRequest failed - size: (%ld bytes)\n", file_size);
      return FALSE;
   }

   // stream samples
   fseek(fp, sp_sound.sample_seek, SEEK_SET);
   for (i = 0; i < sp_sound.no_sample_packets; i++) {
	   fread(packet, 1, PACKETSIZE, fp);
	   awe32StreamSample(&sp_sound);
   }

   // setup SoundFont preset objects
   fseek(fp, sp_sound.preset_seek, SEEK_SET);
   presets[0] = (char *)malloc((unsigned)sp_sound.preset_read_size);
   fread(presets[0], 1, (unsigned)sp_sound.preset_read_size, fp);   
   sp_sound.presets = presets[0];

   if (awe32SetPresets(&sp_sound)) {
	   fclose(fp);
	   Log_Info("AWE: Invalid SoundFont file %s\n", filepath);
	   return FALSE;
   }
   
   fclose(fp);

   // calculate actual ram used
   if (sp_sound.no_sample_packets) {
	   bank_sizes[0] = sp_sound.preset_seek - sp_sound.sample_seek + 160;
	   sp_sound.total_patch_ram -= bank_sizes[0];
   } else
	   bank_sizes[0] = 0; // no sample in file

   Log_Info("AWE: Loaded SF: %s", filepath);
   return TRUE;
}

void awe32_exit(void) {
	int c; int i;
	if (!awe32_initialized) return;

	// stop all notes
	for (c = 0; c < 16; c++) {
		awe32Controller(c, 123, 0);
	}

   /* free allocated memory */
   awe32ReleaseAllBanks(&sp_sound);
   for (i = 0; i < sp_sound.total_banks; i++) {
      if (presets[i]) free(presets[i]);
   }
	awe32Terminate();
	awe32_initialized = false;
}

int awe32_load_patches(char *patches, char *drums) {
	// currently a complete SoundFont is loaded on init
	// optional: patches, load additional SoundFonts
	// (... insert code here ...)
	return true; // success
}

void process_midi_command(int command, int channel, int data1, int data2) {
	int command_type = (command >> 4) & 0x0F;
	switch (command_type) {
	case 0x08: // Note Off
		awe32NoteOff(channel, data1, data2);
		break;
	case 0x09: // Note On
		if (data2 > 0) {
			awe32NoteOn(channel, data1, data2);
		} else {
			awe32NoteOff(channel, data1, 64);
		}
		break;
	case 0x0B: // Control Change
		awe32Controller(channel, data1, data2);
		break;
	case 0x0C: // Program Change
		awe32ProgramChange(channel, data1);
		break;
	case 0x0E: // Pitch Bend
		// AWE32 expects pitch bend as: low 7 bits, high 7 bits
		awe32PitchBend(channel, data1, data2);
		break;
	default:
		// Ignore other commands for now
		break;
	}
}

void awe32_raw_midi(int data) {
	int command_type = 0;
	static int midi_command = 0;
	static int midi_channel = 0;
	static int midi_data1 = 0;
	static int midi_data2 = 0;
	static int midi_state = 0; // 0=await command, 1=await data1, 2=await data2

	if (data & 0x80) {
		// Status byte (command)
		midi_command = data;
		midi_channel = data & 0x0F;
		midi_state = 1; // waiting for first data byte
		return;
	}

	// Data byte
	if (midi_state == 1) {
		midi_data1 = data;

		// Check if this command needs a second data byte
		command_type = (midi_command >> 4) & 0x0F;
		if (command_type == 0x0C || command_type == 0x0D) {
			// Program Change or Channel Pressure - only needs 1 data byte
			process_midi_command(midi_command, midi_channel, midi_data1, 0);
			midi_state = 0;
		} else {
			// Most commands need 2 data bytes
			midi_state = 2;
		}
	} else if (midi_state == 2) {
		midi_data2 = data;
		process_midi_command(midi_command, midi_channel, midi_data1, midi_data2);
		midi_state = 0;
	}
}

/*
void _awe32_key_on(int inst, int note, int bend, int vol, int pan) {
	int channel = 0;
   // Log_Info("key_on: inst=%d, note=%d, vol=%d", inst, note, vol);
    
   channel = (inst >= 128) ? 9 : 0; 
    
   if (inst < 128) {
   	awe32ProgramChange(channel, inst);
	}
    
   awe32NoteOn(channel, note, vol);
}

void awe32_key_on(int inst, int note, int bend, int vol, int pan) {
	// inst = instrument (0-127 = GM instruments, 128+ = drums)
	// note = MIDI note (0-127)
	// bend = pitch bend
	// vol = velocity (0-127)
	// pan = pan position (0-127)

	int channel;

	// drum sounds, channel 9
	if (inst >= 128) {
		channel = 9;
		note = inst - 128; // drum note
	} else {
		// instrument - find free channe,
		// round-robin at channels 0-8, 10-15
		static int current_channel = 0;
		do {
			current_channel = (current_channel + 1) % 16;
		} while (current_channel == 9); // drum channel

		channel = current_channel;
		awe32ProgramChange(channel, inst);
	}

	awe32Controller(channel, 7, vol);  // channel Volume
	awe32Controller(channel, 10, pan); // pan

	// play
	awe32NoteOn(channel, note, vol);
}

void awe32_key_off(int voice) {
	if (voice >= 0 && voice < 32) {
		AWE32_VOICE *v = &awe32_voices[voice];
		if (v->active) {
			awe32NoteOff(v->channel, v->note, 127);
			v->active = false;
		}
	}
}

void awe32_set_volume(int voice, int vol) {
	if (voice >= 0 && voice < 32) {
		AWE32_VOICE *v = &awe32_voices[voice];
		if (v->active) {
			awe32Controller(v->channel, 7, vol);
		}
	}
}

void awe32_set_pitch(int voice, int note, int bend) {
	if (voice >= 0 && voice < 32) {
		AWE32_VOICE *v = &awe32_voices[voice];
		if (v->active) {
			// Pitch Bend setzen
			awe32PitchBend(v->channel, bend & 0x7F, bend >> 7);
		}
	}
}
*/
