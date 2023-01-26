#pragma once
#include <cstdint>
#include <immintrin.h>

const int WINDOW_SIZE = 1024;
const int PIXEL_COUNT = WINDOW_SIZE * WINDOW_SIZE;

const int CELL_WIDTH = 1; // code in draw is hard-coded to have CELL_WIDTH = 1
const int CELLS_SIZE = WINDOW_SIZE*4 / CELL_WIDTH;

#define BYTE std::uint_fast8_t

const std::uint_fast8_t NEIGHBOR_COUNT = 8;
const std::uint_fast8_t NEIGHBOR_COUNT_LOG_2 = 3;
const std::uint_fast32_t OUTPUT_SPACE_SIZE = 1 << NEIGHBOR_COUNT;
const std::uint_fast32_t AVX_SIZE = 256;

union AvxArray {
    alignas(32) BYTE bytes[32];
    __m256i avx;
};