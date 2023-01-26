#pragma once
#include "avx_bit_array.h"
#include <string>

struct Swap {
    BYTE i;
    BYTE j;

    Swap(BYTE i, BYTE j) : i(i), j(j) {}; 

    std::string toString() const {
        std::string out{'0' + i, ',', ' ', '0' + j};
        return out;
    }
};

bool getBit(BYTE byte, int index) {
    return byte & (1 << index);
}

// Conways game of life's rules heavily depend on the differences between 0 to 3 neighbors
// Past that, the exact number is very important.
// Therefore, only the first 3 1s must be sorted correctly.
// A valid sort is one in which the low bits contain 0s and the high bits contain 1s.
bool validSort(BYTE sort) {
    int onesCount = 0;
    for (int i = 0; i < 8; i++) {
        if (getBit(sort, i)) onesCount++;
    }

    // Checks that the 1s are all packed in the high bits
    // Doesn't check past the 3rd 1 for reasons listed above
    if (onesCount > 3) onesCount = 3;
    for (int i = 0; i < onesCount; i++) {
        int bitIndex = 7 - i;
        if (!getBit(sort, bitIndex)) return false;
    }

    return true;
}

AvxArray unallowedOutputs() {
    AvxBitArray outputs;

    for (int i = 0; i < AVX_SIZE; i++) {
        outputs.set(i, !validSort(i));
    }

    return outputs.toArray();
}

// Output contains 1 where the index has kth bit = 1
// 0 <= k < 8
AvxArray onesMask(int k) {
    AvxBitArray outputs;

    for (int i = 0; i < AVX_SIZE; i++) {
        outputs.set(i, getBit(i, k));
    }

    return outputs.toArray();
}

const AvxArray UNALLOWED_OUTPUTS = unallowedOutputs();
const AvxArray ONES_MASKS[8] = {
    onesMask(0),
    onesMask(1),
    onesMask(2),
    onesMask(3),
    onesMask(4),
    onesMask(5),
    onesMask(6),
    onesMask(7)
};

// modifies the list of outputs as if a compare swap was appended
// i must be less than j
void appendCompareSwap(AvxBitArray& outputs, int i, int j) {
    AvxBitArray swappingBits(ONES_MASKS[i]);// contains indecies where ith bit is 1
    AvxBitArray j_Ones(ONES_MASKS[j]);      // contains indecies where jth bit is 1

    j_Ones.invert();            // contains indecies where jth bit is 0
    swappingBits &= j_Ones;     // contains indecies where ith bit is 1 and jth bit is 0

    swappingBits &= outputs;    // contains only indecies where output is 1 and the bit will be swapped
    outputs &= ~swappingBits;   // set outputs to 0 where bits will be swapped out

    int delta = (1 << j) - (1 << i);
    swappingBits >>= delta;     // shift bits over to where they will be swapped to
    outputs |= swappingBits;    // add those bits in
}

void appendCompareSwap(AvxBitArray& outputs, const Swap& swap) {
    appendCompareSwap(outputs, swap.i, swap.j);
}