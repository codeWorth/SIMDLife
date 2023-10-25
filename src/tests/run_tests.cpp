#include "avx_bit_array_tests.h"
#include "net_space_tests.h"
#include "life_utility_test.h"
#include "usage_hashmap_test.h"
#include "xxhash32_test.h"

int main(int argc, char *argv[]) {
    AvxTests::runTests();
    NetSpaceTest::runTests();
    LifeUtilTests::runTests();
    XXHash32Test::runTests();
    UsageHashmapTest::runTests();
}