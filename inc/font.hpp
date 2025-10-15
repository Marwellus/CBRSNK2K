#ifndef FONT_HPP
#define FONT_HPP
#include "inc/types.h"
class Font {
	private:
		void createTable(char* buffer);
	public:
		Font(char* fileName);
		byte Table[256][8];
		int Load(char* filename);
		byte Micro[10][8];
};

#endif