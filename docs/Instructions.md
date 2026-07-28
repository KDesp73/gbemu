# GBZ80 Instruction Implementation Status

Reference: https://rgbds.gbdev.io/docs/v1.0.2/gbz80.7

## Legend

- `[x]` Implemented and working
- `[~]` Case labeled (skeleton exists, logic not yet implemented)
- `[ ]` Not implemented

## Load Instructions

| Instruction | Status | Notes |
|---|---|---|
| LD r8,r8 | [~] | |
| LD r8,n8 | [~] | |
| LD r16,n16 | [~] | |
| LD [HL],r8 | [~] | |
| LD [HL],n8 | [~] | |
| LD r8,[HL] | [~] | |
| LD [r16],A | [~] | |
| LD [n16],A | [~] | |
| LDH [n16],A | [~] | |
| LDH [C],A | [~] | |
| LD A,[r16] | [~] | |
| LD A,[n16] | [~] | |
| LDH A,[n16] | [~] | |
| LDH A,[C] | [~] | |
| LD [HLI],A | [~] | |
| LD [HLD],A | [~] | |
| LD A,[HLI] | [~] | |
| LD A,[HLD] | [~] | |

## 8-bit Arithmetic Instructions

| Instruction | Status | Notes |
|---|---|---|
| ADC A,r8 | [x] | All register variants (B,C,D,E,H,L,[HL],A) |
| ADC A,[HL] | [x] | Handled via r8 encoding |
| ADC A,n8 | [x] | |
| ADD A,r8 | [~] | |
| ADD A,[HL] | [~] | |
| ADD A,n8 | [~] | |
| CP A,r8 | [~] | |
| CP A,[HL] | [~] | |
| CP A,n8 | [~] | |
| DEC r8 | [~] | |
| DEC [HL] | [~] | |
| INC r8 | [~] | Only `INC A` has logic |
| INC [HL] | [~] | |
| SBC A,r8 | [~] | |
| SBC A,[HL] | [~] | |
| SBC A,n8 | [~] | |
| SUB A,r8 | [~] | |
| SUB A,[HL] | [~] | |
| SUB A,n8 | [~] | |

## 16-bit Arithmetic Instructions

| Instruction | Status | Notes |
|---|---|---|
| ADD HL,r16 | [~] | |
| DEC r16 | [~] | |
| INC r16 | [~] | |

## Bitwise Logic Instructions

| Instruction | Status | Notes |
|---|---|---|
| AND A,r8 | [~] | |
| AND A,[HL] | [~] | |
| AND A,n8 | [~] | |
| CPL | [~] | |
| OR A,r8 | [~] | |
| OR A,[HL] | [~] | |
| OR A,n8 | [~] | |
| XOR A,r8 | [~] | |
| XOR A,[HL] | [~] | |
| XOR A,n8 | [~] | |

## Bit Flag Instructions

| Instruction | Status | Notes |
|---|---|---|
| BIT u3,r8 | [x] | All 64 variants (bits 0-7, registers B,C,D,E,H,L,[HL],A) |
| BIT u3,[HL] | [x] | Handled via r8 encoding |
| RES u3,r8 | [ ] | |
| RES u3,[HL] | [ ] | |
| SET u3,r8 | [ ] | |
| SET u3,[HL] | [ ] | |

## Bit Shift Instructions

| Instruction | Status | Notes |
|---|---|---|
| RL r8 | [ ] | |
| RL [HL] | [ ] | |
| RLA | [~] | |
| RLC r8 | [ ] | |
| RLC [HL] | [ ] | |
| RLCA | [~] | |
| RR r8 | [ ] | |
| RR [HL] | [ ] | |
| RRA | [~] | |
| RRC r8 | [ ] | |
| RRC [HL] | [ ] | |
| RRCA | [~] | |
| SLA r8 | [ ] | |
| SLA [HL] | [ ] | |
| SRA r8 | [ ] | |
| SRA [HL] | [ ] | |
| SRL r8 | [ ] | |
| SRL [HL] | [ ] | |
| SWAP r8 | [ ] | |
| SWAP [HL] | [ ] | |

## Jump and Subroutine Instructions

| Instruction | Status | Notes |
|---|---|---|
| CALL n16 | [~] | |
| CALL cc,n16 | [~] | |
| JP HL | [~] | |
| JP n16 | [x] | |
| JP cc,n16 | [~] | |
| JR n16 | [~] | |
| JR cc,n16 | [~] | |
| RET cc | [~] | |
| RET | [~] | |
| RETI | [~] | |
| RST vec | [~] | |

## Carry Flag Instructions

| Instruction | Status | Notes |
|---|---|---|
| CCF | [~] | |
| SCF | [~] | |

## Stack Manipulation Instructions

| Instruction | Status | Notes |
|---|---|---|
| ADD HL,SP | [~] | |
| ADD SP,e8 | [~] | |
| DEC SP | [~] | |
| INC SP | [~] | |
| LD SP,n16 | [~] | |
| LD [n16],SP | [~] | |
| LD HL,SP+e8 | [~] | |
| LD SP,HL | [~] | |
| POP AF | [~] | |
| POP r16 | [~] | |
| PUSH AF | [~] | |
| PUSH r16 | [~] | |

## Interrupt-related Instructions

| Instruction | Status | Notes |
|---|---|---|
| DI | [~] | |
| EI | [~] | |
| HALT | [~] | |

## Miscellaneous Instructions

| Instruction | Status | Notes |
|---|---|---|
| DAA | [~] | |
| NOP | [x] | |
| STOP | [~] | |

---

## Summary

| Category | Implemented | Skeleton | Not Started | Total |
|---|---|---|---|---|
| Load | 0 | 18 | 0 | 18 |
| 8-bit Arithmetic | 3 | 16 | 0 | 19 |
| 16-bit Arithmetic | 0 | 3 | 0 | 3 |
| Bitwise Logic | 0 | 10 | 0 | 10 |
| Bit Flag | 2 | 0 | 4 | 6 |
| Bit Shift | 0 | 4 | 16 | 20 |
| Jump/Subroutine | 1 | 10 | 0 | 11 |
| Carry Flag | 0 | 2 | 0 | 2 |
| Stack Manipulation | 0 | 12 | 0 | 12 |
| Interrupt | 0 | 3 | 0 | 3 |
| Miscellaneous | 1 | 2 | 0 | 3 |
| **Total** | **7** | **80** | **20** | **107** |
