#ifndef GUSDRV_H
#define GUSDRV_H
#include "inc/types.h"
#include "libh/extern.h"
#include "inc/midi.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_VOICES 32
#define MAX_PATCHES 64
#define BUFFER_SIZE 48000

// GUS driver struct
extern MIDI_DRIVER gus_midi_driver;
MIDI_DRIVER* get_gus_midi_driver(void);

// condensed struct for midi instruments
typedef struct {
	int gm_patch_num;
	char name[16];
	dword gus_ptr;			 	 // pointer GUS-DRAM
	long sample_size;
	word sample_rate;			 // pitch calculation
	long start_loop;
	long end_loop;
	long root_frequency;	 	 // base freq for pitch
	byte modes;					 // loop flags etc.
	byte envelope_rate[6];	 // ADSR envelope rates
	byte envelope_offset[6]; // ADSR envelope levels
	byte balance;				 // pan position
	short tune;					 // fine tuning
} Loaded_Patch;

typedef struct {
	byte buffer[3];		// message buffer
	int buffer_pos;		// current position in buffer
	byte running_status; // for MIDI running status
	bool in_sysex;			// currently receiving SysEx
	int expected_bytes;	// expected bytes for current message
} Midi_State;

typedef struct {
   bool active;
	int  voice;
   byte channel;
   byte note;
   byte velocity;
   Loaded_Patch* patch;
   dword frequency;
} Voice_Table;

// driver functions
bool GUS_Detect(int);
bool GUS_Init();
void GUS_Exit(void);
void GUS_Raw_Midi(int data);
int  GUS_Load_Patches(char* patches, char* drums);
// not part of the driver
void GUS_Play_Sample(char* file);

#ifdef __cplusplus
}
#endif

#endif
