#include <bios.h>
#include <dos.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <i86.h>
#include "inc/logger.h"		 // log buffer
#include "inc/types.h"      // bools, bytes, words etc.
#include "inc/timer.h"      // interrupt, t_game_tick, t_cater_tick, t_sound_tick
#include "inc/fileio.h"     // load / safe highscores
#include "inc/dsp.h"			 // play samples
#include "inc/miditmr.h"	 // midi ticker
#include "inc/sound.hpp"    // beeps and boobs
#include "inc/music.hpp"    // midi control
#include "inc/draw.hpp"     // draws stuff
#include "inc/creaturs.hpp" // Snake & Cater
#include "inc/game.hpp"

HighScores Game::_highscores;
int Game::_round = 1;
int Game::_score = 0;
int Game::_lifes = 3;
int Game::_hspos = 0;
int Game::_nextLifeAt = 50;

bool Game::_roundFinished = false;
bool Game::_openExit = false;
bool Game::_gateOpened = false;
bool Game::_extraLife = false;

int Game::_nitroUsed = 0;
int Game::_maxNitro = 20; // number of ticks
int Game::_defaultTicks = TICKS_100MS;	// level dependend
int Game::_currentTicks = TICKS_100MS;	// base speed of the snake
int Game::_frameTicks = 27; // 35 fps (960hz / 27)

ColorCycle* Game::_alerts[6] = { 0 };

static Snake _snake;
static Cater _cater;
// static Boss _boss;

Taunt Game::_taunts[8] = {
	{ "Coward!", "\0", 0 },
	{ "*Almost*", "\0", 120 },
	{ "Seriously?", "\0", 90 },
	{ "Well played - not!", "\0", 75 },
	{ "Oops, genius!", "\0", 60 },
	{ "Bonehead!", "\0", 45 },
	{ "Goofball!", "\0", 30 },
	{ "LOL?!", "\0", 0 }
};

Taunt Game::_congrats[10] = {
	{ "Boom, ERROR!", 0, 0 },
	{ "Laughable!", "laugh.wav", 0 },
	{ "Piece of cake!", "cake.wav", 0 },
	{ "Booooring!", "boring.wav", 0 },
	{ "Beginners luck!", "bluck.wav", 0 },
	{ "You must not win!", "notwin.wav", 0 },
	{ "This cannot be!", "cannotbe.wav", 0 },
	{ "You made it!", "madeit.wav",  0 },
	{ 0,0,0 },
	{ 0,0,0 }
};

// ============================================================================
// pre-game
// ============================================================================

void Game::Intro() {
   if (!__DEBUG) {
		DSP_Play("watcom.wav", false);
		Draw::ImageTransition("watcom.pcx", FadeOut);
   	delay(4000);
		DSP_Play("retronom.wav", false);
		Draw::ImageTransition("retronom.pcx", FadeOut);
		delay(4000);

	   DSP_Play("cbrsnktt.wav", true);
	   Draw::GameIntro();
   }
}

Action Game::MainScreen() {
	delay(50); while(keyboard(SIGNAL)) keyboard(READ);
   int show_screen = 1; char key = 0; bool run = true;
   t_snake_tick = 0;
   
   checkScoreboard();
   Music::PlayMIDI("cbrsnktt.mid");

	// fade transition from previous screen
   Draw::HallOfFame(_highscores, _hspos, Rotate);

	while (run) {
		if (t_midi_tick >= MIDI_Interval) {
			midi_timer_tick();
			t_midi_tick = 0;
		}

		if (t_snake_tick > 8000) {
			t_snake_tick = 0;
			switch (show_screen)	{
			case 0:
				Draw::HallOfFame(_highscores, _hspos, Interlaced);
				show_screen = 1;
				break;
			case 1:
				Draw::HowtoControl(Interlaced);
				show_screen = 0;
				break; 
			default:
				break;
			}
		}

		if (keyboard(SIGNAL)) {
			key = keyboard(READ);
			if (key == 27 || key == 13) run = false;
		}

		delay(1);
	}

	Music::StopMIDI();
	return key == 27 ? Escape : Enter;
}

