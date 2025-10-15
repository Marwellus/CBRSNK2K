#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include <mem.h>
#include "inc/logger.h"
#include "inc/memory.h"
#include "inc/draw.hpp"
#include "inc/field.hpp"
#include "inc/sound.hpp"
#include "inc/creaturs.hpp"

#define TILE_SIZE 6  // x = y

Boss::Boss() { }
Boss::~Boss() { memset(_segments, 0, sizeof(_segments)); }

void Boss::Create() {
   Coords spawn = Field::GetSpawnCoords(ENEMY_SPAWN);
	Length = 8;

	// 6 x 6 tiles
	_asset = Draw::GetGameAsset("boss");
	if (!_asset) { Log_Info("Boss: failed to create"); return; }

	int i = 0, k = 3;
	for (i = 0; i < Length; i++) {
		_segments[i].x = spawn.x; _segments[i].y = spawn.y;
		memcpy(_segments[i].c, _asset->tiles[k].data, 36);

      Level_Map[spawn.y*320+spawn.x] = BOSS_SHADOW;
		DrawTile(_segments[i].x, _segments[i].y, _segments[i].c, 
					 TILE_SIZE, TILE_SIZE, FLD_Buffer);

      k = 4; spawn.x += TILE_SIZE;
	}

	Head = &_segments[0];
   Tail = &_segments[Length-1];
	Created = true;
}

void Boss::Control(uint tick) {
	static int dir_x = 0;
	static int dir_y = 0;
   int center_x = 160;
   int center_y = 100;

   if (_target.x == Head->x && _target.y == Head->y)
       _target = Field::GetRandomApple();

   int delta_x = Head->x - _target.x; 
   int delta_y = Head->y - _target.y;
   bool head_x_clear = true; 
   bool target_x_clear = true;

   // simple path finding system for an asymetric playfield
   if (delta_x != 0)  
   {
      for (int i = 0; i < abs(delta_x); i += 2) {
         int search_x = delta_x > 0 ? (Head->x - i) : (Head->x + i);
         head_x_clear  = (
            head_x_clear && 
            (Level_Map[Head->y * 320 + search_x] != OUTER_WALL && 
             Level_Map[Head->y * 320 + search_x] != CATER_BLOCK)
         );
         target_x_clear = (
            target_x_clear && 
            (Level_Map[_target.y * 320 + search_x] != OUTER_WALL &&
             Level_Map[_target.y * 320 + search_x] != CATER_BLOCK)               
         );
      }

      if (head_x_clear)  
         // move towards apple.x until delta_x = 0
         { dir_y = 0; dir_x = delta_x > 0 ? -1 : 1; }
      else
      if (target_x_clear) 
         // move torwards apple.y until head_x_clear is true
         { dir_x = 0; dir_y = delta_y > 0 ? -1 : 1; }
      else {
         // head to the center first
         dir_x = 0; dir_y = (Head->y - center_y) > 0 ? -1 : 1;
      }
   } 
   else if (delta_y != 0)
   {
      dir_x = 0; dir_y = delta_y > 0 ? -1 : 1;
   }

   Move(dir_x, dir_y);

   byte& object = Level_Map[(Head->y * 320) + Head->x];
   if (object == TILE_EMPTY || object == BRITTLE_WALL)
       object = CATER_SHADOW;
}

void Boss::Move(int dir_x, int dir_y) {
	shiftSegments();
   if (dir_x) Head->x += (dir_x * 2);
   else 
   if (dir_y) Head->y += (dir_y * 2);
}

void Boss::shiftSegments() {
   Tail = &_segments[Length-1];

	for (int idx = Length - 1; idx > 0; idx--) {
		_segments[idx].x = _segments[idx - 1].x;
		_segments[idx].y = _segments[idx - 1].y;
	}

   byte& object = Level_Map[(Tail->y * 320) + Tail->x];
   if (object == BOSS_SHADOW) object = TILE_EMPTY;
}

void Boss::Remove() {
   for (int i = 0; i < Length; i++) {
		Level_Map[(_segments[i].y * 320) + _segments[i].x] = TILE_EMPTY;
	}
}

void Boss::DrawToBuffer() {
	if  (!Created) return;
   for (int i = 0; i < (Length-1); i++) {
		DrawTile(
			_segments[i].x, _segments[i].y, _segments[i].c, 
			TILE_SIZE, TILE_SIZE, FLD_Buffer
		);
	}
}

bool Boss::HasBeenBitten(Segment head) {
   if (!IsReady) return false;
	for (int i = 0; i < Length; i++) {
		BossSeg& seg = _segments[i];
		if (head.x == seg.x && head.y == seg.y && seg.c != 0) return 1;
	}
	return 0;
}

void Boss::Reset() {
	Created = false; IsReady = false;
	for (int i = 0; i < Length; i++) {
		_segments[i].x = 0;
		_segments[i].y = 0;
	}
}