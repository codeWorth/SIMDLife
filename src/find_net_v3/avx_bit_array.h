#pragma once
#include <string>
#include <bitset>
#include <immintrin.h>
#include <algorithm>
#include "constants.h"

#define DEBUG

#define AVX_LOAD(src) _mm256_load_si256(reinterpret_cast<const __m256i*>(src))
#define AVX_STORE(dest, src) _mm256_store_si256(reinterpret_cast<__m256i*>(dest), src)
#define ROT32(x, r) _mm256_or_si256(_mm256_slli_epi32(x, r), _mm256_srli_epi32(x, 32 - r))

// index is the amount of chunks to shift by
const BYTE RIGHT_SHIFTS[4] = {
    0b11100100,     // src[3] => dst[3], 2 => 2, 1 => 1, 0 => 0
    0b10010011,     // src[2] => dst[3], 1 => 2, 0 => 1, 0 => 0
    0b01000000,     // src[1] => dst[3], 0 => 2, 0 => 1, 0 => 0
    0b00000000      // src[0] => dst[3], 0 => 2, 0 => 1, 0 => 0
};

// index is the amount of chunks to shift by
const BYTE LEFT_SHIFTS[4] = {
    0b11100100,     // 0: src[3] => dst[3], 2 => 2, 1 => 1, 0 => 0
    0b11111001,     // 1: src[3] => dst[3], 3 => 2, 2 => 1, 1 => 0
    0b11111110,     // 2: src[3] => dst[3], 3 => 2, 3 => 1, 2 => 0
    0b11111111      // 2: src[3] => dst[3], 3 => 2, 3 => 1, 3 => 0
};

// index is the number of bytes to mask away
const BYTE BOTTOM_MASKS[4] = {
    0b11111111,
    0b00111111,
    0b00001111,
    0b00000011
};

// index is the number of bytes to mask away
const BYTE TOP_MASKS[4] = {
    0b11111111,
    0b11111100,
    0b11110000,
    0b11000000
};


class AvxBitArray {
private:
    __m256i data;

public:
    AvxBitArray(): AvxBitArray(true) {}
    AvxBitArray(bool high) {
        this->data = _mm256_set1_epi8(high ? 0xFF : 0x00);
    }

    AvxBitArray(const AvxBitArray &other) {
        this->data = other.data;
    }

    bool get(int index) const {
        int byteIndex, subByteIndex;
        byteIndex = index / 8;
        subByteIndex = index & 0b111;

        AvxArray values;
        _mm256_store_si256(&values.avx, data);

        return (values.bytes[byteIndex] >> subByteIndex) & 1;
    }

    void zero() {
        data = _mm256_set1_epi8(0x00);
    }

    void set(int index, bool bit) {
        int byteIndex, subByteIndex;
        byteIndex = index / 8;
        subByteIndex = index & 0b111;

        AvxArray values;
        _mm256_store_si256(&values.avx, data);

        if (bit) {
            values.bytes[byteIndex] = (1 << subByteIndex) | values.bytes[byteIndex];
        } else {
            values.bytes[byteIndex] = ~(1 << subByteIndex) & values.bytes[byteIndex];
        }
        
        data = _mm256_load_si256(&values.avx);
    }

    void setAll(const AvxBitArray& other) {
        data = other.data;
    }

    long long popcount() const {
        return _mm_popcnt_u64(_mm256_extract_epi64(data, 0)) 
            + _mm_popcnt_u64(_mm256_extract_epi64(data, 1)) 
            + _mm_popcnt_u64(_mm256_extract_epi64(data, 2)) 
            + _mm_popcnt_u64(_mm256_extract_epi64(data, 3));
    }

    bool none() const {
        auto cmp_result =_mm256_cmpeq_epi8(data, _mm256_set1_epi8(0x00));
        return _mm256_movemask_epi8(cmp_result) == 0xFFFFFFFF;
    }

    AvxBitArray* invert() {
        data = _mm256_xor_si256(data, _mm256_set1_epi8(0xFF));
        return this;
    }

    AvxBitArray operator~() const {
        AvxBitArray out(*this);
        out.invert();
        return out;
    }

    AvxBitArray& operator|=(const AvxBitArray& other) {
        data = _mm256_or_si256(data, other.data);
        return *this;
    }

    AvxBitArray operator|(const AvxBitArray& other) const {
        AvxBitArray out(*this);
        out |= other;
        return out;
    }

    AvxBitArray& operator&=(const AvxBitArray& other) {
        data = _mm256_and_si256(data, other.data);
        return *this;
    }

