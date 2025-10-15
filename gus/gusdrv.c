#include <io.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <math.h>
#include "libh/ultraerr.h"
#include "libh/gf1proto.h"
#include "libh/extern.h"
#include "libh/patch.h"
#include "inc/types.h"
#include "inc/memory.h"
#include "inc/logger.h"
#include "gusdrv.h"

// =============================================================================
// GUS MIDI DRIVER - using GUS SDK 2.22 aka UltraSound Lowlevel ToolKit
// =============================================================================
static bool log_msg = false;

static ULTRA_CFG config;
static bool gus_initialized = false;

static char patch_files[256][16];
static bool patch_table_loaded = false;

static Loaded_Patch active_patches[MAX_PATCHES];
static int active_patch_count = 0;
static int patch_lookup[256];
static Real_Segment dma_buffer;

static int next_voice = 0;

/* support */
static int read_gus_cfg(ULTRA_CFG* config);
static bool parse_default_cfg(FILE* cfg_file);
static char* get_patch_filename(int patch_num);
static int load_single_patch(int gm_patch_num, char *patch_name);
static bool load_patch_meta(
	char* patch_name,
	FILE* patch_file,
	PATCHHEADER* header,
	INSTRUMENTDATA* inst_data,
	LAYERDATA* layer_data,
	PATCHDATA* patch_data
);

/* debug */
static void test_patch();
static void analyze_patch_structure();
static void log_patch(Loaded_Patch* patch);

/* main */
static void process_midi();
static void gus_note_on(byte channel, byte note, byte velocity);
static void gus_note_off(byte channel, byte note, byte velocity);
static void gus_pitch_bend(byte channel, byte data);
static void gus_program_change(byte channel, byte program);
static void gus_control_change(byte channel, byte data1, byte data2);
static int  gus_alloc_voice(int voice);
static int  get_message_length(byte status);
static void handle_realtime_message(byte msg);
static Loaded_Patch* get_loaded_patch(int gm_patch_num);
static ulong calculate_frequency_gus(Loaded_Patch* patch, byte midi_note);

MIDI_DRIVER gus_midi_driver = {
	 0x475553,				 // 'GUS' ID
	 "GUS",					 // name
	 "Gravis UltraSound", // description
	 "gus",					 // ascii name
	 32,						 // voices
	 0,						 // basevoice
	 32,						 // max_voices
	 16,						 // def_voices
	 -1, -1,					 // xmin, xmax (no reserved range)

	 GUS_Detect, // detect
	 GUS_Init,	 // init
	 GUS_Exit,	 // exit
	 NULL,		 // set_mixer_volume (optional)
	 NULL,		 // get_mixer_volume (optional)

	 // MIDI functions
	 GUS_Raw_Midi,		 // raw_midi
	 GUS_Load_Patches, // load_patches
	 NULL,				 // adjust_patches
	 NULL,				 // key_on
	 NULL,				 // key_off
	 NULL,				 // set_volume
	 NULL					 // set_pitch
};

