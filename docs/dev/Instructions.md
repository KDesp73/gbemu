# GBZ80 Instruction Implementation Status

Reference: https://rgbds.gbdev.io/docs/v1.0.2/gbz80.7

## Legend

- `[x]` Implemented and working
- `[~]` Case labeled (skeleton exists, logic not yet implemented)
- `[ ]` Not implemented

## Load Instructions

| Instruction | Status | Notes |
|---|---|---|
| LD r8,r8 | [x] | All register variants via get/set_reg_by_index |
| LD r8,n8 | [x] | All register variants |
| LD r16,n16 | [x] | BC, DE, HL, SP |
| LD [HL],r8 | [x] | All register variants |
| LD [HL],n8 | [x] | |
| LD r8,[HL] | [x] | All register variants |
| LD [r16],A | [x] | BC, DE |
| LD [n16],A | [x] | |
| LDH [n16],A | [x] | LDH [a8],A |
| LDH [C],A | [x] | |
| LD A,[r16] | [x] | BC, DE |
| LD A,[n16] | [x] | |
| LDH A,[n16] | [x] | LDH A,[a8] |
| LDH A,[C] | [x] | |
| LD [HLI],A | [x] | |
| LD [HLD],A | [x] | |
| LD A,[HLI] | [x] | |
| LD A,[HLD] | [x] | |

## 8-bit Arithmetic Instructions

| Instruction | Status | Notes |
|---|---|---|
| ADC A,r8 | [x] | All register variants (B,C,D,E,H,L,[HL],A) |
| ADC A,[HL] | [x] | Handled via r8 encoding |
| ADC A,n8 | [x] | |
| ADD A,r8 | [x] | All register variants |
| ADD A,[HL] | [x] | Handled via r8 encoding |
| ADD A,n8 | [x] | |
| CP A,r8 | [x] | All register variants |
| CP A,[HL] | [x] | Handled via r8 encoding |
| CP A,n8 | [x] | |
| DEC r8 | [x] | All register variants |
| DEC [HL] | [x] | |
| INC r8 | [x] | All register variants |
| INC [HL] | [x] | |
| SBC A,r8 | [x] | All register variants |
| SBC A,[HL] | [x] | Handled via r8 encoding |
| SBC A,n8 | [x] | |
| SUB A,r8 | [x] | All register variants |
| SUB A,[HL] | [x] | Handled via r8 encoding |
| SUB A,n8 | [x] | |

## 16-bit Arithmetic Instructions

| Instruction | Status | Notes |
|---|---|---|
| ADD HL,r16 | [x] | BC, DE, HL, SP |
| DEC r16 | [x] | BC, DE, HL, SP |
| INC r16 | [x] | BC, DE, HL, SP |

## Bitwise Logic Instructions

| Instruction | Status | Notes |
|---|---|---|
| AND A,r8 | [x] | All register variants |
| AND A,[HL] | [x] | Handled via r8 encoding |
| AND A,n8 | [x] | |
| CPL | [x] | |
| OR A,r8 | [x] | All register variants |
| OR A,[HL] | [x] | Handled via r8 encoding |
| OR A,n8 | [x] | |
| XOR A,r8 | [x] | All register variants |
| XOR A,[HL] | [x] | Handled via r8 encoding |
| XOR A,n8 | [x] | |

## Bit Flag Instructions

| Instruction | Status | Notes |
|---|---|---|
| BIT u3,r8 | [x] | All 64 variants (bits 0-7, registers B,C,D,E,H,L,[HL],A) |
| BIT u3,[HL] | [x] | Handled via r8 encoding |
| RES u3,r8 | [x] | All 64 variants |
| RES u3,[HL] | [x] | Handled via r8 encoding |
| SET u3,r8 | [x] | All 64 variants |
| SET u3,[HL] | [x] | Handled via r8 encoding |

## Bit Shift Instructions

