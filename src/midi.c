#include <malloc.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "inc/types.h"
#include "inc/logger.h"
#include "awe/awe32drv.h"
#include "gus/gusdrv.h"
#include "inc/miditmr.h"
#include "inc/midi.h"

/*      Borrowed from Allegro 4.2.3
 *      (awesome) MIDI file player, by the great Shawn Hargreaves
 *      ported by Marwellus
 */

#define INLINE
#define ASSERT(MIDI)

#define MIDI_VOICES 32
#define MIDI_TRACKS 32
#define MIDI_LAYERS 4

typedef struct MIDI_TRACK /* a track in the MIDI file */
{
	unsigned char *pos;			   /* position in track data */
	long timer;					      /* time until next event */
	unsigned char running_status; /* last MIDI event */
} MIDI_TRACK;

typedef struct MIDI_CHANNEL /* a MIDI channel */
{
	int patch;					   /* current sound */
	int volume;					   /* volume controller */
	int pan;					      /* pan position */
	int pitch_bend;				/* pitch bend position */
	int new_volume;				/* cached volume change */
	int new_pitch_bend;			/* cached pitch bend */
	int note[128][MIDI_LAYERS]; /* status of each note */
} MIDI_CHANNEL;

typedef struct MIDI_VOICE /* a voice on the soundcard */
{
	int channel; /* MIDI channel */
	int note;	 /* note (-1 = off) */
	int volume;	 /* note velocity */
	long time;	 /* when note was triggered */
} MIDI_VOICE;

typedef struct WAITING_NOTE /* a stored note-on request */
{
	int channel;
	int note;
	int volume;
} WAITING_NOTE;

typedef struct PATCH_TABLE /* GM -> external synth */
{
	int bank1; /* controller #0 */
	int bank2; /* controller #32 */
	int prog;  /* program change */
	int pitch; /* pitch shift */
} PATCH_TABLE;

static MIDI_DRIVER *midi_driver;

int _midi_volume = 255;

volatile long midi_pos = -1;        /* current position in MIDI file */
volatile long midi_time = 0;	      /* current position in seconds */
static volatile long midi_timers;   /* current position in allegro-timer-ticks */
static long midi_pos_counter;	      /* delta for midi_pos */

volatile long _midi_tick = 0;       /* counter for killing notes */

static void midi_player(void);      /* core MIDI player routine */
static void prepare_to_play(MIDI *midi);
// static void midi_lock_mem(void);

static MIDI *midifile = NULL;       /* the file that is playing */

static int midi_loop = 0;           /* repeat at eof? */

long midi_loop_start = -1;          /* where to loop back to */
long midi_loop_end = -1;            /* loop at this position */

static int midi_semaphore = 0;		/* reentrancy flag */
static int patches_loaded = false;  /* loaded entire patch set? */

static int midi_timer_speed;        /* midi_player's timer speed */
static int midi_pos_speed;	         /* MIDI delta -> midi_pos */
static int midi_speed;		         /* MIDI delta -> timer */
static int midi_new_speed;	         /* for tempo change events */

static int old_midi_volume = -1;    /* stored global volume */

static int midi_alloc_channel;      /* so _midi_allocate_voice */
static int midi_alloc_note;	      /* knows which note the */
static int midi_alloc_vol;	         /* sound is associated with */

static MIDI_TRACK midi_track[MIDI_TRACKS];	   /* the active tracks */
static MIDI_VOICE midi_voice[MIDI_VOICES];	   /* synth voice status */
static MIDI_CHANNEL midi_channel[16];		      /* MIDI channel info */
static WAITING_NOTE midi_waiting[MIDI_VOICES];  /* notes still to be played */
static PATCH_TABLE patch_table[128];		      /* GM -> external synth */

static int midi_seeking; /* set during seeks */
static int midi_looping; /* set during loops */

/* hook functions */
void (*midi_msg_callback)(int msg, int byte1, int byte2) = NULL;
void (*midi_meta_callback)(int type, unsigned char *data, int length) = NULL;
void (*midi_sysex_callback)(unsigned char *data, int length) = NULL;

// Helper functions for Big-Endian reading
static unsigned int read_be_word(FILE *fp) {
	unsigned char b1, b2;
	b1 = fgetc(fp);
	b2 = fgetc(fp);
	return (b1 << 8) | b2;
}

static unsigned long read_be_long(FILE *fp) {
	unsigned char b1, b2, b3, b4;
	b1 = fgetc(fp);
	b2 = fgetc(fp);
	b3 = fgetc(fp);
	b4 = fgetc(fp);
	return (b1 << 24) | (b2 << 16) | (b3 << 8) | b4;
}

static unsigned long read_be_long_size(FILE *fp, int size) {
	unsigned long result = 0; int i;
	for (i = 0; i < size; i++) {
		result = (result << 8) | fgetc(fp);
	}
	return result;
}

