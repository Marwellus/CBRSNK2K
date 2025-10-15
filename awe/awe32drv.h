#ifndef AWE32DRV_H
#define AWE32DRV_H
#include "inc/midi.h"

#ifdef __cplusplus
extern "C" {
#endif

// AWE32 voice mapping
typedef struct {
	int channel;		// MIDI channel
	int note;			// MIDI note
	int instrument; 	// instrument number
	int active;			// is voice active?
} AWE32_VOICE;

// driver struct
extern MIDI_DRIVER awe32_midi_driver;
MIDI_DRIVER* get_awe32_midi_driver(void);

bool awe32_detect(int input);
bool awe32_init();
int awe32_load_sfb(char* filepath);
int awe32_load_patches(char *patches, char *drums);
void awe32_exit(void);
void awe32_key_on(int inst, int note, int bend, int vol, int pan);
void awe32_key_off(int voice);
void awe32_set_volume(int voice, int vol);
void awe32_set_pitch(int voice, int note, int bend);
void process_midi_command(int command, int channel, int data1, int data2);
void awe32_raw_midi(int data);

#ifdef __cplusplus
}
#endif

#endif
