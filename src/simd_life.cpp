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
}

SIMDLife::~SIMDLife() {
	for (int i = 0; i < blocksH; i++) {
		delete[] cells[i];
		delete[] nextCells[i];
		delete[] drawCells[i];
	}
	delete[] cells;
	delete[] nextCells;
	delete[] drawCells;
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
	// 	240, 900, false, false, cells
	// );

	// for (int i = 1; i < height+1; i++) {
	// 	for (int j = 32; j < rowLen - 1; j++) {
	// 		cells[i][j] = (dist(eng) & dist(eng)) & 0xFF;
	// 	}
	// }

	for (int i = 0; i < blocksH; i++) {
		for (int j = 0; j < blocksW; j++) {
			memset(cells[i][j].bytes, ((j/32) % 2 == 0) ? 0x00 : 0xFF, 32);
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
			Utility::nextState(cells, i, j, nextCells[i] + j);
		}
	}

	swapMutex.lock();
	std::swap(cells, nextCells);
	swapMutex.unlock();
}

void SIMDLife::draw(BYTE* pixelBuffer, int px0, int py0) {
	return;
	swapMutex.lock();
	for (int i = 0; i < blocksH; i++) {
		for (int j = 0; j < blocksW; j++) {
			memcpy(drawCells[i][j].bytes, cells[i][j].bytes, 32);
		}
	}
	swapMutex.unlock();

	px0++;	// px0, py0 = (0,0) actually draws starting at (1,1) of the cells
	py0++;

	int startBlockI = py0 / 16;
	int startBlockJ = px0 / 16;
	// last needed block (inclusive)
	int endBlockI = (py0 + WINDOW_HEIGHT - 1) / 16;	// need -1 to not include the last block if the blocks fit perfectly
	int endBlockJ = (px0 + WINDOW_WIDTH - 1) / 16;	// if px0 = 0, WINDOW_WIDTH = 16, then we only need block 0

	AvxBitArray block;
	BYTE* blockPixels = (BYTE*)_mm_malloc(sizeof(BYTE) * 256, 32);

	for (int i = startBlockI; i <= endBlockI; i++) {
		for (int j = startBlockJ; j <= endBlockJ; j++) {
			block.setAll(drawCells[i][j]);
			for (int k = 0; k < 8; k++) {
				block.spreadToBytes(k, blockPixels + (k * 32));
			}

			int blockScreenY = i * 16 - py0;
			int blockScreenX = j * 16 - px0;
			for (int sdy = 0; sdy < 16; sdy++) {
				int sy = sdy + blockScreenY;
				if (sy < 0) continue;
				if (sy >= WINDOW_HEIGHT) continue;

				for (int sdx = 0; sdx < 16; sdx++) {
					int sx = sdx + blockScreenX;
					if (sx < 0) continue;
					if (sx >= WINDOW_WIDTH) continue;
					pixelBuffer[sy * WINDOW_WIDTH + sx] = blockPixels[sdy * 16 + sdx];
				}
			}
		}
	}
}