#pragma once
#include <cstdint>
#include <immintrin.h>

#define BYTE std::uint_fast8_t

const std::uint_fast8_t NEIGHBOR_COUNT = 8;
const std::uint_fast8_t NEIGHBOR_COUNT_LOG_2 = 3;
const std::uint_fast32_t OUTPUT_SPACE_SIZE = 1 << NEIGHBOR_COUNT;
const std::uint_fast32_t AVX_SIZE = 256;


const int WINDOW_WIDTH = AVX_SIZE * 7;   // must be a factor of AVX_SIZE
const int WINDOW_HEIGHT = 1000;
const int PIXEL_COUNT = WINDOW_WIDTH * WINDOW_HEIGHT;

const int CELLS_WIDTH = WINDOW_WIDTH;
const int CELLS_HEIGHT = WINDOW_HEIGHT;

union AvxArray {
    alignas(32) BYTE bytes[32];
    __m256i avx;
};