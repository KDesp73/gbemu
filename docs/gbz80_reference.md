# GBZ80 Instruction Reference

Reference: [RGBDS v1.0.2 gbz80(7)](https://rgbds.gbdev.io/docs/v1.0.2/gbz80.7)

All arithmetic and logic instructions that use register A as a destination can omit the destination, since it is assumed to be register A by default. For example, `OR A,B` and `OR B` are equivalent. Similarly, `CPL` can take an optional A destination: `CPL` and `CPL A` are equivalent.

## Legend

| Abbr | Meaning |
|---|---|
| r8 | Any 8-bit register: A, B, C, D, E, H, L |
| r16 | Any general-purpose 16-bit register: BC, DE, HL |
| n8 | 8-bit integer constant (-128 to 255) |
| n16 | 16-bit integer constant (-32768 to 65535) |
| e8 | 8-bit signed offset (-128 to 127) |
| u3 | 3-bit unsigned bit index (0 to 7) |
| cc | Condition code: Z, NZ, C, NC |
| vec | RST vector: 0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38 |

---

## Load Instructions

### LD r8,r8
Copy the value in the source register into the destination register.
- Cycles: 1
- Bytes: 1
- Flags: None affected

### LD r8,n8
Copy the value n8 into register r8.
- Cycles: 2
- Bytes: 2
- Flags: None affected

### LD r16,n16
Copy the value n16 into register r16.
- Cycles: 3
- Bytes: 3
- Flags: None affected

### LD [HL],r8
Copy the value in register r8 into the byte pointed to by HL.
- Cycles: 2
- Bytes: 1
- Flags: None affected

### LD [HL],n8
Copy the value n8 into the byte pointed to by HL.
- Cycles: 3
- Bytes: 2
- Flags: None affected

### LD r8,[HL]
Copy the value pointed to by HL into register r8.
- Cycles: 2
- Bytes: 1
- Flags: None affected

### LD [r16],A
Copy the value in register A into the byte pointed to by r16.
- Cycles: 2
- Bytes: 1
- Flags: None affected

### LD [n16],A
Copy the value in register A into the byte at address n16.
- Cycles: 4
- Bytes: 3
- Flags: None affected

### LDH [n16],A
Copy the value in register A into the byte at address n16. The destination address n16 is encoded as its 8-bit low byte and assumes a high byte of $FF, so it must be between $FF00 and $FFFF.
- Cycles: 3
- Bytes: 2
- Flags: None affected

### LDH [C],A
Copy the value in register A into the byte at address $FF00+C.
- Cycles: 2
- Bytes: 1
- Flags: None affected

### LD A,[r16]
Copy the byte pointed to by r16 into register A.
- Cycles: 2
- Bytes: 1
- Flags: None affected

### LD A,[n16]
Copy the byte at address n16 into register A.
- Cycles: 4
- Bytes: 3
- Flags: None affected

### LDH A,[n16]
Copy the byte at address n16 into register A. The source address n16 is encoded as its 8-bit low byte and assumes a high byte of $FF, so it must be between $FF00 and $FFFF.
- Cycles: 3
- Bytes: 2
- Flags: None affected

### LDH A,[C]
Copy the byte at address $FF00+C into register A.
- Cycles: 2
- Bytes: 1
- Flags: None affected

### LD [HLI],A
Copy the value in register A into the byte pointed by HL and increment HL afterwards.
- Aliases: `LD [HL+],A`, `LDI [HL],A`
- Cycles: 2
- Bytes: 1
- Flags: None affected

### LD [HLD],A
Copy the value in register A into the byte pointed by HL and decrement HL afterwards.
- Aliases: `LD [HL-],A`, `LDD [HL],A`
- Cycles: 2
- Bytes: 1
- Flags: None affected

### LD A,[HLI]
Copy the byte pointed to by HL into register A, and increment HL afterwards.
- Aliases: `LD A,[HL+]`, `LDI A,[HL]`
- Cycles: 2
- Bytes: 1
- Flags: None affected

### LD A,[HLD]
Copy the byte pointed to by HL into register A, and decrement HL afterwards.
- Aliases: `LD A,[HL-]`, `LDD A,[HL]`
- Cycles: 2
- Bytes: 1
- Flags: None affected

---

## 8-bit Arithmetic Instructions

### ADC A,r8
Add the value in r8 plus the carry flag to A.
- Cycles: 1
- Bytes: 1
- Flags:
  - Z: Set if result is 0
  - N: 0
  - H: Set if overflow from bit 3
  - C: Set if overflow from bit 7

### ADC A,[HL]
Add the byte pointed to by HL plus the carry flag to A.
- Cycles: 2
- Bytes: 1
- Flags: See ADC A,r8

### ADC A,n8
Add the value n8 plus the carry flag to A.
- Cycles: 2
- Bytes: 2
- Flags: See ADC A,r8

### ADD A,r8
Add the value in r8 to A.
- Cycles: 1
- Bytes: 1
- Flags:
  - Z: Set if result is 0
  - N: 0
  - H: Set if overflow from bit 3
  - C: Set if overflow from bit 7

### ADD A,[HL]
Add the byte pointed to by HL to A.
- Cycles: 2
- Bytes: 1
- Flags: See ADD A,r8

### ADD A,n8
Add the value n8 to A.
- Cycles: 2
- Bytes: 2
- Flags: See ADD A,r8

### CP A,r8
ComPare the value in A with the value in r8. This subtracts the value in r8 from A and sets flags accordingly, but discards the result.
- Cycles: 1
- Bytes: 1
- Flags:
  - Z: Set if result is 0
  - N: 1
  - H: Set if borrow from bit 4
  - C: Set if borrow (i.e. if r8 > A)

### CP A,[HL]
ComPare the value in A with the byte pointed to by HL.
- Cycles: 2
- Bytes: 1
- Flags: See CP A,r8

### CP A,n8
ComPare the value in A with the value n8.
- Cycles: 2
- Bytes: 2
- Flags: See CP A,r8

### DEC r8
Decrement the value in register r8 by 1.
- Cycles: 1
- Bytes: 1
- Flags:
  - Z: Set if result is 0
  - N: 1
  - H: Set if borrow from bit 4

### DEC [HL]
Decrement the byte pointed to by HL by 1.
- Cycles: 3
- Bytes: 1
- Flags: See DEC r8

### INC r8
Increment the value in register r8 by 1.
- Cycles: 1
- Bytes: 1
- Flags:
  - Z: Set if result is 0
  - N: 0
  - H: Set if overflow from bit 3

### INC [HL]
Increment the byte pointed to by HL by 1.
- Cycles: 3
- Bytes: 1
- Flags: See INC r8

### SBC A,r8
Subtract the value in r8 and the carry flag from A.
- Cycles: 1
- Bytes: 1
- Flags:
  - Z: Set if result is 0
  - N: 1
  - H: Set if borrow from bit 4
  - C: Set if borrow (i.e. if (r8 + carry) > A)

### SBC A,[HL]
Subtract the byte pointed to by HL and the carry flag from A.
- Cycles: 2
- Bytes: 1
- Flags: See SBC A,r8

### SBC A,n8
Subtract the value n8 and the carry flag from A.
- Cycles: 2
- Bytes: 2
- Flags: See SBC A,r8

### SUB A,r8
Subtract the value in r8 from A.
- Cycles: 1
- Bytes: 1
- Flags:
  - Z: Set if result is 0
  - N: 1
  - H: Set if borrow from bit 4
  - C: Set if borrow (i.e. if r8 > A)

### SUB A,[HL]
Subtract the byte pointed to by HL from A.
- Cycles: 2
- Bytes: 1
- Flags: See SUB A,r8

### SUB A,n8
Subtract the value n8 from A.
- Cycles: 2
- Bytes: 2
- Flags: See SUB A,r8

---

## 16-bit Arithmetic Instructions

### ADD HL,r16
Add the value in r16 to HL.
- Cycles: 2
- Bytes: 1
- Flags:
  - N: 0
  - H: Set if overflow from bit 11
  - C: Set if overflow from bit 15

### ADD HL,SP
Add the value in SP to HL.
- Cycles: 2
- Bytes: 1
- Flags: See ADD HL,r16

### ADD SP,e8
Add the signed value e8 to SP.
- Cycles: 4
- Bytes: 2
- Flags:
  - Z: 0
  - N: 0
  - H: Set if overflow from bit 3
  - C: Set if overflow from bit 7

### DEC r16
Decrement the value in register r16 by 1.
- Cycles: 2
- Bytes: 1
- Flags: None affected

### DEC SP
Decrement the value in register SP by 1.
- Cycles: 2
- Bytes: 1
- Flags: None affected

### INC r16
Increment the value in register r16 by 1.
- Cycles: 2
- Bytes: 1
- Flags: None affected

### INC SP
Increment the value in register SP by 1.
- Cycles: 2
- Bytes: 1
- Flags: None affected

---

## Bitwise Logic Instructions

### AND A,r8
Set A to the bitwise AND between the value in r8 and A.
- Cycles: 1
- Bytes: 1
- Flags:
  - Z: Set if result is 0
  - N: 0
  - H: 1
  - C: 0

### AND A,[HL]
Set A to the bitwise AND between the byte pointed to by HL and A.
- Cycles: 2
- Bytes: 1
- Flags: See AND A,r8

### AND A,n8
Set A to the bitwise AND between the value n8 and A.
- Cycles: 2
- Bytes: 2
- Flags: See AND A,r8

### CPL
ComPLement accumulator (A = ~A); also called bitwise NOT.
- Cycles: 1
- Bytes: 1
- Flags:
  - N: 1
  - H: 1

### OR A,r8
Set A to the bitwise OR between the value in r8 and A.
- Cycles: 1
- Bytes: 1
- Flags:
  - Z: Set if result is 0
  - N: 0
  - H: 0
  - C: 0

### OR A,[HL]
Set A to the bitwise OR between the byte pointed to by HL and A.
- Cycles: 2
- Bytes: 1
- Flags: See OR A,r8

### OR A,n8
Set A to the bitwise OR between the value n8 and A.
- Cycles: 2
- Bytes: 2
- Flags: See OR A,r8

### XOR A,r8
Set A to the bitwise XOR between the value in r8 and A.
- Cycles: 1
- Bytes: 1
- Flags:
  - Z: Set if result is 0
  - N: 0
  - H: 0
  - C: 0

### XOR A,[HL]
Set A to the bitwise XOR between the byte pointed to by HL and A.
- Cycles: 2
- Bytes: 1
- Flags: See XOR A,r8

### XOR A,n8
Set A to the bitwise XOR between the value n8 and A.
- Cycles: 2
- Bytes: 2
- Flags: See XOR A,r8

---

## Bit Flag Instructions

### BIT u3,r8
Test bit u3 in register r8, set the zero flag if bit not set.
- Cycles: 2
- Bytes: 2
- Flags:
  - Z: Set if the selected bit is 0
  - N: 0
  - H: 1

### BIT u3,[HL]
Test bit u3 in the byte pointed by HL, set the zero flag if bit not set.
- Cycles: 3
- Bytes: 2
- Flags: See BIT u3,r8

### RES u3,r8
Set bit u3 in register r8 to 0. Bit 0 is the rightmost one, bit 7 the leftmost one.
- Cycles: 2
- Bytes: 2
- Flags: None affected

### RES u3,[HL]
Set bit u3 in the byte pointed by HL to 0.
- Cycles: 4
- Bytes: 2
- Flags: None affected

### SET u3,r8
Set bit u3 in register r8 to 1. Bit 0 is the rightmost one, bit 7 the leftmost one.
- Cycles: 2
- Bytes: 2
- Flags: None affected

### SET u3,[HL]
Set bit u3 in the byte pointed by HL to 1.
- Cycles: 4
- Bytes: 2
- Flags: None affected

---

## Bit Shift Instructions

### RL r8
Rotate bits in register r8 left, through the carry flag.
- Cycles: 2
- Bytes: 2
- Flags:
  - Z: Set if result is 0
  - N: 0
  - H: 0
  - C: Set according to result

### RL [HL]
Rotate the byte pointed to by HL left, through the carry flag.
- Cycles: 4
- Bytes: 2
- Flags: See RL r8

### RLA
Rotate register A left, through the carry flag.
- Cycles: 1
- Bytes: 1
- Flags:
  - Z: 0
  - N: 0
  - H: 0
  - C: Set according to result

### RLC r8
Rotate register r8 left.
- Cycles: 2
- Bytes: 2
- Flags:
  - Z: Set if result is 0
  - N: 0
  - H: 0
  - C: Set according to result

### RLC [HL]
Rotate the byte pointed to by HL left.
- Cycles: 4
- Bytes: 2
- Flags: See RLC r8

### RLCA
Rotate register A left.
- Cycles: 1
- Bytes: 1
- Flags:
  - Z: 0
  - N: 0
  - H: 0
  - C: Set according to result

### RR r8
Rotate register r8 right, through the carry flag.
- Cycles: 2
- Bytes: 2
- Flags:
  - Z: Set if result is 0
  - N: 0
  - H: 0
  - C: Set according to result

### RR [HL]
Rotate the byte pointed to by HL right, through the carry flag.
- Cycles: 4
- Bytes: 2
- Flags: See RR r8

### RRA
Rotate register A right, through the carry flag.
- Cycles: 1
- Bytes: 1
- Flags:
  - Z: 0
  - N: 0
  - H: 0
  - C: Set according to result

### RRC r8
Rotate register r8 right.
- Cycles: 2
- Bytes: 2
- Flags:
  - Z: Set if result is 0
  - N: 0
  - H: 0
  - C: Set according to result

### RRC [HL]
Rotate the byte pointed to by HL right.
- Cycles: 4
- Bytes: 2
- Flags: See RRC r8

### RRCA
Rotate register A right.
- Cycles: 1
- Bytes: 1
- Flags:
  - Z: 0
  - N: 0
  - H: 0
  - C: Set according to result

### SLA r8
Shift Left Arithmetically register r8.
- Cycles: 2
- Bytes: 2
- Flags:
  - Z: Set if result is 0
  - N: 0
  - H: 0
  - C: Set according to result

### SLA [HL]
Shift Left Arithmetically the byte pointed to by HL.
- Cycles: 4
- Bytes: 2
- Flags: See SLA r8

### SRA r8
Shift Right Arithmetically register r8 (bit 7 is unchanged).
- Cycles: 2
- Bytes: 2
- Flags:
  - Z: Set if result is 0
  - N: 0
  - H: 0
  - C: Set according to result

### SRA [HL]
Shift Right Arithmetically the byte pointed to by HL (bit 7 is unchanged).
- Cycles: 4
- Bytes: 2
- Flags: See SRA r8

### SRL r8
Shift Right Logically register r8.
- Cycles: 2
- Bytes: 2
- Flags:
  - Z: Set if result is 0
  - N: 0
  - H: 0
  - C: Set according to result

### SRL [HL]
Shift Right Logically the byte pointed to by HL.
- Cycles: 4
- Bytes: 2
- Flags: See SRL r8

### SWAP r8
Swap the upper 4 bits in register r8 and the lower 4 ones.
- Cycles: 2
- Bytes: 2
- Flags:
  - Z: Set if result is 0
  - N: 0
  - H: 0
  - C: 0

### SWAP [HL]
Swap the upper 4 bits in the byte pointed by HL and the lower 4 ones.
- Cycles: 4
- Bytes: 2
- Flags: See SWAP r8

---

## Jump and Subroutine Instructions

### CALL n16
Call address n16. Pushes the address of the next instruction on the stack, then jumps to n16.
- Cycles: 6
- Bytes: 3
- Flags: None affected

### CALL cc,n16
Call address n16 if condition cc is met.
- Cycles: 6 taken / 3 untaken
- Bytes: 3
- Flags: None affected

### JP n16
Jump to address n16; effectively, copy n16 into PC.
- Cycles: 4
- Bytes: 3
- Flags: None affected

### JP cc,n16
Jump to address n16 if condition cc is met.
- Cycles: 4 taken / 3 untaken
- Bytes: 3
- Flags: None affected

### JP HL
Jump to address in HL; effectively, copy the value in register HL into PC.
- Cycles: 1
- Bytes: 1
- Flags: None affected

### JR n16
Relative Jump to address n16. The target address is encoded as a signed 8-bit offset from the address immediately following the JR instruction, so it must be between -128 and 127 bytes away.
- Cycles: 3
- Bytes: 2
- Flags: None affected

### JR cc,n16
Relative Jump to address n16 if condition cc is met.
- Cycles: 3 taken / 2 untaken
- Bytes: 2
- Flags: None affected

### RET
Return from subroutine. Basically a POP PC.
- Cycles: 4
- Bytes: 1
- Flags: None affected

### RET cc
Return from subroutine if condition cc is met.
- Cycles: 5 taken / 2 untaken
- Bytes: 1
- Flags: None affected

### RETI
Return from subroutine and enable interrupts. Equivalent to executing EI then RET; IME is set right after this instruction.
- Cycles: 4
- Bytes: 1
- Flags: None affected

### RST vec
Call address vec. Shorter and faster equivalent to CALL for suitable values of vec.
- Cycles: 4
- Bytes: 1
- Flags: None affected

---

## Carry Flag Instructions

### CCF
Complement Carry Flag.
- Cycles: 1
- Bytes: 1
- Flags:
  - N: 0
  - H: 0
  - C: Inverted

### SCF
Set Carry Flag.
- Cycles: 1
- Bytes: 1
- Flags:
  - N: 0
  - H: 0
  - C: 1

---

## Stack Manipulation Instructions

### ADD HL,SP
Add the value in SP to HL.
- Cycles: 2
- Bytes: 1
- Flags: See ADD HL,r16

### ADD SP,e8
Add the signed value e8 to SP.
- Cycles: 4
- Bytes: 2
- Flags:
  - Z: 0
  - N: 0
  - H: Set if overflow from bit 3
  - C: Set if overflow from bit 7

### DEC SP
Decrement the value in register SP by 1.
- Cycles: 2
- Bytes: 1
- Flags: None affected

### INC SP
Increment the value in register SP by 1.
- Cycles: 2
- Bytes: 1
- Flags: None affected

### LD SP,n16
Copy the value n16 into register SP.
- Cycles: 3
- Bytes: 3
- Flags: None affected

### LD [n16],SP
Copy SP & $FF at address n16 and SP >> 8 at address n16 + 1.
- Cycles: 5
- Bytes: 3
- Flags: None affected

### LD HL,SP+e8
Add the signed value e8 to SP and copy the result in HL.
- Cycles: 3
- Bytes: 2
- Flags:
  - Z: 0
  - N: 0
  - H: Set if overflow from bit 3
  - C: Set if overflow from bit 7

### LD SP,HL
Copy register HL into register SP.
- Cycles: 2
- Bytes: 1
- Flags: None affected

### POP AF
Pop register AF from the stack.
- Cycles: 3
- Bytes: 1
- Flags:
  - Z: Set from bit 7 of the popped low byte
  - N: Set from bit 6 of the popped low byte
  - H: Set from bit 5 of the popped low byte
  - C: Set from bit 4 of the popped low byte

### POP r16
Pop register r16 from the stack.
- Cycles: 3
- Bytes: 1
- Flags: None affected

### PUSH AF
Push register AF into the stack.
- Cycles: 4
- Bytes: 1
- Flags: None affected

### PUSH r16
Push register r16 into the stack.
- Cycles: 4
- Bytes: 1
- Flags: None affected

---

## Interrupt-related Instructions

### DI
Disable Interrupts by clearing the IME flag.
- Cycles: 1
- Bytes: 1
- Flags: None affected

### EI
Enable Interrupts by setting the IME flag. The flag is only set after the instruction following EI.
- Cycles: 1
- Bytes: 1
- Flags: None affected

### HALT
Enter CPU low-power consumption mode until an interrupt occurs. Behavior depends on IME flag and pending interrupts:
- IME set: CPU enters low-power mode until an interrupt is about to be serviced. Handler executes normally, then CPU resumes after HALT.
- IME not set, no interrupts pending: CPU resumes when an interrupt becomes pending (handler is not called).
- IME not set, interrupt pending: CPU continues after HALT, but the byte after it is read twice (hardware bug).
- Cycles: -
- Bytes: 1
- Flags: None affected

---

## Miscellaneous Instructions

### DAA
Decimal Adjust Accumulator. Designed to be used after arithmetic instructions (ADD, ADC, SUB, SBC) whose inputs were in BCD, adjusting the result to likewise be in BCD.

If the subtract flag N is set:
1. Initialize adjustment to 0
2. If H is set, add $6 to adjustment
3. If C is set, add $60 to adjustment
4. Subtract adjustment from A

If the subtract flag N is not set:
1. Initialize adjustment to 0
2. If H is set or A & $F > $9, add $6 to adjustment
3. If C is set or A > $99, add $60 to adjustment and set carry flag
4. Add adjustment to A

- Cycles: 1
- Bytes: 1
- Flags:
  - Z: Set if result is 0
  - H: 0
  - C: Set or unaffected depending on the operation

### NOP
No OPeration.
- Cycles: 1
- Bytes: 1
- Flags: None affected

### STOP
Enter CPU very low power mode. Also used to switch between GBC double speed and normal speed CPU modes. The exact behavior is fragile and may interpret its second byte as a separate instruction.
- Cycles: -
- Bytes: 2
- Flags: None affected
