
/**
 * @file main.cpp
 * @authors xbenes49 xpolan09
 * @date 27th november 2018
 * @brief Main module.
 *
 * This module contains main() function.
 */

#include "simlib.h"

#include "simulator.h"

int main() {
    std::cout << style("Model Ropovod - SIMLIB/C++\n", BOLD);
    Init(1,365);
    new Simulator();
    Run();
    return 0;
}
