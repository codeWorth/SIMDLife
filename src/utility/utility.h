#pragma once
#include "XXHash32.h"
#include "avx_bit_array.h"
#include <iostream>

namespace Utility {
	using namespace std;
	BYTE* outputBlock = (BYTE*)_mm_malloc(sizeof(BYTE)*32, 32);

	// Swaps i and j if i > j
	// This algorithm to compare and swap two vectors bitwise was proposed by Astrid Yu
	// Check out her website: https://astrid.tech/
	#define CMP_SWAP(i, j) { \
		auto x = neighbors[i]; \
		neighbors[i] = x & neighbors[j]; \
		neighbors[j] = x | neighbors[j]; \
	}

	AvxBitArray doCmpSwap(AvxBitArray* neighbors, const AvxBitArray& state) {
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

		auto moreThan3_a = neighbors[0] | neighbors[1];
		auto moreThan3_b = neighbors[2] | neighbors[3];
		auto moreThan3 = moreThan3_a | moreThan3_b;
		moreThan3 = moreThan3 | neighbors[4];

		auto atLeast2 = neighbors[6] & neighbors[7];
		auto exactlyThree = neighbors[5].and_not(moreThan3);
		auto twoOrThree = atLeast2.and_not(moreThan3);

		// if we have exactly 3 neighbors we're alive regardless of current state
		// if we have 2 or 3 neighbors, we can be alive if are already 
		return exactlyThree | (twoOrThree & state);
	}

	void nextState(
		AvxArray** cells, int row, int column, 
		AvxArray** nextCells, int nRows, int nColumns
	) {
		AvxBitArray state = AvxBitArray(cells[row][column].bytes);
		AvxBitArray neighbors[8];

		neighbors[3] = state.shiftWordsLeft<1>();			// middle left
		neighbors[4] = state.shiftWordsRight<1>();			// middle right
		neighbors[0] = neighbors[3].shiftLeftBytes<2>();	// top left
		neighbors[1] = state.shiftLeftBytes<2>();			// top middle
		neighbors[2] = neighbors[4].shiftLeftBytes<2>();	// top right
		neighbors[5] = neighbors[3].shiftRightBytes<2>();	// bottom left
		neighbors[6] = state.shiftRightBytes<2>();			// bottom middle
		neighbors[7] = neighbors[4].shiftRightBytes<2>();	// bottom right

		AvxBitArray result = doCmpSwap(neighbors, state);
		result &= _mm256_set_epi64x(0x7FFE7FFE7FFE, 0x7FFE7FFE7FFE7FFE, 0x7FFE7FFE7FFE7FFE, 0x7FFE7FFE7FFE0000); // crop edges out
		result.write(nextCells[row][column].bytes);
	}
	
	AvxBitArray gatherRow(AvxArray** cells, int row, int column, int byteStart) {
		AvxBitArray lower = AvxBitArray(_mm256_i32gather_epi32(
			(int*)cells[row][column].bytes,
			_mm256_set_epi32(byteStart+64*7, byteStart+64*6, byteStart+64*5, byteStart+64*4, byteStart+64*3, byteStart+64*2, byteStart+64, byteStart),
			1
		)); 
		AvxBitArray upper = AvxBitArray(_mm256_i32gather_epi32(
			(int*)cells[row][column].bytes,
			_mm256_set_epi32(byteStart+30+64*7, byteStart+30+64*6, byteStart+30+64*5, byteStart+30+64*4, byteStart+30+64*3, byteStart+30+64*2, byteStart+30+64, byteStart+30),
			1
		));
		return lower.blend(upper, _mm256_set1_epi32(0xFFFF0000));
	}

	void shiftLeft(const AvxBitArray& bits, AvxBitArray& shiftedBits, BYTE leftBorder) {
		shiftedBits = bits.shiftLeft<1>();
		if ((leftBorder & 0x80) != 0) {
			__m256i leftmost = _mm256_set_epi64x(0, 0, 0, 1);
			shiftedBits |= leftmost;
		}
	}

	void shiftRight(const AvxBitArray& bits, AvxBitArray& shiftedBits, BYTE rightBorder) {
		shiftedBits = bits.shiftRight<1>();
		if ((rightBorder & 0x01) != 0) {
			__m256i rightmost = _mm256_set_epi64x(0x8000000000000000, 0, 0, 0);
			shiftedBits |= rightmost;
		}
	}

	// processes the bottom edge of the given row, as well as the top edge of the row below it
	// column % 16 == 0, 0 <= row < nRows-1, 0 <= column < nColumns-16
	void nextStateEdgeH(AvxArray** cells, int row, int column, int nColumns, AvxArray** nextCells) {
		AvxBitArray lower = gatherRow(cells, row, column, 30);
		AvxBitArray upper = gatherRow(cells, row+1, column, 0);

		AvxBitArray neighborAbove = gatherRow(cells, row, column, 28);
		AvxBitArray neighborBelow = gatherRow(cells, row+1, column, 2);
		
		int mc = nColumns-16;
		AvxBitArray neighbors[8];
		shiftLeft(lower, neighbors[3], column > 0 ? cells[row][column-1].bytes[31] : 0);			// middle left
		shiftRight(lower, neighbors[4], column < mc ? cells[row][column+16].bytes[30] : 0);			// middle right
		shiftLeft(neighborAbove, neighbors[0], column > 0 ? cells[row][column-1].bytes[29] : 0);	// top left
		neighbors[1] = neighborAbove;																// top middle
		shiftRight(neighborAbove, neighbors[2], column < mc ? cells[row][column+16].bytes[28] : 0);	// top right
		shiftLeft(upper, neighbors[5], column > 0 ? cells[row+1][column-1].bytes[1] : 0);			// bottom left
		neighbors[6] = upper;																		// bottom middle
		shiftRight(upper, neighbors[7], column < mc ? cells[row+1][column+16].bytes[0] : 0);		// bottom right

		AvxBitArray neighbors2[8];
		neighbors2[0] = neighbors[3];				// top left
		neighbors2[1] = lower;						// top middle
		neighbors2[2] = neighbors[4];				// top right
		neighbors2[3] = neighbors[5];				// middle left
		neighbors2[4] = neighbors[7];				// middle right
		shiftLeft(neighborBelow, neighbors2[5], column > 0 ? 0 : cells[row+1][column-1].bytes[3]);	// bottom left
		neighbors2[6] = neighborBelow;				// bottom middle
		shiftRight(neighborBelow, neighbors2[7], column < mc ? 0 : cells[row+1][column+16].bytes[2]);// bottom right

		AvxBitArray resultLower = doCmpSwap(neighbors, lower);
		AvxBitArray resultUpper = doCmpSwap(neighbors2, upper);

		resultLower.write(outputBlock);
		for (int k = 0; k < 16; k++) {
			nextCells[row][column+k].bytes[30] = outputBlock[k*2];
			nextCells[row][column+k].bytes[31] = outputBlock[k*2 + 1];
		}

		resultUpper.write(outputBlock);
		for (int k = 0; k < 16; k++) {
			nextCells[row+1][column+k].bytes[0] = outputBlock[k*2];
			nextCells[row+1][column+k].bytes[1] = outputBlock[k*2 + 1];
		}
	}
	
	// processes the right edge of the given column, as well as the left edge of the row to the right of it
	// row % 16 == 0, 0 <= row < nRows-16, 0 <= column < nColumns-1
	void nextStateEdgeV(AvxArray** cells, int row, int column, AvxArray** nextCells) {
		AvxBitArray rightEdgeMask = AvxBitArray(_mm256_set1_epi16(0x8000));
		AvxBitArray rightEdge = AvxBitArray(cells[row][column].bytes);
		for (int k = 1; k < 16; k++) {
			rightEdge.shiftWordsRightInPlace<1>();
			AvxBitArray nextEdge = AvxBitArray(cells[row+k][column].bytes);
			nextEdge &= rightEdgeMask;
			rightEdge |= nextEdge;
		}

		AvxBitArray leftEdge = AvxBitArray(cells[row][column+1].bytes);
		leftEdge.shiftWordsLeftInPlace<15>();
		for (int k = 1; k < 16; k++) {
			leftEdge.shiftWordsRightInPlace<1>();
			AvxBitArray nextEdge = AvxBitArray(cells[row+k][column+1].bytes);
			nextEdge.shiftWordsLeftInPlace<15>();
			leftEdge |= nextEdge;
		}

		AvxBitArray leftNeighborMask = AvxBitArray(_mm256_set1_epi16(0x4000));
		AvxBitArray leftNeighbor = AvxBitArray(cells[row][column].bytes);
		leftNeighbor &= leftNeighborMask;
		for (int k = 1; k < 15; k++) {
			leftNeighbor.shiftWordsRightInPlace<1>();
			AvxBitArray nextEdge = AvxBitArray(cells[row+k][column].bytes);
			nextEdge &= leftNeighborMask;
			leftNeighbor |= nextEdge;
		}
		AvxBitArray nextEdge = AvxBitArray(cells[row+15][column].bytes);
		nextEdge &= leftNeighborMask;
		nextEdge.shiftWordsLeftInPlace<1>();
		leftNeighbor |= nextEdge;

		AvxBitArray rightNeighborMask = AvxBitArray(_mm256_set1_epi16(0x0002));
		AvxBitArray rightNeighbor = AvxBitArray(cells[row][column+1].bytes);
		rightNeighbor &= rightNeighborMask;
		rightNeighbor.shiftWordsRightInPlace<1>();
		for (int k = 1; k < 16; k++) {
			AvxBitArray nextEdge = AvxBitArray(cells[row+k][column+1].bytes);
			nextEdge &= rightNeighborMask;
			nextEdge.shiftWordsLeftInPlace(k-1);
			rightNeighbor |= nextEdge;
		}

		AvxBitArray neighborsR[8];
		neighborsR[3] = leftNeighbor;						// middle left
		neighborsR[4] = leftEdge;							// middle right
		neighborsR[0] = neighborsR[3].shiftLeftBytes<2>();	// top left
		neighborsR[1] = rightEdge.shiftLeftBytes<2>();		// top middle
		neighborsR[2] = neighborsR[4].shiftLeftBytes<2>();	// top right
		neighborsR[5] = neighborsR[3].shiftRightBytes<2>();	// bottom left
		neighborsR[6] = rightEdge.shiftRightBytes<2>();		// bottom middle
		neighborsR[7] = neighborsR[4].shiftRightBytes<2>();	// bottom right

		AvxBitArray neighborsL[8];
		neighborsL[3] = rightEdge;							// middle left
		neighborsL[4] = rightNeighbor;						// middle right
		neighborsL[0] = neighborsR[1];						// top left
		neighborsL[1] = neighborsR[2];						// top middle
		neighborsL[5] = neighborsR[6];						// bottom left
		neighborsL[6] = neighborsR[7];						// bottom middle
		neighborsL[2] = neighborsL[4].shiftLeftBytes<2>();	// top right
		neighborsL[7] = neighborsL[4].shiftRightBytes<2>();	// bottom right

		AvxBitArray cropTops = _mm256_set_epi16(
			0x0000, 
			0xFFFF, 
			0xFFFF, 
			0xFFFF, 
			0xFFFF, 
			0xFFFF, 
			0xFFFF, 
			0xFFFF, 
			0xFFFF, 
			0xFFFF, 
			0xFFFF, 
			0xFFFF, 
			0xFFFF, 
			0xFFFF, 
			0xFFFF, 
			0x0000
		);
		rightEdgeMask &= cropTops;

		AvxBitArray resultRight = doCmpSwap(neighborsR, rightEdge);
		AvxBitArray resultLeft = doCmpSwap(neighborsL, leftEdge);

		for (int k = 15; k >= 0; k--) {
			AvxBitArray cur(nextCells[row+k][column].bytes);
			AvxBitArray edge = resultRight & rightEdgeMask;
			cur |= edge;
			cur.write(nextCells[row+k][column].bytes);
			resultRight.shiftWordsLeftInPlace<1>();
		}

		AvxBitArray leftEdgeMask = rightEdgeMask.shiftWordsRight<15>();
		for (int k = 0; k < 16; k++) {
			AvxBitArray cur(nextCells[row+k][column+1].bytes);
			AvxBitArray edge = resultLeft & leftEdgeMask;
			cur |= edge;
			cur.write(nextCells[row+k][column+1].bytes);
			resultLeft.shiftWordsRightInPlace<1>();
		}
	}

	void drawPackedRLE(const char* rle, int x, int y, bool invertX, bool invertY, AvxArray** cells) {
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
					int chunkX = x / 16;
					int chunkY = y / 16;
					int subChunkX = x % 16;
					int subChunkY = y % 16;
					int vectorIndex = subChunkY * 16 + subChunkX;
					int vectorByte = vectorIndex / 8;
					int vectorBit = vectorIndex % 8;

					if (y >= 0 && x >= 0) cells[chunkY][chunkX].bytes[vectorByte] |= 1 << vectorBit;

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