// MIDI note to frequency conversion table (A440 tuning)
static const float midi_note_freq[128] = {
    // Octave -1 (MIDI notes 0-11)
    8.1758f,    8.6620f,    9.1770f,    9.7227f,    10.3009f,   10.9134f,
    11.5623f,   12.2499f,   12.9783f,   13.7500f,   14.5676f,   15.4339f,
    // Octave 0 (MIDI notes 12-23)
    16.3516f,   17.3239f,   18.3540f,   19.4454f,   20.6017f,   21.8268f,
    23.1247f,   24.4997f,   25.9565f,   27.5000f,   29.1352f,   30.8677f,
    // Octave 1 (MIDI notes 24-35)
    32.7032f,   34.6478f,   36.7081f,   38.8909f,   41.2034f,   43.6535f,
    46.2493f,   48.9994f,   51.9131f,   55.0000f,   58.2705f,   61.7354f,
    // Octave 2 (MIDI notes 36-47)
    65.4064f,   69.2957f,   73.4162f,   77.7817f,   82.4069f,   87.3071f,
    92.4986f,   97.9989f,   103.8262f,  110.0000f,  116.5409f,  123.4708f,
    // Octave 3 (MIDI notes 48-59)
    130.8128f,  138.5913f,  146.8324f,  155.5635f,  164.8138f,  174.6141f,
    184.9972f,  195.9977f,  207.6523f,  220.0000f,  233.0819f,  246.9417f,
    // Octave 4 (MIDI notes 60-71) - Middle C = note 60 = 261.63 Hz
    261.6256f,  277.1826f,  293.6648f,  311.1270f,  329.6276f,  349.2282f,
    369.9944f,  391.9954f,  415.3047f,  440.0000f,  466.1638f,  493.8833f,
    // Octave 5 (MIDI notes 72-83) 
    523.2511f,  554.3653f,  587.3295f,  622.2540f,  659.2551f,  698.4565f,
    739.9888f,  783.9909f,  830.6094f,  880.0000f,  932.3275f,  987.7666f,
    // Octave 6 (MIDI notes 84-95)
    1046.5023f, 1108.7305f, 1174.6591f, 1244.5079f, 1318.5102f, 1396.9129f,
    1479.9777f, 1567.9817f, 1661.2188f, 1760.0000f, 1864.6550f, 1975.5332f,
    // Octave 7 (MIDI notes 96-107)
    2093.0045f, 2217.4610f, 2349.3181f, 2489.0159f, 2637.0205f, 2793.8259f,
    2959.9554f, 3135.9635f, 3322.4376f, 3520.0000f, 3729.3101f, 3951.0664f,
    // Octave 8 (MIDI notes 108-119)
    4186.0090f, 4434.9221f, 4698.6363f, 4978.0317f, 5274.0409f, 5587.6517f,
    5919.9108f, 6271.9270f, 6644.8752f, 7040.0000f, 7458.6202f, 7902.1328f,
    // Octave 9 (MIDI notes 120-127)
    8372.0181f, 8869.8442f, 9397.2726f, 9956.0635f, 10548.0818f, 11175.3034f,
    11839.8215f, 12543.8540f
};

// =============================================================================
// IMPLEMENTATION
// =============================================================================

MIDI_DRIVER *get_gus_midi_driver() {
	return &gus_midi_driver;
}

static Real_Segment dma_buffer = {0};

static Midi_State midi_state =  {0};
static Voice_Table voice_table[MAX_VOICES] = {0};
static int channel_patch[16] = {0};

bool GUS_Detect(int port) {
	// get ULTRASND env
	read_gus_cfg(&config);

	if (UltraProbe(config.base_port) == ULTRA_OK) {
		config.dram_size = UltraSizeDram();
		Log_Info("GUS: base: 0x%02X, irq: %d, dma: %d, dram: %d, path: %s",
					config.base_port, config.gf1_irq_num, config.adc_dma_chan, 
					config.dram_size, config.ultrapath);
		return true;
	} else {
		Log_Info("GUS: There's no GUS. Trust me.");
	}

	return false;	
}

bool GUS_Init() {
	FILE* midi_cfg; 
	char file[128]; 
	int n = 0;
	dma_buffer.data = 0;
	dma_buffer.size = BUFFER_SIZE;

	if (gus_initialized) return true;

	if (UltraOpen(&config, MAX_VOICES) != ULTRA_OK) {
		Log_Info("GUS: Failed to initialize for whatever reason!"); return false;
	}
	if (!Real_Malloc(&dma_buffer)) {
		Log_Info("GUS: Cannot allocate measly %dkb real mem!", (BUFFER_SIZE/1000));
		return false;
	} else {
		memset(dma_buffer.data, 0, BUFFER_SIZE);
	}

	// check, if default patches are available (driver package)
	sprintf(file, "%s/MIDI/DEFAULT.CFG", config.ultrapath);
	midi_cfg = fopen(file, "rb");

	if (!midi_cfg) {
		Log_Info("GUS: Failed to load %s", file);
		fclose(midi_cfg); return false;
	}
	parse_default_cfg(midi_cfg);
	fclose(midi_cfg);
	
	/* CLEAR AND SET VOLUMES */
	UltraClearVoices();
	for (n=0;n<MAX_VOICES;n++) { UltraSetVolume(n, 4095); delay(1); }

	/* test */
	// GUS_Play_Sample("cake.wav");
	// GUS_Play_Sample("retronom.wav");

	gus_initialized = true;
	return gus_initialized;
}

void GUS_Exit() {
	int idx; Voice_Table voice;
	if (!gus_initialized) return;

	for (idx=0;idx<MAX_VOICES;idx++) {
		UltraStopVoice(idx);
		voice.active = false;
	}

	UltraClearVoices();
	UltraMemInit(); // free gus dram
	UltraClose();

	Real_Free(dma_buffer.selector);
	gus_initialized = false;
	Log_Info("GUS: bye-bye");
}

