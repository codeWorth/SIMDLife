#pragma once
#include <cstdint>
#include <immintrin.h>
#include <cstring>
#include <iostream>

// #define CHECK_OVERWRITE

template<class Key, class Val, int binSize, class HashFn, class EqualFn>
class UsageHashmap {
public:
    struct iterator {
        uint32_t index;

        iterator(uint32_t index): index(index) {};
        bool operator==(const iterator& rhs) const {
            return index == rhs.index;
        }
        bool operator!=(const iterator& rhs) const {
            return index != rhs.index;
        }
    };

private:
    uint32_t* usages;
    Key* keys;
    Val* vals;
    HashFn hasher;
    EqualFn equaler;
    iterator _end;
    int bins;

    uint32_t getBinIndex(const Key& key) const {
        return (hasher(key) % bins) * binSize;
    }

public:
    UsageHashmap(int bins): bins(bins), _end(-1) {
        uint32_t length = bins*binSize;
        usages = new uint32_t[length];
        memset(usages, 0, length*4);

        int keysBytes = sizeof(Key) * length;
        if (keysBytes % 32 != 0) keysBytes += 32 - (keysBytes % 32);
        keys = (AvxArray*)_mm_malloc(keysBytes, 32);

        int valsBytes = sizeof(Val) * length;
        if (valsBytes % 32 != 0) valsBytes += 32 - (valsBytes % 32);
        vals = (AvxArray*)_mm_malloc(valsBytes, 32);
    }

    ~UsageHashmap() {
        delete[] usages;
        _mm_free(keys);
        _mm_free(vals);
    }

    iterator put(const Key& key, const Val& val) {
        uint32_t index = getBinIndex(key);
        #ifdef CHECK_OVERWRITE
            for (int i = index; i < index+binSize; i++) {
                if (equaler(key, keys[i])) {
                    keys[i] = key;
                    vals[i] = val;
                    return;
                }
            }
        #endif

        uint32_t minIndex = index;
        uint32_t minUsage = usages[index];
        for (int i = index+1; i < index+binSize; i++) {
            if (usages[i] < minUsage) minIndex = i;
            if (usages[i] < minUsage) minUsage = usages[i];
        }

        keys[minIndex] = key;
        vals[minIndex] = val;
        usages[minIndex] = 1;
        return iterator(minIndex);
    }

    void countOrPut(const Key& key, const Val& val) {
        uint32_t index = getBinIndex(key);
        for (int i = index; i < index+binSize; i++) {
            if (usages[i] > 0 && equaler(key, keys[i])) {
                usages[i]++;
                return;
            }
        }

        put(key, val);
    }

    void count(const iterator& iter) {
        usages[iter.index]++;
    }

    iterator find(const Key& key) const {
        uint32_t index = getBinIndex(key);
        for (int i = index; i < index+binSize; i++) {
            if (equaler(key, keys[i])) return iterator(i);
        }
        return _end;
    }

    Val* get(const Key& key) const {
        return get(find(key));
    }

    Val* get(const iterator& iter) const {
        return vals + iter.index;
    }

    const iterator& end() const {
        return _end;
    }

    void printUsages() const {
        for (int i = 0; i < bins; i++) {
            for (int j = 0; j < binSize; j++) {
                std::cout << usages[i*binSize+j] << ", ";
            }
            std::cout << std::endl;
        }
    }
};