// ============================================================================
// init game
// ============================================================================

void Game::Start(Action status) {
	while(keyboard(SIGNAL)) keyboard(READ);
   TransitionType type = Slide;
   char filename[24];

	if (status != Restart) DSP_Play("swoosh3.wav", false);

   if (status == Enter) {
		_score = 0; _round = 1; 
		_lifes = !__DEBUG ? 3 : 5; 
		_nextLifeAt = 50;
		
      Draw::HowtoPlay();
      wait("inter.mid");
      type = Interlaced;
   }

   if (status != Restart) {
      sprintf(filename, "LEVEL0%d.GIF", _round);
      Draw::InitRound(filename, type);
   }

	levelControl(status);
}

void Game::levelControl(Action status) {
   // ressurect snake
   if (status == Restart) {
   	_snake.Create();
      Music::PlayMIDI(Music::LastSong());
      DSP_Play("liftback.wav", false);
      Field::EntryGate(OPEN);
      return;
   }

	_roundFinished = false;
   _gateOpened = false;
   _nitroUsed  = 0;

	Field::Create(_round);
	// _boss.Reset();
   _cater.Reset();
   _snake.Create();

	word n = 0;
   Field::GatesMuted = true;

   switch(_round) {
   case 1:
      _defaultTicks = TICKS_100MS;
      Field::ScatterFood(20,1);		
      Music::PlayMIDI("LEVEL01.MID");		
      break;
   case 2:
      _defaultTicks = TICKS_100MS - 8;
      Field::PlaceObstacles();
      Field::ScatterFood(30,15);
      Music::PlayMIDI("LEVEL02.MID");
      break;
   case 3:
      _defaultTicks = TICKS_100MS - 12;
      Field::ScatterFood(45,15);
      _cater.Create(); // closes exit gate
      Music::PlayMIDI("LEVEL03.MID");
      break;
   case 4:
      _defaultTicks = TICKS_100MS - 4;
      Field::ScatterFood(40,15);
      Music::PlayMIDI("LEVEL04.MID");
      break;
   case 5:
      _defaultTicks = TICKS_100MS - 4;
		for (n=0;n<6;n++) _alerts[n] = NULL;
	   Field::ExitGate(CLOSE);
      Music::PlayMIDI("LEVEL05.MID");
		// _boss.Create();
      break;
   default:
      Field::ScatterFood(5,0);
      break;
   }

	Field::GatesMuted = false;
}

// ============================================================================
// in-game
// ============================================================================

