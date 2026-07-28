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

        // ADD A, r8
        // Cycles: 1 | Bytes: 1
        // Flags: Z 0 H C
        case OP_ADD_A_B:
        case OP_ADD_A_C:
        case OP_ADD_A_D:
        case OP_ADD_A_E:
        case OP_ADD_A_H:
        case OP_ADD_A_L:
        case OP_ADD_A_HL_IND:
        case OP_ADD_A_A:
            break;

        // ADD A, n8
        // Cycles: 2 | Bytes: 2
        // Flags: Z 0 H C
        case OP_ADD_A_n8:
            break;

        // CP A, r8
        // Cycles: 1 | Bytes: 1
        // Flags: Z 1 H C
        case OP_CP_A_B:
        case OP_CP_A_C:
        case OP_CP_A_D:
        case OP_CP_A_E:
        case OP_CP_A_H:
        case OP_CP_A_L:
        case OP_CP_A_HL_IND:
        case OP_CP_A_A:
            break;

        // CP A, n8
        // Cycles: 2 | Bytes: 2
        // Flags: Z 1 H C
        case OP_CP_A_n8:
            break;

        // DEC r8
        // Cycles: 1 | Bytes: 1
        // Flags: Z 1 H -
        case OP_DEC_B:
        case OP_DEC_C:
        case OP_DEC_D:
        case OP_DEC_E:
        case OP_DEC_H:
        case OP_DEC_L:
        case OP_DEC_A:
            break;

        // DEC [HL]
        // Cycles: 3 | Bytes: 1
        // Flags: Z 1 H -
        case OP_DEC_HL_IND:
            break;

        // INC r8
        // Cycles: 1 | Bytes: 1
        // Flags: Z 0 H -
        case OP_INC_B:
        case OP_INC_C:
        case OP_INC_D:
        case OP_INC_E:
        case OP_INC_H:
        case OP_INC_L:
        case OP_INC_A: {
            cpu->a++;
            flag_set(cpu, FLAG_Z, cpu->a == 0);
            flag_set(cpu, FLAG_N, false);
            flag_set(cpu, FLAG_H, (cpu->a & 0x0F) == 0x00);
            break;
        }

        // INC [HL]
        // Cycles: 3 | Bytes: 1
        // Flags: Z 0 H -
        case OP_INC_HL_IND:
            break;

        // SBC A, r8
        // Cycles: 1 | Bytes: 1
        // Flags: Z 1 H C
        case OP_SBC_A_B:
        case OP_SBC_A_C:
        case OP_SBC_A_D:
        case OP_SBC_A_E:
        case OP_SBC_A_H:
        case OP_SBC_A_L:
        case OP_SBC_A_HL_IND:
        case OP_SBC_A_A:
            break;

        // SBC A, n8
        // Cycles: 2 | Bytes: 2
        // Flags: Z 1 H C
        case OP_SBC_A_n8:
            break;

        // SUB A, r8
        // Cycles: 1 | Bytes: 1
        // Flags: Z 1 H C
        case OP_SUB_A_B:
        case OP_SUB_A_C:
        case OP_SUB_A_D:
        case OP_SUB_A_E:
        case OP_SUB_A_H:
        case OP_SUB_A_L:
        case OP_SUB_A_HL_IND:
        case OP_SUB_A_A:
            break;

        // SUB A, n8
        // Cycles: 2 | Bytes: 2
        // Flags: Z 1 H C
        case OP_SUB_A_n8:
            break;

        default:
            return false;
    }
    return true;
}