/* load_midi:
 *  Loads a standard MIDI file, returning a pointer to a MIDI structure,
 *  or NULL on error.
 */
MIDI *load_midi(char *filename) {
	int c; char buf[4]; long data;
	unsigned char *p;
	FILE *fp; MIDI *midi;
	int num_tracks;

	fp = fopen(filename, "rb"); /* open the file */
	if (!fp) { Log_Info("MIDI: Song not found: %s", filename); return NULL; }

	midi = (MIDI*)malloc(sizeof(MIDI)); /* get some memory */
	if (!midi) { fclose(fp); return NULL; }

	for (c = 0; c < MIDI_TRACKS; c++) {
		midi->track[c].data = NULL;
		midi->track[c].len = 0;
	}

	fread(buf, 4, 1, fp); /* read midi header */

	/* Is the midi inside a .rmi file? */
	if (memcmp(buf, "RIFF", 4) == 0) { /* check for RIFF header */
		Log_Info("MIDI: RIFF container detected");

		read_be_long(fp); // skip RIFF size
		while (!feof(fp)) {
			fread(buf, 4, 1, fp); /* RMID chunk? */
			if (memcmp(buf, "RMID", 4) == 0) break;

			data = read_be_long(fp);   // chunk size
			fseek(fp, data, SEEK_CUR); /* skip to next chunk */
		}

		if (feof(fp)) goto err;

		read_be_long(fp);	  // skip RMID size
		read_be_long(fp);	  // skip some more
		fread(buf, 4, 1, fp); /* read midi header */
	}

	if (memcmp(buf, "MThd", 4)) {
		Log_Info("MIDI: Invalid header, expected MThd");
		goto err;
	}

	read_be_long(fp); /* skip header chunk length */

	data = read_be_word(fp); /* MIDI file type */
	if ((data != 0) && (data != 1)) goto err;

	num_tracks = read_be_word(fp); /* number of tracks */	
	if ((num_tracks < 1) || (num_tracks > MIDI_TRACKS)) {
      Log_Info("MIDI: To much tracks: %d", num_tracks);
		goto err;
   }

	midi->divisions = read_be_word(fp); /* beat divisions */

	for (c = 0; c < num_tracks; c++) {  /* read each track */
		fread(buf, 4, 1, fp);		      /* read track header */
		if (memcmp(buf, "MTrk", 4)) {
   		Log_Info("MIDI: Invalid Track, expected MTrk");
         goto err;
      }
		data = read_be_long(fp); /* length of track chunk */

		midi->track[c].len = data;		
		midi->track[c].data = (unsigned char*)malloc(data); /* allocate memory */
		if (!midi->track[c].data) {
         Log_Info("MIDI: malloc for Track data failed");
			goto err;
      }
		/* finally, read track data */
		if (fread(midi->track[c].data, data, 1, fp) != 1)
			goto err;
	}

	if (data > 10) p = (unsigned char*)midi->track[c].data;

	fclose(fp);
	return midi;

/* oh dear... */
err:
   Log_Info("Some shit happened! Go figure!");
	fclose(fp); destroy_midi(midi);
	return NULL;
}

/* destroy_midi:
 *  Frees the memory being used by a MIDI file.
 */
void destroy_midi(MIDI *midi) {
	int c;

	if (midi == midifile)
		stop_midi();

	if (midi) {
		for (c = 0; c < MIDI_TRACKS; c++) {
			if (midi->track[c].data) free(midi->track[c].data);
		}

		free(midi);
	}
}

/* parse_var_len:
 *  The MIDI file format is a strange thing. Time offsets are only 32 bits,
 *  yet they are compressed in a weird variable length format. This routine
 *  reads a variable length integer from a MIDI data stream. It returns the
 *  number read, and alters the data pointer according to the number of
 *  bytes it used.
 */
static unsigned long parse_var_len(unsigned char **data) {
	unsigned long val = **data & 0x7F;

	while (**data & 0x80) {
		(*data)++;
		val <<= 7;
		val += (**data & 0x7F);
	}

	(*data)++;
	return val;
}

/* global_volume_fix:
 *  Converts a note volume, adjusting it according to the global
 *  _midi_volume variable.
 */
static INLINE int global_volume_fix(int vol) {
	if (_midi_volume >= 0)
		return (vol * _midi_volume) / 256;

	return vol;
}

/* sort_out_volume:
 *  Converts a note volume, adjusting it according to the channel volume
 *  and the global _midi_volume variable.
 */
static INLINE int sort_out_volume(int c, int vol) {
	return global_volume_fix((vol * midi_channel[c].volume) / 128);			 
}

/* raw_program_change:
 *  Sends a program change message to a device capable of handling raw
 *  MIDI data, using patch mapping tables. Assumes that midi_driver->raw_midi
 *  isn't NULL, so check before calling it!
 */
