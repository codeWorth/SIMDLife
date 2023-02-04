#pragma once
#include "avx_bit_array.h"
#include <bitset>

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

	void nextState(BYTE** cells, int row, int column, BYTE* nextCells) {
		AvxBitArray neighbors[8];
		AvxBitArray state = AvxBitArray(cells[row] + column);

		

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