static bool instr_load(CPU* cpu, Bus* bus, uint8_t opcode)
{
    switch (opcode) {
        // LD r8, r8
        // Cycles: 1 | Bytes: 1 | Flags: -
        case OP_LD_B_B:
        case OP_LD_B_C:
        case OP_LD_B_D:
        case OP_LD_B_E:
        case OP_LD_B_H:
        case OP_LD_B_L:
        case OP_LD_B_A:
        case OP_LD_C_B:
        case OP_LD_C_C:
        case OP_LD_C_D:
        case OP_LD_C_E:
        case OP_LD_C_H:
        case OP_LD_C_L:
        case OP_LD_C_A:
        case OP_LD_D_B:
        case OP_LD_D_C:
        case OP_LD_D_D:
        case OP_LD_D_E:
        case OP_LD_D_H:
        case OP_LD_D_L:
        case OP_LD_D_A:
        case OP_LD_E_B:
        case OP_LD_E_C:
        case OP_LD_E_D:
        case OP_LD_E_E:
        case OP_LD_E_H:
        case OP_LD_E_L:
        case OP_LD_E_A:
        case OP_LD_H_B:
        case OP_LD_H_C:
        case OP_LD_H_D:
        case OP_LD_H_E:
        case OP_LD_H_H:
        case OP_LD_H_L:
        case OP_LD_H_A:
        case OP_LD_L_B:
        case OP_LD_L_C:
        case OP_LD_L_D:
        case OP_LD_L_E:
        case OP_LD_L_H:
        case OP_LD_L_L:
        case OP_LD_L_A:
        case OP_LD_A_B:
        case OP_LD_A_C:
        case OP_LD_A_D:
        case OP_LD_A_E:
        case OP_LD_A_H:
        case OP_LD_A_L:
        case OP_LD_A_A:
            break;

        // LD r8, n8
        // Cycles: 2 | Bytes: 2 | Flags: -
        case OP_LD_B_n8:
            cpu->b = fetch8(cpu, bus);
            break;
        case OP_LD_C_n8:
        case OP_LD_D_n8:
        case OP_LD_E_n8:
        case OP_LD_H_n8:
        case OP_LD_L_n8:
        case OP_LD_A_n8:
            break;

        // LD r16, n16
        // Cycles: 3 | Bytes: 3 | Flags: -
        case OP_LD_BC_n16:
        case OP_LD_DE_n16:
        case OP_LD_HL_n16:
        case OP_LD_SP_n16:
            break;

        // LD [HL], r8
        // Cycles: 2 | Bytes: 1 | Flags: -
        case OP_LD_HL_IND_B:
        case OP_LD_HL_IND_C:
        case OP_LD_HL_IND_D:
        case OP_LD_HL_IND_E:
        case OP_LD_HL_IND_H:
        case OP_LD_HL_IND_L:
        case OP_LD_HL_IND_A:
            break;

        // LD [HL], n8
        // Cycles: 3 | Bytes: 2 | Flags: -
        case OP_LD_HL_IND_n8:
            break;

        // LD r8, [HL]
        // Cycles: 2 | Bytes: 1 | Flags: -
        case OP_LD_B_HL_IND:
        case OP_LD_C_HL_IND:
        case OP_LD_D_HL_IND:
        case OP_LD_E_HL_IND:
        case OP_LD_H_HL_IND:
        case OP_LD_L_HL_IND:
        case OP_LD_A_HL_IND:
            break;

        // LD [r16], A
        // Cycles: 2 | Bytes: 1 | Flags: -
        case OP_LD_BC_IND_A:
        case OP_LD_DE_IND_A:
            break;

        // LD [n16], A
        // Cycles: 4 | Bytes: 3 | Flags: -
        case OP_LD_a16_IND_A:
            break;

        // LDH [n16], A
        // Cycles: 3 | Bytes: 2 | Flags: -
        case OP_LDH_a8_IND_A:
            break;

        // LDH [C], A
        // Cycles: 2 | Bytes: 1 | Flags: -
        case OP_LDH_C_IND_A:
            break;

        // LD A, [r16]
        // Cycles: 2 | Bytes: 1 | Flags: -
        case OP_LD_A_BC_IND:
        case OP_LD_A_DE_IND:
            break;

        // LD A, [n16]
        // Cycles: 4 | Bytes: 3 | Flags: -
        case OP_LD_A_a16_IND:
            break;

        // LDH A, [n16]
        // Cycles: 3 | Bytes: 2 | Flags: -
        case OP_LDH_A_a8_IND:
            break;

        // LDH A, [C]
        // Cycles: 2 | Bytes: 1 | Flags: -
        case OP_LDH_A_C_IND:
            break;

        // LD [HLI], A
        // Cycles: 2 | Bytes: 1 | Flags: -
        case OP_LD_HL_INC_A:
            break;

        // LD [HLD], A
        // Cycles: 2 | Bytes: 1 | Flags: -
        case OP_LD_HL_DEC_A:
            break;

        // LD A, [HLI]
        // Cycles: 2 | Bytes: 1 | Flags: -
        case OP_LD_A_HL_INC:
            break;

        // LD A, [HLD]
        // Cycles: 2 | Bytes: 1 | Flags: -
        case OP_LD_A_HL_DEC:
            break;

        // LD [n16], SP
        // Cycles: 5 | Bytes: 3 | Flags: -
        case OP_LD_a16_IND_SP:
            break;

        // LD HL, SP+e8
        // Cycles: 3 | Bytes: 2 | Flags: 0 0 H C
        case OP_LD_HL_SP_e8:
            break;

        // LD SP, HL
        // Cycles: 2 | Bytes: 1 | Flags: -
        case OP_LD_SP_HL:
            break;

        default:
            return false;
    }
    return true;
}

