#include "simd_life.h"
#include "utility/utility.h"
#include <immintrin.h>
#include <stdlib.h>
#include <random>

SIMDLife::SIMDLife(int size, std::random_device& rd) : 
	size(size), eng(rd()), dist(0, 255), rowLen(size/8+33)
{
	this->cells = new BYTE*[size+2];
	this->nextCells = new BYTE*[size+2];
	this->drawCells = new BYTE*[size+2];
}

SIMDLife::~SIMDLife() {
	for (int i = 0; i < size+2; i++) {
		delete[] cells[i];
		delete[] nextCells[i];
		delete[] drawCells[i];
	}
	delete[] cells;
	delete[] nextCells;
	delete[] drawCells;
}

void SIMDLife::setup() {
	for (int i = 0; i < size+2; i++) {
		cells[i] = (BYTE*)_mm_malloc(sizeof(BYTE)*rowLen, 32);
		nextCells[i] = (BYTE*)_mm_malloc(sizeof(BYTE)*rowLen, 32);
		drawCells[i] = (BYTE*)_mm_malloc(sizeof(BYTE)*rowLen, 32);

		for (int j = 0; j < rowLen; j++) {
			cells[i][j] = 0x00;
		}
	}

	// Utility::drawPackedRLE(
	// 	"11bo38b$10b2o38b$9b2o39b$10b2o2b2o34b$38bo11b$38b2o8b2o$39b2o7b2o$10b2o2b2o18b2o2b2o10b$2o7b2o39b$2o8b2o38b$11bo38b$34b2o2b2o10b$39b2o9b$38b2o10b$38bo!",
	// 	400, 400, false, false, cells
	// );

	// Utility::drawPackedRLE(
	// 	"bob$2bo$3o!",
	// 	240, 900, false, false, cells
	// );

	for (int i = 1; i < size+1; i++) {
		for (int j = 32; j < rowLen-1; j++) {
			cells[i][j] = (dist(eng) & dist(eng)) & 0xFF;
			nextCells[i][j] = cells[i][j];
			drawCells[i][j] = cells[i][j];
		}
	}
}

void SIMDLife::tick() {
	for (int i = 1; i < size+1; i++) {
		for (int j = 32; j < rowLen-1; j += 32) {
			Utility::nextState(cells, i, j, nextCells[i] + j);
		}
	}

	swapMutex.lock();
	std::swap(cells, nextCells);
	swapMutex.unlock();
}

void SIMDLife::draw(BYTE* pixelBuffer, int px0, int py0) {
	swapMutex.lock();
	for (int i = 1; i < size+1; i++) {
		memcpy(drawCells[i], cells[i], rowLen);
	}
	swapMutex.unlock();

	AvxBitArray row;
	for (int pixI = 0; pixI < WINDOW_SIZE; pixI += 1) {
		int cellI = pixI + 1 - py0;

		for (int pixJ = 0; pixJ < WINDOW_SIZE; pixJ += 32) { // 256 bits means we can only process 32 pixels at a time

			if (cellI < 1 || cellI >= size+1) {
				row.zero();
			} else {
				int cellJ = (pixJ - px0) / 8 + 32;
				// row <<= px0 % 8;
				row.setAllUnaligned(drawCells[cellI] + cellJ);
				row.spreadToBytes();
			}

			row.write(pixelBuffer + (pixI * WINDOW_SIZE + pixJ));
		}
	}
}