int GUS_Load_Patches(char *patches, char *drums) {
	int n; int offset = 128;
	char *patch_name;
	int loaded_count = 0;
	int failed_count = 0;

	UltraMemInit(); // free gus dram

	Log_Info("GUS: loading patches ...");
	if (log_msg) { Log_Info("- #0"); }
	for (n = 0; n < 256; n++) patch_lookup[n] = -1;
	active_patch_count = 0;

	// instrument patches (0-127)
	for (n = 0; n < 128; n++) if (patches[n]) {
		patch_name = get_patch_filename(n);

		if (patch_name) {
			if (load_single_patch(n, patch_name) >= 0)
				loaded_count++;
			else
				failed_count++;
		} else {
			Log_Info("GUS: instr. patch %d not listed", n);
			failed_count++;
		}
	}

	// drum patches (128-255)
	for (n = 0; n < 128; n++) if (drums[n]) {
		int drum_patch_num = offset + n;
		patch_name = get_patch_filename(drum_patch_num);

		if (patch_name) {
			if (load_single_patch(drum_patch_num, patch_name) >= 0) 
				loaded_count++;
			else
				failed_count++;
		} else {
			Log_Info("GUS: drum patch %d not listed", drum_patch_num);
			failed_count++;
		}
	}

	Log_Info("GUS: patches loaded: %d, errors: %d, active: %d",
				loaded_count, failed_count, active_patch_count);

	// test_patch();
	return loaded_count > 0;
}

void GUS_Raw_Midi(int data) {
	byte midi_byte = (byte)data;

	// 1. Handle SysEx messages (we'll probably ignore these)
	if (midi_state.in_sysex) {
		if (midi_byte == 0xF7) midi_state.in_sysex = false;
		return; // Ignore SysEx data
	}

	// control bytes
	if (midi_byte & 0x80) {
		// Real-Time messages (0xF8-0xFF)
		if (midi_byte >= 0xF8) { handle_realtime_message(midi_byte); return; }
		// Start of SysEx
		if (midi_byte == 0xF0) { midi_state.in_sysex = true;  return; }
		// System Common messages (0xF1-0xF7)
		if (midi_byte > 0xF0) { return; }

		// channel message - running status
		midi_state.running_status = midi_byte;
		midi_state.buffer[0] = midi_byte;
		midi_state.buffer_pos = 1;

		// Determine how many data midi_bytes to expect
		midi_state.expected_bytes = get_message_length(midi_byte);
	} else {
		// Data midi_byte - use running status if we have one
		if (midi_state.running_status == 0) { return; }
		// add to buffer
		if (midi_state.buffer_pos == 0) {
			// Using running status
			midi_state.buffer[0] = midi_state.running_status;
			midi_state.buffer_pos = 1;
			midi_state.expected_bytes = get_message_length(midi_state.running_status);
		}
		midi_state.buffer[midi_state.buffer_pos++] = midi_byte;
	}

	// message complete?
	if (midi_state.buffer_pos >= midi_state.expected_bytes) {
		process_midi();
		midi_state.buffer_pos = 0; // Reset for next message
	}
}

static void handle_realtime_message(byte msg) { }

/* play a sample */
void GUS_Play_Sample(char* file) {
	FILE* sample_file; char filepath[128];
	byte control = 0; uint volume = 0;
	uint dram_loc = 0;
	Real_Segment tst_buffer;
	tst_buffer.data = 0; 
	tst_buffer.size = 0;

	sprintf(filepath, "assets/%s", file);
	sample_file = fopen(filepath, "rb");
	if (!sample_file) {
		Log_Info("GUS: failed to load %s", filepath);
		fclose(sample_file); return;
	}

	fseek(sample_file, 0, SEEK_END);
	tst_buffer.size = ftell(sample_file);
	rewind(sample_file);

	fseek(sample_file, 44, SEEK_SET);
	tst_buffer.size -= 44;

	if (Real_Malloc(&tst_buffer)) {
		fread(tst_buffer.data, 1, tst_buffer.size, sample_file);
	} else {
		Log_Info("GUS: no buffer");
		fclose(sample_file); return;
	}

	fclose(sample_file);

	if (UltraMemAlloc(tst_buffer.size, &dram_loc) != ULTRA_OK) {
		Log_Info("GUS: no gus mem"); return;
	}
	
	control |= DMA_8; control |= DMA_CVT_2;
	UltraDownload(tst_buffer.real, control, dram_loc, tst_buffer.size, true);	
	
	Log_Info("GUS: voice: 0, volume: %d", UltraReadVolume(0));
	UltraVoiceOn(0, dram_loc, dram_loc, (dram_loc + tst_buffer.size), 0, 11025);
	delay(100); UltraVoiceOff(0, true);

	Real_Free(tst_buffer.selector);
	UltraMemFree(tst_buffer.size, dram_loc);
}

