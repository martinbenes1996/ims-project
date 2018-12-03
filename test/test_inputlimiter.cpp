#include <cassert>
#include "simlib.h"
#include "../src/tools.h"


int main() {
    InputLimiter il(100);
    
    assert(il.output(10) == 10);
    assert(il.output(0) == 0);
    assert(il.output(-50) == -50);
    assert(il.output(99) == 99);
    assert(il.output(100) == 100);
    assert(il.output(101) == 100);
    assert(il.output(1000) == 100);

    assert(il.rest(10) == 0);
    assert(il.rest(0) == 0);
    assert(il.rest(-50) == 0);
    assert(il.rest(99) == 0);
    assert(il.rest(100) == 0);
    assert(il.rest(101) == 1);
    assert(il.rest(1000) == 900);

}