static void raw_program_change(int channel, int patch) {
	if (channel != 9) {
		/* bank change #1 */
		if (patch_table[patch].bank1 >= 0) {
			midi_driver->raw_midi(0xB0 + channel);
			midi_driver->raw_midi(0);
			midi_driver->raw_midi(patch_table[patch].bank1);
		}

		/* bank change #2 */
		if (patch_table[patch].bank2 >= 0) {
			midi_driver->raw_midi(0xB0 + channel);
			midi_driver->raw_midi(32);
			midi_driver->raw_midi(patch_table[patch].bank2);
		}

		/* program change */
		midi_driver->raw_midi(0xC0 + channel);
		midi_driver->raw_midi(patch_table[patch].prog);

		/* update volume */
		midi_driver->raw_midi(0xB0 + channel);
		midi_driver->raw_midi(7);
		midi_driver->raw_midi(global_volume_fix(midi_channel[channel].volume - 1));
	}
}

/* midi_note_off:
 *  Processes a MIDI note-off event.
 */
static void midi_note_off(int channel, int note) {
	int done = false;
	int voice, layer;
	int c;

	/* can we send raw MIDI data? */
	if (midi_driver->raw_midi) {
		if (channel != 9)
			note += patch_table[midi_channel[channel].patch].pitch;

		midi_driver->raw_midi(0x80 + channel);
		midi_driver->raw_midi(note);
		midi_driver->raw_midi(0);
		return;
	}
}

/* sort_out_pitch_bend:
 *  MIDI pitch bend range is + or - two semitones. The low-level soundcard
 *  drivers can only handle bends up to +1 semitone. This routine converts
 *  pitch bends from MIDI format to our own format.
 */
static INLINE void sort_out_pitch_bend(int *bend, int *note) {
	if (*bend == 0x2000) {
		*bend = 0;
		return;
	}

	(*bend) -= 0x2000;

	while (*bend < 0) {
		(*note)--;
		(*bend) += 0x1000;
	}

	while (*bend >= 0x1000) {
		(*note)++;
		(*bend) -= 0x1000;
	}
}

/* _midi_allocate_voice:
 *  Allocates a MIDI voice in the range min-max (inclusive). This is
 *  intended to be called by the key_on() handlers in the MIDI driver,
 *  and shouldn't be used by any other code.
 */
int midi_allocate_voice(int min, int max) {
	int c;
	int layer;
	int voice = -1;
	long best_time = LONG_MAX;

	if (min < 0)
		min = 0;

	if (max < 0)
		max = midi_driver->voices - 1;

	/* which layer can we use? */
	for (layer = 0; layer < MIDI_LAYERS; layer++)
		if (midi_channel[midi_alloc_channel].note[midi_alloc_note][layer] < 0)
			break;

	if (layer >= MIDI_LAYERS)
		return -1;

	/* find a free voice */
	for (c = min; c <= max; c++) {
		if ((midi_voice[c].note < 0) &&
			(midi_voice[c].time < best_time) &&
			((c < midi_driver->xmin) || (c > midi_driver->xmax))) {
			voice = c;
			best_time = midi_voice[c].time;
		}
	}

	/* if there are no free voices, kill a note to make room */
	if (voice < 0) {
		voice = -1;
		best_time = LONG_MAX;
		for (c = min; c <= max; c++) {
			if ((midi_voice[c].time < best_time) &&
				((c < midi_driver->xmin) || (c > midi_driver->xmax))) {
				voice = c;
				best_time = midi_voice[c].time;
			}
		}
		if (voice >= 0)
			midi_note_off(midi_voice[voice].channel, midi_voice[voice].note);
		else
			return -1;
	}

	/* ok, we got it... */
	midi_voice[voice].channel = midi_alloc_channel;
	midi_voice[voice].note = midi_alloc_note;
	midi_voice[voice].volume = midi_alloc_vol;
	midi_voice[voice].time = _midi_tick;
	midi_channel[midi_alloc_channel].note[midi_alloc_note][layer] = voice;

	return voice + midi_driver->basevoice;
}

/* midi_note_on:
 *  Processes a MIDI note-on event. Tries to find a free soundcard voice,
 *  and if it can't either cuts off an existing note, or if 'polite' is
 *  set, just stores the channel, note and volume in the waiting list.
 */
static void midi_note_on(int channel, int note, int vol, int polite) {
	int c, layer, inst, bend, corrected_note;

	/* it's easy if the driver can handle raw MIDI data */
	if (midi_driver->raw_midi) {
		if (channel != 9)
			note += patch_table[midi_channel[channel].patch].pitch;

		midi_driver->raw_midi(0x90 + channel);
		midi_driver->raw_midi(note);
		midi_driver->raw_midi(vol);
		return;
	}
}

/* all_notes_off:
 *  Turns off all active notes.
 */
static void all_notes_off(int channel) {
	if (midi_driver->raw_midi) {
		midi_driver->raw_midi(0xB0 + channel);
		midi_driver->raw_midi(123);
		midi_driver->raw_midi(0);
		return;
	}
}

