#include "simd_life.h"
#include "utility/utility.h"
#include <immintrin.h>
#include <stdlib.h>
#include <string.h>
#include <random>

// each AVX-256 block is 16x16
SIMDLife::SIMDLife(int width, int height, std::random_device& rd) : 
	width(width), height(height), 
	blocksW(width/16), blocksH(height/16),
	eng(rd()), dist(0, 255)
{
	this->cells = new AvxArray*[blocksH];
	this->nextCells = new AvxArray*[blocksH];
	this->drawCells = new AvxArray*[blocksH];
	this->blockPixels = (BYTE*)_mm_malloc(sizeof(BYTE) * 256, 32);
}

SIMDLife::~SIMDLife() {
	for (int i = 0; i < blocksH; i++) {
		free(cells[i]);
		free(nextCells[i]);
		free(drawCells[i]);
	}
	delete[] cells;
	delete[] nextCells;
	delete[] drawCells;
	free(this->blockPixels);
}

void SIMDLife::setup() {
	for (int i = 0; i < blocksH; i++) {
		cells[i] = (AvxArray*)_mm_malloc(sizeof(AvxArray)*blocksW, 32);
		nextCells[i] = (AvxArray*)_mm_malloc(sizeof(AvxArray)*blocksW, 32);
		drawCells[i] = (AvxArray*)_mm_malloc(sizeof(AvxArray)*blocksW, 32);

		for (int j = 0; j < blocksW; j++) {
			memset(cells[i][j].bytes, 0x00, 32);
		}
	}

	// Utility::drawPackedRLE(
	// 	"11bo38b$10b2o38b$9b2o39b$10b2o2b2o34b$38bo11b$38b2o8b2o$39b2o7b2o$10b2o2b2o18b2o2b2o10b$2o7b2o39b$2o8b2o38b$11bo38b$34b2o2b2o10b$39b2o9b$38b2o10b$38bo!",
	// 	400, 400, false, false, cells
	// );

	// Utility::drawPackedRLE(
	// 	"bob$2bo$3o!",
	// 	1, 10, false, true, cells
	// );


	for (int i = 0; i < blocksH; i++) {
		for (int j = 0; j < blocksW; j++) {
			for (int k = 0; k < 32; k++) {
				cells[i][j].bytes[k] = (dist(eng) & dist(eng)) & 0xFF;
				// if (j < 16 || j >= 32) continue;
				// cells[i][j].bytes[k] = ((i+j) % 2 == 0) ? 0x00 : (0b1010101010101010 >> (k%3));
			}
		}
	}

	for (int i = 0; i < blocksH; i++) {
		for (int j = 0; j < blocksW; j++) {
			memcpy(nextCells[i][j].bytes, cells[i][j].bytes, 32);
			memcpy(drawCells[i][j].bytes, cells[i][j].bytes, 32);
		}
	}
}

void SIMDLife::tick() {
	for (int i = 0; i < blocksH; i++) {
		for (int j = 0; j < blocksW; j++) {
			Utility::nextState(cells, i, j, nextCells, blocksH, blocksW, blockLookup);
		}
	}

	for (int i = 0; i < blocksH-1; i++) {
		for (int j = 0; j < blocksW; j += 16) {
			Utility::nextStateEdgeH(cells, i, j, blocksW, nextCells);
		}
	}

	for (int i = 0; i < blocksH; i += 16) {
		for (int j = 0; j < blocksW-1; j++) {
			Utility::nextStateEdgeV(cells, i, j, nextCells);
		}
	}

	swapMutex.lock();
	std::swap(cells, nextCells);
	swapMutex.unlock();
}

void SIMDLife::draw(BYTE* pixelBuffer, int px0, int py0) {
	swapMutex.lock();
	for (int i = 0; i < blocksH; i++) {
		for (int j = 0; j < blocksW; j++) {
			memcpy(drawCells[i][j].bytes, cells[i][j].bytes, 32);
		}
	}
	swapMutex.unlock();

	int startBlockI = py0 / 16;
	int startBlockJ = px0 / 16;
	// last needed block (inclusive)
	int endBlockI = (py0 + WINDOW_HEIGHT - 1) / 16;	// need -1 to not include the last block if the blocks fit perfectly
	int endBlockJ = (px0 + WINDOW_WIDTH - 1) / 16;	// if px0 = 0, WINDOW_WIDTH = 16, then we only need block 0

	AvxBitArray block;

	for (int i = startBlockI; i <= endBlockI; i++) {
		for (int j = startBlockJ; j <= endBlockJ; j++) {
			block.setAll(drawCells[i][j]);
			for (int k = 0; k < 8; k++) {
				block.spreadToBytes(k, blockPixels + (k * 32));
			}

			int blockSY = i * 16 - py0;
			int blockSX = j * 16 - px0;
			
			int sdy = (blockSY < 0) ? -blockSY : 0;
			int sdyLen = WINDOW_HEIGHT - blockSY;
			if (sdyLen > 16) sdyLen = 16;

			int sdx = 0;
			int sdxLen = 16;
			if (blockSX < 0) {
				sdx = -blockSX;
				sdxLen += blockSX;
				blockSX = 0;
			} else if (WINDOW_WIDTH - blockSX < 16) {
				sdxLen = WINDOW_WIDTH - blockSX;
			}

			for (; sdy < sdyLen; sdy++) {
				int sy = blockSY + sdy;
				memcpy(pixelBuffer + (sy * WINDOW_WIDTH + blockSX), blockPixels + (sdy * 16 + sdx), sdxLen);
			}
		}
	}
}