Action Game::Play() {
   Draw::SwitchRenderMode();	// render to to Buffer
	t_midi_speed = 1.0;			// speed up midi a bit
   Game_Timer_Reset();

	_currentTicks = _defaultTicks;
   bool update_buffer = true;
	word key_code 		 = 0;

	int dir_x = 1, dir_y = 0;
   int chg_x = 1, chg_y = 0;
   byte object = 0x00;
   
	int effect_length = 100;
   int flash_counter = 0;
   uint next_tick 	= 0;
   
	bool entry_closed = false;
   bool close_entry 	= false;
   bool show_effect 	= false;
	bool init_stop  	= false;
   bool stop_snake 	= false;
   bool stop 		 	= false;
   
   /* in-game main loop */
	while(!stop) 
	{
		/* default logics */
		if (t_game_tick >= _defaultTicks) {
			t_game_tick = 0;

         if (!_roundFinished) Draw::DoorIndicator(false);
         if (rand() % 100 > 90) Field::MoveRandomApple(t_sound_tick);

			/* round 3 enemy */
			if (_round == 3) {
				if (!_cater.IsReady) {
					int pos = (_cater.Tail->y * 320 + _cater.Tail->x);
					_cater.IsReady = Field::LevelMap[pos] == GATE_TRIGGER;
					_cater.Move(-1,0); // move into level-area first
					if (_cater.IsReady) Field::ExitGate(CLOSE);
				} else {
					_cater.Control(t_sound_tick);
				}
			}
		}

		/* snake logics */
		if (t_snake_tick >= _currentTicks) { 
			t_snake_tick = 0;

			// switches & doors
         deviceControl(chg_x, chg_y);

         if (!stop_snake) _snake.Move(chg_x, chg_y);
         if (!(object = handleCollision(chg_x, chg_y))) {
            Sound::Beep(300, 8, t_sound_tick); 
			}

			// check shift (turbo) key => speed up snake
			if (object != SLOW_GROUND) _currentTicks = handleTurbo();
         Draw::NitroGauge(_maxNitro, _nitroUsed);

         dir_x = chg_x; dir_y = chg_y;			
         stop = _snake.IsDead || _snake.MadeIt || init_stop;

			// close entry gate after snake passes
         if (close_entry) { 
            DSP_Play("liftdoor.wav", false); Field::EntryGate(CLOSE);
            entry_closed = true; close_entry = false;
         } else if (!entry_closed) {
            int pos = (_snake.Tail->y * 320 + (_snake.Tail->x));
            if (Field::LevelMap[pos] == LEFT_GATE) close_entry = true;
         }

			// snake ate purple apple
			if (_snake.IsDrunk && show_effect) {
				effect_length += 100;
			} else if (_snake.IsDrunk) {
				Draw::ToggleEffect();
				show_effect = true;
			}

			// effect conditions
			if (stop || effect_length <= 0 || object == RGHT_GATE) {
				if (show_effect) Draw::ToggleEffect();
				show_effect = false; effect_length = 100;
			} else if (show_effect) {
				effect_length--;
			}

         _lifes -= _snake.IsDead;
			_snake.IsDrunk = false;

			// indicate things changed
         update_buffer = true;
		}

		/* 35 frames per sec */
      if (t_frame_tick >= _frameTicks) {
			t_frame_tick = 0;

			// round 5 alert systems
			if (_round == 5) alertCycles();

			// update screen
			SNKStats stats = { _round, _lifes, _score };
			_extraLife = _extraLife && flash_counter < 120;
         if (_extraLife) flash_counter++; else flash_counter = 0;

			if (update_buffer) {
				Field::DrawMap();
				_snake.DrawSelf(); 
				_cater.DrawSelf();
				// _boss.DrawToBuffer();
			}			
			t_measure_tick = 0;
         Draw::UpdateScreen(stats, _extraLife, update_buffer);
			update_buffer = false;
      }

		/* sound & midi */
		if (t_sound_tick > next_tick) {
         next_tick = t_sound_tick;
         Sound::ProcessQueue(t_sound_tick);
      }
		if (t_midi_tick >= MIDI_Interval) {
			midi_timer_tick();
			t_midi_tick = 0;
		}

		/* check BIOS keyboard signals */
		if (keyboard(SIGNAL)) {
			key_code = keyboard(READ);
			switch (key_code & 0xFF) {
			case 0:
				switch (key_code >> 8) {
				case 75: // left
					if (dir_x != 1) { chg_x=-1; chg_y=0; }
					break;
				case 77: // right
					if (dir_x != -1) { chg_x=1; chg_y=0; }
					break;
				case 72: // up
					if (dir_y != 1) { chg_x=0; chg_y=-1; }
					break;
				case 80: // down
					if (dir_y != -1) { chg_x=0; chg_y=1; }
					break;
				// debug
				case 25:  // alt + "p"
					stop_snake = stop_snake ? false : true;
					break;
				case 120: case 121: case 122: case 123: // ALT + "1" etc
				case 124: case 125: case 126: case 127:
					init_stop = true;
					break;
				}
				break;
			case 27: // Escape
				init_stop = true;
				break;
			}
		}
	}
   /* in-game main loop END */

	Music::StopMIDI(); 
   Sound::ClearQueue();
   Draw::SwitchRenderMode();
   Draw::DoorIndicator(true);

	t_midi_speed = MIDI_DEFAULT_SPEED;
	while(keyboard(SIGNAL)) keyboard(READ);

	if (_snake.IsDead) {
      if (_lifes <= 0) {
			DSP_Play("evilaugh.wav", false);
         Draw::MessageBox("Rien ne va plus, sucker!");
         wait("deadsnke.mid");
      } else {
         Game::over(false);
         _snake.Remove();
         return Restart;
      }
	} else if (_snake.MadeIt) {
		Game::roundFinished(); _round++;
		char key = wait("madeit.mid");
      if (key == 13) return NextLevel;
	} else if ((key_code >> 8) > 119) {
		_round = (key_code >> 8) - 119;
		return JumpLevel;
	} else {
		Game::over(true);
	}

	// new round or end game
	return MainScreen();
}

