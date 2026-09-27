#pragma once

#include "bass.h"

class Wave {
public:
	static void SetVolume(int vol);
	static int GetVolume();

	Wave(char *filename);
	~Wave();
	void play(bool ignore_settings = false);
	bool isok();
private:
	HSAMPLE sample;
	static int currVol;
};