// ============================================================================
// MIDI functions
// ============================================================================

static void process_midi(void) {
	byte status = midi_state.buffer[0];
	byte channel = status & 0x0F;
	byte message_type = status & 0xF0;
	byte data1 = midi_state.buffer[1];
	byte data2 = midi_state.buffer[2];

	switch (message_type) {
	case 0x80: // note off
		gus_note_off(channel, data1, data2);
		break;
	case 0x90: // note on (or off)
		if (data2 > 0) {
			gus_note_on(channel, data1, data2);
		} else {
			gus_note_off(channel, data1, 64); 
		}
		break;
	case 0xB0:
		gus_control_change(channel, data1, data2);
		break;
	case 0xC0:
		channel_patch[channel] = data1;
		// gus_program_change(channel, data1);
		break;
	case 0xE0:
		gus_pitch_bend(channel, data1 | (data2 << 7));
		break;
		// add other message types as needed
	}
}

static void gus_note_on(byte channel, byte note, byte velocity) {
	int voice; Loaded_Patch *patch;
	ulong frequency; int patch_num;
	ulong volume; byte gus_mode = 0;
	int semitone_diff; float multiplier;

	// Get the patch for this channel
	patch_num = (channel == 9) ? (128 + note) : channel_patch[channel];
	patch = get_loaded_patch(patch_num);
	if (!patch) { Log_Info("GUS: error, patch: %d", patch_num); return; }

	voice = gus_alloc_voice(next_voice);

	semitone_diff = note - 60;
	multiplier = pow(2.0f, semitone_diff / 12.0f);
	frequency = (ulong)(patch->sample_rate * multiplier);
	volume = (ulong)(4095.0 * pow((float)velocity / 127.0, 2.0));

	// Store voice info
	voice_table[voice].voice	= voice;
	voice_table[voice].note		= note;
	voice_table[voice].active	= true;
	voice_table[voice].channel	= channel;
	voice_table[voice].velocity	= velocity;
	voice_table[voice].patch		= patch;
	voice_table[voice].frequency 	= frequency;

	// translate patch mode into gus replay mode ... 🙄
	if (patch->modes & 0x01)  gus_mode |= 0x04; // VC_DATA_TYPE
	if (patch->modes & 0x04)  gus_mode |= 0x08; // VC_LOOP_ENABLE
	if (patch->modes & 0x08)  gus_mode |= 0x10; // VC_BI_LOOP
	if (patch->modes & 0x10)  gus_mode |= 0x40; // VC_DIRECT

	UltraSetVoice(voice, patch->gus_ptr);
	UltraSetBalance(voice, patch->balance);
	UltraSetFrequency(voice, frequency);
	UltraSetVolume(voice, volume); // GUS uses 0-4095 for volume
	UltraStartVoice(
		voice, 
		patch->gus_ptr,
		patch->gus_ptr+patch->start_loop, 
		patch->gus_ptr+patch->end_loop, 
		gus_mode
	);

	if (log_msg)
	Log_Info("GUS Note On: [%d], Voice=%d, Ch=%d, Note=%d, Freq=%lu Hz", 
				patch_num, voice, channel, note, frequency);
}

static void gus_note_off(byte channel, byte note, byte velocity) {
	int idx; Voice_Table voice;
	// find voice playing note
	for (idx = 0; idx < MAX_VOICES; idx++) {
		voice = voice_table[idx];
		if (voice.active && voice.channel == channel && voice.note == note) {
			UltraStopVoice(idx);
			voice.active = false;
			break;
		}
	}
}

static void gus_program_change(byte channel, byte program) {
	channel_patch[channel] = program;
	// Could also send all notes off for this channel
}