// ============================================================================
// in-game methods
// ============================================================================

int Game::handleTurbo() {
	static int turbo = 1;

	// left/right SHIFT key
	int boost = (keyboard(SPECIAL) & 0x03) ? 1 : 0;

	if (boost && turbo && _nitroUsed < _maxNitro) {
		_nitroUsed++;
		return _defaultTicks - 24;
	}

	turbo = _nitroUsed <= 0 ? 1 : 0;
	_nitroUsed -= _nitroUsed <= 0 ? 0 : 1;
	return _defaultTicks;
}

void Game::deviceControl(int dir_x, int dir_y) {
	static int ticks = 0;
	int check_pos = ((_snake.Head->y+(dir_y*2))*320)
						 + _snake.Head->x+(dir_x*2);

   byte object   = Field::LevelMap[check_pos];

	// block triggering for two ticks
	if (ticks > 0 && ticks <= 2) { ticks++; return; } else ticks = 0;

   if (object >= 0x41 && object <= 0x49) {
		Field::ToggleDevice(object); ticks++;
	}

   if (_openExit && Field::DeviceState(Field::SwitchCodes[0])) {
      _openExit = false; _gateOpened = true;
   } else if (_gateOpened && !Field::DeviceState(Field::SwitchCodes[0])) {
      _openExit = true; _gateOpened = false;
   }
}

byte Game::handleCollision(int dir_x, int dir_y) {
   // biting hurts
	if (_cater.HasBeenBitten(*_snake.Head)
		 || _snake.HasBeenBitten(*_cater.Head)
		 || _snake.BitItself()) {
		_snake.IsDead = true;
		Sound::OuchBeep();
		return true;
	}

	// bumping into stuff
	int obj_pos = (_snake.Head->y*320)+_snake.Head->x;
   byte& object = Field::LevelMap[obj_pos];

	/* zones that trigger alerts */
	if (_round == 5) handleAlerts(object);

	switch (object) {
      case RGHT_GATE:
         // just return code      
         return object; 
		case SLOW_GROUND:
         // slow down snake in special areas
			_currentTicks = TICKS_100MS;
			return object;
		case RED_APPLE:
		case SPECIAL_ITEM:
         // normal "cyberegg", 1pt  / some special item
			object = TILE_EMPTY;
         Sound::HealthyBeep(t_sound_tick);
         Field::UpdateApple(_snake.Head->x, _snake.Head->y, true);
			handleScores(1);
			_snake.Grow();
			return 0x00;
		case BAD_APPLE:
         // poisoned "cyberegg", 2pts
			object = TILE_EMPTY; 
         Sound::SickBeep(t_sound_tick);
         Field::UpdateApple(_snake.Head->x, _snake.Head->y, true);
			handleScores(2);
			_snake.IsDrunk = true;
			return 0x00;
		case GOLD_APPLE:
         // golden "cyberegg", 10pts
			object = TILE_EMPTY;
			Sound::GoldyBeep(t_sound_tick);
         Field::UpdateApple(_snake.Head->x, _snake.Head->y, true);
			handleScores(10);
			return 0x00;
      case DEBRIS_A:
      case DEBRIS_B: 
      case BRITTLE_WALL:
		case OUTER_WALL:
			Sound::HitWallBeep();
         _snake.HeadCrash(dir_x, dir_y);
         Sound::OuchBeep();
			_snake.IsDead = true;
			return object;
      case LEVEL_EXIT: // the level goal
         _snake.MadeIt =  true;
         return 0x00;
		default:
			break;
	}

	return 0x00;
}

