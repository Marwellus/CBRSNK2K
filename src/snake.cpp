#include <dos.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <mem.h>
#include "inc/types.h"
#include "inc/logger.h"
#include "inc/memory.h"
#include "inc/draw.hpp"
#include "inc/sound.hpp"
#include "inc/field.hpp"
#include "inc/creaturs.hpp"

Snake::Snake() { }
Snake::~Snake() { memset(_segments, 0, sizeof(_segments)); }

void Snake::Create() {
	reset(); Sound::Booep(100, 1000);
   Coords spawn = Field::GetSpawnCoords(SNAKE_SPAWN);
	Length = 11; // 10 + 1 "special"

   IsReady = false;
	MadeIt = false; IsDrunk = false;
	IsDead = false; Crashed = false;

	// snake colors, 2x2 tiles
	_asset = Draw::GetGameAsset("snake");

   spawn.x += (Length * 2);
	int i, k = 2;
	for (i = 0; i < Length; i++) {
		_segments[i].x = spawn.x; 
      _segments[i].y = spawn.y;
		memcpy(_segments[i].c, _asset->tiles[k].data, 4);

      Level_Map[(spawn.y * 320) + spawn.x] = SNAKE_SHADOW;
		Draw::MultiPxl(_segments[i].x, _segments[i].y, _segments[i].c, FLD_Buffer);

      k = k == 2 || k == 0 ? 1 : 0;
		spawn.x -= 2;
	}

	// last one = clean-up segment
	memcpy(_segments[Length-1].c, _asset->tiles[3].data, 4);

	Head = &_segments[0]; 
   Tail = &_segments[Length-1];
}

void Snake::Move(int dir_x, int dir_y) {
	shiftSegments();
	
	if (dir_x) Head->x += (dir_x * 2);
	else
	if (dir_y) Head->y += (dir_y * 2);

   byte& object = Level_Map[(Head->y * 320) + Head->x];
   if (object == TILE_EMPTY) object = SNAKE_SHADOW;
}

void Snake::shiftSegments() {
   Tail = &_segments[Length];

	for (int i = Length; i > 0; i--) {
		_segments[i].x = _segments[i-1].x;
		_segments[i].y = _segments[i-1].y;
	}

   byte& object = Level_Map[(Tail->y * 320) + Tail->x];
   if (object == SNAKE_SHADOW) object = TILE_EMPTY;
}

void Snake::Grow() {
	if (Length > 75) return;

	// clean-up segment becomes regular segment
   memcpy(_segments[Length-1].c, _segments[Length-3].c, 4);

	// append new clean-up segment
	_segments[Length].x = _segments[Length-1].x;
	_segments[Length].y = _segments[Length-1].y;
	memcpy(_segments[Length].c, _asset->tiles[3].data, 4);

   Tail = &_segments[Length];
   Length++;
}

bool Snake::BitItself() {
	for (int i = Length - 1; i > 0; i--) {
		Segment& seg = _segments[i];
		if (Head->x == seg.x && Head->y == seg.y && seg.c != 0) return 1;
	}
	return 0;
}

bool Snake::HasBeenBitten(Segment head) {
	for (int i = 0; i < Length; i++) {
		Segment& seg = _segments[i];
		if (head.x == seg.x && head.y == seg.y && seg.c != 0) return 1;
	}
	return 0;
}

void Snake::HeadCrash(int dir_x, int dir_y) {
   for(int i = 0; i < 6; i++) {
      Level_Map[(_segments[i].y * 320) + _segments[i].x] = TILE_EMPTY;
      Draw::Pixel(_segments[i].x, _segments[i].y, 0, FLD_Buffer);

      // scatter segments sideways
      _segments[i].x += dir_y ? (rand() % 4)-2 : 0;
      _segments[i].y += dir_x ? (rand() % 4)-2 : 0;
      Snake::draw(_segments[i].x, _segments[i].y, _segments[i].c);
   }
}

void Snake::Remove() {
   for (int i = 0; i < Length; i++) {
		Level_Map[(_segments[i].y * 320) + _segments[i].x] = TILE_EMPTY;
	}
}

void Snake::DrawSelf() {
   Snake::draw(Head->x, Head->y, Head->c);
   for (int i = Length-1; i > 0; i--) {
      Snake::draw(_segments[i].x, _segments[i].y, _segments[i].c);
   }
}

void Snake::draw(int pos_x, int pos_y, byte* clr) {
   int map_pos; int idx = 0;
   byte color[4]; memcpy(color, clr, 4);

   for (int off_x = 0; off_x < PXL_SIZE; off_x++)
	{
      for (int off_y = 0; off_y < PXL_SIZE; off_y++) 
		{
         map_pos = ((pos_y + off_y) * 320) + (pos_x + off_x);
         if (Bit_Mask[map_pos] == GFX_MASK_0 || color[idx] == 0xFF) 
				color[idx] = 0;
         idx++;
      }
   }

   Draw::MultiPxl(pos_x, pos_y, color, FLD_Buffer);
}

void Snake::reset() {
	for (int i = 0; i < 100; i++) {
		_segments[i].x = 0; _segments[i].y = 0;
		memcpy(_segments[i].c, 0, 4);
	}
   Head->x = 0; Head->y = 0;
}