static void gus_control_change(byte channel, byte data1, byte data2) {

}

static void gus_pitch_bend(byte channel, byte data)  {

}

// ============================================================================
// midi functions support
// ============================================================================

static ulong calculate_frequency_gus(Loaded_Patch* patch, byte midi_note) {
	float target_freq; ulong frequency;
	float tune_factor; float tune_cents;

	target_freq = midi_note_freq[midi_note & 0x7F];
	if (patch->tune != 0 && patch->tune != 512) {
		tune_cents = ((float)(patch->tune - 512) / 512.0f) * 100.0f;
		tune_factor = pow(2.0f, tune_cents / 1200.0f);
		target_freq *= tune_factor;
	}
	
	frequency = (ulong)target_freq;
	return frequency;
}

static int gus_alloc_voice(int voice) {
   if (UltraVoiceStopped(voice)) return voice;
   next_voice = (next_voice + 1) % MAX_VOICES;
   UltraStopVoice(next_voice); 
   return next_voice;
}

static Loaded_Patch* get_loaded_patch(int gm_patch_num) {
	int index; Loaded_Patch* patch;
	if (gm_patch_num < 0 || gm_patch_num >= 256) return NULL;
	
	index = patch_lookup[gm_patch_num];
	if (index < 0 || index >= active_patch_count) return NULL;

	patch = &active_patches[index];
	if (log_msg) log_patch(patch);

	return patch;
}

static int get_message_length(byte status) {
	switch (status & 0xF0) {
	case 0x80:	 // note off
	case 0x90:	 // note on
	case 0xA0:	 // aftertouch
	case 0xB0:	 // control change
	case 0xE0:	 // pitch bend
		return 3; // status + 2 data bytes
	case 0xC0:	 // program change
	case 0xD0:	 // channel pressure
		return 2; // status + 1 data byte
	default:
		return 1; // unknown or system message
	}
}

// ============================================================================
// configuration & setup
// ============================================================================

/* modified UltraGetCfg(...) from lib */
static int read_gus_cfg(ULTRA_CFG* config) {
	char *ptr;

	config->base_port = 0x240;
	config->dram_dma_chan = 3;
	config->adc_dma_chan  = 3;
	config->gf1_irq_num   = 7;
	config->midi_irq_num  = 7;

	ptr = getenv("ULTRASND");
	if (ptr == NULL)
		return (FALSE);
	else
		sscanf(ptr, "%x,%d,%d,%d,%d", 
				 &config->base_port,
				 &config->dram_dma_chan,
				 &config->adc_dma_chan,
				 &config->gf1_irq_num,
				 &config->midi_irq_num);

	// check path, if none found, assume default
	ptr = getenv("ULTRADIR");
	if (ptr != NULL) 
		sprintf(config->ultrapath, ptr);
	else
		sprintf(config->ultrapath, "C:\\ULTRASND");

	return (TRUE);
}

/* GUS drivers come with a set of patch files for midi and a
 * DEFAULT.CFG, telling which file is which MIDI instr. */
static bool parse_default_cfg(FILE* cfg_file) {
	char line[128]; char patch_name[16];
	char dummy[32]; char base_name[16];
	bool in_multipatch;
	int patch_num, volume;
	int detune, line_count;
	int multipatch_base, drum_note;
	int actual_patch;

	line_count = 0;
	in_multipatch = false;
	multipatch_base = 0;

	memset(patch_files, 0, sizeof(patch_files));
	rewind(cfg_file);

	while (fgets(line, sizeof(line), cfg_file)) {
		line_count++;

		// skip comments
		if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;

		if (strstr(line, "begin_multipatch")) {
			memset(dummy,0,32); memset(base_name,0,16);
			if (sscanf(line, "%d-%d %s %s %s",
						  &multipatch_base, &patch_num, dummy, dummy, base_name) >= 3) {
				in_multipatch = true;
				if (log_msg) Log_Info("GUS: multipatches: %d-%d, Base: %s", 
										multipatch_base, patch_num, base_name);
				continue;
			}
		}

		if (strstr(line, "end_multipatch")) {
			in_multipatch = false; 
			multipatch_base = 0;
			continue;
		}

		// skip override patches for now
		if (strstr(line, "override_patch")) continue;

		// parse entry row
		if (!in_multipatch) {
			// format: "patch_num patch_name [volume] [detune]"
			volume = 100; detune = 0; // defaults
			if (sscanf(line, "%d %s %d %d", &patch_num, patch_name, &volume, &detune) >= 2) {
				if (patch_num >= 0 && patch_num < 256) {
					strncpy(patch_files[patch_num], patch_name, 15);
					patch_files[patch_num][15] = '\0';
				}
			}
		} else {
			// format: "drum_note patch_name [parameter]"
			drum_note = 0;
			if (sscanf(line, "%d %s", &drum_note, patch_name) >= 2) {
				actual_patch = multipatch_base + drum_note;
				if (actual_patch >= 0 && actual_patch < 256) {
					strncpy(patch_files[actual_patch], patch_name, 15);
					patch_files[actual_patch][15] = '\0';
				}
			}
		}
	}

	patch_table_loaded = true;
	if (log_msg) Log_Info("GUS: DEFAULT.CFG succesfully parsed, %d rows processed", line_count);
	return patch_table_loaded;
}

