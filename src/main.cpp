
#include "simlib.h"

#include "simulator.h"

int main() {
    std::cout << style("Model Ropovod - SIMLIB/C++\n", BOLD);
    Init(1,365);
    new Simulator();
    Run();
    return 0;
}