| Instruction | Status | Notes |
|---|---|---|
| RL r8 | [x] | CB prefix, all variants |
| RL [HL] | [x] | CB prefix, handled via r8 encoding |
| RLA | [x] | |
| RLC r8 | [x] | CB prefix, all variants |
| RLC [HL] | [x] | CB prefix, handled via r8 encoding |
| RLCA | [x] | |
| RR r8 | [x] | CB prefix, all variants |
| RR [HL] | [x] | CB prefix, handled via r8 encoding |
| RRA | [x] | |
| RRC r8 | [x] | CB prefix, all variants |
| RRC [HL] | [x] | CB prefix, handled via r8 encoding |
| RRCA | [x] | |
| SLA r8 | [x] | CB prefix, all variants |
| SLA [HL] | [x] | CB prefix, handled via r8 encoding |
| SRA r8 | [x] | CB prefix, all variants |
| SRA [HL] | [x] | CB prefix, handled via r8 encoding |
| SRL r8 | [x] | CB prefix, all variants |
| SRL [HL] | [x] | CB prefix, handled via r8 encoding |
| SWAP r8 | [x] | CB prefix, all variants |
| SWAP [HL] | [x] | CB prefix, handled via r8 encoding |

## Jump and Subroutine Instructions

| Instruction | Status | Notes |
|---|---|---|
| CALL n16 | [x] | |
| CALL cc,n16 | [x] | NZ, Z, NC, C |
| JP HL | [x] | |
| JP n16 | [x] | |
| JP cc,n16 | [x] | NZ, Z, NC, C |
| JR n8 | [x] | |
| JR cc,n8 | [x] | NZ, Z, NC, C |
| RET cc | [x] | NZ, Z, NC, C |
| RET | [x] | |
| RETI | [x] | |
| RST vec | [x] | All 8 vectors (0x00-0x38) |

## Carry Flag Instructions

| Instruction | Status | Notes |
|---|---|---|
| CCF | [x] | |
| SCF | [x] | |

## Stack Manipulation Instructions

| Instruction | Status | Notes |
|---|---|---|
| ADD HL,SP | [x] | Handled in 16-bit arithmetic |
| ADD SP,e8 | [x] | |
| DEC SP | [x] | |
| INC SP | [x] | |
| LD SP,n16 | [x] | |
| LD [n16],SP | [x] | |
| LD HL,SP+e8 | [x] | |
| LD SP,HL | [x] | |
| POP AF | [x] | Lower nibble of F masked to 0 |
| POP r16 | [x] | BC, DE, HL |
| PUSH AF | [x] | |
| PUSH r16 | [x] | BC, DE, HL |

## Interrupt-related Instructions

| Instruction | Status | Notes |
|---|---|---|
| DI | [x] | |
| EI | [x] | |
| HALT | [x] | |

## Miscellaneous Instructions

| Instruction | Status | Notes |
|---|---|---|
| DAA | [x] | |
| NOP | [x] | |
| STOP | [x] | Consumes the trailing n8 byte |

---

## Summary

| Category | Implemented | Skeleton | Not Started | Total |
|---|---|---|---|---|
| Load | 18 | 0 | 0 | 18 |
| 8-bit Arithmetic | 19 | 0 | 0 | 19 |
| 16-bit Arithmetic | 3 | 0 | 0 | 3 |
| Bitwise Logic | 10 | 0 | 0 | 10 |
| Bit Flag | 6 | 0 | 0 | 6 |
| Bit Shift | 20 | 0 | 0 | 20 |
| Jump/Subroutine | 11 | 0 | 0 | 11 |
| Carry Flag | 2 | 0 | 0 | 2 |
| Stack Manipulation | 12 | 0 | 0 | 12 |
| Interrupt | 3 | 0 | 0 | 3 |
| Miscellaneous | 3 | 0 | 0 | 3 |
| **Total** | **107** | **0** | **0** | **107** |
