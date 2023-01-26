#include "tests/avx_bit_array_tests.h"
#include "tests/net_space_tests.h"

int main(int argc, char *argv[]) {
    AvxTests::runTests();
    NetSpaceTest::runTests();
}