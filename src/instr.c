#include "emu.h"
#include "opcodes.h"
#include <stdint.h>


static bool instr_8bit_arithm(CPU* cpu, Bus* bus, uint8_t opcode)
{
    switch (opcode) {
        // ADC A, r8
        // Add the value in r8 plus the carry flag to A.
        // Cycles: 1 (2 if r8 is [HL]) | Bytes: 1
        // Flags:
        //   Z: Set if result is 0.
        //   N: 0
        //   H: Set if overflow from bit 3.
        //   C: Set if overflow from bit 7.
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
            break;
        }

        // ADC A, n8
        // Add the value n8 plus the carry flag to A
        // Cycles: 2 | Bytes: 2
        case OP_ADC_A_n8: {
            uint8_t value = fetch8(cpu, bus);
            uint8_t carry = flag_get(cpu, FLAG_C) ? 1 : 0;

            uint16_t result = cpu->a + value + carry;

            flag_set(cpu, FLAG_Z, (uint8_t)result == 0);
            flag_set(cpu, FLAG_N, false);
            flag_set(cpu, FLAG_H, ((cpu->a & 0x0F) + (value & 0x0F) + carry) > 0x0F);
            flag_set(cpu, FLAG_C, result > 0xFF);

            cpu->a = (uint8_t)result;
            break;
        }

        case OP_INC_A: {
            cpu->a++;
            flag_set(cpu, FLAG_Z, cpu->a == 0);
            flag_set(cpu, FLAG_N, false);
            flag_set(cpu, FLAG_H, (cpu->a & 0x0F) == 0x00);
            break;
        }

        default:
            return false;
    }
    return true;
}

static bool instr_load(CPU* cpu, Bus* bus, uint8_t opcode)
{
    switch (opcode) {
        case OP_LD_B_n8:
            cpu->b = fetch8(cpu, bus);
            break;

        default:
            return false;
    }
    return true;
}

static bool instr_16bit_arithm(CPU* cpu, Bus* bus, uint8_t opcode)
{
    switch (opcode) {
        default:
            return false;
    }
    return true;
}

static bool instr_bitwise(CPU* cpu, Bus* bus, uint8_t opcode)
{
    switch (opcode) {
        default:
            return false;
    }
    return true;
}

static bool instr_bit_flag(CPU* cpu, Bus* bus, uint8_t opcode)
{
    switch (opcode) {
        default:
            return false;
    }
    return true;
}

static bool instr_bit_shift(CPU* cpu, Bus* bus, uint8_t opcode)
{
    switch (opcode) {
        default:
            return false;
    }
    return true;
}

static bool instr_jumps(CPU* cpu, Bus* bus, uint8_t opcode)
{
    switch (opcode) {
        case OP_JP_a16: {
            cpu->pc = fetch16(cpu, bus);
            break;
        }
        default:
            return false;
    }
    return true;
}

static bool instr_carry(CPU* cpu, Bus* bus, uint8_t opcode)
{
    switch (opcode) {
        default:
            return false;
    }
    return true;
}

static bool instr_stack(CPU* cpu, Bus* bus, uint8_t opcode)
{
    switch (opcode) {
        default:
            return false;
    }
    return true;
}

static bool instr_interrupts(CPU* cpu, Bus* bus, uint8_t opcode)
{
    switch (opcode) {
        default:
            return false;
    }
    return true;
}

static bool instr_misc(CPU* cpu, Bus* bus, uint8_t opcode)
{
    switch (opcode) {
        default:
            return false;
    }
    return true;
}

typedef bool (*instrfn)(CPU*, Bus*, uint8_t);

static instrfn instr_handlers[] = {
    instr_8bit_arithm,
    instr_16bit_arithm,
    instr_bit_flag,
    instr_bit_shift,
    instr_bitwise,
    instr_carry,
    instr_interrupts,
    instr_jumps,
    instr_load,
    instr_misc,
    instr_stack
};


static int instr_cb(CPU* cpu, Bus* bus)
{
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
    
    return 0; // error
}

int instr(CPU* cpu, Bus* bus, uint8_t opcode)
{
    if (opcode == OP_PREFIX) {
        return instr_cb(cpu, bus);
    }

    int cycles = get_opcode_cycles(opcode, false);
    for(size_t i = 0; i < sizeof(instr_handlers) / sizeof(instr_handlers[0]); ++i) {
        instrfn handler = instr_handlers[i];
        if(handler(cpu, bus, opcode)) return cycles;
    }
    return 0; // error
}
