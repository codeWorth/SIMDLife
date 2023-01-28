#pragma once
#include "avx_bit_array.h"
#include <bitset>
#include <iostream>

const uint32_t LSB = 0b1;
const uint32_t MSB = 0b10000000000000000000000000000000;

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

	void shiftLeft(const AvxBitArray& bits, AvxBitArray& shiftedBits, BYTE rightBorder) {
		// When drawn onscreen, the least byte (index 0-7) will be on the left.
		// Therefore, shifting left by 8 should remove this byte.
		// Counterintuitively, this is accomplished by a right shift.
		shiftedBits = bits >> 1;
		if ((rightBorder & 0x01) != 0) {
			__m256i rightmost = _mm256_set_epi64x(0x8000000000000000, 0, 0, 0);
			shiftedBits |= rightmost;
		}
	}

	void shiftRight(const AvxBitArray& bits, AvxBitArray& shiftedBits, BYTE leftBorder) {
		shiftedBits = bits << 1;
		if ((leftBorder & 0x80) != 0) {
			__m256i leftmost = _mm256_set_epi64x(0, 0, 0, 1);
			shiftedBits |= leftmost;
		}
	}

	void nextState(BYTE** cells, int row, int column, BYTE* nextCells) {
		auto state = AvxBitArray(cells[row] + column);

		AvxBitArray neighbors[8];
		neighbors[1].setAll(cells[row-1] + column); 						// top middle
		neighbors[6].setAll(cells[row+1] + column); 						// bottom middle
		shiftRight(neighbors[1], neighbors[0], cells[row-1][column-1 ]);	// top left
		shiftLeft (neighbors[1], neighbors[2], cells[row-1][column+32]);	// top right
		shiftRight(state,        neighbors[3], cells[row  ][column-1 ]);	// middle left
		shiftLeft (state,        neighbors[4], cells[row  ][column+32]);	// middle right
		shiftRight(neighbors[6], neighbors[5], cells[row+1][column-1 ]);	// bottom left
		shiftLeft (neighbors[6], neighbors[7], cells[row+1][column+32]);	// bottom right

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

		auto moreThan3_a = neighbors[0] | neighbors[1];
		auto moreThan3_b = neighbors[2] | neighbors[3];
		auto moreThan3 = moreThan3_a | moreThan3_b;
		moreThan3 = moreThan3 | neighbors[4];

		auto atLeast2 = neighbors[6] & neighbors[7];
		auto exactlyThree = neighbors[5].and_not(moreThan3);
		auto twoOrThree = atLeast2.and_not(moreThan3);

		// if we have exactly 3 neighbors we're alive regardless of current state
		// if we have 2 or 3 neighbors, we can be alive if are already
		auto result = exactlyThree | (twoOrThree & state); 
		result.write(nextCells);
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