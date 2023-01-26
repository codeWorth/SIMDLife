#include "simd_life.h"
#include <immintrin.h>
#include <stdlib.h>
#include <random>

SIMDLife::SIMDLife(int size, std::random_device& rd) : 
	size(size), eng(rd()), dist(0, 255), rowLen(size/8+33)
{
	this->cells = new uint8_t*[size+2];
	this->nextCells = new uint8_t*[size+2];
	this->drawCells = new uint8_t*[size+2];
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
		cells[i] = (uint8_t*)_mm_malloc(sizeof(uint8_t)*rowLen, 32);
		nextCells[i] = (uint8_t*)_mm_malloc(sizeof(uint8_t)*rowLen, 32);
		drawCells[i] = (uint8_t*)_mm_malloc(sizeof(uint8_t)*rowLen, 32);

		for (int j = 0; j < rowLen; j++) {
			cells[i][j] = 0x00;
		}
	}

	util.drawPackedRLE(
		"11bo38b$10b2o38b$9b2o39b$10b2o2b2o34b$38bo11b$38b2o8b2o$39b2o7b2o$10b2o2b2o18b2o2b2o10b$2o7b2o39b$2o8b2o38b$11bo38b$34b2o2b2o10b$39b2o9b$38b2o10b$38bo!",
		400, 400, false, false, cells
	);

	for (int i = 1; i < size+1; i++) {
		for (int j = 32; j < rowLen-1; j++) {
			// cells[i][j] = (dist(eng) & dist(eng)) & 0xFF;
			nextCells[i][j] = cells[i][j];
			drawCells[i][j] = cells[i][j];
		}
	}
}

void SIMDLife::tick() {
	__m256i nextState = _mm256_setzero_si256();
	for (int i = 1; i < size+1; i++) {
		for (int j = 32; j < rowLen-1; j += 32) {
			util.nextState(cells, i, j, nextState);
			_mm256_store_si256((__m256i*)&nextCells[i][j], nextState);
		}
	}

	swapMutex.lock();
	std::swap(cells, nextCells);
	swapMutex.unlock();
}

void SIMDLife::draw(char* pixelBuffer) {
	swapMutex.lock();
	for (int i = 1; i < size+1; i++) {
		memcpy(drawCells[i], cells[i], rowLen);
	}
	swapMutex.unlock();

	__m256i maskBits = _mm256_set1_epi64x(0x0102040810204080LL);
	__m256i zeros = _mm256_setzero_si256();
	__m256i ones = _mm256_set1_epi8(0xFF);

	for (int pixI = 0; pixI < WINDOW_SIZE; pixI += CELL_WIDTH) {
		int cellI = pixI / CELL_WIDTH + 1;

		for (int pixJ = 0; pixJ < WINDOW_SIZE; pixJ += 32) { // 256 bits = 32 bytes = 32 pixels at a time
			int cellJ = pixJ / 8 / CELL_WIDTH + 32;
			uint8_t a = drawCells[cellI][cellJ  ];
			uint8_t b = drawCells[cellI][cellJ+1];
			uint8_t c = drawCells[cellI][cellJ+2];
			uint8_t d = drawCells[cellI][cellJ+3];

			__m256i fullBytes = _mm256_set_epi8( // not sure why reverse arg order needed
				d, d, d, d, d, d, d, d,
				c, c, c, c, c, c, c, c,
				b, b, b, b, b, b, b, b,
				a, a, a, a, a, a, a, a
			);
			fullBytes = _mm256_and_si256(fullBytes, maskBits);
			fullBytes = _mm256_cmpeq_epi8(fullBytes, zeros);
			fullBytes = _mm256_xor_si256(fullBytes, ones); // same as bitwise not
			
			_mm256_store_si256((__m256i*)&pixelBuffer[pixI * WINDOW_SIZE + pixJ], fullBytes);
			
		}
	}
}