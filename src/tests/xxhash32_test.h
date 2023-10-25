#pragma once
#include "../utility/XXHash32.h"
#include "../constants.h"
#include <string>
#include <iostream>
#include <cstdlib>
#include <cstring>

namespace XXHash32Test {
    using namespace std;

    string printPassed(bool passed) {
        return passed ? "passed" : "failed";
    }

    bool sameHash() {
        AVX256_Hash hasher;
        AvxArray arrA;
        memset(arrA.bytes, 3, 32);
        uint32_t a = hasher(arrA);
        AvxArray arrB;
        memset(arrB.bytes, 3, 32);
        uint32_t b = hasher(arrB);
        return a == b;
    }

    bool diffHashes() {
        AVX256_Hash hasher;
        AvxArray arr;
        memset(arr.bytes, 3, 32);
        uint32_t a = hasher(arr);
        arr.bytes[3] = 0xFF;
        uint32_t b = hasher(arr);
        return a != b;
    }

    bool equals() {
        AVX256_Equal cmp;
        AvxArray arrA;
        memset(arrA.bytes, 3, 32);
        AvxArray arrB;
        memset(arrB.bytes, 3, 32);
        return cmp(arrA, arrB);
    }

    bool notEquals() {
        AVX256_Equal cmp;
        AvxArray arrA;
        memset(arrA.bytes, 3, 32);
        AvxArray arrB;
        memset(arrB.bytes, 3, 32);
        arrB.bytes[3] = 0xFF;
        return !cmp(arrA, arrB);
    }

    void runTests() {
        cout << "Testing XXHash32... " << endl;
        cout << "\tTest: same hash... " << printPassed(sameHash()) << endl;
        cout << "\tTest: different hash... " << printPassed(diffHashes()) << endl;
        cout << "\tTest: checks equal... " << printPassed(equals()) << endl;
        cout << "\tTest: checks not equal... " << printPassed(notEquals()) << endl;
    }
};