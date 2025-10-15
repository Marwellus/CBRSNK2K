#include <dos.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mem.h>
#include "inc/types.h"
#include "inc/logger.h"
#include "inc/fileio.h"
#include "inc/memory.h"
#include "inc/dsp.h"
#include "inc/sound.hpp"
#include "inc/draw.hpp"
#include "inc/field.hpp"

#define CC_STATE  0x4F // color: blue
#define COLOR_ON  0x3F // color: green
#define COLOR_OFF 0x3E // color: red
#define DISABLED  0x16 // color: lightgrey

int Field::FoodAvailable = 0;
int Field::TotalApples = 0;
bool Field::GatesMuted = false;
// ptr for game class
addr* Field::LevelMap = NULL;

// 0 - 3 are reserved for default devices
SNKDevice Field::SNKDevices[MAX_DEVICES] = { 0 };

/* up to 8 "switches" per level are connected to 8 "devices",
   these have a fixed "+/- 0x10 relation", i.E. switch 0x43 will
   ALWAYS be connected to device 0x53 and vice versa */
byte Field::SwitchCodes[8] = { // action triggers
   0x41, 0x42, 0x43, 0x44,
   0x45, 0x46, 0x47, 0x48
};
byte Field::DeviceCodes[8] = { // lights or whatever
   0x51, 0x52, 0x53, 0x54,
   0x55, 0x56, 0x57, 0x58
};
byte Field::AlertCodes[6][2] = { // map_value, attached color
	{ 0x30, 64 }, { 0x31, 66 }, { 0x32, 65 },
	{ 0x33, 67 }, { 0x34, 63 }, { 0x35, 63 }
};

Apple Field::_apples[MAX_APPLES] = { 0 };
GameAsset* Field::_asset	= NULL;
byte* Field::_redApple 		= NULL;
byte* Field::_purpleApple 	= NULL;
byte* Field::_goldApple 	= NULL;
byte* Field::_special		= NULL;
byte* Field::_vWall 			= NULL;
byte* Field::_hWall 			= NULL;

addr* Field::_leftGate 		= NULL;
addr* Field::_rghtGate 		= NULL;
addr* Field::_coreGate 		= NULL;
addr* Field::_snakeStart 	= NULL;
addr* Field::_caterStart 	= NULL;
addr* Field::_goldApples[8] = { 0 };

/** setup current round**/

void Field::Create(int round) {
	// access for game class
	Field::LevelMap = Level_Map;
		
	addr* zero = Level_Map;
	/* ptr defaults */
   _leftGate = zero;
	_rghtGate = zero;
	_coreGate = zero;
   _snakeStart = zero; 
	_caterStart = zero;

	_asset = Draw::GetGameAsset("field");
	_redApple	 = _asset->tiles[0].data;
	_purpleApple = _asset->tiles[1].data;
	_goldApple	 = _asset->tiles[2].data;
	_vWall		 = _asset->tiles[3].data;
	_hWall		 = _asset->tiles[4].data;
	_special		 = _asset->tiles[5].data;
   
   Draw::ClearFieldBuffer();
   resetApples();
   setupMap();
	setupRound(round);
}

