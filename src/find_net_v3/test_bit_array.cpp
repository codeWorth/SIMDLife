#include "avx_bit_array.h"
#include "constants.h"
#include <iostream>

using namespace std;

#define SIZE 256

void printPassed(bool passed) {
    if (passed) {
        cout << "passed" << endl;
    } else {
        cout << "failed" << endl;
    }
}

void setsZeros() {
    cout << "Test: set zeros... ";

    AvxBitArray bitArray(false);
    for (size_t i = 0; i < SIZE; i++) {
        if (bitArray.get(i)) {
            printPassed(false);
            return;
        }
    }

    printPassed(true);
}

void setsOnes() {
    cout << "Test: set ones... ";

    AvxBitArray bitArray(true);
    for (size_t i = 0; i < SIZE; i++) {
        if (!bitArray.get(i)) {
            printPassed(false);
            return;
        }
    }

    printPassed(true);
}

void setsBits() {
    cout << "Test: set bits... ";
    AvxBitArray bitArray;

    for (size_t i = 0; i < SIZE; i++) {
        bitArray.set(i, i % 3 == 0);
    }

    for (size_t i = 0; i < SIZE; i++) {
        if (bitArray.get(i) != (i % 3 == 0)) {
            printPassed(false);
            return;
        }
    }

    printPassed(true);
}

void countsBits() {
    cout << "Test: count bits... ";
    AvxBitArray bitArray;

    int count = 0;
    for (size_t i = 0; i < SIZE; i++) {
        bitArray.set(i, i % 3 == 0);
        if (i % 3 == 0) count++;
    }

    printPassed(count == bitArray.popcount());
}

void checksZero() {
    cout << "Test: checks zero... ";
    AvxBitArray bitArray(false);
    printPassed(bitArray.none());
}

void clearsBits() {
    cout << "Test: clears bits... ";
    AvxBitArray bitArray(true);
    bitArray.zero();
    printPassed(bitArray.none());
}

int main(int argc, char *argv[]) {

    setsZeros();
    setsOnes();
    setsBits();
    countsBits();
    clearsBits();

    
    // AvxBitArray shiftTests(false);
    // for (size_t i = 0; i < size; i++) {
    //     shiftTests.set(i, i % 3 == 0);
    // }

    // cout << "shifting left (creation): " << endl;
    // for (size_t i = size - 32; i < size; i++) {
    //     cout << "\t" << i << "\t" << (shiftTests << i).toString(32) << endl;
    // }
    // cout << endl;

    // cout << "shifting right (assignment): " << endl;
    // for (size_t i = 0; i < 32; i++) {
    //     AvxBitArray test;
    //     test >>= i;
    //     cout << "\t" << i << "\t" << test.toString(32) << endl;
    // }
    // cout << endl;

    // AvxBitArray cmpTest(false);
    // cout << cmpTest.toString() << (cmpTest == shiftTests ? " == " : " != ") << shiftTests.toString() << endl;

    // for (size_t i = 0; i < size; i++) {
    //     cmpTest.set(i, shiftTests.get(i));
    // }
    // cout << cmpTest.toString() << (cmpTest == shiftTests ? " == " : " != ") << shiftTests.toString() << endl;

    // AvxBitArray a(false);
    // AvxBitArray b(true);
    // cout << "all 1: " << (a | b).toString() << endl;
    // a.set(6, true);
    // a &= b;
    // cout << "all 0 (except #6): " << a.toString() << endl;
}