/* !ONLY works in tandem with load_single_patch()! */
static bool load_patch_meta(
	char* patch_name, FILE* patch_file,
	PATCHHEADER* header, INSTRUMENTDATA* instrument,
	LAYERDATA* layer, PATCHDATA* patch
) {
	fread(header, sizeof(PATCHHEADER), 1, patch_file);
	if (strncmp(header->header, GF1_HEADER_TEXT, 10) != 0) {
		if (log_msg) Log_Info("GUS: illegal patch header: %s", patch_name);
		fclose(patch_file); return false;
	}

	if (log_msg) Log_Info("GUS: loading patch %s - Instrumente: %d, Waveforms: %d", 
            			patch_name, header->instruments, header->wave_forms);

	if (header->instruments < 1) {
		if (log_msg) Log_Info("GUS: patch %s has no instruments", patch_name);
		fclose(patch_file); return false;
	}
	fread(instrument, sizeof(INSTRUMENTDATA), 1, patch_file);

	if (instrument->layers < 1) {
		if (log_msg) Log_Info("GUS: instrument %s got no layer", patch_name);
		fclose(patch_file); return false;
	}
	fread(layer, sizeof(LAYERDATA), 1, patch_file);

	if (log_msg) Log_Info("GUS: layer %d - dupl: %d, size: %ld, samples: %d",
							layer->layer, layer->layer_duplicate, 
							layer->layer_size, layer->samples);

	if (layer->samples < 1) {
		if (log_msg) Log_Info("GUS: layer got no samples: %s", patch_name);
		fclose(patch_file); return false;
	}
	fread(patch, sizeof(PATCHDATA), 1, patch_file); // POS 335 after read

	if (log_msg) Log_Info("GUS: sample %s - size: %ld, rate: %d, loop: %ld-%ld",
							patch->wave_name,   patch->wave_size,
							patch->sample_rate, patch->start_loop,
							patch->end_loop);

	return true;
}

static char* get_patch_filename(int patch_num) {
    if (!patch_table_loaded || patch_num < 0 || patch_num >= 256)
        return NULL;
    
    if (strlen(patch_files[patch_num]) > 0)
        return patch_files[patch_num];
    
    return NULL;
}