/* all_sound_off:
 *  Turns off sound.
 */
static void all_sound_off(int channel) {
	if (midi_driver->raw_midi) {
		midi_driver->raw_midi(0xB0 + channel);
		midi_driver->raw_midi(120);
		midi_driver->raw_midi(0);
		return;
	}
}

/* reset_controllers:
 *  Resets volume, pan, pitch bend, etc, to default positions.
 */
static void reset_controllers(int channel) {
	midi_channel[channel].new_volume = 128;
	midi_channel[channel].new_pitch_bend = 0x2000;

	if (midi_driver->raw_midi) {
		midi_driver->raw_midi(0xB0 + channel);
		midi_driver->raw_midi(121);
		midi_driver->raw_midi(0);
	}

	switch (channel % 3) {
	case 0:
		midi_channel[channel].pan = ((channel / 3) & 1) ? 60 : 68;
		break;
	case 1:
		midi_channel[channel].pan = 104;
		break;
	case 2:
		midi_channel[channel].pan = 24;
		break;
	}

	if (midi_driver->raw_midi) {
		midi_driver->raw_midi(0xB0 + channel);
		midi_driver->raw_midi(10);
		midi_driver->raw_midi(midi_channel[channel].pan);
	}
}

/* update_controllers:
 *  Checks cached controller information and updates active voices.
 */
static void update_controllers(void) {
	int c, c2, vol, bend, note;

	for (c = 0; c < 16; c++) {
		/* check for volume controller change */
		if ((midi_channel[c].volume != midi_channel[c].new_volume) || (old_midi_volume != _midi_volume)) {
			midi_channel[c].volume = midi_channel[c].new_volume;
			if (midi_driver->raw_midi) {
				midi_driver->raw_midi(0xB0 + c);
				midi_driver->raw_midi(7);
				midi_driver->raw_midi(global_volume_fix(midi_channel[c].volume - 1));
			}
		}

		/* check for pitch bend change */
		if (midi_channel[c].pitch_bend != midi_channel[c].new_pitch_bend) {
			midi_channel[c].pitch_bend = midi_channel[c].new_pitch_bend;
			if (midi_driver->raw_midi) {
				midi_driver->raw_midi(0xE0 + c);
				midi_driver->raw_midi(midi_channel[c].pitch_bend & 0x7F);
				midi_driver->raw_midi(midi_channel[c].pitch_bend >> 7);
			}
		}
	}

	old_midi_volume = _midi_volume;
}

/* process_controller:
 *  Deals with a MIDI controller message on the specified channel.
 */
static void process_controller(int channel, int ctrl, int data) {
	switch (ctrl) {

	case 7: /* main volume */
		midi_channel[channel].new_volume = data + 1;
		break;

	case 10: /* pan */
		midi_channel[channel].pan = data;
		if (midi_driver->raw_midi) {
			midi_driver->raw_midi(0xB0 + channel);
			midi_driver->raw_midi(10);
			midi_driver->raw_midi(data);
		}
		break;

	case 120: /* all sound off */
		all_sound_off(channel);
		break;

	case 121: /* reset all controllers */
		reset_controllers(channel);
		break;

	case 123: /* all notes off */
	case 124: /* omni mode off */
	case 125: /* omni mode on */
	case 126: /* poly mode off */
	case 127: /* poly mode on */
		all_notes_off(channel);
		break;

	default:
		if (midi_driver->raw_midi) {
			midi_driver->raw_midi(0xB0 + channel);
			midi_driver->raw_midi(ctrl);
			midi_driver->raw_midi(data);
		}
		break;
	}
}

/* process_meta_event:
 *  Processes the next meta-event on the specified track.
 */
static void process_meta_event(unsigned char **pos, long *timer) {
	unsigned char metatype = *((*pos)++);
	long length = parse_var_len(pos);
	long tempo;

	if (midi_meta_callback)
		midi_meta_callback(metatype, *pos, length);

	if (metatype == 0x2F) { /* end of track */
		*pos = NULL;
		*timer = LONG_MAX;
		return;
	}

	if (metatype == 0x51) { /* tempo change */
		tempo = (*pos)[0] * 0x10000L + (*pos)[1] * 0x100 + (*pos)[2];
      midi_new_speed = (tempo * TIMERS_PER_SECOND) / 1000000L;
		midi_new_speed /= midifile->divisions;
      // Log_Info("MIDI: tempo: %d, new speed: %d", tempo, midi_new_speed);
	}

	(*pos) += length;
}

/* process_midi_event:
 *  Processes the next MIDI event on the specified track.
 */