static bool instr_16bit_arithm(CPU* cpu, Bus* bus, uint8_t opcode)
{
    switch (opcode) {
        // ADD HL, r16
        // Cycles: 2 | Bytes: 1 | Flags: - 0 H C
        case OP_ADD_HL_BC:
        case OP_ADD_HL_DE:
        case OP_ADD_HL_HL:
            break;

        // ADD HL, SP
        // Cycles: 2 | Bytes: 1 | Flags: - 0 H C
        case OP_ADD_HL_SP:
            break;

        // ADD SP, e8
        // Cycles: 4 | Bytes: 2 | Flags: 0 0 H C
        case OP_ADD_SP_e8:
            break;

        // DEC r16
        // Cycles: 2 | Bytes: 1 | Flags: -
        case OP_DEC_BC:
        case OP_DEC_DE:
        case OP_DEC_HL:
            break;

        // DEC SP
        // Cycles: 2 | Bytes: 1 | Flags: -
        case OP_DEC_SP:
            break;

        // INC r16
        // Cycles: 2 | Bytes: 1 | Flags: -
        case OP_INC_BC:
        case OP_INC_DE:
        case OP_INC_HL:
            break;

        // INC SP
        // Cycles: 2 | Bytes: 1 | Flags: -
        case OP_INC_SP:
            break;

        default:
            return false;
    }
    return true;
}

static bool instr_bitwise(CPU* cpu, Bus* bus, uint8_t opcode)
{
    switch (opcode) {
        // AND A, r8
        // Cycles: 1 | Bytes: 1 | Flags: Z 0 1 0
        case OP_AND_A_B:
        case OP_AND_A_C:
        case OP_AND_A_D:
        case OP_AND_A_E:
        case OP_AND_A_H:
        case OP_AND_A_L:
        case OP_AND_A_HL_IND:
        case OP_AND_A_A:
            break;

        // AND A, n8
        // Cycles: 2 | Bytes: 2 | Flags: Z 0 1 0
        case OP_AND_A_n8:
            break;

        // CPL
        // Cycles: 1 | Bytes: 1 | Flags: - 1 1 -
        case OP_CPL:
            break;

        // OR A, r8
        // Cycles: 1 | Bytes: 1 | Flags: Z 0 0 0
        case OP_OR_A_B:
        case OP_OR_A_C:
        case OP_OR_A_D:
        case OP_OR_A_E:
        case OP_OR_A_H:
        case OP_OR_A_L:
        case OP_OR_A_HL_IND:
        case OP_OR_A_A:
            break;

        // OR A, n8
        // Cycles: 2 | Bytes: 2 | Flags: Z 0 0 0
        case OP_OR_A_n8:
            break;

        // XOR A, r8
        // Cycles: 1 | Bytes: 1 | Flags: Z 0 0 0
        case OP_XOR_A_B:
        case OP_XOR_A_C:
        case OP_XOR_A_D:
        case OP_XOR_A_E:
        case OP_XOR_A_H:
        case OP_XOR_A_L:
        case OP_XOR_A_HL_IND:
        case OP_XOR_A_A:
            break;

        // XOR A, n8
        // Cycles: 2 | Bytes: 2 | Flags: Z 0 0 0
        case OP_XOR_A_n8:
            break;

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
        // RLCA
        // Cycles: 1 | Bytes: 1 | Flags: 0 0 0 C
        case OP_RLCA:
            break;

        // RRCA
        // Cycles: 1 | Bytes: 1 | Flags: 0 0 0 C
        case OP_RRCA:
            break;

        // RLA
        // Cycles: 1 | Bytes: 1 | Flags: 0 0 0 C
        case OP_RLA:
            break;

        // RRA
        // Cycles: 1 | Bytes: 1 | Flags: 0 0 0 C
        case OP_RRA:
            break;

        default:
            return false;
    }
    return true;
}