static int load_single_patch(int gm_patch_num, char* patch_name) {
	FILE* patch_file; char filepath[256]; uint i;
	PATCHHEADER header; INSTRUMENTDATA instrument;
	LAYERDATA   layer;  PATCHDATA patch;
	uint chunks, remains, patch_index;
	uint sample_size, part_sample;
	uint dram_base_loc = 0, dram_ptr = 0;
	byte control = 0;

	// space left?
	if (active_patch_count >= MAX_PATCHES) {
		Log_Info("GUS: to many patches, skipping %s", patch_name); 
		return -1; 
	}

	/* open file and load meta data */

	sprintf(filepath, "%s/MIDI/%s.pat", config.ultrapath, patch_name);
	patch_file = fopen(filepath, "rb");
	if (!patch_file) { Log_Info("GUS: file not found: %s", filepath); return -1; }

	// lots of checking and logging
	if (!load_patch_meta(patch_name, patch_file,
		 &header, &instrument, &layer, &patch)) 
		return -1;

	/* load audio data */

	// samples must not be bigger than ~250kb
	sample_size = patch.wave_size;
	if (UltraMemAlloc(sample_size, &dram_base_loc) != ULTRA_OK) {
		Log_Info("GUS: allocating onboard mem failed"); return -1;
	}

	// chunk sample data, if necessary
	chunks = 1; remains = 0;
	if (sample_size > BUFFER_SIZE) {
		remains = sample_size % BUFFER_SIZE;
		chunks  = sample_size / BUFFER_SIZE + (remains > 0 ? 1 : 0);
		part_sample = BUFFER_SIZE;
	} else {
		part_sample = sample_size;
	}

	dram_ptr = dram_base_loc; 
	control |= DMA_8; control |= DMA_CVT_2;
	if (log_msg) {
		Log_Info("GUS: size: %d, (%d), chunks: %d, remains: %d",
					part_sample, BUFFER_SIZE, chunks, remains);
		Log_Info("GUS: buffer: 0x%08X, dram base: 0x%08X, ctrl: 0x%02X",
					dma_buffer.real, dram_base_loc, control);
	}

	// load sample to GUS dram
	for (i = 1; i <= chunks; i++) {
		fread(dma_buffer.data, 1, part_sample, patch_file); delay(10);

		if (UltraDownload(dma_buffer.real, control, dram_ptr, part_sample, true) != ULTRA_OK) {
			if (log_msg) Log_Info("GUS: download into GUS-DRAM failed: %s", patch_name);
			UltraMemFree(sample_size, dram_base_loc); fclose(patch_file); return -1;
		}

		part_sample = ((i + 1) * BUFFER_SIZE) < sample_size ? BUFFER_SIZE : remains;
		dram_ptr = dram_base_loc + (i * part_sample);
	}

	/* save meta/patch data for use */

	patch_index = active_patch_count++;
	strncpy(active_patches[patch_index].name, patch_name, 14);

	active_patches[patch_index].gm_patch_num	 = gm_patch_num;
	active_patches[patch_index].name[15] 		 = '\0';
	active_patches[patch_index].gus_ptr 		 = dram_base_loc;
	active_patches[patch_index].sample_size	 = sample_size;
	active_patches[patch_index].root_frequency = patch.root_frequency;
	active_patches[patch_index].sample_rate 	 = patch.sample_rate;
	active_patches[patch_index].start_loop 	 = patch.start_loop;
	active_patches[patch_index].end_loop 		 = patch.end_loop;
	active_patches[patch_index].modes 			 = patch.modes;
	active_patches[patch_index].balance 		 = patch.balance;
	active_patches[patch_index].tune 			 = patch.tune;

	for (i = 0; i < 6; i++) {
		active_patches[patch_index].envelope_rate[i] = patch.envelope_rate[i];
		active_patches[patch_index].envelope_offset[i] = patch.envelope_offset[i];
	}

	patch_lookup[gm_patch_num] = patch_index;

	if (log_msg) {
		Log_Info("GUS: patch %d (%s) successfully loaded - DRAM: 0x%08lX, size: %ld",
					gm_patch_num, patch_name, dram_base_loc, sample_size);
		Log_Info("- #%d", active_patch_count);
	}

	fclose(patch_file);
	return patch_index;
}


// ============================================================================
// DEBUG
// ============================================================================

static void log_patch(Loaded_Patch* patch) {
	Log_Info("=== get_loaded_patch: Found patch %d ===", patch->gm_patch_num);
	Log_Info("  Name: %s", patch->name);
	Log_Info("  GM Patch Num: %d", patch->gm_patch_num);
	Log_Info("  GUS DRAM Ptr: 0x%08lX", patch->gus_ptr);
	Log_Info("  Sample Size: %ld bytes", patch->sample_size);
	Log_Info("  Sample Rate: %d Hz", patch->sample_rate);
	Log_Info("  Start Loop: %ld", patch->start_loop);
	Log_Info("  End Loop: %ld", patch->end_loop);
	Log_Info("  Root Frequency: %ld", patch->root_frequency);
	Log_Info("  Tune: %d", patch->tune);
	Log_Info("  Balance: %d (0=left, 7=center, 15=right)", patch->balance);
	Log_Info("  Modes: 0x%02X", patch->modes);
	if (patch->modes & 0x01) Log_Info("    - Looping enabled");
	if (patch->modes & 0x02) Log_Info("    - Bidirectional loop");
	if (patch->modes & 0x04) Log_Info("    - Reverse");
	if (patch->modes & 0x08) Log_Info("    - Sustain");
	if (patch->modes & 0x10) Log_Info("    - Envelope");
	Log_Info("  Envelope Rates: %02X %02X %02X %02X %02X %02X",
				patch->envelope_rate[0], patch->envelope_rate[1],
				patch->envelope_rate[2], patch->envelope_rate[3],
				patch->envelope_rate[4], patch->envelope_rate[5]);
	Log_Info("  Envelope Offsets: %02X %02X %02X %02X %02X %02X",
				patch->envelope_offset[0], patch->envelope_offset[1],
				patch->envelope_offset[2], patch->envelope_offset[3],
				patch->envelope_offset[4], patch->envelope_offset[5]);
	Log_Info("=================================");
}

