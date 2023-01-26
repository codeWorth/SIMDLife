#pragma once
#include "avx_bit_array.h"
#include <map>

namespace NetOutputs {
    using namespace std;

    struct CompareAvxArray {
        bool operator()(const AvxArray& a, const AvxArray& b) const {
            AvxBitArray a_(a);
            AvxBitArray b_(b);
            return a_ < b_;
        }
    };

    class Cache {
    private:
        const short MIN_ACCEPTED_DEPTH = 2; // only accept entries to cache with at least this much depth
        map<AvxArray, short, CompareAvxArray> cache;

    public:
        // returns the minimum depth to find a solution from the given output (inclusive)
        short minDepth(const AvxArray& outputs) const {
            auto elem = cache.find(outputs);
            if (elem == cache.end()) {
                return 0;
            } else {
                return elem->second;
            }
        }

        // should be called when the minumum depth to reach a solution from the given output is found
        // the real depth may be greater or equal to minDepth
        void minDepthFound(const AvxArray& outputs, short minDepth) {
            if (minDepth < MIN_ACCEPTED_DEPTH) return;

            auto elem = cache.find(outputs);
            if (elem == cache.end()) {
                cache.insert({outputs, minDepth});
            } else if (minDepth > elem->second) {   // only update minDepth if a larger value is supplied
                elem->second = minDepth;
            }
        }

        size_t size() const {
            return cache.size();
        }
    };
}
