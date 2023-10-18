#pragma once
#include <cstdint>
#include <immintrin.h>

#define BYTE std::uint_fast8_t

const std::uint_fast8_t NEIGHBOR_COUNT = 8;
const std::uint_fast8_t NEIGHBOR_COUNT_LOG_2 = 3;
const std::uint_fast32_t OUTPUT_SPACE_SIZE = 1 << NEIGHBOR_COUNT;
const std::uint_fast32_t AVX_SIZE = 256;


const int WINDOW_WIDTH = 112 * 16 - 2;  // must be a factor of 16 - 2 (border should not be shown)
const int WINDOW_HEIGHT = 63 * 16 - 2;  // must be a factor of 16 - 2 (border should not be shown)
const int PIXEL_COUNT = WINDOW_WIDTH * WINDOW_HEIGHT;

const int CELLS_WIDTH = 112 * 16 * 4;   // must be a factor of 16
const int CELLS_HEIGHT = 63 * 16 * 4;   // must be a factor of 16  

union AvxArray {
    alignas(32) BYTE bytes[32];
    __m256i avx;
};