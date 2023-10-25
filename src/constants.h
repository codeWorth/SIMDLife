#pragma once
#include <cstdint>
#include <immintrin.h>

#define BYTE std::uint_fast8_t

const std::uint_fast8_t NEIGHBOR_COUNT = 8;
const std::uint_fast8_t NEIGHBOR_COUNT_LOG_2 = 3;
const std::uint_fast32_t OUTPUT_SPACE_SIZE = 1 << NEIGHBOR_COUNT;
const std::uint_fast32_t AVX_SIZE = 256;


const int WINDOW_WIDTH = 112 * 16; // things seem to break when this isn't a nice number, ideally it would be - 2 for width and height
const int WINDOW_HEIGHT = 63 * 16;

const int CELLS_WIDTH = AVX_SIZE * 28;   // must be a factor of AVX_SIZE
const int CELLS_HEIGHT = AVX_SIZE * 13;   // must be a factor of AVX_SIZE  

union AvxArray {
    alignas(32) BYTE bytes[32];
    __m256i avx;

    AvxArray& operator=(const AvxArray& other) {
        memcpy(bytes, other.bytes, 32);
        return *this;
    }
};