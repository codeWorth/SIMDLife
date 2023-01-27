#pragma once

#include <mutex>
#include <random>

#include "life.h"
#include "constants.h"

class SIMDLife: Life {
public:
	SIMDLife(int size, std::random_device& rd);
	~SIMDLife();

	void setup();
	void tick();
	void draw(char* pixelBuffer);

private:
	
	std::default_random_engine eng;
	std::uniform_int_distribution<uint8_t> dist;
	const int size;
	const int rowLen;

	BYTE** cells;
	BYTE** nextCells;

	std::mutex swapMutex;
	BYTE** drawCells;

};