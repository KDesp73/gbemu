#include "emu.h"
#include <stdio.h>

int main(){
    CPU cpu = {0};
    Bus bus = {0};
    PPU ppu = {0};
    Timer timer = {0};

    cpu_init(&cpu);
    ppu_init(&ppu);
    timer_init(&timer);

    return 0;
}