/* converts color-map into collision map */
void Field::setupMap() {
	addr* zero = Level_Map;		// alias for zero pos
   addr* map_ptr = Level_Map;	// alias for loop ptr
	byte color_byte, map_byte;

   // translates color codes to map codes
   byte color2map[16] = { 
      TILE_BLOCKED,  // 00 outside of playable area
      TILE_BLOCKED,  // 01 block object spawn
      TILE_EMPTY,    // 02 apples & stuff can be placed
      LEVEL_EXIT,    // 03 snake must hit to leave level
      OUTER_WALL,    // 04 snake must not hit
      LEFT_GATE,     // 05 left gate
      RGHT_GATE,     // 06 right gate
      CORE_GATE,     // 07 core gate
      GATE_TRIGGER,  // 08 close gate trigger (cater)
      CATER_BLOCK,   // 09 block path for cater
      SNAKE_SPAWN,   // 10 spawn point
      BRITTLE_WALL,  // 11 special wall 
      ENEMY_SPAWN,   // 12 spawn point
      SPECIAL_ITEM,  // 13 special item
      GOLD_APPLE,    // 14 gold apple
      TILE_EMPTY,    // 15 reserved
   };
   
   // +16 offset for array
   int idx_offset = 16; bool skip_dev = false;
   // 0 to 3 are reserved for entry and exit gate
   setDefaultDevices(); int dev_idx = 4;
   byte color2special[MAX_DEVICES] = {
      0,0,0,0, 0,0,0,0,
      0,0,0,0, 0,0,0,0	
   };   
   memcpy(&color2special[0], SwitchCodes, 8); // color 16 to 23
   memcpy(&color2special[8], DeviceCodes, 8); // color 24 to 31

	for (int idx = 0; idx < MAP_SIZE; idx++) {
      color_byte = map_ptr[idx];

      // catch exit gate switch position
      if (color_byte == 16) {
         if (SNKDevices[EXIT_GATE_SWITCH].pos == zero)
             SNKDevices[EXIT_GATE_SWITCH].pos = (Level_Map + idx);
			map_ptr[idx] = SNKDevices[EXIT_GATE_SWITCH].type;
      }
      // catch exit gate device position
      if (color_byte == 24 && SNKDevices[EXIT_GATE_DEVICE].pos == zero) {
         SNKDevices[EXIT_GATE_DEVICE].pos = (Level_Map + idx);
      }

		// alert zone(s), array to color offset: -32
		if (color_byte >= 32 && color_byte <= 37) {
			map_ptr[idx] = AlertCodes[color_byte-32][0];
		}

      // catch other, level dependend devices (excluding exit gate)
      if ((color_byte > 16 && color_byte < 32) && color_byte != 24) {
         skip_dev = false; byte connected_device = 0;
         map_byte = color2special[color_byte - idx_offset];
         
         // skip already found codes
         for (int n = 0; n < MAX_DEVICES; n++) {
            if (SNKDevices[n].type == map_byte) { skip_dev = true; break; } 
         }

         if (!skip_dev && dev_idx < MAX_DEVICES) {
            // devices are connected to each other (i.E. switch <=> door)
            connected_device = (map_byte < 0x50) ? map_byte + 0x10 : map_byte - 0x10;
				// Log_Info("FLD: [%d] dev: 0x%02X", dev_idx, map_byte);

            SNKDevices[dev_idx].type    = map_byte;
            SNKDevices[dev_idx].pos     = (Level_Map + idx);
            SNKDevices[dev_idx].state   = false;
            SNKDevices[dev_idx].con_to  = connected_device;
            SNKDevices[dev_idx].enabled = false;
            SNKDevices[dev_idx].func    = NULL;

            // draw state indicator
            if (map_byte > 0x50) {
					drawPixelAt(SNKDevices[dev_idx].pos, DISABLED);
				}

            dev_idx++;
         }

			map_ptr[idx] = map_byte;
         continue; // skip rest of loop
      }

      // catch standard colors (except 15 white)
      if (color_byte > 14) continue;
      map_byte = color2map[color_byte];

      // get pos of gates/snake/cater (1st byte, ignore rest)
      if (map_byte == LEFT_GATE && _leftGate == zero) _leftGate = zero + idx;
      if (map_byte == RGHT_GATE && _rghtGate == zero) _rghtGate = zero + idx;
		if (map_byte == CORE_GATE && _coreGate == zero) _coreGate = zero + idx;
      if (map_byte == SNAKE_SPAWN && _snakeStart == zero) _snakeStart = zero + idx;
      if (map_byte == ENEMY_SPAWN && _caterStart == zero) _caterStart = zero + idx;
      
      if (map_byte == GOLD_APPLE) { /* -_- */ }

      // replace color byte
      map_ptr[idx] = map_byte;
	}

   Field::DrawMap();
}

