#pragma once
#include "avx_bit_array.h"
#include <bitset>

namespace Utility {
	using namespace std;

	// Swaps i and j if i > j
	// This algorithm to compare and swap two vectors bitwise was proposed by Astrid Yu
	// Check out her website: https://astrid.tech/
	#define CMP_SWAP(i, j) { \
		auto x = neighbors[i]; \
		neighbors[i] = x & neighbors[j]; \
		neighbors[j] = x | neighbors[j]; \
	}

	void nextState(AvxArray** cells, int row, int column, AvxArray** nextCells, int nRows, int nColumns) {
		AvxBitArray neighbors[8];
		AvxBitArray state = AvxBitArray(cells[row][column].bytes);

		neighbors[3] = state.shiftWordsLeft(1);	// middle left
		// if (column > 0) {
		// 	AvxBitArray left = AvxBitArray(cells[row][column - 1].bytes).shiftWordsRight(15);
		// 	neighbors[3] |= left;
		// }

		neighbors[4] = state.shiftWordsRight(1);	// middle right
		// if (column < nColumns-1) {
		// 	AvxBitArray right = AvxBitArray(cells[row][column + 1].bytes).shiftWordsLeft(15);
		// 	neighbors[4] |= right;
		// }

		neighbors[1] = state << 16;			// top middle
		neighbors[0] = neighbors[3] << 16;	// top left
		neighbors[2] = neighbors[4] << 16;	// top right
		// if (row > 0) {
		// 	AvxBitArray above = AvxBitArray(cells[row-1][column].bytes);
		// 	above >>= 256-16;
		// 	neighbors[1] |= above;

		// 	AvxBitArray topLeft = above.shiftWordsLeft(1);
		// 	if (column > 0 && (cells[row-1][column-1].bytes[31] & 0x80) != 0) {
		// 		__m256i topLeftCell = _mm256_set_epi64x(0, 0, 0, 1);
		// 		topLeft |= topLeftCell;
		// 	}
		// 	neighbors[0] |= topLeft;

		// 	AvxBitArray topRight = above.shiftWordsRight(1);
		// 	if (column < nColumns-1 && (cells[row-1][column+1].bytes[30] & 0x01) != 0) {
		// 		__m256i topRightCell = _mm256_set_epi64x(0, 0, 0, 0x8000);
		// 		topRight |= topRightCell;
		// 	}
		// 	neighbors[2] |= topRight;
		// }

		neighbors[6] = state >> 16;			// bottom middle
		neighbors[5] = neighbors[3] >> 16;	// bottom left
		neighbors[7] = neighbors[4] >> 16;	// bottom right
		// if (row < nRows-1) {
		// 	AvxBitArray below = AvxBitArray(cells[row+1][column].bytes);
		// 	below <<= 256-16;
		// 	neighbors[6] |= below;

		// 	AvxBitArray bottomLeft = below.shiftWordsLeft(1);
		// 	if (column > 0 && (cells[row+1][column-1].bytes[1] & 0x80) != 0) {
		// 		__m256i bottomLeftCell = _mm256_set_epi64x(0x1000000000000, 0, 0, 0);
		// 		bottomLeft |= bottomLeftCell;
		// 	}
		// 	neighbors[5] |= bottomLeft;

		// 	AvxBitArray bottomRight = below.shiftWordsRight(1);
		// 	if (column < nColumns-1 && (cells[row+1][column+1].bytes[0] & 0x01) != 0) {
		// 		__m256i bottomRightCell = _mm256_set_epi64x(0x8000000000000000, 0, 0, 0);
		// 		bottomRight |= bottomRightCell;
		// 	}
		// 	neighbors[7] |= bottomRight;
		// }

		// From find_net
		CMP_SWAP(0, 4);
		CMP_SWAP(1, 5);
		CMP_SWAP(2, 6);
		CMP_SWAP(3, 7);

		CMP_SWAP(0, 2);
		CMP_SWAP(1, 3);
		CMP_SWAP(4, 6);
		CMP_SWAP(5, 7);

		CMP_SWAP(2, 3);
		CMP_SWAP(4, 5);
		CMP_SWAP(6, 7);

		CMP_SWAP(3, 5);
		CMP_SWAP(3, 6);
		CMP_SWAP(5, 6);
		
		// From Batcher (wolfram alpha generator)
		// CMP_SWAP(0, 4);
		// CMP_SWAP(1, 5);
		// CMP_SWAP(2, 6);
		// CMP_SWAP(3, 7);
		// CMP_SWAP(0, 2);
		// CMP_SWAP(1, 3);
		// CMP_SWAP(4, 6);
		// CMP_SWAP(5, 7);
		// CMP_SWAP(2, 4);
		// CMP_SWAP(3, 5);
		// CMP_SWAP(0, 1);
		// CMP_SWAP(2, 3);
		// CMP_SWAP(4, 5);
		// CMP_SWAP(6, 7);
		// CMP_SWAP(1, 4);
		// CMP_SWAP(3, 6);
		// CMP_SWAP(1, 2);
		// CMP_SWAP(3, 4);
		// CMP_SWAP(5, 6);

		auto moreThan3_a = neighbors[0] | neighbors[1];
		auto moreThan3_b = neighbors[2] | neighbors[3];
		auto moreThan3 = moreThan3_a | moreThan3_b;
		moreThan3 = moreThan3 | neighbors[4];

		auto atLeast2 = neighbors[6] & neighbors[7];
		auto exactlyThree = neighbors[5].and_not(moreThan3);
		auto twoOrThree = atLeast2.and_not(moreThan3);

		// if we have exactly 3 neighbors we're alive regardless of current state
		// if we have 2 or 3 neighbors, we can be alive if are already
		AvxBitArray result = exactlyThree | (twoOrThree & state); 
		result.write(nextCells[row][column].bytes);
	}

	void drawPackedRLE(const char* rle, int x, int y, bool invertX, bool invertY, BYTE** cells) {
		int charIndex = 0;
		int count = 0;
		int origX = x;

		while (true) {
			char c = rle[charIndex];
			if (c == '!') {
				break;
			}

			if (c == 'b') {
				if (count == 0) {
					count = 1;
				}
				x += count * (invertX ? -1 : 1);
				count = 0;
			} else if (c == 'o') {
				if (count == 0) {
					count = 1;
				}
				while (count > 0) {
					int byteIndex = x / 8;
					int subByteIndex = x - byteIndex * 8;
					cells[y+1][byteIndex+32] |= 1 << subByteIndex;

					x += (invertX ? -1 : 1);
					count--;
				}
			} else if (c == '$') {
				y += (invertY ? 1 : -1);
				x = origX;
			} else {
				int n = c - '0';
				count *= 10;
				count += n;
			}

			charIndex++;
		}
	}

}