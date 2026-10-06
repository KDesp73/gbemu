#include "gbemu.h"
#include <string.h>

void gb_debugger_init(gb_debugger* dbg, gb_cpu* cpu, gb_bus* bus)
{
    memset(dbg, 0, sizeof(*dbg));
    dbg->cpu = cpu;
    dbg->bus = bus;
}

bool gb_debugger_has_breakpoint(const gb_debugger* dbg, uint16_t addr)
{
    for (int i = 0; i < dbg->num_breakpoints; i++) {
        if (dbg->breakpoints[i] == addr) return true;
    }
    return false;
}

bool gb_debugger_add_breakpoint(gb_debugger* dbg, uint16_t addr)
{
    if (gb_debugger_has_breakpoint(dbg, addr)) return false;
    if (dbg->num_breakpoints >= GB_DEBUGGER_MAX_BREAKPOINTS) return false;
    dbg->breakpoints[dbg->num_breakpoints++] = addr;
    return true;
}

bool gb_debugger_remove_breakpoint(gb_debugger* dbg, uint16_t addr)
{
    for (int i = 0; i < dbg->num_breakpoints; i++) {
        if (dbg->breakpoints[i] == addr) {
            dbg->breakpoints[i] = dbg->breakpoints[--dbg->num_breakpoints];
            return true;
        }
    }
    return false;
}

bool gb_debugger_toggle_breakpoint(gb_debugger* dbg, uint16_t addr)
{
    if (gb_debugger_has_breakpoint(dbg, addr)) {
        gb_debugger_remove_breakpoint(dbg, addr);
        return false;
    }
    gb_debugger_add_breakpoint(dbg, addr);
    return true;
}

void gb_debugger_pause(gb_debugger* dbg)
{
    dbg->paused = true;
    dbg->steps_remaining = 0;
}

void gb_debugger_continue(gb_debugger* dbg)
{
    dbg->paused = false;
    dbg->steps_remaining = 0;
    // The PC we are stopped on may itself sit behind a breakpoint (that is
    // usually why we are paused); exempt the next check so execution can move.
    dbg->skip_check_once = true;
}

void gb_debugger_toggle_pause(gb_debugger* dbg)
{
    if (dbg->paused) gb_debugger_continue(dbg);
    else gb_debugger_pause(dbg);
}

void gb_debugger_step(gb_debugger* dbg, int n)
{
    if (n < 1) n = 1;
    dbg->steps_remaining = n;
    dbg->paused = false;
    dbg->skip_check_once = true;
}

void gb_debugger_step_over(gb_debugger* dbg)
{
    uint16_t pc = dbg->cpu->pc;
    uint8_t op = gb_bus_read(dbg->bus, pc);

    bool is_call = false;
    switch (op) {
    case 0xC4: case 0xCC: case 0xD4: case 0xDC: // CALL NZ/Z/NC/C
    case 0xCD:                                   // CALL a16
    case 0xC7: case 0xCF: case 0xD7: case 0xDF: // RST $00/$08/$10/$18
    case 0xE7: case 0xEF: case 0xF7: case 0xFF: // RST $20/$28/$30/$38
        is_call = true;
        break;
    default:
        break;
    }

    if (is_call) {
        dbg->temp_bp = (uint16_t)(pc + gb_instruction_length(dbg->bus, pc));
        dbg->has_temp_bp = true;
        gb_debugger_continue(dbg);
    } else {
        gb_debugger_step(dbg, 1);
    }
}

bool gb_debugger_hit(gb_debugger* dbg, uint16_t pc)
{
    bool hit = false;

    if (dbg->has_temp_bp && dbg->temp_bp == pc) {
        dbg->has_temp_bp = false;
        hit = true;
    } else if (gb_debugger_has_breakpoint(dbg, pc)) {
        hit = true;
    }

    if (hit) {
        dbg->paused = true;
        dbg->steps_remaining = 0;
    }
    return hit;
}
