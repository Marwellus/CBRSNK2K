#ifndef MIDI_H
#define MIDI_H
#include "inc/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// MIDI DRIVER for Allegro MIDI engine
// =============================================================================

#define MIDI_TRACKS 32

typedef struct MIDI {
	int divisions;              /* number of ticks per quarter note */
	struct {
		unsigned char *data;    /* MIDI message stream */
		int len;			    /* length of the track data */
	} track[MIDI_TRACKS];
} MIDI;

typedef struct MIDI_DRIVER {
    int id;                     // driver ID
    char *name;                 // driver name
    char *desc;                 // description string
    char *ascii_name;           // ASCII name
    int voices;                 // available voices
    int basevoice;              // first voice number
    int max_voices;             // max simultaneous voices
    int def_voices;             // default number of voices
    int xmin, xmax;             // reserved voice range
    
    // core functions
    bool (*detect)(int input);
    bool (*init)();
    void (*exit)(void);
    int (*set_mixer_volume)(int volume);
    int (*get_mixer_volume)(void);
    
    // MIDI functions
    void (*raw_midi)(int data);
    int (*load_patches)(char *patches, char *drums);
    void (*adjust_patches)(char *patches, char *drums);
    void (*key_on)(int inst, int note, int bend, int vol, int pan);
    void (*key_off)(int voice);
    void (*set_volume)(int voice, int vol);
    void (*set_pitch)(int voice, int note, int bend);
} MIDI_DRIVER;

// MIDI functions
MIDI *load_midi(char *filename);
int midi_init(MIDI_DRIVER* driver);
int load_patches(MIDI *midi);
int play_midi(MIDI *midi, int loop);
int midi_seek(long target);
void destroy_midi(MIDI *midi);
void midi_exit(void);
void stop_midi(void);
void midi_pause(void);
void midi_resume(void);

#ifdef __cplusplus
}
#endif

#endif
