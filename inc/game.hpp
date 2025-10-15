#ifndef GAME_HPP
#define GAME_HPP
#include "inc/types.h"
#include "inc/fileio.h"
#include "inc/draw.hpp"

typedef enum {
	Enter			= 0,
	Escape		= 1,
	Restart		= 2,
	NextLevel	= 3,
	JumpLevel	= 4
} Action;

struct Taunt {
	char txt[32];
	char smpl[16];
	int sco;
};

class Game {
	private:
		// props
      static bool _roundFinished;
      static bool _openExit;
      static bool _gateOpened;
      static bool _extraLife;
		static bool _showEnemy;
		static int _round;
		static int _score;
		static int _lifes;
      static int _nextLifeAt;
		static int _hspos;
		static int _nitroUsed;
		static int _defaultTicks;
		static int _currentTicks;
		static int _frameTicks;
		static int _midiTicks;
		static int _maxNitro;
		static Taunt _taunts[8];
		static Taunt _congrats[10];
      static HighScores _highscores;
		static ColorCycle* _alerts[6];
		/* init game */
		static void levelControl(Action status);
      /* in-game */
		static bool handleScores(int pts);
		static int  handleTurbo();
		static byte handleCollision(int dir_x, int dir_y);
      static void deviceControl(int dir_x, int dir_y);
		/* round 5 */
		static void handleAlerts(byte object);
		static void alertCycles();
		/* after game */
      static void over(bool hit_escape);
		static void roundFinished();
		static void checkScoreboard();
      static HighScores newHighscore(HighScores highscores);
      static char* newHighscoreBox();
      static char* getPlayerHandle(int x, int y, byte color);
		static char wait(char* play_jingle);
	public:
		/* pre game */
		static void Intro();
      static Action MainScreen();
      /* init game */
		static void Start(Action status);
      static Action Play();
      /* end of game */
		static void Exit();
		static void Dispose();
};

#endif