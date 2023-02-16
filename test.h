#ifndef TEST_H_INCLUDED
#define TEST_H_INCLUDED

#include "proof.h"

bool testlog2(); // should be 0

bool testfilter(unsigned n); // should be 1

const char * testsat1(); // should be "Okay"

unsigned testsat2(unsigned n); // should be 0

bool testDAG(unsigned n); // should be 1

// Check if iterators to assertions are sorted in ascending number.
bool checkassiters
    (Assertions const & assertions, Assiters const & assiters); // should be 1

bool testMCTS(std::size_t sizelimit, double const exploration[2]); // should be 1

#endif // TEST_H_INCLUDED
