#pragma once
#include "../utility/utility.h"
#include <string>
#include <iostream>

namespace LifeUtilTests {
    using namespace std;

    string printPassed(bool passed) {
        return passed ? "passed" : "failed";
    }

    bool shiftsLeft() {
        AvxBitArray src;
        AvxBitArray dst;

        for (size_t i = 0; i < AVX_SIZE; i++) {
            src.set(i, i % 3 == 0);
        }

        Utility::shiftLeft(src, dst, false);
        for (size_t i = 0; i < AVX_SIZE - 1; i++) {
            if (src.get(i + 1) != dst.get(i)) return false;
        }
        if (dst.get(255)) return false;

        Utility::shiftLeft(src, dst, true);
        if (!dst.get(255)) return false;

        return true;
    }

    bool shiftsRight() {
        AvxBitArray src;
        AvxBitArray dst;

        for (size_t i = 0; i < AVX_SIZE; i++) {
            src.set(i, i % 3 == 0);
        }

        Utility::shiftRight(src, dst, false);
        for (size_t i = 0; i < AVX_SIZE - 1; i++) {
            if (src.get(i) != dst.get(i + 1)) return false;
        }
        if (dst.get(0)) return false;

        Utility::shiftRight(src, dst, true);
        if (!dst.get(0)) return false;

        return true;
    }

    void runTests() {
        cout << "Testing Life Utility... " << endl;
        cout << "\tTest: shifts left... " << printPassed(shiftsLeft()) << endl;
        cout << "\tTest: shifts right... " << printPassed(shiftsRight()) << endl;
    }
}