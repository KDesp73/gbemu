#include "emu.h"
#include "opcodes.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define BOOL(x) ((x) ? "true" : "false")

void cpu_init(CPU* cpu)
{
    cpu->pc = 0x0100; // ROM execution begins at 0x0100
    cpu->af = 0x01B0;
    cpu->bc = 0x0013;
    cpu->de = 0x00D8;
    cpu->hl = 0x014D;
    cpu->sp = 0xFFFE;
}

void cpu_dump_fd(CPU cpu, FILE* fd)
{
    fprintf(fd, "AF: %d\ta: %d\tf: %d\n", cpu.af, cpu.a, cpu.f);
    fprintf(fd, "AF: %d\tb: %d\tc: %d\n", cpu.bc, cpu.b, cpu.c);
    fprintf(fd, "AF: %d\td: %d\te: %d\n", cpu.de, cpu.d, cpu.e);
    fprintf(fd, "HL: %d\th: %d\tl: %d\n", cpu.hl, cpu.h, cpu.l);

    fprintf(fd, "SP: %d\tPC: %d\n", cpu.sp, cpu.pc);
    fprintf(fd, "IME: %s\tHALTED: %s\n", BOOL(cpu.ime), BOOL(cpu.halted));
}

uint8_t get_reg_by_index(CPU* cpu, Bus* bus, uint8_t index)
{
    switch (index & 0x07) { // Mask to lower 3 bits for safety
        case 0: return cpu->b;
        case 1: return cpu->c;
        case 2: return cpu->d;
        case 3: return cpu->e;
        case 4: return cpu->h;
        case 5: return cpu->l;
        case 6: return bus_read(bus, cpu->hl); // Memory access at address [HL]
        case 7: return cpu->a;
        default: return 0; // Unreachable
    }
}

void set_reg_by_index(CPU* cpu, Bus* bus, uint8_t index, uint8_t value)
{
    switch (index & 0x07) {
        case 0: cpu->b = value; break;
        case 1: cpu->c = value; break;
        case 2: cpu->d = value; break;
        case 3: cpu->e = value; break;
        case 4: cpu->h = value; break;
        case 5: cpu->l = value; break;
        case 6: bus_write(bus, cpu->hl, value); break; // Memory write at address [HL]
        case 7: cpu->a = value; break;
    }
}

void flag_set(CPU* cpu, Flag flag, bool value)
{
    if (value) {
        cpu->f |= flag;
    } else {
        cpu->f &= ~flag;
    }
}

bool flag_get(const CPU* cpu, Flag flag)
{ 
    return (cpu->f & flag) != 0;
}

int cpu_step(CPU* cpu, Bus* bus)
{
    // 1. Fetch
    uint8_t opcode = bus_read(bus, cpu->pc++);

    // 2. Decode & Execute
    int cycles = get_opcode_cycles(opcode, false);
    switch (opcode) {
        case OP_NOP:
            return cycles;

        /*
         * ADC A, r8
         * Add the value in r8 plus the carry flag to A.
         * Cycles: 1 (2 if r8 is [HL]) | Bytes: 1
         * Flags:
         *   Z: Set if result is 0.
         *   N: 0
         *   H: Set if overflow from bit 3.
         *   C: Set if overflow from bit 7.
         */
        case OP_ADC_A_B:
        case OP_ADC_A_C:
        case OP_ADC_A_D:
        case OP_ADC_A_E:
        case OP_ADC_A_H:
        case OP_ADC_A_L:
        case OP_ADC_A_HL_IND:
        case OP_ADC_A_A: {
            // NOTE: `opcode & 0x07` because lowest 3 bits store the target
            // register ID
            uint8_t value = get_reg_by_index(cpu, bus, opcode & 0x07);
            uint8_t carry = flag_get(cpu, FLAG_C) ? 1 : 0;
            
            // Perform 16-bit math to catch 8-bit overflow (Carry)
            uint16_t result = cpu->a + value + carry;
            
            // Set Flags
            flag_set(cpu, FLAG_Z, (uint8_t)result == 0);
            flag_set(cpu, FLAG_N, false);
            // Half-carry checks if lower 4 bits overflow bit 3
            flag_set(cpu, FLAG_H, ((cpu->a & 0x0F) + (value & 0x0F) + carry) > 0x0F);
            // Carry checks if 8-bit result exceeds 0xFF
            flag_set(cpu, FLAG_C, result > 0xFF);

            cpu->a = (uint8_t)result;
            return cycles;
        }
        
        /*
         * ADC A, n8
         * Add the value n8 plus the carry flag to A
         * Cycles: 2 | Bytes: 2
         *
         */
        case OP_ADC_A_n8: {
            uint8_t value = get_reg_by_index(cpu, bus, opcode & 0x07);
        }

        case OP_INC_A:
            cpu->a++;
            flag_set(cpu, FLAG_Z, cpu->a == 0);
            flag_set(cpu, FLAG_N, false);
            flag_set(cpu, FLAG_H, (cpu->a & 0x0F) == 0x00);
            return cycles;

        case OP_JP_a16: {
            cpu->pc = fetch16(cpu, bus);
            return cycles;
        }

        case OP_LD_B_n8:
            cpu->b = fetch8(cpu, bus);
            return cycles;

        case OP_PREFIX:
            return cpu_execute_cb(cpu, bus);

        default:
            fprintf(stderr, "Unhandled opcode: 0x%02X at PC: 0x%04X\n", opcode, cpu->pc - 1);
            exit(1);
    }
}

int cpu_execute_cb(CPU* cpu, Bus* bus) {
    uint8_t cb_opcode = bus_read(bus, cpu->pc++);
    int cycles = get_opcode_cycles(cb_opcode, true); // CB cycle lookup

    uint8_t reg_idx = cb_opcode & 0x07;         // Lowest 3 bits
    uint8_t bit_pos = (cb_opcode >> 3) & 0x07;  // Middle 3 bits

    if (cb_opcode >= 0x40 && cb_opcode <= 0x7F) {
        uint8_t val = get_reg_by_index(cpu, bus, reg_idx);
        flag_set(cpu, FLAG_Z, (val & (1 << bit_pos)) == 0);
        flag_set(cpu, FLAG_N, false);
        flag_set(cpu, FLAG_H, true);
        return cycles;
    }

    // TODO: Add handlers for RLC, RRC, RL, RR, SLA, SRA, SWAP, SRL, RES, SET...
    
    return cycles;
}