static void process_midi_event(unsigned char **pos, unsigned char *running_status, long *timer) {
	unsigned char byte1, byte2;
	int channel;
	unsigned char event;
	long l;

	event = *((*pos)++);

	if (event & 0x80) { /* regular message */
		/* no running status for sysex and meta-events! */
		if ((event != 0xF0) && (event != 0xF7) && (event != 0xFF))
			*running_status = event;
		byte1 = (*pos)[0];
		byte2 = (*pos)[1];
	} else { /* use running status */
		byte1 = event;
		byte2 = (*pos)[0];
		event = *running_status;
		(*pos)--;
	}

	/* program callback? */
	if ((midi_msg_callback) &&
		(event != 0xF0) && (event != 0xF7) && (event != 0xFF))
		midi_msg_callback(event, byte1, byte2);

	channel = event & 0x0F;

	switch (event >> 4) {

	case 0x08: /* note off */
		// Log_Info("MIDI: Note Off - Ch:%d Note:%d Vel:%d", channel, byte1, byte2);
		midi_note_off(channel, byte1);
		(*pos) += 2;
		break;

	case 0x09: /* note on */
		// Log_Info("MIDI: Note On - Ch:%d Note:%d Vel:%d", channel, byte1, byte2);
		midi_note_on(channel, byte1, byte2, 1);
		(*pos) += 2;
		break;

	case 0x0A: /* note aftertouch */
		// Log_Info("MIDI: Note Aftertouch - Ch:%d Note:%d Press:%d", channel, byte1, byte2);
		(*pos) += 2;
		break;

	case 0x0B: /* control change */
		// Log_Info("MIDI: Control Change - Ch:%d Ctrl:%d Val:%d", channel, byte1, byte2);
		process_controller(channel, byte1, byte2);
		(*pos) += 2;
		break;

	case 0x0C: /* program change */
		// Log_Info("MIDI: Program Change - Ch:%d Prog:%d", channel, byte1);
		midi_channel[channel].patch = byte1;
		if (midi_driver->raw_midi)
			raw_program_change(channel, byte1);
		(*pos) += 1;
		break;

	case 0x0D: /* channel aftertouch */
		// Log_Info("MIDI: Channel Aftertouch - Ch:%d Press:%d", channel, byte1);
		(*pos) += 1;
		break;

	case 0x0E: /* pitch bend */
		// Log_Info("MIDI: Pitch Bend - Ch:%d Val:%d", channel, byte1 + (byte2 << 7));
		midi_channel[channel].new_pitch_bend = byte1 + (byte2 << 7);
		(*pos) += 2;
		break;

	case 0x0F: /* special event */
		switch (event) {
		case 0xF0: /* sysex */
		case 0xF7:
			l = parse_var_len(pos);
			// Log_Info("MIDI: SysEx - Len:%ld", l);
			if (midi_sysex_callback)
				midi_sysex_callback(*pos, l);
			(*pos) += l;
			break;

		case 0xF2: /* song position */
			// Log_Info("MIDI: Song Position - Pos:%d", byte1 + (byte2 << 7));
			(*pos) += 2;
			break;

		case 0xF3: /* song select */
			// Log_Info("MIDI: Song Select - Song:%d", byte1);
			(*pos)++;
			break;

		case 0xFF: /* meta-event */
			// Log_Info("MIDI: Meta-Event");
			process_meta_event(pos, timer);
			break;

		default:
			/* the other special events don't have any data bytes,
		   so we don't need to bother skipping past them */
			// Log_Info("MIDI: Special Event 0x%02X", event);
			break;
		}
		break;

	default:
		/* something has gone badly wrong if we ever get to here */
		// Log_Info("MIDI: Unknown event type 0x%02X", event >> 4);
		break;
	}
}

/* midi_player:
 *  The core MIDI player: to be used as a timer callback.
 */
