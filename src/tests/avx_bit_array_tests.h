#pragma once
#include "../utility/avx_bit_array.h"
#include "../constants.h"
#include <string>
#include <iostream>

namespace AvxTests {
    using namespace std;

    string printPassed(bool passed) {
        return passed ? "passed" : "failed";
    }

    bool setsZeros() {
        AvxBitArray bitArray(false);
        for (size_t i = 0; i < AVX_SIZE; i++) {
            if (bitArray.get(i)) return false;
        }

        return true;
    }

    bool setsOnes() {
        AvxBitArray bitArray(true);
        for (size_t i = 0; i < AVX_SIZE; i++) {
            if (!bitArray.get(i)) return false;
        }

        return true;
    }

    bool setsBits() {
        AvxBitArray bitArray;

        for (size_t i = 0; i < AVX_SIZE; i++) {
            bitArray.set(i, i % 3 == 0);
        }

        for (size_t i = 0; i < AVX_SIZE; i++) {
            if (bitArray.get(i) != (i % 3 == 0)) return false;
        }

        return true;
    }

    bool getsBits() {
        AvxBitArray bitArray;

        for (size_t i = 0; i < AVX_SIZE; i++) {
            bitArray.set(i, (i % 3 == 0 || i % 7 == 0));
        }
        for (size_t i = 0; i < AVX_SIZE; i++) {
            if (bitArray.get(i) != (i % 3 == 0 || i % 7 == 0)) return false;
        }

        return true;
    }

    bool countsBits() {
        AvxBitArray bitArray;

        int count = 0;
        for (size_t i = 0; i < AVX_SIZE; i++) {
            bitArray.set(i, i % 3 == 0);
            if (i % 3 == 0) count++;
        }

        return count == bitArray.popcount();
    }

    bool checksZero() {
        AvxBitArray bitArray(false);
        return bitArray.none();
    }

    bool clearsBits() {
        AvxBitArray bitArray(true);
        bitArray.zero();
        return bitArray.none();
    }

    bool shiftsLeft() {
        AvxBitArray origBitArray(false);
        for (size_t i = 0; i < AVX_SIZE; i++) {
            origBitArray.set(i, i % 3 == 0);
        }

        for (size_t n = 0; n < AVX_SIZE; n++) {
            AvxBitArray bitArray(origBitArray);

            bitArray <<= n;
            for (size_t i = 0; i < AVX_SIZE; i++) {
                bool expected = (i >= n) && ((i + 3*100 - n) % 3 == 0);
                if (bitArray.get(i) != expected) return false;
                if ((origBitArray << n).get(i) != expected) return false;
        
            }
        }

        return true;
    }

    bool shiftsRight() {
        AvxBitArray origBitArray(false);
        for (size_t i = 0; i < AVX_SIZE; i++) {
            origBitArray.set(i, i % 3 == 0);
        }

        for (size_t n = 0; n < AVX_SIZE; n++) {
            AvxBitArray bitArray(origBitArray);

            bitArray >>= n;
            for (size_t i = 0; i < AVX_SIZE; i++) {
                bool expected = (i < AVX_SIZE - n) && ((i + n) % 3 == 0);
                if (bitArray.get(i) != expected) return false;
                if ((origBitArray >> n).get(i) != expected) return false;
            }
        }

        return true;
    }

    bool invertsBits() {
        AvxBitArray bitArray(false);
        for (size_t i = 0; i < AVX_SIZE; i++) {
            bitArray.set(i, i % 3 == 0);
        }

        AvxBitArray inverted = ~bitArray;
        bitArray.invert();
        for (size_t i = 0; i < AVX_SIZE; i++) {
            if (bitArray.get(i) == (i % 3 == 0)) return false;
            if (inverted.get(i) == (i % 3 == 0)) return false;
        }

        return true;
    }

