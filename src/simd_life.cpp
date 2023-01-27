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

	Utility::drawPackedRLE(
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
	for (int i = 1; i < size+1; i++) {
		for (int j = 32; j < rowLen-1; j += 32) {
			Utility::nextState(cells, i, j, nextCells[i]);
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

	// 0th byte has only lowest bit, 1st byte has 2nd lowest bit, 2nd byte has 3rd lowest bit, etc
	__m256i maskBits = _mm256_set1_epi64x(0x0102040810204080LL);
	__m256i ones = _mm256_set1_epi8(0xFF);

	for (int pixI = 0; pixI < WINDOW_SIZE; pixI += CELL_WIDTH) {
		int cellI = pixI / CELL_WIDTH + 1;

		for (int pixJ = 0; pixJ < WINDOW_SIZE; pixJ += 32) { // 256 bits = 32 bytes = 32 pixels at a time
			int cellJ = pixJ / 8 / CELL_WIDTH + 32;
			__m128i cellArray = _mm_load_si128((__m128i*)&drawCells[cellI][cellJ]);

			__m256i A = _mm256_broadcastb_epi8(cellArray);	// spread each byte across the entire 256 bit vector
			cellArray = _mm_srli_epi32(cellArray, 8);
			__m256i B = _mm256_broadcastb_epi8(cellArray);
			cellArray = _mm_srli_epi32(cellArray, 8);
			__m256i C = _mm256_broadcastb_epi8(cellArray);
			cellArray = _mm_srli_epi32(cellArray, 8);
			__m256i D = _mm256_broadcastb_epi8(cellArray);

			A = _mm256_blend_epi32(A, B, 0b11001100);	// put A in the lowest 64 bits and B in the next lowest 64 bits
			C =  _mm256_blend_epi32(C, D, 0b11001100);
			__m256i fullBytes = _mm256_blend_epi32(A, C, 0b11110000);	// put A/B in the bottom bits and C/D in the top bits

			fullBytes = _mm256_and_si256(fullBytes, maskBits);	// extract each packed bit to be the only one present in the byte
			fullBytes = _mm256_xor_si256(fullBytes, ones); // bitwise not, only the bytes with a high bit will be != 0xFF
			fullBytes = _mm256_cmpeq_epi8(fullBytes, ones);	// check which bytes had a 1, filling the byte with 1s if they do
			
			_mm256_store_si256((__m256i*)&pixelBuffer[pixI * WINDOW_SIZE + pixJ], fullBytes);
			
		}
	}
}