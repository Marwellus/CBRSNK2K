#ifndef CREATURS_HPP
#define CREATURS_HPP
#include "inc/types.h"
#include "inc/field.hpp"
#include "inc/draw.hpp"

#define SNAKE_SHADOW 0x5E // 5nakE
#define CATER_SHADOW 0xCE // CatEr
#define BOSS_SHADOW  0xBC // BossCater
#define TYPE_APPLE 0xA0

struct Segment { int x, y; byte c[4]; };
struct BossSeg { int x, y; byte c[36]; };

class Cater;
class Snake {
	private:
		Segment _segments[100];
		GameAsset* _asset;
		void reset();
      void draw(int pos_x, int pos_y, byte* color);
		void shiftSegments();

	public:
		Segment* Head;
      Segment* Tail;
		int Length;
      bool IsReady;
		bool IsDrunk;
		bool IsDead;
		bool Crashed;
		bool MadeIt;

		Snake();
		~Snake();
      void DrawSelf();
		bool BitItself();
		bool HasBeenBitten(Segment head);
		void Create();
		void Grow();
		void Move(int dir_x, int dir_y);
      void HeadCrash(int dir_x, int dir_y);
      void Remove();
};

class Cater {
	private:
		Segment _segments[20];
      Apple _target;
		GameAsset* _asset;
		
		void shiftSegments();
      void draw(int pos_x, int pos_y, byte* clr, addr* buffer);
	public:
		Segment* Head;
      Segment* Tail;
		int Length;
		bool Created;
		bool IsReady;

		Cater();
		~Cater();
		void Create();
		void Control(uint tick);
		void Move(int dir_x, int dir_y);
      void DrawSelf();
		bool HasBeenBitten(Segment head);
      void Remove();
		void Reset();
};


class Boss {
	private:
		BossSeg _segments[10];
		GameAsset* _asset;
      Apple _target;

		void shiftSegments();
	public:
		BossSeg* Head;
      BossSeg* Tail;
		int Length;
		bool Created;
		bool IsReady;

		Boss();
		~Boss();
		void Create();
		void Control(uint tick);
		void Move(int dir_x, int dir_y);
      void DrawToBuffer();
		bool HasBeenBitten(Segment head);
      void Remove();
		void Reset();
};

#endif