static bool instr_jumps(CPU* cpu, Bus* bus, uint8_t opcode)
{
    switch (opcode) {
        // JP n16
        // Cycles: 4 | Bytes: 3 | Flags: -
        case OP_JP_a16: {
            cpu->pc = fetch16(cpu, bus);
            break;
        }

        // JP cc, n16
        // Cycles: 4 taken / 3 untaken | Bytes: 3 | Flags: -
        case OP_JP_NZ_a16:
        case OP_JP_Z_a16:
        case OP_JP_NC_a16:
        case OP_JP_C_a16:
            break;

        // JP HL
        // Cycles: 1 | Bytes: 1 | Flags: -
        case OP_JP_HL:
            break;

        // JR e8
        // Cycles: 3 | Bytes: 2 | Flags: -
        case OP_JR_e8:
            break;

        // JR cc, e8
        // Cycles: 3 taken / 2 untaken | Bytes: 2 | Flags: -
        case OP_JR_NZ_e8:
        case OP_JR_Z_e8:
        case OP_JR_NC_e8:
        case OP_JR_C_e8:
            break;

        // CALL n16
        // Cycles: 6 | Bytes: 3 | Flags: -
        case OP_CALL_a16:
            break;

        // CALL cc, n16
        // Cycles: 6 taken / 3 untaken | Bytes: 3 | Flags: -
        case OP_CALL_NZ_a16:
        case OP_CALL_Z_a16:
        case OP_CALL_NC_a16:
        case OP_CALL_C_a16:
            break;

        // RET
        // Cycles: 4 | Bytes: 1 | Flags: -
        case OP_RET:
            break;

        // RET cc
        // Cycles: 5 taken / 2 untaken | Bytes: 1 | Flags: -
        case OP_RET_NZ:
        case OP_RET_Z:
        case OP_RET_NC:
        case OP_RET_C:
            break;

        // RETI
        // Cycles: 4 | Bytes: 1 | Flags: -
        case OP_RETI:
            break;

        // RST vec
        // Cycles: 4 | Bytes: 1 | Flags: -
        case OP_RST_S00:
        case OP_RST_S08:
        case OP_RST_S10:
        case OP_RST_S18:
        case OP_RST_S20:
        case OP_RST_S28:
        case OP_RST_S30:
        case OP_RST_S38:
            break;

        default:
            return false;
    }
    return true;
}

static bool instr_carry(CPU* cpu, Bus* bus, uint8_t opcode)
{
    switch (opcode) {
        // CCF
        // Cycles: 1 | Bytes: 1 | Flags: - 0 0 C
        case OP_CCF:
            break;

        // SCF
        // Cycles: 1 | Bytes: 1 | Flags: - 0 0 1
        case OP_SCF:
            break;

        default:
            return false;
    }
    return true;
}

static bool instr_stack(CPU* cpu, Bus* bus, uint8_t opcode)
{
    switch (opcode) {
        // POP r16
        // Cycles: 3 | Bytes: 1 | Flags: - (POP AF sets Z N H C)
        case OP_POP_BC:
        case OP_POP_DE:
        case OP_POP_HL:
        case OP_POP_AF:
            break;

        // PUSH r16
        // Cycles: 4 | Bytes: 1 | Flags: -
        case OP_PUSH_BC:
        case OP_PUSH_DE:
        case OP_PUSH_HL:
        case OP_PUSH_AF:
            break;

        default:
            return false;
    }
    return true;
}

static bool instr_interrupts(CPU* cpu, Bus* bus, uint8_t opcode)
{
    switch (opcode) {
        // DI
        // Cycles: 1 | Bytes: 1 | Flags: -
        case OP_DI:
            break;

        // EI
        // Cycles: 1 | Bytes: 1 | Flags: -
        case OP_EI:
            break;

        // HALT
        // Cycles: - | Bytes: 1 | Flags: -
        case OP_HALT:
            break;

        default:
            return false;
    }
    return true;
}

static bool instr_misc(CPU* cpu, Bus* bus, uint8_t opcode)
{
    switch (opcode) {
        // DAA
        // Cycles: 1 | Bytes: 1 | Flags: Z - 0 C
        case OP_DAA:
            break;

        // NOP
        // Cycles: 1 | Bytes: 1 | Flags: -
        case OP_NOP:
            break;

        // STOP
        // Cycles: - | Bytes: 2 | Flags: -
        case OP_STOP_n8:
            break;

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
