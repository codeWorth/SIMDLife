#pragma once
#include "../net_space.h"
#include "../../constants.h"
#include <string>
#include <iostream>
#include <cstdlib>

namespace NetSpaceTest {
    using namespace std;

    string printPassed(bool passed) {
        return passed ? "passed" : "failed";
    }

    bool getsBit() {
        return getBit(0b010010000, 4) && !getBit(0b010010000, 5);
    }

    bool checksValidSort() {
        if (validSort(0b10100000)) return false;
        if (validSort(0b10000100)) return false;
        if (validSort(0b11011111)) return false;
        
        if (!validSort(0b00000000)) return false;
        if (!validSort(0b10000000)) return false;
        if (!validSort(0b11100000)) return false;
        if (!validSort(0b11100101)) return false;

        return true;
    }

    bool generatesUnallowedOutputs() {
        AvxBitArray unallowed(unallowedOutputs());
        for (int i = 0; i < 256; i++) {
            if (validSort(i) == unallowed.get(i)) return true;
        }

        return true;
    }

    bool generatesOnesMasks() {
        for (int k = 0; k < 8; k++) {
            AvxBitArray mask(onesMask(k));

            for (int i = 0; i < 256; i++) {
                if (mask.get(i) != getBit(i, k)) return false;
            }
        }

        return true;
    }

    bool appendsCmpSwap() {
        srand(1);

        for (int n = 0; n < 2000; n++) {
            int i = rand() % 7;
            int j = rand() % (7 - i) + i + 1;

            AvxBitArray outputs;
            for (int i = 0; i < 256; i++) {
                outputs.set(i, rand() % 2);
            }

            AvxBitArray checkOutputs = outputs;
            for (int output = 0; output < 256; output++) {
                bool goodState = !getBit(output, i) && getBit(output, j); // output[i] == 0 && output[j] == 1
                if (!goodState) continue;

                int badOutput = output & ~(1 << j);
                badOutput |= (1 << i);

                if (outputs.get(badOutput)) {
                    checkOutputs.set(output, true);
                }
            }

            for (int output = 0; output < 256; output++) {
                bool badState = getBit(output, i) && !getBit(output, j); // output[i] == 1 && output[j] == 0
                if (!badState) continue;

                checkOutputs.set(output, false);
            }

            appendCompareSwap(outputs, i, j);
            if (outputs != checkOutputs) return false;
        }

        return true;
    }

    void runTests() {
        cout << "Testing NetSpace... " << endl;
        cout << "\tTest: gets bit... " << printPassed(getsBit()) << endl;
        cout << "\tTest: checks valid sort... " << printPassed(checksValidSort()) << endl;
        cout << "\tTest: generates unallowed outputs... " << printPassed(generatesUnallowedOutputs()) << endl;
        cout << "\tTest: generates ones masks... " << printPassed(generatesOnesMasks()) << endl;
        cout << "\tTest: appends compare swap... " << printPassed(appendsCmpSwap()) << endl;
    }
};