    bool orsBits() {
        AvxBitArray a(false);
        for (size_t i = 0; i < AVX_SIZE; i++) {
            a.set(i, i % 3 == 0);
        }

        AvxBitArray b(false);
        for (size_t i = 0; i < AVX_SIZE; i++) {
            b.set(i, i % 7 == 0);
        }

        AvxBitArray c = a | b;
        a |= b;

        for (size_t i = 0; i < AVX_SIZE; i++) {
            if (a.get(i) != (i % 3 == 0 || i % 7 == 0)) return false;
            if (c.get(i) != (i % 3 == 0 || i % 7 == 0)) return false;
        }

        return true;
    }

    bool andsBits() {
        AvxBitArray a(false);
        for (size_t i = 0; i < AVX_SIZE; i++) {
            a.set(i, i % 3 == 0);
        }

        AvxBitArray b(false);
        for (size_t i = 0; i < AVX_SIZE; i++) {
            b.set(i, i % 7 == 0);
        }

        AvxBitArray c = a & b;
        AvxBitArray out = a.and_not(b);
        a &= b;

        for (size_t i = 0; i < AVX_SIZE; i++) {
            if (a.get(i) != (i % 3 == 0 && i % 7 == 0)) return false;
            if (c.get(i) != (i % 3 == 0 && i % 7 == 0)) return false;
            if (out.get(i) != (i % 3 == 0 && i % 7 != 0)) return false;
        }

        return true;
    }

    bool xorsBits() {
        AvxBitArray a(false);
        for (size_t i = 0; i < AVX_SIZE; i++) {
            a.set(i, i % 3 == 0);
        }

        AvxBitArray b(false);
        for (size_t i = 0; i < AVX_SIZE; i++) {
            b.set(i, i % 7 == 0);
        }

        AvxBitArray c = a ^ b;
        a ^= b;

        for (size_t i = 0; i < AVX_SIZE; i++) {
            if (a.get(i) != ((i % 3 == 0) ^ (i % 7 == 0))) return false;
            if (c.get(i) != ((i % 3 == 0) ^ (i % 7 == 0))) return false;
        }

        return true;
    }

    bool checksEquality() {
        AvxBitArray a(false);
        for (size_t i = 0; i < AVX_SIZE; i++) {
            a.set(i, i % 3 == 0);
        }

        AvxBitArray b(false);
        for (size_t i = 0; i < AVX_SIZE; i++) {
            b.set(i, i % 3 == 0);
        }

        if (a != b) {
            return false;
        }

        a.set(0, false);
        return a != b;
    }

    bool checksLessThan() {
        AvxBitArray a(false);
        AvxBitArray b(false);

        if (a < b || b < a) return false;

        a.set(10, true);
        b.set(23, true);
        if (a < b || !(b < a)) return false;

        a.zero();
        b.zero();
        a.set(200, true);
        b.set(40, true);
        if (b < a || !(a < b)) return false;

        a.zero();
        b.zero();
        a.set(40, true);
        b.set(40, true);
        a.set(150, true);
        b.set(250, true);
        if (a < b || !(b < a)) return false;

        return true;
    }

    void runTests() {
        cout << "Testing AvxBitArray... " << endl;
        cout << "\tTest: sets zeros... " << printPassed(setsZeros()) << endl;
        cout << "\tTest: sets ones... " << printPassed(setsOnes()) << endl;
        cout << "\tTest: sets bits... " << printPassed(setsBits()) << endl;
        cout << "\tTest: gets bits... " << printPassed(getsBits()) << endl;
        cout << "\tTest: counts bits... " << printPassed(countsBits()) << endl;
        cout << "\tTest: clears bits... " << printPassed(clearsBits()) << endl;
        cout << "\tTest: shifts left... " << printPassed(shiftsLeft()) << endl;
        cout << "\tTest: shifts right... " << printPassed(shiftsRight()) << endl;
        cout << "\tTest: invert bits... " << printPassed(invertsBits()) << endl;
        cout << "\tTest: ors bits... " << printPassed(orsBits()) << endl;
        cout << "\tTest: ands bits... " << printPassed(andsBits()) << endl;
        cout << "\tTest: xors bits... " << printPassed(xorsBits()) << endl;
        cout << "\tTest: checks equality... " << printPassed(checksEquality()) << endl;
        cout << "\tTest: checks less than... " << printPassed(checksLessThan()) << endl;
    }

};