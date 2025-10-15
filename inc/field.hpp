#ifndef FIELD_HPP
#define FIELD_HPP
#include "inc/types.h"
#include "draw.hpp"

#define MAX_APPLES   100
#define MAX_DEVICES 	16

#define MAP_SIZE     64000 // just like vga space
#define TILE_EMPTY   0x00 // stuff can be placed here
#define TILE_BLOCKED 0x01 // nothing can be placed

#define GATE_TRIGGER 0x2E // closes gate behind cater
#define CATER_BLOCK  0x2F // keeps cater outside

// values 0x30 - 0x60 used as devices/alerts
#define FRST_ALERT	0x30
#define LAST_ALERT	0x35
#define GATE_ALERT	0x34

#define SLOW_GROUND  0x70 // slows snake down
#define DEBRIS_A     0x7A // works like regular wall
#define DEBRIS_B     0x7B // but meant for drawing sectors
#define OUTER_WALL   0x7F // playfield absolut border
#define BRITTLE_WALL 0x7E // deadly for snake, ok for cater

#define SNAKE_SPAWN  0x80 // spawn pos. for snake
#define ENEMY_SPAWN  0x8A // spawn pos. for cater/boss

#define LEFT_GATE    0x90 // entry for snake
#define RGHT_GATE    0x91 // exit for snake (entry for cater)
#define CORE_GATE		0x92 // core gate (or just another gate)
#define LEVEL_EXIT   0x9E // trigger tile, ends current round

#define RED_APPLE    0xA0 // 1pt, snake grows 1 segment
#define BAD_APPLE    0xA1 // 2pt, makes snake "drunk"
#define GOLD_APPLE   0xA2 // 10pt, does nothing (yet)
#define SPECIAL_ITEM 0xAF // XXpt, level depend kind of item

/* LEFT = ENTRY, RIGHT = EXIT */
#define ETRY_GATE_SWITCH 0
#define ETRY_GATE_DEVICE 1
#define EXIT_GATE_SWITCH 2
#define EXIT_GATE_DEVICE 3
#define OPEN 1
#define CLOSE 0

struct SNKDevice {
   byte type;
   addr* pos;
   bool state;
   bool enabled;
   byte con_to;
   void (*func)(bool state);
};

struct Coords { word x, y; byte val; };
struct Apple {	word x, y; bool bad, eaten; };
class Field {
   private:
      static Apple _apples[MAX_APPLES];
		static GameAsset* _asset;
		static byte* _redApple;		// 2x2 tile (colors)
		static byte* _purpleApple;	// 2x2 tile (colors)
		static byte* _goldApple;	// 2x2 tile (colors)
		static byte* _special;		// 2x2 tile (colors)
		static byte* _vWall;			// 2x2 tile (colors)
		static byte* _hWall;			// 2x2 tile (colors)
      static addr* _leftGate;
      static addr* _rghtGate;
      static addr* _coreGate;
      static addr* _goldApples[8];
      static addr* _snakeStart;
      static addr* _caterStart;
      static void setupMap();
      static void setupRound(int round);
      static void resetApples();
      static void setDefaultDevices();
      static void setDevice(byte swtch_code, void(*func)(bool));
		static word getDeviceIds(byte code);
      static void drawPixelAt(addr* map_addr, byte color);
	public:
		static int FoodAvailable;
      static int TotalApples;
      static SNKDevice SNKDevices[MAX_DEVICES];
      static byte SwitchCodes[8];
      static byte DeviceCodes[8];
		static byte AlertCodes[6][2];
		static addr* LevelMap;	// ptr for game class
		static bool GatesMuted;	// mute all gate sounds
		
		/* core elements */
		static void Create(int round);
		static void ScatterFood(int amount, int foul);
		static void PlaceObstacles();
      static void EntryGate(bool state);
      static void ExitGate(bool state);
      static void MoveRandomApple(uint tick);
		static void DrawMap();
		static void DrawMap(addr* buffer);
		
		/* devices */
		static void ToggleDevice(byte swtch_code);
      static void EnableDevice(byte swtch_code, bool enable);
      static bool DeviceState(byte swtch_code);
		
		/* round 5 elements */
		static void CoreGate(bool state);
		static void ResetSwitches();
		static void SetGateSwitches(bool);
		
		/* snake or cater related */
		static Coords GetSpawnCoords(byte id);
      static Apple GetRandomApple();
      static void UpdateApple(int abs_x, int abs_y, bool eat);


		static void Debug();
};

#endif
