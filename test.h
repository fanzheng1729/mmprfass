#ifndef TEST_H_INCLUDED
#define TEST_H_INCLUDED

#include "proof.h"

bool pretest(); // should be 1

// Check if iterators to assertions are sorted in ascending number.
bool checkassiters
    (Assertions const & assertions, Assiters const & assiters); // should be 1

#endif // TEST_H_INCLUDED