    AvxBitArray operator&(const AvxBitArray& other) const {
        AvxBitArray out(*this);
        out &= other;
        return out;
    }

    void and_out(const AvxBitArray& other, AvxBitArray& out) const {
        out.data = _mm256_and_si256(data, other.data);
    }

    AvxBitArray& operator^=(const AvxBitArray& other) {
        data = _mm256_xor_si256(data, other.data);
        return *this;
    }

    AvxBitArray operator^(const AvxBitArray& other) const {
        AvxBitArray out(*this);
        out ^= other;
        return out;
    }

    AvxBitArray& operator<<=(BYTE amount) {
        if (amount >= AVX_SIZE) {
            zero();
            return *this;
        }

        BYTE chunk_shift;
        chunk_shift = amount / 64; // number of full data we need to shift over
        amount -= chunk_shift * 64; // number of extra bits needed after the chunk shift happens

        auto rollover = _mm256_permute4x64_epi64(data, LEFT_SHIFTS[1]); // align the "next" data with the "current" (does not shfit in zeros)
        rollover = _mm256_slli_epi64(rollover, 64 - amount); // grab only the bits which will be missing after shift
        rollover = _mm256_blend_epi32(_mm256_set1_epi8(0x00), rollover, BOTTOM_MASKS[1]); // remove the garbage at the top

        data = _mm256_srli_epi64(data, amount); // do the main bitshift on the "current" data, it's a right shift because LSB = index 0, but LSB is on the right
        data = _mm256_or_si256(data, rollover); // copy in the bits which got missed at the top of each chunk

        data = _mm256_permute4x64_epi64(data, LEFT_SHIFTS[chunk_shift]); // shift left by the given number of data (does not shift in zeros)
        data = _mm256_blend_epi32(_mm256_set1_epi8(0x00), data, BOTTOM_MASKS[chunk_shift]); // cut off the garabge

        return *this;
    }

    AvxBitArray operator<<(BYTE amount) const {
        AvxBitArray out(*this);
        out <<= amount;
        return out;
    }

    AvxBitArray& operator>>=(BYTE amount) {
        if (amount >= AVX_SIZE) {
            zero();
            return *this;
        }

        BYTE chunk_shift;
        chunk_shift = amount / 64;
        amount -= chunk_shift * 64;

        auto rollover = _mm256_permute4x64_epi64(data, RIGHT_SHIFTS[1]);
        rollover = _mm256_srli_epi64(rollover, 64 - amount);
        rollover = _mm256_blend_epi32(_mm256_set1_epi8(0x00), rollover, TOP_MASKS[1]);

        data = _mm256_slli_epi64(data, amount);
        data = _mm256_or_si256(data, rollover);

        data = _mm256_permute4x64_epi64(data, RIGHT_SHIFTS[chunk_shift]);
        data = _mm256_blend_epi32(_mm256_set1_epi8(0x00), data, TOP_MASKS[chunk_shift]);

        return *this;
    }

    AvxBitArray operator>>(BYTE amount) const {
        AvxBitArray out(*this);
        out >>= amount;
        return out;
    }

    bool operator==(const AvxBitArray& other) const {
        auto cmp_result =_mm256_cmpeq_epi8(data, other.data);
        return _mm256_movemask_epi8(cmp_result) == 0xFFFFFFFF;
    }

    // compares each bit array in big endian
    bool operator<(const AvxBitArray& other) const {
        // A contains 1s where this <= other
        // B contains 1s where other <= this
        unsigned int A = ~_mm256_movemask_epi8(_mm256_cmpgt_epi64(this->data, other.data));
        unsigned int B = ~_mm256_movemask_epi8(_mm256_cmpgt_epi64(other.data, this->data));

        // if they are equal, A and B will both be 1s, returning false
        // if this < other, then A will contain a 1 where B has a 0, returning true
        // if this < other, then A will contain a 0 where B has a 1, returning false
        return A > B;
    }

    std::string toString() const {
        return toString(AVX_SIZE);
    }

    std::string toString(int size) const {
        const std::string sep = " ";
        
        AvxArray values;
        _mm256_store_si256(&values.avx, data);

        std::string out = "";
        for (size_t i = 0; i < size / 8; i++) {
            if (i != 0) out += sep;
            std::string bits = std::bitset<8>(values.bytes[i]).to_string();
            std::reverse(bits.begin(), bits.end());
            out += bits;
        }
        return out;
    }
};