static void midi_player(void) {
	int c;
	long l;
	int active;

	if (!midifile) return;

	if (midi_semaphore) {
		midi_timer_speed += BPS_TO_TIMER(MIDI_TIMER_FREQUENCY);
		install_int_ex(midi_player, BPS_TO_TIMER(MIDI_TIMER_FREQUENCY));
		return;
	}

	midi_semaphore = true;
	_midi_tick++;

	midi_timers += midi_timer_speed;
	midi_time = midi_timers / TIMERS_PER_SECOND;

do_it_all_again:

	// Log_Info("=== MIDI Player Tick %ld, timer_speed=%ld ===", _midi_tick, midi_timer_speed);

	for (c = 0; c < MIDI_VOICES; c++)
		midi_waiting[c].note = -1;

	/* deal with each track in turn... */
	for (c = 0; c < MIDI_TRACKS; c++) {
		if (midi_track[c].pos) {
			midi_track[c].timer -= midi_timer_speed;
			// Log_Info("Track %d: timer now = %ld", c, midi_track[c].timer);

			/* while events are waiting, process them */
			while (midi_track[c].timer <= 0) {
				process_midi_event((unsigned char **)&midi_track[c].pos,
				   				    &midi_track[c].running_status,
					   			    &midi_track[c].timer);

				/* read next time offset */
				if (midi_track[c].pos) {
					l = parse_var_len((unsigned char **)&midi_track[c].pos);
					// Log_Info("Track %d: Next delta time = %ld", c, l);
					l *= midi_speed;
					midi_track[c].timer += l;
				}
			}
		}
	}

	/* update global position value */
	midi_pos_counter -= midi_timer_speed;

	while (midi_pos_counter <= 0) {
		midi_pos_counter += midi_pos_speed;
		midi_pos++;
	}

	/* tempo change? */
	if (midi_new_speed > 0) {
		for (c = 0; c < MIDI_TRACKS; c++) {
			if (midi_track[c].pos) {
				midi_track[c].timer /= midi_speed;
				midi_track[c].timer *= midi_new_speed;
			}
		}
		midi_pos_counter /= midi_speed;
		midi_pos_counter *= midi_new_speed;

		midi_speed = midi_new_speed;
		midi_pos_speed = midi_new_speed * midifile->divisions;
		midi_new_speed = -1;
	}

	/* figure out how long until we need to be called again */
   active = 0;
   midi_timer_speed = LONG_MAX;
	for (c = 0; c < MIDI_TRACKS; c++) {
		if (midi_track[c].pos) {
			active = 1;
			if (midi_track[c].timer > 0 && midi_track[c].timer < midi_timer_speed)
					midi_timer_speed = midi_track[c].timer;
		}
	}
   // Log_Info("MIDI: timer_speed: %d", midi_timer_speed);

	/* end of the music? */
	if ((!active) || ((midi_loop_end > 0) && (midi_pos >= midi_loop_end))) {
		if ((midi_loop) && (!midi_looping)) {
			if (midi_loop_start > 0) {
				remove_int(midi_player);
				midi_semaphore = false;
				midi_looping = true;
				if (midi_seek(midi_loop_start) != 0) {
					midi_looping = false;
					stop_midi();
					return;
				}
				midi_looping = false;
				midi_semaphore = true;
				goto do_it_all_again;
			} else {
				for (c = 0; c < 16; c++) {
					all_notes_off(c);
					all_sound_off(c);
				}
				prepare_to_play(midifile);
				goto do_it_all_again;
			}
		} else {
			stop_midi();
			midi_semaphore = false;
			return;
		}
	}

	/* reprogram the timer */
	if (midi_timer_speed < BPS_TO_TIMER(MIDI_TIMER_FREQUENCY))
		midi_timer_speed = BPS_TO_TIMER(MIDI_TIMER_FREQUENCY);

	if (!midi_seeking)
		install_int_ex(midi_player, midi_timer_speed);

	/* controller changes are cached and only processed here, so we can
	   condense streams of controller data into just a few voice updates */
	update_controllers();

	/* and deal with any notes that are still waiting to be played */
	for (c = 0; c < MIDI_VOICES; c++) {
		if (midi_waiting[c].note >= 0)
			midi_note_on(midi_waiting[c].channel, midi_waiting[c].note,
						    midi_waiting[c].volume, 0);
   }

	midi_semaphore = false;
}

/* midi_init:
 *  Sets up the MIDI player ready for use. Returns non-zero on failure.
 */
int midi_init(MIDI_DRIVER* driver) {
	int c, c2, c3;
	char **argv;
	int argc;
	char buf[32], tmp[64];

	patches_loaded = false;
	midi_driver = driver;

	for (c = 0; c < 16; c++) {
		midi_channel[c].volume = midi_channel[c].new_volume = 128;
		midi_channel[c].pitch_bend = midi_channel[c].new_pitch_bend = 0x2000;

		for (c2 = 0; c2 < 128; c2++)
			for (c3 = 0; c3 < MIDI_LAYERS; c3++)
				midi_channel[c].note[c2][c3] = -1;
	}

	for (c = 0; c < MIDI_VOICES; c++) {
		midi_voice[c].note = -1;
		midi_voice[c].time = 0;
	}

	for (c = 0; c < 128; c++) {
		patch_table[c].bank1 = -1;
		patch_table[c].bank2 = -1;
		patch_table[c].prog = c;
		patch_table[c].pitch = 0;
	}

	return 0;
}

/* midi_exit:
 *  Turns off all active notes and removes the timer handler.
 */
void midi_exit(void) {
	stop_midi();
}

/* load_patches:
 *  Scans through a MIDI file and identifies which patches it uses, passing
 *  them to the soundcard driver so it can load whatever samples are
 *  neccessary.
 */
