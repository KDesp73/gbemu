#include "emu.h"
#include <stdio.h>

int main(){
    CPU cpu = {0};
    cpu_init(&cpu);
    cpu_dump(cpu);

    Bus bus = {0};

    Timer timer = {0};
    timer_init(&timer);


    return 0;
}
