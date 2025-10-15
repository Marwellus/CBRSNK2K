#ifndef MUSIC_HPP
#define MUSIC_HPP

#define USE_AWE 0
#define USE_GUS 1

struct MIDI;
struct MIDI_DRIVER;

class Music {
private:
   static bool _enabled;
   static bool _playing;
   static char _currentSong[16];
   static MIDI_DRIVER* _midiDriver;
public:
    static bool Init(int hw_type);
    static void Dispose();
    static bool PlayMIDI(char* filename);
    static bool PlayMIDILoop(char* filename);
    static void StopMIDI();
    static bool IsPlaying();
    static char* LastSong();
};

#endif