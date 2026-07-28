#include "emu.h"
#include "opcodes.h"
#include <stdint.h>


static bool instr_8bit_arithm(CPU* cpu, Bus* bus, uint8_t opcode)
{
    switch (opcode) {
        // ADC A, r8
        // Cycles: 1 (2 if [HL]) | Bytes: 1 | Flags: Z 0 H C
        case OP_ADC_A_B:
        case OP_ADC_A_C:
        case OP_ADC_A_D:
        case OP_ADC_A_E:
        case OP_ADC_A_H:
        case OP_ADC_A_L:
        case OP_ADC_A_HL_IND:
        case OP_ADC_A_A: {
            uint8_t value = get_reg_by_index(cpu, bus, opcode & 0x07);
            uint8_t carry = flag_get(cpu, FLAG_C) ? 1 : 0;
            uint16_t result = cpu->a + value + carry;
            flag_set(cpu, FLAG_Z, (uint8_t)result == 0);
            flag_set(cpu, FLAG_N, false);
            flag_set(cpu, FLAG_H, ((cpu->a & 0x0F) + (value & 0x0F) + carry) > 0x0F);
            flag_set(cpu, FLAG_C, result > 0xFF);
            cpu->a = (uint8_t)result;
            break;
        }

        // ADC A, n8
        // Cycles: 2 | Bytes: 2 | Flags: Z 0 H C
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
        // Cycles: 1 (2 if [HL]) | Bytes: 1 | Flags: Z 0 H C
        case OP_ADD_A_B:
        case OP_ADD_A_C:
        case OP_ADD_A_D:
        case OP_ADD_A_E:
        case OP_ADD_A_H:
        case OP_ADD_A_L:
        case OP_ADD_A_HL_IND:
        case OP_ADD_A_A: {
            uint8_t value = get_reg_by_index(cpu, bus, opcode & 0x07);
            uint16_t result = cpu->a + value;
            flag_set(cpu, FLAG_Z, (uint8_t)result == 0);
            flag_set(cpu, FLAG_N, false);
            flag_set(cpu, FLAG_H, ((cpu->a & 0x0F) + (value & 0x0F)) > 0x0F);
            flag_set(cpu, FLAG_C, result > 0xFF);
            cpu->a = (uint8_t)result;
            break;
        }

        // ADD A, n8
        // Cycles: 2 | Bytes: 2 | Flags: Z 0 H C
        case OP_ADD_A_n8: {
            uint8_t value = fetch8(cpu, bus);
            uint16_t result = cpu->a + value;
            flag_set(cpu, FLAG_Z, (uint8_t)result == 0);
            flag_set(cpu, FLAG_N, false);
            flag_set(cpu, FLAG_H, ((cpu->a & 0x0F) + (value & 0x0F)) > 0x0F);
            flag_set(cpu, FLAG_C, result > 0xFF);
            cpu->a = (uint8_t)result;
            break;
        }

        // CP A, r8
        // Cycles: 1 (2 if [HL]) | Bytes: 1 | Flags: Z 1 H C
        case OP_CP_A_B:
        case OP_CP_A_C:
        case OP_CP_A_D:
        case OP_CP_A_E:
        case OP_CP_A_H:
        case OP_CP_A_L:
        case OP_CP_A_HL_IND:
        case OP_CP_A_A: {
            uint8_t value = get_reg_by_index(cpu, bus, opcode & 0x07);
            uint16_t result = cpu->a - value;
            flag_set(cpu, FLAG_Z, (uint8_t)result == 0);
            flag_set(cpu, FLAG_N, true);
            flag_set(cpu, FLAG_H, (cpu->a & 0x0F) < (value & 0x0F));
            flag_set(cpu, FLAG_C, cpu->a < value);
            break;
        }

        // CP A, n8
        // Cycles: 2 | Bytes: 2 | Flags: Z 1 H C
        case OP_CP_A_n8: {
            uint8_t value = fetch8(cpu, bus);
            uint16_t result = cpu->a - value;
            flag_set(cpu, FLAG_Z, (uint8_t)result == 0);
            flag_set(cpu, FLAG_N, true);
            flag_set(cpu, FLAG_H, (cpu->a & 0x0F) < (value & 0x0F));
            flag_set(cpu, FLAG_C, cpu->a < value);
            break;
        }

        // DEC r8
        // Cycles: 1 (3 if [HL]) | Bytes: 1 | Flags: Z 1 H -
        case OP_DEC_B:
        case OP_DEC_C:
        case OP_DEC_D:
        case OP_DEC_E:
        case OP_DEC_H:
        case OP_DEC_L:
        case OP_DEC_A: {
            uint8_t reg_idx = opcode & 0x07;
            uint8_t value = get_reg_by_index(cpu, bus, reg_idx);
            uint8_t result = value - 1;
            set_reg_by_index(cpu, bus, reg_idx, result);
            flag_set(cpu, FLAG_Z, result == 0);
            flag_set(cpu, FLAG_N, true);
            flag_set(cpu, FLAG_H, (value & 0x0F) == 0x00);
            break;
        }

        // DEC [HL]
        // Cycles: 3 | Bytes: 1 | Flags: Z 1 H -
        case OP_DEC_HL_IND: {
            uint8_t value = bus_read(bus, cpu->hl);
            uint8_t result = value - 1;
            bus_write(bus, cpu->hl, result);
            flag_set(cpu, FLAG_Z, result == 0);
            flag_set(cpu, FLAG_N, true);
            flag_set(cpu, FLAG_H, (value & 0x0F) == 0x00);
            break;
        }

        // INC r8
        // Cycles: 1 (3 if [HL]) | Bytes: 1 | Flags: Z 0 H -
        case OP_INC_B:
        case OP_INC_C:
        case OP_INC_D:
        case OP_INC_E:
        case OP_INC_H:
        case OP_INC_L:
        case OP_INC_A: {
            uint8_t reg_idx = opcode & 0x07;
            uint8_t value = get_reg_by_index(cpu, bus, reg_idx);
            uint8_t result = value + 1;
            set_reg_by_index(cpu, bus, reg_idx, result);
            flag_set(cpu, FLAG_Z, result == 0);
            flag_set(cpu, FLAG_N, false);
            flag_set(cpu, FLAG_H, (value & 0x0F) == 0x0F);
            break;
        }

        // INC [HL]
        // Cycles: 3 | Bytes: 1 | Flags: Z 0 H -
        case OP_INC_HL_IND: {
            uint8_t value = bus_read(bus, cpu->hl);
            uint8_t result = value + 1;
            bus_write(bus, cpu->hl, result);
            flag_set(cpu, FLAG_Z, result == 0);
            flag_set(cpu, FLAG_N, false);
            flag_set(cpu, FLAG_H, (value & 0x0F) == 0x0F);
            break;
        }

        // SBC A, r8
        // Cycles: 1 (2 if [HL]) | Bytes: 1 | Flags: Z 1 H C
        case OP_SBC_A_B:
        case OP_SBC_A_C:
        case OP_SBC_A_D:
        case OP_SBC_A_E:
        case OP_SBC_A_H:
        case OP_SBC_A_L:
        case OP_SBC_A_HL_IND:
        case OP_SBC_A_A: {
            uint8_t value = get_reg_by_index(cpu, bus, opcode & 0x07);
            uint8_t carry = flag_get(cpu, FLAG_C) ? 1 : 0;
            int16_t result = cpu->a - value - carry;
            flag_set(cpu, FLAG_Z, (uint8_t)result == 0);
            flag_set(cpu, FLAG_N, true);
            flag_set(cpu, FLAG_H, ((cpu->a & 0x0F) - (value & 0x0F) - carry) < 0);
            flag_set(cpu, FLAG_C, result < 0);
            cpu->a = (uint8_t)result;
            break;
        }

        // SBC A, n8
        // Cycles: 2 | Bytes: 2 | Flags: Z 1 H C
        case OP_SBC_A_n8: {
            uint8_t value = fetch8(cpu, bus);
            uint8_t carry = flag_get(cpu, FLAG_C) ? 1 : 0;
            int16_t result = cpu->a - value - carry;
            flag_set(cpu, FLAG_Z, (uint8_t)result == 0);
            flag_set(cpu, FLAG_N, true);
            flag_set(cpu, FLAG_H, ((cpu->a & 0x0F) - (value & 0x0F) - carry) < 0);
            flag_set(cpu, FLAG_C, result < 0);
            cpu->a = (uint8_t)result;
            break;
        }

        // SUB A, r8
        // Cycles: 1 (2 if [HL]) | Bytes: 1 | Flags: Z 1 H C
        case OP_SUB_A_B:
        case OP_SUB_A_C:
        case OP_SUB_A_D:
        case OP_SUB_A_E:
        case OP_SUB_A_H:
        case OP_SUB_A_L:
        case OP_SUB_A_HL_IND:
        case OP_SUB_A_A: {
            uint8_t value = get_reg_by_index(cpu, bus, opcode & 0x07);
            uint16_t result = cpu->a - value;
            flag_set(cpu, FLAG_Z, (uint8_t)result == 0);
            flag_set(cpu, FLAG_N, true);
            flag_set(cpu, FLAG_H, (cpu->a & 0x0F) < (value & 0x0F));
            flag_set(cpu, FLAG_C, cpu->a < value);
            cpu->a = (uint8_t)result;
            break;
        }

        // SUB A, n8
        // Cycles: 2 | Bytes: 2 | Flags: Z 1 H C
        case OP_SUB_A_n8: {
            uint8_t value = fetch8(cpu, bus);
            uint16_t result = cpu->a - value;
            flag_set(cpu, FLAG_Z, (uint8_t)result == 0);
            flag_set(cpu, FLAG_N, true);
            flag_set(cpu, FLAG_H, (cpu->a & 0x0F) < (value & 0x0F));
            flag_set(cpu, FLAG_C, cpu->a < value);
            cpu->a = (uint8_t)result;
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
        case OP_LD_A_A: {
            uint8_t dest = (opcode >> 3) & 0x07;
            uint8_t src = opcode & 0x07;
            uint8_t value = get_reg_by_index(cpu, bus, src);
            set_reg_by_index(cpu, bus, dest, value);
            break;
        }

        // LD r8, n8
        // Cycles: 2 | Bytes: 2 | Flags: -
        case OP_LD_B_n8:
        case OP_LD_C_n8:
        case OP_LD_D_n8:
        case OP_LD_E_n8:
        case OP_LD_H_n8:
        case OP_LD_L_n8:
        case OP_LD_A_n8: {
            uint8_t dest = (opcode >> 3) & 0x07;
            uint8_t value = fetch8(cpu, bus);
            set_reg_by_index(cpu, bus, dest, value);
            break;
        }

        // LD r16, n16
        // Cycles: 3 | Bytes: 3 | Flags: -
        case OP_LD_BC_n16: {
            cpu->bc = fetch16(cpu, bus);
            break;
        }
        case OP_LD_DE_n16: {
            cpu->de = fetch16(cpu, bus);
            break;
        }
        case OP_LD_HL_n16: {
            cpu->hl = fetch16(cpu, bus);
            break;
        }
        case OP_LD_SP_n16: {
            cpu->sp = fetch16(cpu, bus);
            break;
        }

        // LD [HL], r8
        // Cycles: 2 | Bytes: 1 | Flags: -
        case OP_LD_HL_IND_B:
        case OP_LD_HL_IND_C:
        case OP_LD_HL_IND_D:
        case OP_LD_HL_IND_E:
        case OP_LD_HL_IND_H:
        case OP_LD_HL_IND_L:
        case OP_LD_HL_IND_A: {
            uint8_t src = opcode & 0x07;
            uint8_t value = get_reg_by_index(cpu, bus, src);
            bus_write(bus, cpu->hl, value);
            break;
        }

        // LD [HL], n8
        // Cycles: 3 | Bytes: 2 | Flags: -
        case OP_LD_HL_IND_n8: {
            uint8_t value = fetch8(cpu, bus);
            bus_write(bus, cpu->hl, value);
            break;
        }

        // LD r8, [HL]
        // Cycles: 2 | Bytes: 1 | Flags: -
        case OP_LD_B_HL_IND:
        case OP_LD_C_HL_IND:
        case OP_LD_D_HL_IND:
        case OP_LD_E_HL_IND:
        case OP_LD_H_HL_IND:
        case OP_LD_L_HL_IND:
        case OP_LD_A_HL_IND: {
            uint8_t dest = (opcode >> 3) & 0x07;
            uint8_t value = bus_read(bus, cpu->hl);
            set_reg_by_index(cpu, bus, dest, value);
            break;
        }

        // LD [r16], A
        // Cycles: 2 | Bytes: 1 | Flags: -
        case OP_LD_BC_IND_A: {
            bus_write(bus, cpu->bc, cpu->a);
            break;
        }
        case OP_LD_DE_IND_A: {
            bus_write(bus, cpu->de, cpu->a);
            break;
        }

        // LD [n16], A
        // Cycles: 4 | Bytes: 3 | Flags: -
        case OP_LD_a16_IND_A: {
            uint16_t addr = fetch16(cpu, bus);
            bus_write(bus, addr, cpu->a);
            break;
        }

        // LDH [n16], A
        // Cycles: 3 | Bytes: 2 | Flags: -
        case OP_LDH_a8_IND_A: {
            uint8_t a8 = fetch8(cpu, bus);
            bus_write(bus, 0xFF00 + a8, cpu->a);
            break;
        }

        // LDH [C], A
        // Cycles: 2 | Bytes: 1 | Flags: -
        case OP_LDH_C_IND_A: {
            bus_write(bus, 0xFF00 + cpu->c, cpu->a);
            break;
        }

        // LD A, [r16]
        // Cycles: 2 | Bytes: 1 | Flags: -
        case OP_LD_A_BC_IND: {
            cpu->a = bus_read(bus, cpu->bc);
            break;
        }
        case OP_LD_A_DE_IND: {
            cpu->a = bus_read(bus, cpu->de);
            break;
        }

        // LD A, [n16]
        // Cycles: 4 | Bytes: 3 | Flags: -
        case OP_LD_A_a16_IND: {
            uint16_t addr = fetch16(cpu, bus);
            cpu->a = bus_read(bus, addr);
            break;
        }

        // LDH A, [n16]
        // Cycles: 3 | Bytes: 2 | Flags: -
        case OP_LDH_A_a8_IND: {
            uint8_t a8 = fetch8(cpu, bus);
            cpu->a = bus_read(bus, 0xFF00 + a8);
            break;
        }

        // LDH A, [C]
        // Cycles: 2 | Bytes: 1 | Flags: -
        case OP_LDH_A_C_IND: {
            cpu->a = bus_read(bus, 0xFF00 + cpu->c);
            break;
        }

        // LD [HLI], A
        // Cycles: 2 | Bytes: 1 | Flags: -
        case OP_LD_HL_INC_A: {
            bus_write(bus, cpu->hl, cpu->a);
            cpu->hl++;
            break;
        }

        // LD [HLD], A
        // Cycles: 2 | Bytes: 1 | Flags: -
        case OP_LD_HL_DEC_A: {
            bus_write(bus, cpu->hl, cpu->a);
            cpu->hl--;
            break;
        }

        // LD A, [HLI]
        // Cycles: 2 | Bytes: 1 | Flags: -
        case OP_LD_A_HL_INC: {
            cpu->a = bus_read(bus, cpu->hl);
            cpu->hl++;
            break;
        }

        // LD A, [HLD]
        // Cycles: 2 | Bytes: 1 | Flags: -
        case OP_LD_A_HL_DEC: {
            cpu->a = bus_read(bus, cpu->hl);
            cpu->hl--;
            break;
        }

        // LD [n16], SP
        // Cycles: 5 | Bytes: 3 | Flags: -
        case OP_LD_a16_IND_SP: {
            uint16_t addr = fetch16(cpu, bus);
            bus_write(bus, addr, cpu->sp & 0xFF);
            bus_write(bus, addr + 1, cpu->sp >> 8);
            break;
        }

        // LD HL, SP+e8
        // Cycles: 3 | Bytes: 2 | Flags: 0 0 H C
        case OP_LD_HL_SP_e8: {
            int8_t e8 = (int8_t)fetch8(cpu, bus);
            uint16_t result = cpu->sp + e8;
            flag_set(cpu, FLAG_Z, false);
            flag_set(cpu, FLAG_N, false);
            flag_set(cpu, FLAG_H, ((cpu->sp & 0x0F) + (e8 & 0x0F)) > 0x0F);
            flag_set(cpu, FLAG_C, ((cpu->sp & 0xFF) + (e8 & 0xFF)) > 0xFF);
            cpu->hl = result;
            break;
        }

        // LD SP, HL
        // Cycles: 2 | Bytes: 1 | Flags: -
        case OP_LD_SP_HL: {
            cpu->sp = cpu->hl;
            break;
        }

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
        case OP_ADD_HL_BC: {
            uint32_t result = cpu->hl + cpu->bc;
            flag_set(cpu, FLAG_N, false);
            flag_set(cpu, FLAG_H, ((cpu->hl & 0x0FFF) + (cpu->bc & 0x0FFF)) > 0x0FFF);
            flag_set(cpu, FLAG_C, result > 0xFFFF);
            cpu->hl = (uint16_t)result;
            break;
        }
        case OP_ADD_HL_DE: {
            uint32_t result = cpu->hl + cpu->de;
            flag_set(cpu, FLAG_N, false);
            flag_set(cpu, FLAG_H, ((cpu->hl & 0x0FFF) + (cpu->de & 0x0FFF)) > 0x0FFF);
            flag_set(cpu, FLAG_C, result > 0xFFFF);
            cpu->hl = (uint16_t)result;
            break;
        }
        case OP_ADD_HL_HL: {
            uint32_t result = cpu->hl + cpu->hl;
            flag_set(cpu, FLAG_N, false);
            flag_set(cpu, FLAG_H, ((cpu->hl & 0x0FFF) + (cpu->hl & 0x0FFF)) > 0x0FFF);
            flag_set(cpu, FLAG_C, result > 0xFFFF);
            cpu->hl = (uint16_t)result;
            break;
        }

        // ADD HL, SP
        // Cycles: 2 | Bytes: 1 | Flags: - 0 H C
        case OP_ADD_HL_SP: {
            uint32_t result = cpu->hl + cpu->sp;
            flag_set(cpu, FLAG_N, false);
            flag_set(cpu, FLAG_H, ((cpu->hl & 0x0FFF) + (cpu->sp & 0x0FFF)) > 0x0FFF);
            flag_set(cpu, FLAG_C, result > 0xFFFF);
            cpu->hl = (uint16_t)result;
            break;
        }

        // ADD SP, e8
        // Cycles: 4 | Bytes: 2 | Flags: 0 0 H C
        case OP_ADD_SP_e8: {
            int8_t e8 = (int8_t)fetch8(cpu, bus);
            flag_set(cpu, FLAG_Z, false);
            flag_set(cpu, FLAG_N, false);
            flag_set(cpu, FLAG_H, ((cpu->sp & 0x0F) + (e8 & 0x0F)) > 0x0F);
            flag_set(cpu, FLAG_C, ((cpu->sp & 0xFF) + (e8 & 0xFF)) > 0xFF);
            cpu->sp += e8;
            break;
        }

        // DEC r16
        // Cycles: 2 | Bytes: 1 | Flags: -
        case OP_DEC_BC: {
            cpu->bc--;
            break;
        }
        case OP_DEC_DE: {
            cpu->de--;
            break;
        }
        case OP_DEC_HL: {
            cpu->hl--;
            break;
        }

        // DEC SP
        // Cycles: 2 | Bytes: 1 | Flags: -
        case OP_DEC_SP: {
            cpu->sp--;
            break;
        }

        // INC r16
        // Cycles: 2 | Bytes: 1 | Flags: -
        case OP_INC_BC: {
            cpu->bc++;
            break;
        }
        case OP_INC_DE: {
            cpu->de++;
            break;
        }
        case OP_INC_HL: {
            cpu->hl++;
            break;
        }

        // INC SP
        // Cycles: 2 | Bytes: 1 | Flags: -
        case OP_INC_SP: {
            cpu->sp++;
            break;
        }

        default:
            return false;
    }
    return true;
}

static bool instr_bitwise(CPU* cpu, Bus* bus, uint8_t opcode)
{
    switch (opcode) {
        // AND A, r8
        // Cycles: 1 (2 if [HL]) | Bytes: 1 | Flags: Z 0 1 0
        case OP_AND_A_B:
        case OP_AND_A_C:
        case OP_AND_A_D:
        case OP_AND_A_E:
        case OP_AND_A_H:
        case OP_AND_A_L:
        case OP_AND_A_HL_IND:
        case OP_AND_A_A: {
            uint8_t value = get_reg_by_index(cpu, bus, opcode & 0x07);
            cpu->a &= value;
            flag_set(cpu, FLAG_Z, cpu->a == 0);
            flag_set(cpu, FLAG_N, false);
            flag_set(cpu, FLAG_H, true);
            flag_set(cpu, FLAG_C, false);
            break;
        }

        // AND A, n8
        // Cycles: 2 | Bytes: 2 | Flags: Z 0 1 0
        case OP_AND_A_n8: {
            uint8_t value = fetch8(cpu, bus);
            cpu->a &= value;
            flag_set(cpu, FLAG_Z, cpu->a == 0);
            flag_set(cpu, FLAG_N, false);
            flag_set(cpu, FLAG_H, true);
            flag_set(cpu, FLAG_C, false);
            break;
        }

        // CPL
        // Cycles: 1 | Bytes: 1 | Flags: - 1 1 -
        case OP_CPL: {
            cpu->a = ~cpu->a;
            flag_set(cpu, FLAG_N, true);
            flag_set(cpu, FLAG_H, true);
            break;
        }

        // OR A, r8
        // Cycles: 1 (2 if [HL]) | Bytes: 1 | Flags: Z 0 0 0
        case OP_OR_A_B:
        case OP_OR_A_C:
        case OP_OR_A_D:
        case OP_OR_A_E:
        case OP_OR_A_H:
        case OP_OR_A_L:
        case OP_OR_A_HL_IND:
        case OP_OR_A_A: {
            uint8_t value = get_reg_by_index(cpu, bus, opcode & 0x07);
            cpu->a |= value;
            flag_set(cpu, FLAG_Z, cpu->a == 0);
            flag_set(cpu, FLAG_N, false);
            flag_set(cpu, FLAG_H, false);
            flag_set(cpu, FLAG_C, false);
            break;
        }

        // OR A, n8
        // Cycles: 2 | Bytes: 2 | Flags: Z 0 0 0
        case OP_OR_A_n8: {
            uint8_t value = fetch8(cpu, bus);
            cpu->a |= value;
            flag_set(cpu, FLAG_Z, cpu->a == 0);
            flag_set(cpu, FLAG_N, false);
            flag_set(cpu, FLAG_H, false);
            flag_set(cpu, FLAG_C, false);
            break;
        }

        // XOR A, r8
        // Cycles: 1 (2 if [HL]) | Bytes: 1 | Flags: Z 0 0 0
        case OP_XOR_A_B:
        case OP_XOR_A_C:
        case OP_XOR_A_D:
        case OP_XOR_A_E:
        case OP_XOR_A_H:
        case OP_XOR_A_L:
        case OP_XOR_A_HL_IND:
        case OP_XOR_A_A: {
            uint8_t value = get_reg_by_index(cpu, bus, opcode & 0x07);
            cpu->a ^= value;
            flag_set(cpu, FLAG_Z, cpu->a == 0);
            flag_set(cpu, FLAG_N, false);
            flag_set(cpu, FLAG_H, false);
            flag_set(cpu, FLAG_C, false);
            break;
        }

        // XOR A, n8
        // Cycles: 2 | Bytes: 2 | Flags: Z 0 0 0
        case OP_XOR_A_n8: {
            uint8_t value = fetch8(cpu, bus);
            cpu->a ^= value;
            flag_set(cpu, FLAG_Z, cpu->a == 0);
            flag_set(cpu, FLAG_N, false);
            flag_set(cpu, FLAG_H, false);
            flag_set(cpu, FLAG_C, false);
            break;
        }

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
        case OP_RLCA: {
            uint8_t bit7 = (cpu->a >> 7) & 1;
            cpu->a = (cpu->a << 1) | bit7;
            flag_set(cpu, FLAG_Z, false);
            flag_set(cpu, FLAG_N, false);
            flag_set(cpu, FLAG_H, false);
            flag_set(cpu, FLAG_C, bit7);
            break;
        }

        // RRCA
        // Cycles: 1 | Bytes: 1 | Flags: 0 0 0 C
        case OP_RRCA: {
            uint8_t bit0 = cpu->a & 1;
            cpu->a = (cpu->a >> 1) | (bit0 << 7);
            flag_set(cpu, FLAG_Z, false);
            flag_set(cpu, FLAG_N, false);
            flag_set(cpu, FLAG_H, false);
            flag_set(cpu, FLAG_C, bit0);
            break;
        }

        // RLA
        // Cycles: 1 | Bytes: 1 | Flags: 0 0 0 C
        case OP_RLA: {
            uint8_t bit7 = (cpu->a >> 7) & 1;
            uint8_t old_carry = flag_get(cpu, FLAG_C) ? 1 : 0;
            cpu->a = (cpu->a << 1) | old_carry;
            flag_set(cpu, FLAG_Z, false);
            flag_set(cpu, FLAG_N, false);
            flag_set(cpu, FLAG_H, false);
            flag_set(cpu, FLAG_C, bit7);
            break;
        }

        // RRA
        // Cycles: 1 | Bytes: 1 | Flags: 0 0 0 C
        case OP_RRA: {
            uint8_t bit0 = cpu->a & 1;
            uint8_t old_carry = flag_get(cpu, FLAG_C) ? 1 : 0;
            cpu->a = (cpu->a >> 1) | (old_carry << 7);
            flag_set(cpu, FLAG_Z, false);
            flag_set(cpu, FLAG_N, false);
            flag_set(cpu, FLAG_H, false);
            flag_set(cpu, FLAG_C, bit0);
            break;
        }

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
        case OP_JP_NZ_a16: {
            uint16_t addr = fetch16(cpu, bus);
            if (!flag_get(cpu, FLAG_Z)) cpu->pc = addr;
            break;
        }
        case OP_JP_Z_a16: {
            uint16_t addr = fetch16(cpu, bus);
            if (flag_get(cpu, FLAG_Z)) cpu->pc = addr;
            break;
        }
        case OP_JP_NC_a16: {
            uint16_t addr = fetch16(cpu, bus);
            if (!flag_get(cpu, FLAG_C)) cpu->pc = addr;
            break;
        }
        case OP_JP_C_a16: {
            uint16_t addr = fetch16(cpu, bus);
            if (flag_get(cpu, FLAG_C)) cpu->pc = addr;
            break;
        }

        // JP HL
        // Cycles: 1 | Bytes: 1 | Flags: -
        case OP_JP_HL: {
            cpu->pc = cpu->hl;
            break;
        }

        // JR e8
        // Cycles: 3 | Bytes: 2 | Flags: -
        case OP_JR_e8: {
            int8_t e8 = (int8_t)fetch8(cpu, bus);
            cpu->pc += e8;
            break;
        }

        // JR cc, e8
        // Cycles: 3 taken / 2 untaken | Bytes: 2 | Flags: -
        case OP_JR_NZ_e8: {
            int8_t e8 = (int8_t)fetch8(cpu, bus);
            if (!flag_get(cpu, FLAG_Z)) cpu->pc += e8;
            break;
        }
        case OP_JR_Z_e8: {
            int8_t e8 = (int8_t)fetch8(cpu, bus);
            if (flag_get(cpu, FLAG_Z)) cpu->pc += e8;
            break;
        }
        case OP_JR_NC_e8: {
            int8_t e8 = (int8_t)fetch8(cpu, bus);
            if (!flag_get(cpu, FLAG_C)) cpu->pc += e8;
            break;
        }
        case OP_JR_C_e8: {
            int8_t e8 = (int8_t)fetch8(cpu, bus);
            if (flag_get(cpu, FLAG_C)) cpu->pc += e8;
            break;
        }

        // CALL n16
        // Cycles: 6 | Bytes: 3 | Flags: -
        case OP_CALL_a16: {
            uint16_t addr = fetch16(cpu, bus);
            cpu->sp -= 2;
            bus_write(bus, cpu->sp, cpu->pc & 0xFF);
            bus_write(bus, cpu->sp + 1, cpu->pc >> 8);
            cpu->pc = addr;
            break;
        }

        // CALL cc, n16
        // Cycles: 6 taken / 3 untaken | Bytes: 3 | Flags: -
        case OP_CALL_NZ_a16: {
            uint16_t addr = fetch16(cpu, bus);
            if (!flag_get(cpu, FLAG_Z)) {
                cpu->sp -= 2;
                bus_write(bus, cpu->sp, cpu->pc & 0xFF);
                bus_write(bus, cpu->sp + 1, cpu->pc >> 8);
                cpu->pc = addr;
            }
            break;
        }
        case OP_CALL_Z_a16: {
            uint16_t addr = fetch16(cpu, bus);
            if (flag_get(cpu, FLAG_Z)) {
                cpu->sp -= 2;
                bus_write(bus, cpu->sp, cpu->pc & 0xFF);
                bus_write(bus, cpu->sp + 1, cpu->pc >> 8);
                cpu->pc = addr;
            }
            break;
        }
        case OP_CALL_NC_a16: {
            uint16_t addr = fetch16(cpu, bus);
            if (!flag_get(cpu, FLAG_C)) {
                cpu->sp -= 2;
                bus_write(bus, cpu->sp, cpu->pc & 0xFF);
                bus_write(bus, cpu->sp + 1, cpu->pc >> 8);
                cpu->pc = addr;
            }
            break;
        }
        case OP_CALL_C_a16: {
            uint16_t addr = fetch16(cpu, bus);
            if (flag_get(cpu, FLAG_C)) {
                cpu->sp -= 2;
                bus_write(bus, cpu->sp, cpu->pc & 0xFF);
                bus_write(bus, cpu->sp + 1, cpu->pc >> 8);
                cpu->pc = addr;
            }
            break;
        }

        // RET
        // Cycles: 4 | Bytes: 1 | Flags: -
        case OP_RET: {
            uint8_t lo = bus_read(bus, cpu->sp);
            uint8_t hi = bus_read(bus, cpu->sp + 1);
            cpu->sp += 2;
            cpu->pc = (hi << 8) | lo;
            break;
        }

        // RET cc
        // Cycles: 5 taken / 2 untaken | Bytes: 1 | Flags: -
        case OP_RET_NZ: {
            if (!flag_get(cpu, FLAG_Z)) {
                uint8_t lo = bus_read(bus, cpu->sp);
                uint8_t hi = bus_read(bus, cpu->sp + 1);
                cpu->sp += 2;
                cpu->pc = (hi << 8) | lo;
            }
            break;
        }
        case OP_RET_Z: {
            if (flag_get(cpu, FLAG_Z)) {
                uint8_t lo = bus_read(bus, cpu->sp);
                uint8_t hi = bus_read(bus, cpu->sp + 1);
                cpu->sp += 2;
                cpu->pc = (hi << 8) | lo;
            }
            break;
        }
        case OP_RET_NC: {
            if (!flag_get(cpu, FLAG_C)) {
                uint8_t lo = bus_read(bus, cpu->sp);
                uint8_t hi = bus_read(bus, cpu->sp + 1);
                cpu->sp += 2;
                cpu->pc = (hi << 8) | lo;
            }
            break;
        }
        case OP_RET_C: {
            if (flag_get(cpu, FLAG_C)) {
                uint8_t lo = bus_read(bus, cpu->sp);
                uint8_t hi = bus_read(bus, cpu->sp + 1);
                cpu->sp += 2;
                cpu->pc = (hi << 8) | lo;
            }
            break;
        }

        // RETI
        // Cycles: 4 | Bytes: 1 | Flags: -
        case OP_RETI: {
            uint8_t lo = bus_read(bus, cpu->sp);
            uint8_t hi = bus_read(bus, cpu->sp + 1);
            cpu->sp += 2;
            cpu->pc = (hi << 8) | lo;
            cpu->ime = true;
            break;
        }

        // RST vec
        // Cycles: 4 | Bytes: 1 | Flags: -
        case OP_RST_S00:
        case OP_RST_S08:
        case OP_RST_S10:
        case OP_RST_S18:
        case OP_RST_S20:
        case OP_RST_S28:
        case OP_RST_S30:
        case OP_RST_S38: {
            uint16_t vec = opcode & 0x38;
            cpu->sp -= 2;
            bus_write(bus, cpu->sp, cpu->pc & 0xFF);
            bus_write(bus, cpu->sp + 1, cpu->pc >> 8);
            cpu->pc = vec;
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
        // CCF
        // Cycles: 1 | Bytes: 1 | Flags: - 0 0 C
        case OP_CCF: {
            flag_set(cpu, FLAG_N, false);
            flag_set(cpu, FLAG_H, false);
            flag_set(cpu, FLAG_C, !flag_get(cpu, FLAG_C));
            break;
        }

        // SCF
        // Cycles: 1 | Bytes: 1 | Flags: - 0 0 1
        case OP_SCF: {
            flag_set(cpu, FLAG_N, false);
            flag_set(cpu, FLAG_H, false);
            flag_set(cpu, FLAG_C, true);
            break;
        }

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
        case OP_POP_BC: {
            uint8_t lo = bus_read(bus, cpu->sp++);
            uint8_t hi = bus_read(bus, cpu->sp++);
            cpu->bc = (hi << 8) | lo;
            break;
        }
        case OP_POP_DE: {
            uint8_t lo = bus_read(bus, cpu->sp++);
            uint8_t hi = bus_read(bus, cpu->sp++);
            cpu->de = (hi << 8) | lo;
            break;
        }
        case OP_POP_HL: {
            uint8_t lo = bus_read(bus, cpu->sp++);
            uint8_t hi = bus_read(bus, cpu->sp++);
            cpu->hl = (hi << 8) | lo;
            break;
        }
        case OP_POP_AF: {
            uint8_t lo = bus_read(bus, cpu->sp++);
            uint8_t hi = bus_read(bus, cpu->sp++);
            cpu->af = (hi << 8) | (lo & 0xF0);
            break;
        }

        // PUSH r16
        // Cycles: 4 | Bytes: 1 | Flags: -
        case OP_PUSH_BC: {
            cpu->sp -= 2;
            bus_write(bus, cpu->sp, cpu->bc & 0xFF);
            bus_write(bus, cpu->sp + 1, cpu->bc >> 8);
            break;
        }
        case OP_PUSH_DE: {
            cpu->sp -= 2;
            bus_write(bus, cpu->sp, cpu->de & 0xFF);
            bus_write(bus, cpu->sp + 1, cpu->de >> 8);
            break;
        }
        case OP_PUSH_HL: {
            cpu->sp -= 2;
            bus_write(bus, cpu->sp, cpu->hl & 0xFF);
            bus_write(bus, cpu->sp + 1, cpu->hl >> 8);
            break;
        }
        case OP_PUSH_AF: {
            cpu->sp -= 2;
            bus_write(bus, cpu->sp, cpu->af & 0xFF);
            bus_write(bus, cpu->sp + 1, cpu->af >> 8);
            break;
        }

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
        case OP_DI: {
            cpu->ime = false;
            break;
        }

        // EI
        // Cycles: 1 | Bytes: 1 | Flags: -
        case OP_EI: {
            cpu->ime = true;
            break;
        }

        // HALT
        // Cycles: - | Bytes: 1 | Flags: -
        case OP_HALT: {
            cpu->halted = true;
            break;
        }

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
        case OP_DAA: {
            uint16_t a = cpu->a;
            if (!flag_get(cpu, FLAG_N)) {
                if (flag_get(cpu, FLAG_H) || (a & 0x0F) > 9) a += 0x06;
                if (flag_get(cpu, FLAG_C) || a > 0x9F) a += 0x60;
            } else {
                if (flag_get(cpu, FLAG_H)) a = (a - 6) & 0xFF;
                if (flag_get(cpu, FLAG_C)) a -= 0x60;
            }
            flag_set(cpu, FLAG_H, false);
            if ((a & 0x100) == 0x100) flag_set(cpu, FLAG_C, true);
            cpu->a = (uint8_t)a;
            flag_set(cpu, FLAG_Z, cpu->a == 0);
            break;
        }

        // NOP
        // Cycles: 1 | Bytes: 1 | Flags: -
        case OP_NOP: {
            break;
        }

        // STOP
        // Cycles: - | Bytes: 2 | Flags: -
        case OP_STOP_n8: {
            fetch8(cpu, bus);
            break;
        }

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
    int cycles = get_cb_opcode_cycles(cb_opcode);

    uint8_t reg_idx = cb_opcode & 0x07;
    uint8_t bit_pos = (cb_opcode >> 3) & 0x07;
    uint8_t group = (cb_opcode >> 6) & 0x03;

    uint8_t value = get_reg_by_index(cpu, bus, reg_idx);

    if (group == 0x01) {
        // BIT u3, r8 (0x40-0x7F)
        flag_set(cpu, FLAG_Z, (value & (1 << bit_pos)) == 0);
        flag_set(cpu, FLAG_N, false);
        flag_set(cpu, FLAG_H, true);
        return cycles;
    }

    uint8_t result;
    switch (group) {
        case 0x00: {
            // Rotate/shift group (0x00-0x3F)
            switch (bit_pos) {
                case 0x00: { // RLC
                    uint8_t bit7 = (value >> 7) & 1;
                    result = (value << 1) | bit7;
                    flag_set(cpu, FLAG_C, bit7);
                    break;
                }
                case 0x01: { // RRC
                    uint8_t bit0 = value & 1;
                    result = (value >> 1) | (bit0 << 7);
                    flag_set(cpu, FLAG_C, bit0);
                    break;
                }
                case 0x02: { // RL
                    uint8_t bit7 = (value >> 7) & 1;
                    uint8_t old_carry = flag_get(cpu, FLAG_C) ? 1 : 0;
                    result = (value << 1) | old_carry;
                    flag_set(cpu, FLAG_C, bit7);
                    break;
                }
                case 0x03: { // RR
                    uint8_t bit0 = value & 1;
                    uint8_t old_carry = flag_get(cpu, FLAG_C) ? 1 : 0;
                    result = (value >> 1) | (old_carry << 7);
                    flag_set(cpu, FLAG_C, bit0);
                    break;
                }
                case 0x04: { // SLA
                    uint8_t bit7 = (value >> 7) & 1;
                    result = value << 1;
                    flag_set(cpu, FLAG_C, bit7);
                    break;
                }
                case 0x05: { // SRA
                    uint8_t bit7 = value & 0x80;
                    result = (value >> 1) | bit7;
                    flag_set(cpu, FLAG_C, value & 1);
                    break;
                }
                case 0x06: { // SWAP
                    result = ((value & 0x0F) << 4) | ((value & 0xF0) >> 4);
                    flag_set(cpu, FLAG_C, false);
                    break;
                }
                case 0x07: { // SRL
                    result = value >> 1;
                    flag_set(cpu, FLAG_C, value & 1);
                    break;
                }
                default:
                    return 0;
            }
            flag_set(cpu, FLAG_Z, result == 0);
            flag_set(cpu, FLAG_N, false);
            flag_set(cpu, FLAG_H, false);
            set_reg_by_index(cpu, bus, reg_idx, result);
            return cycles;
        }

        case 0x02: {
            // RES u3, r8 (0x80-0xBF)
            result = value & ~(1 << bit_pos);
            set_reg_by_index(cpu, bus, reg_idx, result);
            return cycles;
        }

        case 0x03: {
            // SET u3, r8 (0xC0-0xFF)
            result = value | (1 << bit_pos);
            set_reg_by_index(cpu, bus, reg_idx, result);
            return cycles;
        }

        default:
            return 0;
    }
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
