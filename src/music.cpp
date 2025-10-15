#include <dos.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "inc/logger.h"
#include "inc/types.h"
#include "inc/timer.h"
#include "inc/midi.h"
#include "awe/awe32drv.h"
#include "gus/gusdrv.h"
#include "inc/music.hpp"

// =============================================================================
// MIDI MUSIC SYSTEM
// =============================================================================

bool Music::_enabled = false;
bool Music::_playing = false;
char Music::_currentSong[16] = { 0 };

MIDI_DRIVER* Music::_midiDriver = NULL;

bool Music::Init(int hw_type) {
	bool init_success = false;
	if (_enabled) return true;

	switch (hw_type) {
	case USE_AWE:
		/* check for AWE32 */
		_midiDriver = get_awe32_midi_driver();
		if (_midiDriver->detect(0x620))
			if (!_midiDriver->init()) 
				Log_Info("AWE32 initialization failed");
			else
				init_success = true;
		else
			Log_Info("AWE32 not detected");
		break;
	case USE_GUS:
		/* check for GUS */
		_midiDriver = get_gus_midi_driver();
		if (_midiDriver->detect(NULL)) {
			if (!_midiDriver->init()) 
				Log_Info("GUS initialization failed");
			else
				init_success = true;
		} else {
			Log_Info("GUS not detected");
		}
		break;
	default:
		Log_Info("Music: unknown device");
		break;
	}

	if (init_success && midi_init(_midiDriver) == 0) {
		Log_Info("Music System initialized");
		_enabled = true;
	}

	return _enabled;
}

void Music::Dispose() {
	if (!_enabled) return;
	stop_midi();
	midi_exit();
	_midiDriver->exit();
	_enabled = false;
}

bool Music::PlayMIDI(char *filename) {
	if (!_enabled) return false;
	char filepath[256] = "assets\\";
   strcat(filepath, filename);
   strcpy(_currentSong, filename);

	MIDI *midi = load_midi(filepath);
	if (!midi) {
		Log_Info("Failed to load MIDI: %s", filepath);
		return false;
	}

	// Log_Info("Playing MIDI: %s", filename);
	_playing = (play_midi(midi, false) == 0);
	return _playing;
}

bool Music::PlayMIDILoop(char* filename) {
   if (!_enabled) return false;
	MIDI *midi = load_midi(filename);
	if (!midi) return false;

	return (play_midi(midi, true) == 0);
}

void Music::StopMIDI() {
   if (!_enabled) return;
	_playing = false;
	stop_midi();
}

bool Music::IsPlaying() {
	return _playing;
}

char* Music::LastSong() {
   return _currentSong;
}