#pragma once

#include <stdint.h> // for uint32_t and uint64_t
#include <immintrin.h> // for AVX instructions and rotate
#include "avx_bit_array.h"

namespace XXHash32 {

    #define XXH_PRIME32_1  0x9E3779B1U  /*!< 0b10011110001101110111100110110001 */
    #define XXH_PRIME32_2  0x85EBCA77U  /*!< 0b10000101111010111100101001110111 */
    #define XXH_PRIME32_3  0xC2B2AE3DU  /*!< 0b11000010101100101010111000111101 */
    #define XXH_PRIME32_4  0x27D4EB2FU  /*!< 0b00100111110101001110101100101111 */
    #define XXH_PRIME32_5  0x165667B1U  /*!< 0b00010110010101100110011110110001 */

    #define XXH_rotl32(x,r) (((x) << (r)) | ((x) >> (32 - (r))))
    #define AVX_rotl32(x,r) (_mm_or_si128(_mm_slli_epi32((x), (r)), _mm_srli_epi32((x), 32 - (r))))
    #define PS_I(x) _mm_castps_si128((x))
    #define I_PS(x) _mm_castsi128_ps((x))

    extern uint32_t vertAdd(__m128i x);
    extern __m128i XXH32_round(__m128i acc, __m128i input);
    extern uint32_t XXH32_avalanche(uint32_t hash);
    extern uint32_t XXH32_finalize(uint32_t hash, __m128i input);
    extern uint32_t XXH32_hash(const AvxArray& input, uint32_t seed);
    
} // namespace XXHash32

class AVX256_Hash {
    public:
        size_t operator()(const AvxArray& x) const;
};

class AVX256_Equal {
    public:
        bool operator()(const AvxArray& a1, const AvxArray& a2) const;
};
