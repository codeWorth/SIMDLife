#include "XXHash32.h"

uint32_t XXHash32::vertAdd(__m128i x) {
    __m128i shuf = PS_I(_mm_movehdup_ps(I_PS(x)));  // broadcast elements 3,1 to 2,0
    __m128i sums = _mm_add_epi32(x, shuf);
    shuf = PS_I(_mm_movehl_ps(I_PS(shuf), I_PS(sums)));  // high half -> low half
    sums = _mm_add_epi32(sums, shuf);
    return _mm_cvtss_si32(I_PS(sums));
}

__m128i XXHash32::XXH32_round(__m128i acc, __m128i input) {
    acc = _mm_add_epi32(acc, _mm_mul_epi32(input, _mm_set1_epi32(XXH_PRIME32_2)));
    acc = AVX_rotl32(acc, 13);
    acc = _mm_mul_epi32(acc, _mm_set1_epi32(XXH_PRIME32_1));
    return acc;
}

uint32_t XXHash32::XXH32_avalanche(uint32_t hash) {
    hash ^= hash >> 15;
    hash *= XXH_PRIME32_2;
    hash ^= hash >> 13;
    hash *= XXH_PRIME32_3;
    hash ^= hash >> 16;
    return hash;
}

uint32_t XXHash32::XXH32_finalize(uint32_t hash, __m128i input) {
    __m128i mult = _mm_mul_epi32(input, _mm_set1_epi32(XXH_PRIME32_3));
    uint32_t v0 = _mm_extract_epi32(mult, 0);
    uint32_t v1 = _mm_extract_epi32(mult, 1);
    uint32_t v2 = _mm_extract_epi32(mult, 2);
    uint32_t v3 = _mm_extract_epi32(mult, 3);

    hash += v0;
    hash = XXH_rotl32(hash, 17) * XXH_PRIME32_4;
    hash += v1;
    hash = XXH_rotl32(hash, 17) * XXH_PRIME32_4;
    hash += v2;
    hash = XXH_rotl32(hash, 17) * XXH_PRIME32_4;
    hash += v3;
    hash = XXH_rotl32(hash, 17) * XXH_PRIME32_4;

    return XXH32_avalanche(hash);
}

uint32_t XXHash32::XXH32_hash(const AvxArray& input, uint32_t seed) {
    __m128i lower = _mm_load_si128((__m128i*)input.bytes);
    __m128i upper = _mm_load_si128((__m128i*)(input.bytes + 16));

    // just xor everything
    // __m128i xor128 = _mm_xor_si128(lower, upper);
    // __m128i shuf = PS_I(_mm_movehdup_ps(I_PS(xor128)));  // broadcast elements 3,1 to 2,0
    // __m128i sums = _mm_xor_si128(xor128, shuf);
    // shuf = PS_I(_mm_movehl_ps(I_PS(shuf), I_PS(sums)));  // high half -> low half
    // sums = _mm_xor_si128(sums, shuf);
    // return _mm_cvtss_si32(I_PS(sums));

    __m128i primes = _mm_set_epi32(-XXH_PRIME32_1, 0, XXH_PRIME32_2, XXH_PRIME32_1 + XXH_PRIME32_2);
    upper = _mm_add_epi32(primes, _mm_set1_epi32(seed));
    upper = XXH32_round(upper, upper);
    upper = _mm_mul_epi32(upper, _mm_set_epi32(0x40000, 0x1000, 0x80, 0x2));
    uint32_t h32 = vertAdd(upper) + 32;

    return XXH32_finalize(h32, lower);
}

size_t AVX256_Hash::operator()(const AvxArray& x) const {
    return XXHash32::XXH32_hash(x, 0x00e6f2e5);
}

bool AVX256_Equal::operator()(const AvxArray& a1, const AvxArray& a2) const {
    AvxBitArray a1Avx(a1);
    AvxBitArray a2Avx(a2);
    return a1Avx == a2Avx;
}