/* devices / interactive stuff */
void Field::setupRound(int round) {
   // default
   EnableDevice(SwitchCodes[0], false);
   setDevice(DeviceCodes[0], Field::ExitGate);

   switch (round) {
      case 1:
         // add some more fun?
         break;
      case 2:         
         // add some more fun?
         break;
      case 3:
         // add some more fun?
         break;
      case 4:
         // add some more fun?
         break;
      case 5:
			FoodAvailable = 8; // special_item's
			EnableDevice(SwitchCodes[1], true);
			EnableDevice(SwitchCodes[2], true);
			EnableDevice(SwitchCodes[3], true);
			EnableDevice(SwitchCodes[4], true);
			setDevice(DeviceCodes[1], Field::CoreGate);
			setDevice(DeviceCodes[2], Field::CoreGate);
			setDevice(DeviceCodes[3], Field::CoreGate);
			setDevice(DeviceCodes[4], Field::CoreGate);
			CoreGate(CLOSE);
         break;                                    
      default:
         return;
   }
}

/* setup entry / exit gate */
void Field::setDefaultDevices() {
	addr* zero = Level_Map; // zero pos of ptr
   // left (entry) gate switch (not toggable)
   SNKDevices[ETRY_GATE_SWITCH].type    = 0x40;
   SNKDevices[ETRY_GATE_SWITCH].pos     = zero;
   SNKDevices[ETRY_GATE_SWITCH].state   = true; // = on
   SNKDevices[ETRY_GATE_SWITCH].con_to  = 0x50;
   SNKDevices[ETRY_GATE_SWITCH].enabled = true;
   // left (entry) gate (not toggable)
   SNKDevices[ETRY_GATE_DEVICE].type    = 0x50;
   SNKDevices[ETRY_GATE_DEVICE].pos     = zero;
   SNKDevices[ETRY_GATE_DEVICE].state   = true; // = open
   SNKDevices[ETRY_GATE_DEVICE].con_to  = 0x40;
   SNKDevices[ETRY_GATE_DEVICE].enabled = true;
   // right (exit) gate switch (toggable, map specific position)
   SNKDevices[EXIT_GATE_SWITCH].type    = 0x41;
   SNKDevices[EXIT_GATE_SWITCH].pos     = zero;
   SNKDevices[EXIT_GATE_SWITCH].state   = false; // = off
   SNKDevices[EXIT_GATE_SWITCH].con_to  = 0x51;
   SNKDevices[EXIT_GATE_SWITCH].enabled = false;
   // right (exit) gate (toggable, map specific position)
   SNKDevices[EXIT_GATE_DEVICE].type    = 0x51;
   SNKDevices[EXIT_GATE_DEVICE].pos     = zero;
   SNKDevices[EXIT_GATE_DEVICE].state   = false; // = closed
   SNKDevices[EXIT_GATE_DEVICE].con_to  = 0x41;
   SNKDevices[EXIT_GATE_DEVICE].enabled = false;
}

/* setup level depended devices */
void Field::setDevice(byte swtch_code, void(*func)(bool)) {
   word ids = 0; if (!(ids = getDeviceIds(swtch_code))) return;
   word sdx = ids >> 8, ddx = ids & 0xFF;
   
   // doesn't matter, which is called
   SNKDevices[sdx].func = func;
   SNKDevices[ddx].func = func;
}

/* reset apples array */
void Field::resetApples() {
	FoodAvailable = 0; TotalApples = 0;
	for (int i = 0; i < MAX_APPLES; i++) {
		_apples[i].x = 0; _apples[i].y = 0;
      _apples[i].bad = false; 
      _apples[i].eaten = false;
	}
}

/** called by game class **/

