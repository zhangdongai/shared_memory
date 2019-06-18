#include "memory_segment.h"
#include "test.h"

int main(int argc, char** argv) {
    C c;
    c.assign_data(1314, 756, 762394, 10923.434, {45, 323, 43, 544}, "who is your daddy");

    MemorySegment<C> sm;
    sm.init();
    sm.write_mem(c);

    C c1;
    c1.key(c.key());
    sm.read_mem(&c1);
    c1.print();

    getchar();
    return 0;
}