int load_patches(MIDI *midi) {
	char patches[128], drums[128];
	unsigned char *p, *end;
	unsigned char running_status, event;
	long l;
	int c;

	for (c = 0; c < 128; c++) /* initialise to unused */
		patches[c] = drums[c] = false;

	patches[0] = true; /* always load the piano */

	for (c = 0; c < MIDI_TRACKS; c++) { /* for each track... */
		p = midi->track[c].data;
		end = p + midi->track[c].len;
		running_status = 0;

		while (p < end) { /* work through data stream */
			event = *p;
			if (event & 0x80) { /* regular message */
				p++;
				if ((event != 0xF0) && (event != 0xF7) && (event != 0xFF))
					running_status = event;
			} else /* use running status */
				event = running_status;

			switch (event >> 4) {

			case 0x0C: /* program change! */
				patches[*p] = true;
				p++;
				break;

			case 0x09: /* note on, is it a drum? */
				if ((event & 0x0F) == 9)
					drums[*p] = true;
				p += 2;
				break;

			case 0x08: /* note off */
			case 0x0A: /* note aftertouch */
			case 0x0B: /* control change */
			case 0x0E: /* pitch bend */
				p += 2;
				break;

			case 0x0D: /* channel aftertouch */
				p += 1;
				break;

			case 0x0F: /* special event */
				switch (event) {
				case 0xF0: /* sysex */
				case 0xF7:
					l = parse_var_len((unsigned char **)&p);
					p += l;
					break;

				case 0xF2: /* song position */
					p += 2;
					break;

				case 0xF3: /* song select */
					p++;
					break;

				case 0xFF: /* meta-event */
					p++;
					l = parse_var_len((unsigned char **)&p);
					p += l;
					break;

				default:
					/* the other special events don't have any data bytes,
				   so we don't need to bother skipping past them */
					break;
				}
				break;

			default:
				/* something has gone badly wrong if we ever get to here */
				break;
			}

			if (p < end) /* skip time offset */
				parse_var_len((unsigned char **)&p);
		}
	}

	/* tell the driver to do its stuff */
	return midi_driver->load_patches(patches, drums);
}

/* prepare_to_play:
 *  Sets up all the global variables needed to play the specified file.
 */
static void prepare_to_play(MIDI *midi) {
	int c;
	ASSERT(midi);

	for (c = 0; c < 16; c++)
		reset_controllers(c);

	update_controllers();

	midifile = midi;
	midi_pos = 0;
	midi_timers = 0;
	midi_time = 0;
	midi_pos_counter = 0;

   if (midifile->divisions >= TIMERS_PER_SECOND / 2) {
      midi_speed = 1; // set minimum for safety
   } else {
      midi_speed = TIMERS_PER_SECOND / 2 / midifile->divisions;
   }

   // Log_Info("MIDI: speed @ %d", midi_speed);

	midi_new_speed = -1;
	midi_pos_speed = midi_speed * midifile->divisions;
	midi_timer_speed = 0;
	midi_seeking = 0;
	midi_looping = 0;

	for (c = 0; c < 16; c++) {
		midi_channel[c].patch = 0;
		if (midi_driver->raw_midi)
			raw_program_change(c, 0);
	}

	for (c = 0; c < MIDI_TRACKS; c++) {
		if (midi->track[c].data) {
			midi_track[c].pos = midi->track[c].data;
			midi_track[c].timer = parse_var_len((unsigned char **)&midi_track[c].pos);
			// Log_Info("Track %d: Initial delta time = %ld", c, midi_track[c].timer);
			midi_track[c].timer *= midi_speed;
			// Log_Info("Track %d: After speed multiply = %ld (speed=%ld)", c, midi_track[c].timer, midi_speed);
		} else {
			midi_track[c].pos = NULL;
			midi_track[c].timer = LONG_MAX;
		}
		midi_track[c].running_status = 0;
	}
}

/* play_midi:
 *  Starts playing the specified MIDI file. If loop is set, the MIDI file
 *  will be repeated until replaced with something else, otherwise it will
 *  stop at the end of the file. Passing a NULL MIDI file will stop whatever
 *  music is currently playing: allegro.h defines the macro stop_midi() to
 *  be play_midi(NULL, false); Returns non-zero if an error occurs (this
 *  may happen if a patch-caching wavetable driver is unable to load the
 *  required samples).
 */
int play_midi(MIDI *midi, int loop) {
	int c;
	remove_int(midi_player);

	for (c = 0; c < 16; c++) {
		all_notes_off(c);
		all_sound_off(c);
	}

	if (midi) {
		if (!patches_loaded && !load_patches(midi))
			 Log_Info("MIDI: No patches loaded");

      midi_loop = loop;
		midi_loop_start = -1;
		midi_loop_end = -1;

		prepare_to_play(midi);
		install_int(midi_player, 20);
	} else {
		midifile = NULL;
		if (midi_pos > 0)
			midi_pos = -midi_pos;
		else if (midi_pos == 0)
			midi_pos = -1;
      
	}

	return 0;
}

/* play_looped_midi:
 *  Like play_midi(), but the file loops from the specified end position
 *  back to the specified start position (the end position can be -1 to
 *  indicate the end of the file).
 */
