#pragma once
#include "../utility/usage_hashmap.h"
#include "../utility/XXHash32.h"
#include "../constants.h"
#include <string>
#include <iostream>
#include <cstdlib>
#include <cstring>

namespace UsageHashmapTest {
    using namespace std;

    string printPassed(bool passed) {
        return passed ? "passed" : "failed";
    }

    bool findsNothing() {
        UsageHashmap<AvxArray, AvxArray, 2, AVX256_Hash, AVX256_Equal> map(10);
        AvxArray key;
        memset(key.bytes, 1, 32);
        return map.find(key) == map.end();
    }

    bool getsNothing() {
        UsageHashmap<AvxArray, AvxArray, 2, AVX256_Hash, AVX256_Equal> map(10);
        return map.get(UsageHashmap<AvxArray, AvxArray, 2, AVX256_Hash, AVX256_Equal>::iterator(0))->bytes[0] == 0;
    }
    
    bool puts() {
        UsageHashmap<AvxArray, AvxArray, 2, AVX256_Hash, AVX256_Equal> map(10);
        AvxArray key;
        AvxArray val;
        memset(key.bytes, 1, 32);
        memset(val.bytes, 2, 32);
        
        auto index = map.put(key, val);
        auto got = map.get(index);
        return got->bytes[0] == 2;
    }

    bool countOrPutInitial() {
        UsageHashmap<AvxArray, AvxArray, 2, AVX256_Hash, AVX256_Equal> map(10);
        AvxArray key;
        AvxArray val;
        memset(key.bytes, 1, 32);
        memset(val.bytes, 2, 32);
        
        map.countOrPut(key, val);
        return map.get(key)->bytes[0] == 2;
    }

    bool getsWithinBin() {
        UsageHashmap<AvxArray, AvxArray, 2, AVX256_Hash, AVX256_Equal> map(1);
        AvxArray key1;
        AvxArray val1;
        memset(key1.bytes, 1, 32);
        memset(val1.bytes, 1, 32);
        map.put(key1, val1);

        AvxArray key2;
        AvxArray val2;
        memset(key2.bytes, 2, 32);
        memset(val2.bytes, 2, 32);
        map.put(key2, val2);

        return map.get(key1)->bytes[0] == 1 && map.get(key2)->bytes[0] == 2;
    }

    bool evictsLeastUsage() {
        UsageHashmap<AvxArray, AvxArray, 2, AVX256_Hash, AVX256_Equal> map(1);
        AvxArray key1;
        AvxArray val1;
        memset(key1.bytes, 1, 32);
        memset(val1.bytes, 1, 32);
        map.countOrPut(key1, val1);

        AvxArray key2;
        AvxArray val2;
        memset(key2.bytes, 2, 32);
        memset(val2.bytes, 2, 32);
        map.countOrPut(key2, val2);
        map.countOrPut(key2, val2);

        AvxArray key3;
        AvxArray val3;
        memset(key3.bytes, 3, 32);
        memset(val3.bytes, 3, 32);
        map.countOrPut(key3, val3);
        
        bool evict1st = map.get(key3)->bytes[0] == 3 && map.get(key2)->bytes[0] == 2;
        // key2 count = 2, key3 count = 1

        map.countOrPut(key3, val3);
        map.countOrPut(key3, val3); // key2 count = 2, key3 count = 3
        map.countOrPut(key1, val1);
        bool evict2nd = map.get(key1)->bytes[0] == 1 && map.get(key3)->bytes[0] == 3;

        return evict1st && evict2nd;
    }

    void runTests() {
        cout << "Testing UsageHashmap... " << endl;
        cout << "\tTest: finds nothing... " << printPassed(findsNothing()) << endl;
        cout << "\tTest: gets nothing... " << printPassed(getsNothing()) << endl;
        cout << "\tTest: puts value... " << printPassed(puts()) << endl;
        cout << "\tTest: puts for count and put if empty... " << printPassed(countOrPutInitial()) << endl;
        cout << "\tTest: puts multiple in bin... " << printPassed(getsWithinBin()) << endl;
        cout << "\tTest: evicts least used... " << printPassed(evictsLeastUsage()) << endl;
    }
};