bool Game::handleScores(int add_pts) {
   _score += add_pts;
   
   // every 50pts gives an extra life
   if (_score >= _nextLifeAt) {
      DSP_Play("xtralife.wav", false);
      _extraLife = true; _lifes++;
      _nextLifeAt += 50;
   }

	// goal reached, activate the exit gate
	if (Field::FoodAvailable == 0 && !_roundFinished) {
       _roundFinished = true; _openExit = true;
      Sound::Booep(100,1000,t_sound_tick);
      Field::EnableDevice(Field::SwitchCodes[0], true);       
      return true;
   }

	return false;
}

// ============================================================================
// round dependend in-game
// ============================================================================

/* (R5) toggle alerts on and off */
void Game::handleAlerts(byte object) {
	static int ticks = 0;

	// block triggering for a few ticks
	if (ticks > 0 && ticks <= 4) { ticks++; return; } else ticks = 0;

	if (object >= FRST_ALERT && object <= LAST_ALERT) {
		int sector = 0, a_id = -1, n = 0;

		for (n = 0; n < 6; n++) if (object == Field::AlertCodes[n][0]) break;
		sector = Field::AlertCodes[n][1];

		// gate close proximity alert
		if (Field::AlertCodes[n][0] == GATE_ALERT)
			Field::ResetSwitches();

		// add new cycle or reset existing
		ColorCycle* cyc = Draw::GetOrAddCycle(sector); cyc->count = 0;
		for (n = 0; n < 6; n++) if (_alerts[n] == cyc) { a_id = n; break; }
		if (a_id == -1) {
			for (n = 0; n < 6; n++) if (_alerts[n] == NULL) { a_id = n; break; }
			_alerts[a_id] = cyc;
		}

		ticks++;
	}
}

/* (R5) handle alert cycles */
void Game::alertCycles() {
	static bool current_state = false;
	static word freq = 200; 
	static int dir = 20;
	bool alerted = false;

	for (int n = 0; n < 6; n++) {
		if (_alerts[n] != NULL) {
			Draw::ColorPulse(_alerts[n], false);
			_alerts[n]->count++; alerted = true;
		
			// turn alert off after 240 ticks
			if (_alerts[n]->count >= 240) {
				Draw::ColorPulse(_alerts[n], true);
				_alerts[n] = NULL;
			}
		}
	}

	if (alerted) {
		Sound::Siren(freq, false); freq += dir;
		dir = (freq >= 300) ? -20 : (freq <= 100) ? 20 : dir;
	} else {
		Sound::Siren(100, true); freq = 100;
	}

	/* toggle only on change */
	if (current_state != alerted) {
		Field::SetGateSwitches(!alerted);
		current_state = alerted;
	}
}

// ============================================================================
// after game
// ============================================================================

void Game::over(bool hit_escape) {
	char msg[24] = { 0 };

	if (hit_escape) sprintf(msg, _taunts[0].txt);
	else 
	for (int n = 1; n < 8; n++) {
		if (_score >= _taunts[n].sco) { 
			sprintf(msg, _taunts[n].txt); break;
		}
	}

   Draw::SaveCurrentScreen();
	Draw::MessageBox(msg);
   wait(NULL);
   Draw::RestoreCurrentScreen();
}

void Game::roundFinished() {
	char text[16]; sprintf(text, _congrats[_round].txt);
	Draw::MessageBox(text);	
	DSP_Play(_congrats[_round].smpl, false);
}

void Game::checkScoreboard() {
   if (!_highscores.loaded) _highscores = Load_High_Scores();
	if (_score > 0) {
		_highscores = newHighscore(_highscores);
		Save_High_Scores(_highscores);
	}
}