void Field::ScatterFood(int amount, int foul) {
	word field_x, field_y; byte* apple_tile; 
	FoodAvailable = amount;
	TotalApples   = amount + foul;   

   if (TotalApples > MAX_APPLES) {
      Log_Info("Too many apples! Total: %d, Max: %d", TotalApples, MAX_APPLES);
      return;
   }

	int div = foul > 0 ? (int)(amount / foul) : 0;
	
   for (int i = 0; i < TotalApples; i++) {
		bool isBad = false;
		if (div > 0 && !(i % div) && foul > 0) {
			isBad = true; foul--;
		}

		do { 
         field_x = (rand() % 160) * 2; 
         field_y = (rand() % 100) * 2; 
      } while (
         Level_Map[field_y * 320 + field_x] != TILE_EMPTY
      );

      // write apple onto collision map
		Level_Map[field_y * 320 + field_x] = isBad ? BAD_APPLE : RED_APPLE;      
      // save apple
      _apples[i].x = field_x; _apples[i].y = field_y; 
      _apples[i].bad = isBad; _apples[i].eaten = false;	
      // draw apple into field buffer
		apple_tile = isBad ? _purpleApple : _redApple;

		// make scattering visible for the effect
		Draw::MultiPxl(field_x, field_y, apple_tile);
		if (isBad) Sound::Beep(300, 20); else Sound::Beep(100 + (i * 10), 20);
		delay(20);
	}

	Field::ExitGate(CLOSE);
	DrawMap();
}

void Field::PlaceObstacles() {
	int field_x, field_y, k, idx, pos;
	int obstacles[4][4] = {
		{ DEBRIS_A, DEBRIS_A, DEBRIS_B, TILE_BLOCKED },
		{ DEBRIS_A, DEBRIS_A, TILE_BLOCKED, DEBRIS_B },
		{ DEBRIS_B, TILE_BLOCKED, DEBRIS_A, DEBRIS_A },
		{ TILE_BLOCKED, DEBRIS_B, DEBRIS_A, DEBRIS_A }
	};

	for (int i = 0; i <= 15; i++) {
		do {
			field_x = (rand() % 160) * 2;
			field_y = (rand() % 100) * 2;
		} while (
         Level_Map[field_y * 320 + field_x] != TILE_EMPTY
      );

		idx = 0;
		k = (rand() % 4);
		for (int row = 0; row < 4; row += 2) {
			for (int col = 0; col < 4; col += 2) {
				pos = (field_y + row) * 320 + (field_x + col);
				Level_Map[pos] = obstacles[k][idx];
				idx++;
			}
		}
	}

	Field::DrawMap(VGA_Address);
}

void Field::MoveRandomApple(uint tick) {
	if (!FoodAvailable) return;
	
	int tries = 0, abs_x, abs_y, idx;
	byte object;

	// try to pick a not already eaten apple
	do { idx = (rand() % (TotalApples-1)); tries++; } 
   while (_apples[idx].eaten == true && tries < 15);   
	// if no luck, do nothing
	if (_apples[idx].eaten) return;

   abs_x = _apples[idx].x; abs_y = _apples[idx].y; tries = 0;

   // try to find a free place either in some x or y direction
	if (rand() % 10 > 5) {
		do { abs_x = _apples[idx].x; abs_x += (rand() % 10 > 5) ? 2 : -2;
           object = Level_Map[_apples[idx].y * 320 + abs_x]; tries++; 
      } while (object != TILE_EMPTY && tries < 5);
	} else {
		do { abs_y = _apples[idx].y; abs_y += (rand() % 10 > 5) ? 2 : -2;
           object = Level_Map[abs_y * 320 + _apples[idx].x]; tries++; 
      } while (object != TILE_EMPTY && tries < 5);		
	}

   // if found free place, move to new position
	if (object == TILE_EMPTY) {
      Level_Map[_apples[idx].y * 320 + _apples[idx].x] = TILE_EMPTY;
      Draw::Pixel(_apples[idx].x, _apples[idx].y, 0x00, FLD_Buffer);
      
		_apples[idx].x = abs_x; _apples[idx].y = abs_y;
   	Level_Map[abs_y * 320 + abs_x] = _apples[idx].bad ? BAD_APPLE : RED_APPLE;

      Draw::MultiPxl(_apples[idx].x, _apples[idx].y,
                     _apples[idx].bad ? _purpleApple : _redApple,
                     FLD_Buffer);
      Sound::Beep(800, 30, tick);
	}
}

