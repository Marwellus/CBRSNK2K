#ifndef SOUND_HPP
#define SOUND_HPP

#define MAX_CHANNELS 18
#define MAX_AUDIO_EVENTS 32
#define PLAY_LIMIT 128

typedef struct {
   uint next_tick;
   byte channel;
   word frequencies[PLAY_LIMIT];
   word durations[PLAY_LIMIT];
   word count;
   word played;
   bool active;
} AudioEvent;

// for later use
typedef struct {
    byte operators[4][6];  // AM,VIB,EGT,KSR,MULT
    byte levels[4];        // output Levels
    byte envelopes[4][4];  // AR,DR,SL,RR
    byte waveforms[4];     // waveform select
    byte connection;       // algorithm
    byte feedback;
} OPL3Instrument;

class Sound {
   private:
      static bool _oplReady;
		static bool _dspReady;
		static bool _midiReady;
      static int _activeChannel[MAX_CHANNELS];
      static AudioEvent _audioQueue[MAX_AUDIO_EVENTS];
      static void queueSequence(word freqs[], word durations[], word count, uint current_tick);
	public:
      static void Init(int hw_type, bool midi);
      static void ProcessQueue(uint t_sound_tick);
      static void ClearQueue();
      
		static void Beep(word freq, word ms, uint tick);
		static void Booep(word from, word to, uint tick);      
		static void HealthyBeep(uint tick);
      static void GoldyBeep(uint tick);
		static void SickBeep(uint tick);
		static void Burp(uint tick);
		static void HitWallBeep(uint tick);
		static void OuchBeep(uint tick);
		static void Siren(word freq, bool turn_off);
      
      /* debug/test */
      static void TestBeep();

      /* used outside game loop */
      static void Beep(word freq, word ms);
      static void Booep(word from, word to);
		static void OuchBeep();
		static void HitWallBeep();
      
      static void Dispose();
};

#endif