HighScores Game::newHighscore(HighScores old_table) {
   HighScores new_table; char player_handle[4];
   
   char score_str[3]; int pos = 0, score_int = 0;
   for (pos = 0; pos < 8; pos++) {
      sprintf(score_str, old_table.entry[pos].score);
      score_int = (int)((score_str[0]-'0')*100)
					 + ((score_str[1]-'0')*10)
					 + ((score_str[2]-'0'));
      if (_score > score_int) break;
   }

	_hspos = pos;
   if (_score > score_int) {
		sprintf(player_handle, newHighscoreBox());
      for (int row = 0; row < 8; row++) {
         if (row < pos) {
				memcpy(&new_table.entry[row], &old_table.entry[row], 
                   sizeof(ScoreEntry));
         } else  if (row == pos) {
				sprintf(new_table.entry[row].score, "%03d", _score);
				sprintf(new_table.entry[row].handle, player_handle);
         } else {
				memcpy(&new_table.entry[row], &old_table.entry[row-1],
                   sizeof(ScoreEntry));
			}
      }
	   return new_table;
	}

	return old_table;
}

char* Game::newHighscoreBox() {
	char *message = "<<<< new highscores >>>>";
	static char player_handle[4] = {0, 0, 0, 0};
	int width = strlen(message) * 9;
	int x = 160 - (width / 2);
	int y = 95;

	int sx = x - 10;
	int ex = x + width + 10;
	int sy = y - 5;
	int ey = y + 22;

	Draw::BorderedBox(sx, sy, ex, ey);
	Draw::Print(message, x + 1, y + 1, 0);
	Draw::Print(message, x, y, 28);
	Draw::Print("your name: ", x + 45, y + 10, 4);
	sprintf(player_handle, getPlayerHandle(x + 144, y + 10, 4));

	return player_handle;
}

char* Game::getPlayerHandle(int x, int y, byte color) {
   while (keyboard(SIGNAL)) keyboard(READ);
	static char input[4] = { 0, 0, 0, 0 };   
   char ass[4] = { 0 }; strcpy(ass, "ass");
	int pos = 0; int key_code; 

	for (int i = 0; i < 3; i++) Draw::Letter(x + (i * 9), y, '_', color);

	while (pos < 3) {
		static int blink = 15;
		if (t_midi_tick >= MIDI_Interval) {
			midi_timer_tick();
			t_midi_tick = 0;
		}

		if (++blink > 30) {
			Draw::Letter(x + (pos * 9), y, '_', 0);
			if (blink > 60) blink = 0;
		} else {
			Draw::Letter(x + (pos * 9), y, '_', 96);
		}

		if (keyboard(SIGNAL)) {
			key_code = keyboard(READ);
         if (!(key_code = key_code & 0xFF)) continue;

			if (key_code == 27) {
				strcpy(input, "CWD");
				return input;
			}
			if ((key_code >= 'a' && key_code <= 'z') || 
             (key_code >= 'A' && key_code <= 'Z') ||
             (key_code >= '0' && key_code <= '9')) {
            if (key_code >= 'a' && key_code <= 'z') key_code = '+';
				input[pos] = key_code == '+' ? ass[pos] : key_code;
				
            Draw::Letter(x + (pos * 9), y, 0xDB, 96);
				Draw::Letter(x + (pos * 9), y, input[pos], color);
				pos++;
			} else if (key_code == 8 && pos > 0) {
				Draw::Letter(x + (pos * 9), y, '_', color);
				pos--;
				input[pos] = 0;
				Draw::Letter(x + (pos * 9), y, 0xDB, 96);
				Draw::Letter(x + (pos * 9), y, '_', color);
			}
		}
		delay(10);
	}

	return input;
}

char Game::wait(char* play_jingle) {
   while(keyboard(SIGNAL)) keyboard(READ);
   if (play_jingle) Music::PlayMIDI(play_jingle);

	int run = 1; char key = 0;
	while(run) {
		if (t_midi_tick >= MIDI_Interval) {
			midi_timer_tick();
			t_midi_tick = 0;
		}

		if(keyboard(SIGNAL)) {
			key = keyboard(READ);
			if (key == 27 || key == 13) run = 0;
		}

		delay(1);
	}

   if (play_jingle) Music::StopMIDI();
	return key;
}

// ============================================================================
// end of game
// ============================================================================

void Game::Exit() {
   Music::StopMIDI();
   Draw::ScrollCredits();
}

void Game::Dispose() {
	// nothing to do yet
}