void Field::EntryGate(bool state) {
   SNKDevices[ETRY_GATE_DEVICE].state = state;

	for (int i = 0; i < 6; i += 2) {
      *(_leftGate + (i * 320)) = state ? LEFT_GATE : OUTER_WALL;
      drawPixelAt(_leftGate + (i * 320), state ? 116 : 121);
   }

	if (!GatesMuted) DSP_PlayLeft(state ? "liftback.wav" : "liftdoor.wav");
}

void Field::ExitGate(bool state) {
   SNKDevices[EXIT_GATE_DEVICE].state = state;

	for (int i = 0; i < 6; i += 2) {
      *(_rghtGate + (i * 320)) = state ? RGHT_GATE : OUTER_WALL;
      drawPixelAt(_rghtGate + (i * 320), state ? 116 : 121);
	}
	
	if (!GatesMuted) DSP_PlayRight(state ? "liftback.wav" : "liftdoor.wav");
}

void Field::ToggleDevice(byte swtch_code) {
	word ids; if (!(ids = getDeviceIds(swtch_code))) return;
   word s_idx = ids >> 8, d_idx = ids & 0xFF;

   // toggle switch, if enabled
   if (SNKDevices[s_idx].enabled) {
      int offset = (int)(SNKDevices[s_idx].pos - Level_Map);
      SNKDevices[s_idx].state = SNKDevices[s_idx].state ? false : true;
      Draw::ToggleSwitch(offset, SNKDevices[s_idx].state);
   }

   // toggle connected device, if enabled
   if (SNKDevices[d_idx].enabled) {
      SNKDevices[d_idx].state = SNKDevices[d_idx].state ? false : true;
		if (SNKDevices[d_idx].func) {
			SNKDevices[d_idx].func(SNKDevices[d_idx].state);
		}
   }
}

void Field::EnableDevice(byte swtch_code, bool enable) {
   word ids = 0; if (!(ids = getDeviceIds(swtch_code))) return;
   word sdx = ids >> 8, ddx = ids & 0xFF;

   SNKDevices[sdx].enabled = enable;
   SNKDevices[ddx].enabled = enable;

   drawPixelAt(SNKDevices[ddx].pos, SNKDevices[ddx].enabled ? COLOR_ON : COLOR_OFF);
}

bool Field::DeviceState(byte swtch_code) {
   word ids = 0; if (!(ids = getDeviceIds(swtch_code))) return false;
   word sdx = ids >> 8, ddx = ids & 0xFF;
   return SNKDevices[ddx].state;
}

/* round 5 elements */

/* (R5) open/close core gate */
void Field::CoreGate(bool state) {
	static bool gate_is_open = true;
	bool open = SNKDevices[4].state && SNKDevices[5].state &&
				   SNKDevices[6].state && SNKDevices[7].state && state;
	
	if (gate_is_open == open) return; else gate_is_open = open;
	
	for (int x = 0; x < 8; x += 2) {
		*(_coreGate + x) = open ? CORE_GATE : OUTER_WALL;
		drawPixelAt(_coreGate + x, open ? 116 : 121);
	}
	
	if (!GatesMuted) DSP_Play(open ? "liftback.wav" : "liftdoor.wav", false);
}

/* (R5) set all core gate switches to off */
void Field::ResetSwitches() {
	int offset;
	// set all core gate switches to off
	for (int n = 4; n <= 11; n++) {
		SNKDevices[n].state = false;
		if (n <= 7) {
			offset = (int)(SNKDevices[n].pos - Level_Map);
			Draw::ToggleSwitch(offset, SNKDevices[n].state);
		}
	}
	// close core gate
	CoreGate(CLOSE);
}