static void test_patch(void) {
	Loaded_Patch* patch;
	ulong freq_c4;
	ulong freq_a4;
	
	Log_Info("=== Testing Default Patches ===");
	
	// test piano (patch 0)
	patch = get_loaded_patch(0);
	if (patch) {
		// analyze_patch_structure();
		Log_Info("Piano: rate=%d, root=%ld, tune=%d", 
					patch->sample_rate, patch->root_frequency, patch->tune);

		Log_Info("- gus style calculation -");
		freq_c4 = calculate_frequency_gus(patch, 60); // ~261
		freq_a4 = calculate_frequency_gus(patch, 69); // ~440
		Log_Info("C4: %lu Hz, A4: %lu Hz", freq_c4, freq_a4);

		channel_patch[0] = 0;
		gus_note_on(0, 60, 100);
	}

	// test a drum
	patch = get_loaded_patch(128 + 35);
	if (patch) {
		Log_Info("Bass Drum: rate=%d, root=%ld", 
					patch->sample_rate, patch->root_frequency);
	}
}

static void analyze_patch_structure() {
	FILE* patch_file; char* filepath;

	PATCHHEADER header;
	INSTRUMENTDATA instrument;
	LAYERDATA layer;
	unsigned char buffer[128];
	long pos; int i;
	
	sprintf(filepath, "%s/MIDI/%s.pat", config.ultrapath, "acpiano");
	patch_file = fopen(filepath, "rb");

	if (!patch_file) { Log_Info("File not found! %s", filepath); return; }

	fseek(patch_file, 0, SEEK_SET);
	fread(&header, sizeof(PATCHHEADER), 1, patch_file);
	Log_Info("=== PATCH STRUCTURE ===");
	Log_Info("Header ID: %.10s", header.header);
	Log_Info("Header size: %ld bytes", sizeof(PATCHHEADER));
	
	pos = ftell(patch_file);
	Log_Info("INSTRUMENTDATA at position: %ld", pos);
	fread(&instrument, sizeof(INSTRUMENTDATA), 1, patch_file);
	
	pos = ftell(patch_file);
	Log_Info("LAYERDATA at position: %ld", pos);
	fread(&layer, sizeof(LAYERDATA), 1, patch_file);
	
	pos = ftell(patch_file);
	Log_Info("PATCHDATA at position: %ld", pos);
	
	fread(buffer, 1, 64, patch_file);
	
	Log_Info("=== RAW PATCHDATA ===");
	Log_Info("First 64 bytes at position %ld:", pos);
	for (i = 0; i < 64; i += 16) {
		Log_Info("%04X: %02X %02X %02X %02X %02X %02X %02X %02X  %02X %02X %02X %02X %02X %02X %02X %02X",
					i,
					buffer[i+0], buffer[i+1], buffer[i+2], buffer[i+3],
					buffer[i+4], buffer[i+5], buffer[i+6], buffer[i+7],
					buffer[i+8], buffer[i+9], buffer[i+10], buffer[i+11],
					buffer[i+12], buffer[i+13], buffer[i+14], buffer[i+15]);
	}
	
	// Try to interpret as PATCHDATA fields
	Log_Info("Wave name (0-6): %.7s", buffer);
	Log_Info("Word @ offset 20: %d", *(unsigned short*)(buffer + 20));
	Log_Info("Word @ offset 22: %d", *(unsigned short*)(buffer + 22));  
	Log_Info("Long @ offset 24: %ld", *(unsigned long*)(buffer + 24));
	Log_Info("Long @ offset 28: %ld", *(unsigned long*)(buffer + 28));
	Log_Info("Long @ offset 32: %ld", *(unsigned long*)(buffer + 32));
}

