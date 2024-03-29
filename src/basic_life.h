#pragma once

#include <stdint.h>
#include <mutex>
#include <random>
#include "life.h"

class BasicLife: Life {
public:
	BasicLife(int width, int height, std::random_device& rd);
	~BasicLife();

	void setup();
	void tick();
	void draw(BYTE* pixelBuffer, int px0, int py0);

private:
	const int width;
	const int height;
	std::default_random_engine eng;
	std::uniform_int_distribution<uint8_t> dist;

	uint8_t** cells;
	uint8_t** nextCells;
	std::mutex swapMutex;
	
};