int play_looped_midi(MIDI *midi, int loop_start, int loop_end) {
	if (play_midi(midi, true) != 0) return -1;

	midi_loop_start = loop_start;
	midi_loop_end = loop_end;

	return 0;
}

/* stop_midi:
 *  Stops whatever MIDI file is currently playing.
 */
void stop_midi(void) {
	play_midi(NULL, false);
}

/* midi_pause:
 *  Pauses the currently playing MIDI file.
 */
void midi_pause(void) {
	int c;

	if (!midifile) return;
	remove_int(midi_player);

	for (c = 0; c < 16; c++) {
		all_notes_off(c);
		all_sound_off(c);
	}
}

/* midi_resume:
 *  Resumes playing a paused MIDI file.
 */
void midi_resume(void) {
	if (!midifile) return;
	install_int_ex(midi_player, midi_timer_speed);
}

int midi_seek(long target)
{
   MIDI *old_midifile;
   int old_patch[16];
   int old_volume[16];
   int old_pan[16];
   int old_pitch_bend[16];
   int c;

   if (!midifile) return -1;

   /* first stop the player */
   midi_pause();

   /* store current settings */
   for (c=0; c<16; c++) {
      old_patch[c] = midi_channel[c].patch;
      old_volume[c] = midi_channel[c].volume;
      old_pan[c] = midi_channel[c].pan;
      old_pitch_bend[c] = midi_channel[c].pitch_bend;
   }

   old_midifile = midifile;

   /* set flag to tell midi_player not to reinstall itself */
   midi_seeking = 1;

   /* are we seeking backwards? If so, skip back to the start of the file */
   if (target <= midi_pos)
      prepare_to_play(midifile);

   /* now sit back and let midi_player get to the position */
   while ((midi_pos < target) && (midi_pos >= 0)) {
      int mmpc = midi_pos_counter;
      int mmp = midi_pos;

      mmpc -= midi_timer_speed;
      while (mmpc <= 0) {
         mmpc += midi_pos_speed;
         mmp++;
      }

      if (mmp >= target)
	 break;

      midi_player();
   }

   /* restore previously saved variables */
   midi_seeking = 0;

   if (midi_pos >= 0) {
      /* refresh the driver with any changed parameters */
      if (midi_driver->raw_midi) {
			for (c=0; c<16; c++) {
				/* program change (this sets the volume as well) */
				if ((midi_channel[c].patch != old_patch[c]) || (midi_channel[c].volume != old_volume[c]))
					raw_program_change(c, midi_channel[c].patch);

				/* pan */
				if (midi_channel[c].pan != old_pan[c]) {
					midi_driver->raw_midi(0xB0+c);
					midi_driver->raw_midi(10);
					midi_driver->raw_midi(midi_channel[c].pan);
				}

				/* pitch bend */
				if (midi_channel[c].pitch_bend != old_pitch_bend[c]) {
					midi_driver->raw_midi(0xE0+c);
					midi_driver->raw_midi(midi_channel[c].pitch_bend & 0x7F);
					midi_driver->raw_midi(midi_channel[c].pitch_bend >> 7);
				}
			}
		}

      /* if we didn't hit the end of the file, continue playing */
      if (!midi_looping) install_int(midi_player, 20);

      return 0;
   }

   if ((midi_loop) && (!midi_looping)) {  /* was file looped? */
      prepare_to_play(old_midifile);
      install_int(midi_player, 20);
      return 2;                           /* seek past EOF => file restarted */
   }

   return 1;                              /* seek past EOF => file stopped */
}

/* get_midi_length:
 *  Returns the length, in seconds, of the specified midi. This will stop any
 *  currently playing midi. Don't call it too often, since it simulates playing
 *  all of the midi to get the time even if the midi contains tempo changes.
 */
int get_midi_length(MIDI *midi) {
	play_midi(midi, 0);
	while (midi_pos < 0); /* Without this, midi_seek won't work. */
	midi_seek(INT_MAX);
	return midi_time;
}

/* midi_out:
 *  Inserts MIDI command bytes into the output stream, in realtime.
 */
void midi_out(unsigned char *data, int length) {
	unsigned char *pos = data;
	unsigned char running_status = 0;
	long timer = 0;
	ASSERT(data);

	midi_semaphore = true;
	_midi_tick++;

	while (pos < data + length)
		process_midi_event((unsigned char **)&pos, &running_status, &timer);

	update_controllers();

	midi_semaphore = false;
}

/* load_midi_patches:
 *  Tells the MIDI driver to preload the entire sample set.
 */
int load_midi_patches(void) {
	char patches[128], drums[128];
	int c, ret;

	for (c = 0; c < 128; c++)
		patches[c] = drums[c] = true;

	midi_semaphore = true;
	ret = midi_driver->load_patches(patches, drums);
	midi_semaphore = false;

	patches_loaded = true;

	return ret;
}
