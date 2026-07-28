#include "emu.h"
#include <stdio.h>

int main(){
    CPU cpu = {0};
    Bus bus = {0};

    cpu_init(&cpu);
    cpu_dump(cpu);

    return 0;
}
