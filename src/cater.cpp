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

Cater::Cater() { Created = false; }
Cater::~Cater() { memset(_segments, 0, sizeof(_segments)); }

void Cater::Create() {
   Coords spawn = Field::GetSpawnCoords(ENEMY_SPAWN);
	Length = 11; // 10 + 1 "special"

	// cater colors, 2x2 tiles
	_asset = Draw::GetGameAsset("cater");

   int i = 0, k = 2; spawn.x -= Length * 2;
	for (i = 0; i < Length; i++) {
		_segments[i].x = spawn.x; 
      _segments[i].y = spawn.y;
		memcpy(_segments[i].c, _asset->tiles[k].data, 4);

      Level_Map[spawn.y * 320 + spawn.x] = CATER_SHADOW;
		Draw::MultiPxl(_segments[i].x, _segments[i].y, _segments[i].c, FLD_Buffer);

      k = k == 2 || k == 0 ? 1 : 0;
      spawn.x += 2;
	}

	// last segment = clean-up segment
	memcpy(_segments[Length-1].c, _asset->tiles[3].data, 4);

	Head = &_segments[0];
   Tail = &_segments[Length-1];
	Created = true;

   // set first random apple as target
   _target = Field::GetRandomApple();
}

void Cater::Control(uint tick) {
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

   byte pattern[4] = { 88, 86, 86, 88 };
   byte& object = Level_Map[(Head->y * 320) + Head->x];
   if ((object & TYPE_APPLE) == TYPE_APPLE) {
      Sound::Burp(tick);
      Field::UpdateApple(Head->x, Head->y, false);		
   } else if (object == TILE_EMPTY || object == BRITTLE_WALL) {
      object = CATER_SHADOW;
      Cater::draw(Head->x, Head->y, pattern, VGA_Image);
   }
}

void Cater::Move(int dir_x, int dir_y) {
	shiftSegments();
   if (dir_x) Head->x += (dir_x * 2);
   else 
   if (dir_y) Head->y += (dir_y * 2);
}

void Cater::shiftSegments() {
   Tail = &_segments[Length-1];

	for (int idx = Length - 1; idx > 0; idx--) {
		_segments[idx].x = _segments[idx - 1].x;
		_segments[idx].y = _segments[idx - 1].y;
	}

   byte& object = Level_Map[(Tail->y * 320) + Tail->x];
   if (object == CATER_SHADOW) object = TILE_EMPTY;
}

void Cater::Remove() {
   for (int i = 0; i < Length; i++) {
		Level_Map[(_segments[i].y * 320) + _segments[i].x] = TILE_EMPTY;
	}
}

void Cater::DrawSelf() {
	if  (!Created) return;
   Cater::draw(Head->x, Head->y, Head->c, FLD_Buffer);
   for (int i = Length-1; i > 0; i--) {
      Cater::draw(_segments[i].x, _segments[i].y, _segments[i].c, FLD_Buffer);
   }
}

bool Cater::HasBeenBitten(Segment head) {
   if (!IsReady) return false;
	for (int i = 0; i < Length; i++) {
		Segment& seg = _segments[i];
		if (head.x == seg.x && head.y == seg.y && seg.c != 0) return 1;
	}
	return 0;
}

void Cater::Reset() {
	Created = false; IsReady = false;
	for (int i = 0; i < Length; i++) {
		_segments[i].x = 0;
		_segments[i].y = 0;
		memcpy(_segments[i].c, 0, 4);
      Head->x = 0; Head->y = 0;
	}
}

void Cater::draw(int pos_x, int pos_y, byte* clr, addr* buffer) {
   int map_pos; int idx = 0;
   byte color[4]; memcpy(color, clr, 4);

   for (int off_x = 0; off_x < PXL_SIZE; off_x++) {
      for (int off_y = 0; off_y < PXL_SIZE; off_y++) {
         map_pos = (pos_y + off_y) * 320 + (pos_x + off_x);
         if (Bit_Mask[map_pos] == GFX_MASK_1 || color[idx] == 0xFF)
				color[idx] = 0;
         idx++;
      }
   }

   Draw::MultiPxl(pos_x, pos_y, color, buffer);
}