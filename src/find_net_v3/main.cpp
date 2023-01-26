#include "net_space.h"
#include "outputs_cache.h"
#include <deque>
#include <iostream>
#include <chrono>

// for GIVEN_DEPTH = 7...
// enabling ORDER_PARALLEL caused total time ~43 seconds => ~31 seconds
// adding NetOutputs::Cache caused total time ~31 seconds => ~5 seconds

#define ORDER_PARALLEL

using namespace std;
const int GIVEN_DEPTH = 8;
const Swap GIVEN_SWAPS[GIVEN_DEPTH] = {
    {0, 4},
    {1, 5},
    {2, 6},
    {3, 7},
    {0, 2},
    {1, 3},
    {4, 6},
    {5, 7},
    // {2, 4},
    // {3, 5},
    // {0, 1},
    // {2, 3}
};
const int MAX_DEPTH = 14 - GIVEN_DEPTH;

unsigned long long currentTime() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

bool validOutputs(const AvxBitArray& outputs) {
    AvxBitArray unallowed(UNALLOWED_OUTPUTS);
    return (outputs & unallowed).none();
}

bool validSwap(int i, int j) {
    return i < 7 && j < 8 && i < j;
}

// forward declare since they reference each other
bool trySwap(
    int i, int j, 
    AvxBitArray& outputs, 
    short depth, short maxDepth, 
    deque<Swap>& net, const AvxArray& savedOutputs, NetOutputs::Cache& outputsCache
);

bool findNet(
    AvxBitArray& outputs,
    short depth,
    short maxDepth,
    deque<Swap>& net,
    NetOutputs::Cache& outputsCache
) {
    if (validOutputs(outputs)) {
        return true;
    }

    AvxArray savedOutputs = outputs.toArray();
    short minDepthFromHere = outputsCache.minDepth(savedOutputs);
    if (depth + minDepthFromHere >= maxDepth) {
        return false;
    }
    
    for (int i = 0; i < 7; i++) {
        #ifdef ORDER_PARALLEL 
            if (i == net.front().i || i == net.front().j) continue; // skip options that overlap i for now
        #endif

        for (int j = i + 1; j < 8; j++) {
            #ifdef ORDER_PARALLEL 
                if (j == net.front().i || j == net.front().j) continue; // skip options that overlap j for now
            #endif

            if (trySwap(i, j, outputs, depth, maxDepth, net, savedOutputs, outputsCache)) return true;
        }
    }

    #ifdef ORDER_PARALLEL
        // try options where i overlaps
        int i = net.front().i;
        for (int j = i + 1; j < 8; j++) {
            if (j == net.front().i || j == net.front().j) continue; // skip options that overlap j for now

            if (trySwap(i, j, outputs, depth, maxDepth, net, savedOutputs, outputsCache)) return true;
        }
        i = net.front().j;
        if (i < 7) {
            for (int j = i + 1; j < 8; j++) {
                if (j == net.front().i || j == net.front().j) continue; // skip options that overlap j for now

                if (trySwap(i, j, outputs, depth, maxDepth, net, savedOutputs, outputsCache)) return true;
            }
        }

        // try options where j overlaps
        for (int i = 0; i < 7; i++) {
            if (i == net.front().i || i == net.front().j) continue; // skip options that overlap i for now

            int j = net.front().i;
            if (validSwap(i, j) && trySwap(i, j, outputs, depth, maxDepth, net, savedOutputs, outputsCache)) return true;

            j = net.front().j;
            if (validSwap(i, j) && trySwap(i, j, outputs, depth, maxDepth, net, savedOutputs, outputsCache)) return true;
        }


        // options where both overlap don't need to be tried
        // i2 == i1 and j2 == j1 generates the same result
        // i2 == j1 and j2 == i1 is always an invalid swap (i2 > j2)
    #endif

    // if we will return false, this
    outputsCache.minDepthFound(savedOutputs, maxDepth - depth);
    return false;
}

// tries a given swap, returns true if it found the solution
bool trySwap(
    int i, int j,
    AvxBitArray& outputs,
    short depth,
    short maxDepth,
    deque<Swap>& net,
    const AvxArray& savedOutputs,
    NetOutputs::Cache& outputsCache
) {
    appendCompareSwap(outputs, i, j);   // perform the swap
    net.emplace_front(i, j);            // add it to the current net

    // check if this net will succeed
    if (findNet(outputs, depth + 1, maxDepth, net, outputsCache)) {
        return true;
    } else {    // if this net fails...
        outputs.setAll(savedOutputs);   // reset the outputs to before the current swap
        net.pop_front();    // remove that swap from the net
        return false;
    }
}

int main(int argc, char const *argv[]){

    NetOutputs::Cache outputsCache;
    deque<Swap> net;
    AvxBitArray outputs(true);  // When no swaps have happened, any output is possible
    for (int i = 0; i < GIVEN_DEPTH; i++) {
        appendCompareSwap(outputs, GIVEN_SWAPS[i]);
        net.push_front(GIVEN_SWAPS[i]);
    }

    AvxArray savedStartingPoint = outputs.toArray();
    unsigned long long start = currentTime();
    bool foundSolution = false;

    for (int depth = 1; depth <= MAX_DEPTH; depth++) {
        cout << "Finding net with max depth = " << depth << "... " << endl;
        foundSolution = findNet(outputs, 0, depth, net, outputsCache);
        cout << "\t" << "Cache has " << outputsCache.size() << " entries" << endl;
        
        if (foundSolution) break;
        
        outputs.setAll(savedStartingPoint);
    }

    if (foundSolution) {
        while (!net.empty()) {
            cout << "\t" << net.back().toString() << endl;
            net.pop_back();
        }
        cout << endl;
    } else {
        cout << "Failed to find solution!" << endl;
    }

    unsigned long long end = currentTime();
    cout << "Took " << (end - start) << " milliseconds" << endl;

    return 0;
}
