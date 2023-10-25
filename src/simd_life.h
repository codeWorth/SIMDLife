#pragma once

#include <mutex>
#include <random>

#include "life.h"
#include "constants.h"
#include "utility/XXHash32.h"
#include <unordered_set>

class SIMDLife: Life {
public:
	SIMDLife(int width, int height, std::random_device& rd);
	~SIMDLife();

	void setup();
	void tick();
	void draw(BYTE* pixelBuffer, int px0, int py0);

private:
	
	std::default_random_engine eng;
	std::uniform_int_distribution<uint8_t> dist;
	const int width; // given width in cells
	const int height; // given height in cells
	const int blocksW; // width in blocks (block is 16x16 cells)
	const int blocksH; // height in blocks

	AvxArray** cells;
	AvxArray** nextCells;

	std::mutex swapMutex;
	AvxArray** drawCells;

	BYTE* blockPixels;
};