/* (R5) de-/activate core gate switches */
void Field::SetGateSwitches(bool state) {
	EnableDevice(SwitchCodes[1], state);
	EnableDevice(SwitchCodes[2], state);
	EnableDevice(SwitchCodes[3], state);
	EnableDevice(SwitchCodes[4], state);
}

/** called by snake and/or cater class **/

Coords Field::GetSpawnCoords(byte id) {
   int offset; Coords pos;
   if (id == SNAKE_SPAWN) offset = (int)(_snakeStart - Level_Map);
   if (id == ENEMY_SPAWN) offset = (int)(_caterStart - Level_Map);
   pos.x = offset % 320; pos.y = offset / 320; pos.val = 0x00;
   return pos;
}

Apple Field::GetRandomApple() {
	int tries = 0, idx = 0;
	do { idx = (rand() % (TotalApples-1)); tries++; }
   while (_apples[idx].eaten == true && tries < 15);
	return _apples[idx];
}

void Field::UpdateApple(int abs_x, int abs_y, bool eat) {
   if (eat) Draw::Pixel(abs_x, abs_y, 0x00, FLD_Buffer);

	for (int i = 0; i < TotalApples; i++) {
		if (_apples[i].x == abs_x && _apples[i].y == abs_y) {
         if (eat) {
            if (!_apples[i].bad) FoodAvailable--;
            _apples[i].eaten = true; break;
         } else if (!_apples[i].bad) {
            // turn into purple apple
            Level_Map[abs_y * 320 + abs_x] = BAD_APPLE;
            FoodAvailable--; // purple apples are optional food
            _apples[i].bad = true; break;
         }
      }
   }	
}

/** supportive methods **/

word Field::getDeviceIds(byte swtch_code) {
   static word result = 0;
   /* s_idx => SWITCH, d_idx => connected DEVICE */
   word s_idx = -1, d_idx = -1;
   /* right ORDER must be ensured for the following loop */
   if  (swtch_code > 0x50) swtch_code -= 0x10;

   for(word n = 0; n < MAX_DEVICES; n++) {
      if (SNKDevices[n].type == swtch_code) { s_idx = n; }
      if (SNKDevices[n].con_to == swtch_code) { d_idx = n; }
   }

   if (s_idx == -1 || d_idx == -1) {
      Log_Info("Field: faulty/non-existing switch 0x%02X sdx: %d, ddx: %d",
					swtch_code, s_idx, d_idx);
      return NULL;
   }

   result = (s_idx << 8) | d_idx;
   return result;
}

/** internal draw methods **/
void Field::DrawMap() { DrawMap(FLD_Buffer); }
void Field::DrawMap(addr* buffer) {
	int x, y, c; byte* tile = NULL;
	char obj;

	for (x = 0; x < 320; x += 2) {
		for (y = 0; y < 200; y += 2) {
			obj = Level_Map[(y * 320) + x];
			switch (obj) {
				case RED_APPLE:
					tile = _redApple; break;
				case BAD_APPLE:
					tile = _purpleApple; break;
				case DEBRIS_A: 
					tile = _vWall; break;
				case DEBRIS_B: 
					tile = _hWall; break;
				case GOLD_APPLE:
				 	tile = _goldApple; break;
				case SPECIAL_ITEM:
					tile = _special; break;
				default:
					tile = NULL;
			}

			if (tile) Draw::MultiPxl(x, y, tile, buffer);
		}
	}
}

void Field::drawPixelAt(addr* map_addr, byte color) {
   int offset = (int)(map_addr - Level_Map);
   int x = offset % 320, y = offset / 320;
   Draw::Pixel(x, y, color, VGA_Image);
}

/** base methods **/

void Field::Debug() {
	for (int x = 0; x < 320; x += 2) {
		for (int y = 0; y < 200; y += 2) {
			byte obj = Level_Map[(y * 320) + x];
         // Pxl pxl = {x, y, 0, 0, 15}; // TODO later
         // if (obj) Video::Write(pxl);
		}
	}
	getch();
}
