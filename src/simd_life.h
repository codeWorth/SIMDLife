#pragma once

#include <mutex>
#include <random>

#include "life.h"
#include "constants.h"

class SIMDLife: Life {
public:
	SIMDLife(int width, int height, std::random_device& rd);
	~SIMDLife();

	void setup();
	void tick();
	void draw(BYTE* pixelBuffer, int& px0, int& py0);

private:
	
	std::default_random_engine eng;
	std::uniform_int_distribution<uint8_t> dist;
	const int width;
	const int height;
	const int chunksAcross;
	const int chunksDown;

	// cells are stored in 16x16 squares, squares are arranged as normal (scan across)
	BYTE** cells;
	BYTE** nextCells;

	std::mutex swapMutex;
	BYTE** drawCells;

};