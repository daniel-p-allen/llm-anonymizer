// Entry point for the test binary.
//
// doctest generates main() here and nowhere else — src/program.cpp is deliberately
// left out of the test build for exactly that reason. This file holds nothing but
// the macro, so the tests themselves stay in files named after what they cover.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
