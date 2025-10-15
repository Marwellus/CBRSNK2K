#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <signal.h>
#include <string.h>
#include "inc/types.h"
#include "inc/logger.h"
#include "inc/timer.h"
#include "inc/fileio.h"
#include "inc/video.hpp"
#include "inc/draw.hpp"
#include "inc/sound.hpp"
#include "inc/field.hpp"
#include "inc/game.hpp"

bool __MIDI   = true;
byte __DEVICE = 0;
struct arg { 
	char* str; 
	addr* param; 
	int val; 
};
arg args[8] = {
	{ "\0", NULL, 0 },
	{ "--debug", &__DEBUG, true },
	{ "--nomidi", &__MIDI, false },
	{ "--awe", &__DEVICE, 0 },
	{ "--gus", &__DEVICE, 1 }
};
void parseParams(int argc, char* argv[]) {
	for (int n=1; n<=argc; n++) {
		for (int k=1; k<=4; k++) if (!strcmp(argv[n], args[k].str)) {
			*args[k].param = args[k].val;
			printf("%s\n", args[k].str);
		}
	}
}

void instant_quit(int s) {
	printf("MAIN: Quit signal");
	Game::Dispose();
	Draw::Dispose();
	Video::Dispose();
   Sound::Dispose();
   Dispose_Assets();
	Video::SetScreen(1);
	exit(s);
}

int main(int argc, char* argv[]) {
	signal(SIGINT, instant_quit);	
	parseParams(argc, argv);

	Game_Timer_Init();

   if (!Init_Assets() || !Video::Init()) {
		printf("ERROR: Game failed to initialize!\n");
		printf("Not enough memory or compatiblility issues\n");
		printf("Start this game in real-mode\n");
		instant_quit(1);
	}
	
	delay(1000); // show info delay
	Video::SetScreen(MD13H);

	Sound::Init(__DEVICE, __MIDI);
	Draw::Init();
	Game::Intro();

	if (Game::MainScreen() != Escape) 
	{
   	Action action = Enter;
		while (action != Escape) 
		{
			Game::Start(action);
			action = Game::Play();
		}
	}

	if (!__DEBUG) Game::Exit();

	Game::Dispose();
	Draw::Dispose();
	Video::Dispose();
   Sound::Dispose();
   Dispose_Assets();

	Video::SetScreen(1);
	Video::SplashScreen();

	if (__DEBUG) Log_Dump();
	return 0;
};