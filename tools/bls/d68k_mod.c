/* 68000 disassembler.
 */

#include "bls.h"
#include "blsparse.h"
#include "blsaddress.h"
#include <stdio.h>
#include <setjmp.h>
#include <string.h>
#include <stdlib.h>

typedef uint16_t u16;
typedef int16_t i16;

// Instruction operand type
enum iop {
  //  Bits  Description
  op_none,    //        No operand
  op_ccr,     //        CCR
  op_sr,      //        SR
  op_bra,     // 07..00 Relative branch

  ope_imm,    //        Immediate in extension word
  ope_ims,    //        Signed immediate in extension word
  ope_rl,     //        MOVEM register list (A7-A0/D7-D0)
  ope_rld,    //        MOVEM pre-decrement register list (D0-D7/A0-A7)
  ope_dp,     //        Relative pointer in extension word
  ope_mmovem, //        MOVEM immediate address then register list

  opo_dis8v,  // 07..00 Immediate displacement
  opo_dis8p,  // 07..00 Immediate pointer displacement
  opo_vect,   // 03..00 Trap vector

  op2_imm,    // 11..09 Second operand immediate (1..8)
  op1_Dn,     // 02..00 Dn
  op2_Dn,     // 11..09 Dn
  op1_An,     // 02..00 An
  op2_An,     // 11..09 An
  op1_AnPD,   // 02..00 -(An)
  op2_AnPD,   // 11..09 -(An)
  op1_AnPI,   // 02..00 (An)+
  op2_AnPI,   // 11..09 (An)+
  op1_EA,     // 05..00 Effective address
  op2_EA,     // 11..06 Effective address (reversed mode/register)
  op1e_AnDis  // 02..00 Address register + 16 bits displacement in ext word
};

struct instr {
  char      name[12];
  u16       code;
  u16       mask;
  enum iop  op1; // First operand
  enum iop  op2; // Second operand
  int       size; // Operation size
  int       cycles; // Cycle count
};
typedef struct instr *iptr;


// Matching order is important
struct instr m68k_instr[] = {
#define cc(ipre, ipost, opcode, mask, op1, op2, size, cycles) \
  {ipre "T"  ipost, opcode | 0x0000, mask, op1, op2, size, cycles}, \
  {ipre "F"  ipost, opcode | 0x0100, mask, op1, op2, size, cycles}, \
  {ipre "HI" ipost, opcode | 0x0200, mask, op1, op2, size, cycles}, \
  {ipre "LS" ipost, opcode | 0x0300, mask, op1, op2, size, cycles}, \
  {ipre "CC" ipost, opcode | 0x0400, mask, op1, op2, size, cycles}, \
  {ipre "CS" ipost, opcode | 0x0500, mask, op1, op2, size, cycles}, \
  {ipre "NE" ipost, opcode | 0x0600, mask, op1, op2, size, cycles}, \
  {ipre "EQ" ipost, opcode | 0x0700, mask, op1, op2, size, cycles}, \
  {ipre "VC" ipost, opcode | 0x0800, mask, op1, op2, size, cycles}, \
  {ipre "VS" ipost, opcode | 0x0900, mask, op1, op2, size, cycles}, \
  {ipre "PL" ipost, opcode | 0x0A00, mask, op1, op2, size, cycles}, \
  {ipre "MI" ipost, opcode | 0x0B00, mask, op1, op2, size, cycles}, \
  {ipre "GE" ipost, opcode | 0x0C00, mask, op1, op2, size, cycles}, \
  {ipre "LT" ipost, opcode | 0x0D00, mask, op1, op2, size, cycles}, \
  {ipre "GT" ipost, opcode | 0x0E00, mask, op1, op2, size, cycles}, \
  {ipre "LE" ipost, opcode | 0x0F00, mask, op1, op2, size, cycles}

  {"ADDX.B", 0xD100, 0xF1F8, op1_Dn, op2_Dn, 1, 4},
  {"ADDX.B", 0xD108, 0xF1F8, op1_AnPD, op2_AnPD, 1, 4},
  {"ADDX.W", 0xD140, 0xF1F8, op1_Dn, op2_Dn, 2, 4},
  {"ADDX.W", 0xD148, 0xF1F8, op1_AnPD, op2_AnPD, 2, 4},
  {"ADDX.L", 0xD180, 0xF1F8, op1_Dn, op2_Dn, 4, 8},
  {"ADDX.L", 0xD188, 0xF1F8, op1_AnPD, op2_AnPD, 4, 8},

  {"ABCD", 0xC100, 0xF1F8, op1_Dn, op2_Dn, 1, 6},
  {"ABCD", 0xC108, 0xF1F8, op1_AnPD, op2_AnPD, 1, 4},

  {"SBCD", 0x8100, 0xF1F8, op1_Dn, op2_Dn, 1, 6},
  {"SBCD", 0x8108, 0xF1F8, op1_AnPD, op2_AnPD, 1, 4},

  {"ADD.B", 0xD000, 0xF1C0, op1_EA, op2_Dn, 1, 4},
  {"ADD.B", 0xD100, 0xF1C0, op2_Dn, op1_EA, 1, 4},
  {"ADD.W", 0xD040, 0xF1C0, op1_EA, op2_Dn, 2, 4},
  {"ADD.W", 0xD140, 0xF1C0, op2_Dn, op1_EA, 2, 4},
  {"ADD.L", 0xD080, 0xF1C0, op1_EA, op2_Dn, 4, 8},
  {"ADD.L", 0xD180, 0xF1C0, op2_Dn, op1_EA, 4, 8},

  {"ADDA.W", 0xD0C0, 0xF1C0, op1_EA, op2_An, 2, 8},
  {"ADDA.L", 0xD1C0, 0xF1C0, op1_EA, op2_An, 4, 8},

  {"ADDI.B", 0x0600, 0xFFC0, ope_ims, op1_EA, 1, 4},
  {"ADDI.W", 0x0640, 0xFFC0, ope_ims, op1_EA, 2, 4},
  {"ADDI.L", 0x0680, 0xFFC0, ope_ims, op1_EA, 4, 8},

  {"ADDQ.B", 0x5000, 0xF1C0, op2_imm, op1_EA, 1, 4},
  {"ADDQ.W", 0x5040, 0xF1C0, op2_imm, op1_EA, 2, 4},
  {"ADDQ.L", 0x5080, 0xF1C0, op2_imm, op1_EA, 4, 8},

  {"EXG", 0xC140, 0xF1F8, op2_Dn, op1_Dn, 4, 6},
  {"EXG", 0xC148, 0xF1F8, op2_An, op1_An, 4, 6},
  {"EXG", 0xC188, 0xF1F8, op2_Dn, op1_An, 4, 6},

  {"AND.B", 0xC000, 0xF1C0, op1_EA, op2_Dn, 1, 4},
  {"AND.B", 0xC100, 0xF1C0, op2_Dn, op1_EA, 1, 4},
  {"AND.W", 0xC040, 0xF1C0, op1_EA, op2_Dn, 2, 4},
  {"AND.W", 0xC140, 0xF1C0, op2_Dn, op1_EA, 2, 4},
  {"AND.L", 0xC080, 0xF1C0, op1_EA, op2_Dn, 4, 8},
  {"AND.L", 0xC180, 0xF1C0, op2_Dn, op1_EA, 4, 8},

  {"ANDI", 0x023C, 0xFFFF, ope_imm, op_ccr, 1, 8},
  {"ANDI", 0x027C, 0xFFFF, ope_imm, op_sr, 2, 8},
  {"ANDI.B", 0x0200, 0xFFC0, ope_imm, op1_EA, 1, 4},
  {"ANDI.W", 0x0240, 0xFFC0, ope_imm, op1_EA, 2, 4},
  {"ANDI.L", 0x0280, 0xFFC0, ope_imm, op1_EA, 4, 8},

  {"ASR.B", 0xE000, 0xF1F8, op2_imm, op1_Dn, 1, -1},
  {"ASR.B", 0xE020, 0xF1F8, op2_Dn, op1_Dn, 1, -1},
  {"ASR.W", 0xE040, 0xF1F8, op2_imm, op1_Dn, 2, -1},
  {"ASR.W", 0xE060, 0xF1F8, op2_Dn, op1_Dn, 2, -1},
  {"ASR.L", 0xE080, 0xF1F8, op2_imm, op1_Dn, 4, -1},
  {"ASR.L", 0xE0A0, 0xF1F8, op2_Dn, op1_Dn, 4, -1},
  {"ASR.B", 0xE0C0, 0xFFC0, op1_EA, op_none, 1, 4},

  {"ASL.B", 0xE100, 0xF1F8, op2_imm, op1_Dn, 1, -1},
  {"ASL.B", 0xE120, 0xF1F8, op2_Dn, op1_Dn, 1, -1},
  {"ASL.W", 0xE140, 0xF1F8, op2_imm, op1_Dn, 2, -1},
  {"ASL.W", 0xE160, 0xF1F8, op2_Dn, op1_Dn, 2, -1},
  {"ASL.L", 0xE180, 0xF1F8, op2_imm, op1_Dn, 4, -1},
  {"ASL.L", 0xE1A0, 0xF1F8, op2_Dn, op1_Dn, 4, -1},
  {"ASL.B", 0xE1C0, 0xFFC0, op1_EA, op_none, 1, 4},

  {"BRA.W", 0x6000, 0xFFFF, op_bra, op_none, 2, 10},
  {"BRA.B", 0x6000, 0xFF00, op_bra, op_none, 2, 10},
  {"BSR.W", 0x6100, 0xFFFF, op_bra, op_none, 2, 18},
  {"BSR.B", 0x6100, 0xFF00, op_bra, op_none, 2, 18},
  cc("B", ".W", 0x6000, 0xFFFF, op_bra, op_none, 2, -1),
  cc("B", ".B", 0x6000, 0xFF00, op_bra, op_none, 1, -1),

  {"MOVEP.W", 0x0108, 0xF1F8, op1e_AnDis, op2_Dn, 2, 8},
  {"MOVEP.L", 0x0148, 0xF1F8, op1e_AnDis, op2_Dn, 4, 8},
  {"MOVEP.W", 0x0188, 0xF1F8, op2_Dn, op1e_AnDis, 2, 8},
  {"MOVEP.L", 0x01C8, 0xF1F8, op2_Dn, op1e_AnDis, 4, 8},

  {"BCHG.L", 0x0140, 0xF1C0, op2_Dn, op1_EA, 4, 8},
  {"BCHG.B", 0x0840, 0xFFC0, ope_ims, op1_EA, 1, 4},
  {"BCLR.L", 0x0180, 0xF1C0, op2_Dn, op1_EA, 4, 10},
  {"BCLR.B", 0x0880, 0xFFC0, ope_ims, op1_EA, 1, 4},
  {"BSET.L", 0x01C0, 0xF1C0, op2_Dn, op1_EA, 4, 8},
  {"BSET.B", 0x08C0, 0xFFC0, ope_ims, op1_EA, 1, 4},
  {"BTST.L", 0x0100, 0xF1C0, op2_Dn, op1_EA, 4, 6},
  {"BTST.B", 0x0800, 0xFFC0, ope_ims, op1_EA, 1, 4},

  {"CHK.W", 0x4180, 0xF1C0, op1_EA, op2_Dn, 2, 10},

  {"CLR.B", 0x4200, 0xFFC0, op1_EA, op_none, 1, 4},
  {"CLR.W", 0x4240, 0xFFC0, op1_EA, op_none, 2, 4},
  {"CLR.L", 0x4280, 0xFFC0, op1_EA, op_none, 4, 6},

  {"CMP.B", 0xB000, 0xF1C0, op1_EA, op2_Dn, 1, 4},
  {"CMP.W", 0xB040, 0xF1C0, op1_EA, op2_Dn, 2, 4},
  {"CMP.L", 0xB080, 0xF1C0, op1_EA, op2_Dn, 4, 6},

  {"CMPA.W", 0xB0C0, 0xF1C0, op1_EA, op2_An, 2, 6},
  {"CMPA.L", 0xB1C0, 0xF1C0, op1_EA, op2_An, 4, 6},

  {"CMPI.B", 0x0C00, 0xFFC0, ope_ims, op1_EA, 1, 4},
  {"CMPI.W", 0x0C40, 0xFFC0, ope_ims, op1_EA, 2, 4},
  {"CMPI.L", 0x0C80, 0xFFC0, ope_ims, op1_EA, 4, 6},

  {"CMPM.B", 0xB108, 0xF1F8, op1_AnPI, op2_AnPI, 1, 0},
  {"CMPM.W", 0xB148, 0xF1F8, op1_AnPI, op2_AnPI, 2, 0},
  {"CMPM.L", 0xB188, 0xF1F8, op1_AnPI, op2_AnPI, 4, 0},

  {"DBRA", 0x51C8, 0xFFF8, op1_Dn, ope_dp, 2, 10},
  cc("DB", "", 0x50C8, 0xFFF8, op1_Dn, ope_dp, 2, -1),

  {"DIVS", 0x81C0, 0xF1C0, op1_EA, op2_Dn, 2, -1},
  {"DIVU", 0x80C0, 0xF1C0, op1_EA, op2_Dn, 2, -1},
  {"MULS", 0xC1C0, 0xF1C0, op1_EA, op2_Dn, 2, -1},
  {"MULU", 0xC0C0, 0xF1C0, op1_EA, op2_Dn, 2, -1},

  {"EOR.B", 0xB100, 0xF1C0, op2_Dn, op1_EA, 1, 4},
  {"EOR.W", 0xB140, 0xF1C0, op2_Dn, op1_EA, 2, 4},
  {"EOR.L", 0xB180, 0xF1C0, op2_Dn, op1_EA, 4, 8},

  {"EORI", 0x0A3C, 0xFFFF, ope_imm, op_ccr, 1, 8},
  {"EORI.B", 0x0A00, 0xFFC0, ope_imm, op1_EA, 1, 4},
  {"EORI.W", 0x0A40, 0xFFC0, ope_imm, op1_EA, 2, 4},
  {"EORI.L", 0x0A80, 0xFFC0, ope_imm, op1_EA, 4, 8},

  {"EXT.W", 0x4880, 0xFFF8, op1_Dn, op_none, 2, 4},
  {"EXT.L", 0x48C0, 0xFFF8, op1_Dn, op_none, 4, 4},

  {"ILLEGAL", 0x4AFC, 0xFFFF, op_none, op_none, 0, 4},

  {"JMP", 0x4EC0, 0xFFC0, op1_EA, op_none, 4, 0},
  {"JSR", 0x4E80, 0xFFC0, op1_EA, op_none, 4, 8},

  {"SWAP", 0x4840, 0xFFF8, op1_Dn, op_none, 2, 4},

  {"PEA", 0x4840, 0xFFC0, op1_EA, op_none, 4, 4},
  {"LEA", 0x41C0, 0xF1C0, op1_EA, op2_An, 4, -4},

  {"LINK", 0x4E50, 0xFFF8, op1_An, ope_ims, 2, 8},

  {"LSR.B", 0xE008, 0xF1F8, op2_imm, op1_Dn, 1, -1},
  {"LSR.B", 0xE028, 0xF1F8, op2_Dn, op1_Dn, 1, -1},
  {"LSR.W", 0xE048, 0xF1F8, op2_imm, op1_Dn, 2, -1},
  {"LSR.W", 0xE068, 0xF1F8, op2_Dn, op1_Dn, 2, -1},
  {"LSR.L", 0xE088, 0xF1F8, op2_imm, op1_Dn, 4, -1},
  {"LSR.L", 0xE0A8, 0xF1F8, op2_Dn, op1_Dn, 4, -1},
  {"LSR.B", 0xE2C0, 0xFFC0, op1_EA, op_none, 1, -1},

  {"LSL.B", 0xE108, 0xF1F8, op2_imm, op1_Dn, 1, -1},
  {"LSL.B", 0xE128, 0xF1F8, op2_Dn, op1_Dn, 1, -1},
  {"LSL.W", 0xE148, 0xF1F8, op2_imm, op1_Dn, 2, -1},
  {"LSL.W", 0xE168, 0xF1F8, op2_Dn, op1_Dn, 2, -1},
  {"LSL.L", 0xE188, 0xF1F8, op2_imm, op1_Dn, 4, -1},
  {"LSL.L", 0xE1A8, 0xF1F8, op2_Dn, op1_Dn, 4, -1},
  {"LSL.B", 0xE3C0, 0xFFC0, op1_EA, op_none, 1, -1},

  {"ROR.B", 0xE018, 0xF1F8, op2_imm, op1_Dn, 1, -1},
  {"ROR.B", 0xE038, 0xF1F8, op2_Dn, op1_Dn, 1, -1},
  {"ROR.W", 0xE058, 0xF1F8, op2_imm, op1_Dn, 2, -1},
  {"ROR.W", 0xE078, 0xF1F8, op2_Dn, op1_Dn, 2, -1},
  {"ROR.L", 0xE098, 0xF1F8, op2_imm, op1_Dn, 4, -1},
  {"ROR.L", 0xE0B8, 0xF1F8, op2_Dn, op1_Dn, 4, -1},
  {"ROR.B", 0xE6C0, 0xFFC0, op1_EA, op_none, 1, -1},

  {"ROL.B", 0xE118, 0xF1F8, op2_imm, op1_Dn, 1, -1},
  {"ROL.B", 0xE138, 0xF1F8, op2_Dn, op1_Dn, 1, -1},
  {"ROL.W", 0xE158, 0xF1F8, op2_imm, op1_Dn, 2, -1},
  {"ROL.W", 0xE178, 0xF1F8, op2_Dn, op1_Dn, 2, -1},
  {"ROL.L", 0xE198, 0xF1F8, op2_imm, op1_Dn, 4, -1},
  {"ROL.L", 0xE1B8, 0xF1F8, op2_Dn, op1_Dn, 4, -1},
  {"ROL.B", 0xE7C0, 0xFFC0, op1_EA, op_none, 1, -1},

  {"ROXR.B", 0xE010, 0xF1F8, op2_imm, op1_Dn, 1, -1},
  {"ROXR.B", 0xE030, 0xF1F8, op2_Dn, op1_Dn, 1, -1},
  {"ROXR.W", 0xE050, 0xF1F8, op2_imm, op1_Dn, 2, -1},
  {"ROXR.W", 0xE070, 0xF1F8, op2_Dn, op1_Dn, 2, -1},
  {"ROXR.L", 0xE090, 0xF1F8, op2_imm, op1_Dn, 4, -1},
  {"ROXR.L", 0xE0B0, 0xF1F8, op2_Dn, op1_Dn, 4, -1},
  {"ROXR.B", 0xE4C0, 0xFFC0, op1_EA, op_none, 1, -1},

  {"ROXL.B", 0xE110, 0xF1F8, op2_imm, op1_Dn, 1, -1},
  {"ROXL.B", 0xE130, 0xF1F8, op2_Dn, op1_Dn, 1, -1},
  {"ROXL.W", 0xE150, 0xF1F8, op2_imm, op1_Dn, 2, -1},
  {"ROXL.W", 0xE170, 0xF1F8, op2_Dn, op1_Dn, 2, -1},
  {"ROXL.L", 0xE190, 0xF1F8, op2_imm, op1_Dn, 4, -1},
  {"ROXL.L", 0xE1B0, 0xF1F8, op2_Dn, op1_Dn, 4, -1},
  {"ROXL.B", 0xE5C0, 0xFFC0, op1_EA, op_none, 1, -1},

  cc("S", "", 0x50C0, 0xFFC0, op1_EA, op_none, 1, -1),

  {"MOVEA.W", 0x3040, 0xF1C0, op1_EA, op2_An, 2, 4},
  {"MOVEA.L", 0x2040, 0xF1C0, op1_EA, op2_An, 2, 4},

  {"MOVE", 0x42C0, 0xFFC0, op_ccr, op1_EA, 2, 12},
  {"MOVE", 0x44C0, 0xFFC0, op1_EA, op_ccr, 2, 12},
  {"MOVE", 0x46C0, 0xFFC0, op1_EA, op_sr, 2, 12},
  {"MOVE", 0x40C0, 0xFFC0, op_sr, op1_EA, 2, 6},

  // Pre-decrement modes
  {"MOVEM.W", 0x48A0, 0xFFF8, ope_rld, op1_EA, 2, 2},
  {"MOVEM.L", 0x48E0, 0xFFF8, ope_rld, op1_EA, 4, -2},
  // Other MOVEM modes
  {"MOVEM.W", 0x4CB8, 0xFFFE, ope_mmovem, op_none, 2, 8},
  {"MOVEM.L", 0x4CF8, 0xFFFE, ope_mmovem, op_none, 4, 4},
  {"MOVEM.W", 0x4880, 0xFFC0, ope_rl, op1_EA, 2, 4},
  {"MOVEM.L", 0x48C0, 0xFFC0, ope_rl, op1_EA, 4, 0},
  {"MOVEM.W", 0x4C80, 0xFFC0, op1_EA, ope_rl, 2, 8},
  {"MOVEM.L", 0x4CC0, 0xFFC0, op1_EA, ope_rl, 4, 8},

  {"MOVEQ", 0x7000, 0xF100, opo_dis8v, op2_Dn, 4, 4},

  {"NBCD", 0x4800, 0xFFC0, op1_EA, op_none, 1, 6},

  {"NEG.B", 0x4400, 0xFFC0, op1_EA, op_none, 1, 4},
  {"NEG.W", 0x4440, 0xFFC0, op1_EA, op_none, 2, 4},
  {"NEG.L", 0x4480, 0xFFC0, op1_EA, op_none, 4, 6},

  {"NEGX.B", 0x4000, 0xFFC0, op1_EA, op_none, 1, 8},
  {"NEGX.W", 0x4040, 0xFFC0, op1_EA, op_none, 2, 8},
  {"NEGX.L", 0x4080, 0xFFC0, op1_EA, op_none, 4, 12},

  {"NOP", 0x4E71, 0xFFFF, op_none, op_none, 0, 4},

  {"NOT.B", 0x4600, 0xFFC0, op1_EA, op_none, 1, 4},
  {"NOT.W", 0x4640, 0xFFC0, op1_EA, op_none, 2, 4},
  {"NOT.L", 0x4680, 0xFFC0, op1_EA, op_none, 4, 6},

  {"OR.B", 0x8000, 0xF1C0, op1_EA, op2_Dn, 1, 4},
  {"OR.B", 0x8100, 0xF1C0, op2_Dn, op1_EA, 1, 4},
  {"OR.W", 0x8040, 0xF1C0, op1_EA, op2_Dn, 2, 4},
  {"OR.W", 0x8140, 0xF1C0, op2_Dn, op1_EA, 2, 4},
  {"OR.L", 0x8080, 0xF1C0, op1_EA, op2_Dn, 4, 8},
  {"OR.L", 0x8180, 0xF1C0, op2_Dn, op1_EA, 4, 8},

  {"ORI", 0x003C, 0xFFFF, ope_imm, op_ccr, 1, 8},
  {"ORI", 0x007C, 0xFFFF, ope_imm, op_sr, 2, 8},
  {"ORI.B", 0x0000, 0xFFC0, ope_imm, op1_EA, 1, 4},
  {"ORI.W", 0x0040, 0xFFC0, ope_imm, op1_EA, 2, 4},
  {"ORI.L", 0x0080, 0xFFC0, ope_imm, op1_EA, 4, 8},

  {"RESET", 0x4E70, 0xFFFF, op_none, op_none, 0, 132},
  {"RTE", 0x4E73, 0xFFFF, op_none, op_none, 0, 20},
  {"RTR", 0x4E77, 0xFFFF, op_none, op_none, 0, 20},
  {"RTS", 0x4E75, 0xFFFF, op_none, op_none, 0, 16},

  {"STOP", 0x4E72, 0xFFFF, ope_imm, op_none, 2, 4},

  {"SUBX.B", 0x9100, 0xF1F8, op1_Dn, op2_Dn, 1, 4},
  {"SUBX.B", 0x9108, 0xF1F8, op1_AnPD, op2_AnPD, 1, 6},
  {"SUBX.W", 0x9140, 0xF1F8, op1_Dn, op2_Dn, 2, 4},
  {"SUBX.W", 0x9148, 0xF1F8, op1_AnPD, op2_AnPD, 2, 6},
  {"SUBX.L", 0x9180, 0xF1F8, op1_Dn, op2_Dn, 4, 8},
  {"SUBX.L", 0x9188, 0xF1F8, op1_AnPD, op2_AnPD, 4, 10},

  {"SUB.B", 0x9000, 0xF1C0, op1_EA, op2_Dn, 1, 4},
  {"SUB.B", 0x9100, 0xF1C0, op2_Dn, op1_EA, 1, 4},
  {"SUB.W", 0x9040, 0xF1C0, op1_EA, op2_Dn, 2, 4},
  {"SUB.W", 0x9140, 0xF1C0, op2_Dn, op1_EA, 2, 4},
  {"SUB.L", 0x9080, 0xF1C0, op1_EA, op2_Dn, 4, 8},
  {"SUB.L", 0x9180, 0xF1C0, op2_Dn, op1_EA, 4, 8},

  {"SUBA.W", 0x90C0, 0xF1C0, op1_EA, op2_An, 2, 8},
  {"SUBA.L", 0x91C0, 0xF1C0, op1_EA, op2_An, 4, 8},

  {"SUBI.B", 0x0400, 0xFFC0, ope_ims, op1_EA, 1, 4},
  {"SUBI.W", 0x0440, 0xFFC0, ope_ims, op1_EA, 2, 4},
  {"SUBI.L", 0x0480, 0xFFC0, ope_ims, op1_EA, 4, 8},

  {"SUBQ.B", 0x5100, 0xF1C0, op2_imm, op1_EA, 1, 4},
  {"SUBQ.W", 0x5140, 0xF1C0, op2_imm, op1_EA, 2, 4},
  {"SUBQ.L", 0x5180, 0xF1C0, op2_imm, op1_EA, 4, 8},

  {"TAS", 0x4AC0, 0xFFC0, op1_EA, op_none, 1, 4},

  {"TRAP", 0x4E40, 0xFFF0, opo_vect, op_none, 0, 38},
  {"TRAPV", 0x4E76, 0xFFFF, op_none, op_none, 0, 4},

  {"TST.B", 0x4A00, 0xFFC0, op1_EA, op_none, 1, 4},
  {"TST.W", 0x4A40, 0xFFC0, op1_EA, op_none, 2, 4},
  {"TST.L", 0x4A80, 0xFFC0, op1_EA, op_none, 4, 4},

  {"MOVE.B", 0x1000, 0x3000, op1_EA, op2_EA, 1, 4},
  {"MOVE.W", 0x3000, 0x3000, op1_EA, op2_EA, 2, 4},
  {"MOVE.L", 0x2000, 0x3000, op1_EA, op2_EA, 4, 8},

  {"UNLK", 0x4E58, 0xFFF8, op1_An, op_none, 0, 12},
#undef cc
};

static char *targetdata;
static int tsize;
static const u8 *code;
static int size;
static int instructions;
static u32 address;
static int labels;
static int showcycles;
static int *suspicious;
static int cycles;
static int cyclesmax;
static int lsize;
static bankconfig banks;

static u32 startaddr;
static u32 endaddr;
static u32 opcode;
static u32 baseaddr;
static iptr op;

static jmp_buf error_jmp;

// Errors returned by longjmp
const char *d68k_error(int64_t r)
{
  if(r >= 0) {
    return "Disassembly successful";
  }

  switch(r) {
  case -1 : return "Fetched past the end of code";

  case -2 : return "Not enough space to store disassembled string";
  }

  return "Invalid error code";
}

u32 fetch(int bytes)
{
  if(size < bytes) {
    longjmp(error_jmp, 1);
  }

  u32 mask = 0xFFFFFFFF;

  if(bytes == 1) {
    bytes = 2;
    mask = 0xFF;
  }

  u32 v = getint(code, bytes);
  code += bytes;
  size -= bytes;
  address += bytes;
  return v & mask;
}

#define TPRINTF(...) do { int TPRINTF_sz;\
  if(tsize) TPRINTF_sz = snprintf(targetdata, tsize, __VA_ARGS__); else longjmp(error_jmp, 2); \
  tsize -= TPRINTF_sz; \
  lsize += TPRINTF_sz; \
  targetdata += TPRINTF_sz; \
  } while(0)

#define TALIGN(sz) do { \
  if(tsize) *(targetdata++) = ' '; else longjmp(error_jmp, 2); \
  --tsize; \
  ++lsize; \
  } while(lsize < (sz))


typedef struct dsym {
  struct dsym *next;
  char name[256];
  chipaddr val;
} *dsymptr;

dsymptr dsymtable = 0;

void d68k_freesymbols()
{
  dsymptr s;
  dsymptr n;

  for(s = dsymtable; s; s = n) {
    n = s->next;
    free(s);
  }
}

void setdsym(const char *name, chipaddr val)
{
  dsymptr s;

  for(s = dsymtable; s; s = s->next) {
    if(strcmp(name, s->name) == 0) {
      strcpy(s->name, name);
      s->val = val;
      return;
    }
  }

  s = malloc(sizeof(*s));
  strcpy(s->name, name);
  s->val = val;
  s->next = dsymtable;
  dsymtable = s;
}

const char *getdsymat(u32 val)
{
  bankconfig bc = banks;
  dsymptr s;

  for(s = dsymtable; s; s = s->next) {
    sv addr = chip2bank(s->val, &bc);

    if(addr == val) {
      if(s->val.chip != chip_none) {
        // Update current bank
        banks.bus = bc.bus;
        banks.bank[s->val.chip] = bc.bank[s->val.chip];
      }

      return s->name;
    }
  }

  return 0;
}

void d68k_readsymbols(const char *filename)
{
  FILE *f = fopen(filename, "r");

  if(!f) {
    filename = "build_blsgen/blsgen.md";
    f = fopen(filename, "r");
  }

  if(!f) {
    filename = "blsgen.md";
    f = fopen(filename, "r");
  }

  if(!f) {
    return;
  }

  d68k_freesymbols();

  char line[4096] = "";

  while(!feof(f) && strcmp(line, "Symbols\n")) {
    fgets(line, 4096, f);
  }

  if(!feof(f)) {
    fgets(line, 4096, f);

    if(!feof(f) && !strcmp(line, "=======\n")) {
      fgets(line, 4096, f);

      if(!feof(f) && !strcmp(line, "\n")) {
        while(!feof(f)) {
          fgets(line, 4096, f);
          int slen = strnlen(line, 4096);

          if(slen < 30) {
            break;
          }

          line[slen - 1] = '\0'; // erase trailing \n
          const char *chip = &line[5];
          skipblanks(&chip);
          line[9] = '\0';
          chipaddr value = {chip_parse(chip), parse_int(&line[10])};
          setdsym(&line[30], value);
        }
      }
    }
  }

  fclose(f);
}

void print_label(u32 addr)
{
  const char *name = getdsymat(addr);

  if(name) {
    TPRINTF("%s", name);
    return;
  }

  char n[8];
  snprintf(n, 8, "a%06X", addr);
  TPRINTF("%s", n);
  chipaddr ca = bank2chip(&banks, addr);
  setdsym(n, ca);
}

void print_pc_offset(int offset)
{
  u32 a = baseaddr + offset;

  if(labels && ((a >= startaddr && a < endaddr) || getdsymat(a))) {
    print_label(a);
  } else {
    TPRINTF("$%06X", a);
  }
}

void print_rl(u32 list)
{
  if(!list) {
    ++*suspicious;
    TPRINTF("none");
    return;
  }

  int fr = 1; // First register
  int ls = 0; // Last set
  int sc = 0; // Set count
  char rt; // Register type
  int rn; // Register number

  for(rt = 'D'; rt >= 'A'; rt -= 'D' - 'A') {
    for(rn = 0; rn <= 7; ++rn) {
      int r = list & 1;

      if(r && !ls) {
        ls = 1;
        sc = 1;

        if(fr) {
          fr = 0;
        } else {
          TPRINTF("/");
        }

        TPRINTF("%c%d", rt, rn);
      } else if(r && ls) {
        ++sc;
      } else if(!r && ls) {
        ls = 0;

        if(sc >= 2) {
          TPRINTF("-%c%d", rt, rn - 1);
        }
      }

      list >>= 1;
    }

    if(ls) {
      ls = 0;

      if(sc >= 2) {
        TPRINTF("-%c%d", rt, rn - 1);
      }
    }
  }
}

void print_ea(int mode, int reg)
{
  switch(mode) {
  case 0: { /* Dn */
    TPRINTF("D%d", (reg));
    cycles += 0;
    break;
  }

  case 1: { /* An */
    TPRINTF("A%d", (reg));
    cycles += 0;
    break;
  }

  case 2: { /* (An) */
    TPRINTF("(A%d)", (reg));
    cycles += op->size == 4 ? 8 : 4;
    break;
  }

  case 3: { /* (An)+ */
    TPRINTF("(A%d)+", (reg));
    cycles += op->size == 4 ? 8 : 4;
    break;
  }

  case 4: { /* -(An) */
    TPRINTF("-(A%d)", (reg));
    cycles += op->size == 4 ? 10 : 6;
    break;
  }

  case 5: { /* d16(An) */
    int offset = (i16)fetch(2);
    TPRINTF("%d(A%d)", offset, (reg));
    cycles += op->size == 4 ? 12 : 8;
    break;
  }

  case 6: { /* d8(An,Xn) */
    u32 flags = fetch(2);
    int offset = (int)(char)(flags & 0xFF);
    flags >>= 8;
    TPRINTF("%d(A%d,%c%d.%c)", offset, (reg), (flags & 0x80) ? 'A':'D', (flags >> 4) & 7, (flags & 0x08) ? 'L' : 'W');

    if(flags & 0x07) {
      TPRINTF("\t; Illegal");
      ++*suspicious;
    }

    cycles += op->size == 4 ? 14 : 10;
    break;
  }

  case 7: { /* Other modes */
    switch(reg) {
    case 0: { /* xxx.W */
      u32 addr = fetch(2); if(addr >= 0x8000) {
        addr |= 0xFF0000;
      }

      if(labels && addr >= startaddr && addr < endaddr) {
        print_label(addr);
      } else if(labels && (op->code & 0xFF00) == 0x4E && getdsymat(addr)) {
        // JMP to a known target
        print_label(addr);
      } else {
        TPRINTF("$%06X", addr);
      }

      TPRINTF(".W");
      cycles += op->size == 4 ? 12 : 8;
      break;
    }

    case 1: { /* xxx.L */
      u32 addr = fetch(4);

      if(labels && addr >= startaddr && addr < endaddr) {
        print_label(addr);
      } else if(labels && (op->code & 0xFF00) == 0x4E && getdsymat(addr)) {
        // JMP to a known target
        print_label(addr);
      } else {
        TPRINTF("$%06X", addr);
      }

      TPRINTF(".L");
      cycles += op->size == 4 ? 16 : 12;
      break;
    }

    case 2: { /* d16(PC) */
      int offset = (int)(i16)fetch(2);
      print_pc_offset(offset);
      TPRINTF("(PC)");
      cycles += op->size == 4 ? 12 : 8;
      break;
    }

    case 3: { /* d8(PC,Xn) */
      u32 flags = fetch(2);
      int offset = (int)(char)(flags & 0xFF);
      flags >>= 8;
      TPRINTF("%d(PC,%c%d.%c)", offset, (flags & 0x80) ? 'A':'D', (flags >> 4) & 7, (flags & 0x08) ? 'L' : 'W');

      if(flags & 0x07) {
        TPRINTF(" ; Illegal");
        ++*suspicious;
      }

      cycles += op->size == 4 ? 14 : 10;
      break;
    }

    case 4: { /* #xxx */
      u32 data = fetch(op->size);
      TPRINTF("#$%0*X", op->size * 2, data);
      cycles += op->size == 4 ? 8 : 4;
      break;
    }
    }

    break;
  }
  }
}

void print_operand(enum iop ot)
{
  u32 u;
  int s;

  switch(ot) {
  case op_none: break;

  case op_ccr:
    TPRINTF("CCR");
    cycles += 8;
    break;

  case op_sr:
    TPRINTF("SR");
    cycles += 8;
    break;

  case ope_imm:
    u = fetch(op->size);
    TPRINTF("#$%0*X", op->size * 2, u);
    cycles += 0;
    break;

  case ope_ims:
    s = fetch(op->size);
    signext(&s, op->size * 8);
    TPRINTF("#%d", s);
    cycles += op->size == 4 ? 8 : 4;
    break;

  case ope_rl:
    u = fetch(2);
    print_rl(u);
    {
      u32 bit;

      for(bit = 0; bit < 16; ++bit) {
        if(u & (1 << bit)) {
          cycles += (op->size == 4 ? 8 : 4);
        }
      }
    }
    break;

  case ope_rld:
    u = fetch(2);
    {
      // Inverse bits and display normally
      u32 inverse = 0;
      int i = 0;

      for(i = 0; i < 16; ++i) {
        inverse <<= 1;
        inverse |= u & 1;
        u >>= 1;
      }

      print_rl(inverse);

      u32 bit;

      for(bit = 0; bit < 16; ++bit) {
        if(inverse & (1 << bit)) {
          cycles += (op->size == 4 ? 8 : 4);
        }
      }
    }
    break;

  case ope_mmovem:
    u = fetch(2);
    {
      u32 bit;

      for(bit = 0; bit < 16; ++bit) {
        if(u & (1 << bit)) {
          cycles += (op->size == 4 ? 8 : 4);
        }
      }
    }
    print_ea((opcode >> 3) & 7, opcode & 7);
    TPRINTF(", ");
    print_rl(u);
    break;

  case op_bra:
    s = (int)(char)(opcode);

    if(!s) {
    case ope_dp:
      s = fetch(2);
      signext(&s, 16);
    }

    print_pc_offset(s);
    cycles += 0;
    break;

  case opo_dis8v:
    s = opcode;
    signext(&s, 8);
    TPRINTF("#%d", s);
    cycles += 0;
    break;

  case opo_dis8p:
    s = opcode;
    signext(&s, 8);
    print_pc_offset(s);
    cycles += 0;
    break;

  case opo_vect:
    TPRINTF("#%d", opcode & 0xF);
    break;

  case op2_imm:
    u = (opcode >> 9) & 7;

    if(!u) {
      u = 8;
    }

    TPRINTF("#%u", u);
    cycles += 0;
    break;

  case op1_Dn:
    TPRINTF("D%d", opcode & 7);
    cycles += 0;
    break;

  case op2_Dn:
    TPRINTF("D%d", (opcode >> 9) & 7);
    cycles += 0;
    break;

  case op1_An:
    TPRINTF("A%d", opcode & 7);
    cycles += 0;
    break;

  case op2_An:
    TPRINTF("A%d", (opcode >> 9) & 7);
    cycles += 0;
    break;

  case op1_AnPD:
    TPRINTF("-(A%d)", opcode & 7);
    cycles += op->size == 4 ? 10 : 7;
    break;

  case op2_AnPD:
    TPRINTF("-(A%d)", (opcode >> 9) & 7);
    cycles += op->size == 4 ? 10 : 7;
    break;

  case op1_AnPI:
    TPRINTF("(A%d)+", opcode & 7);
    cycles += op->size == 4 ? 8 : 4;
    break;

  case op2_AnPI:
    TPRINTF("(A%d)+", (opcode >> 9) & 7);
    cycles += op->size == 4 ? 8 : 4;
    break;

  case op1_EA:
    print_ea((opcode >> 3) & 7, opcode & 7);
    break;

  case op2_EA:
    print_ea((opcode >> 6) & 7, (opcode >> 9) & 7);
    break;

  case op1e_AnDis:
    s = fetch(2);
    signext(&s, 16);

    if(s) {
      TPRINTF("%d(A%d)", s, opcode & 7);
    } else {
      TPRINTF("(A%d)", opcode & 7);
    }

    cycles += op->size == 4 ? 16 : 8;
    break;
  }
}

static void compute_base_cycles()
{
  // Compute base cycles for non-trivial instructions

  if((op->code & 0xF000) == 0xE000) { // ASL/ASR/LSL/LSR/ROL/ROR/ROXL/ROXR
    // Shift / rotate instruction
    cycles = (op->size == 4) ? 8 : 6;

    if(op->op1 == op2_imm) {
      // 2*n
      cycles += 2 * ((opcode >> 9) & 7);
    } else if(op->op1 == op2_Dn) {
      // 2*0-2*31
      cyclesmax += 126;
    } else {
      // Shift one bit
      cycles = 8;
    }
  }

  else if((op->code & 0xF000) == 0x6000) { // Bcc
    // Conditional branch
    cycles = 10;
    cyclesmax = op->size == 1 ? -2 : 2; // cyclesmax is difference when branch is not taken
  }

  else if((op->code & 0xF0F8) == 0x50C8) { // DBcc
    // Decrement and conditional branch
    cycles = 10;
    cyclesmax = 4;
  }

  else if((op->code & 0xF1C0) == 0x81C0) { // DIVS
    cycles = 126;
    cyclesmax = 14;
  }

  else if((op->code & 0xF1C0) == 0x80C0) { // DIVU
    cycles = 142;
    cyclesmax = 16;
  }

  else if((op->code & 0xF0C0) == 0xC0C0) { // MULS/MULU
    cycles = 38;
    cyclesmax = 32;
  }

  else if((op->code & 0xF0C0) == 0x50C0) { // Scc
    if(op->code >> 3 & 7) {
      // Scc <M>
      cycles = 8;
    } else {
      cycles = 4;
      cyclesmax = 2;
    }
  }
}


static void d68k_pass()
{
  while(size && tsize && instructions) {
    lsize = 0;

    if(labels) {
      const char *l = getdsymat(address);

      if(l) {
        TPRINTF("%s", l);
      }
    }

    TALIGN(8);

    --instructions;
    opcode = fetch(2);
    baseaddr = address;

    u32 c;

    for(c = 0; c < (sizeof(m68k_instr) / sizeof(struct instr)); ++c) {
      op = &m68k_instr[c];

      if((opcode & op->mask) != op->code) {
        continue;
      }

      TPRINTF("%s", op->name);

      // Compute cycles
      cycles = op->cycles;
      cyclesmax = 0; // Upper range of cycles

      if(cycles == -1) {
        compute_base_cycles();
      }

      if(op->op1 != op_none) {
        TALIGN(16);
        print_operand(op->op1);
      }

      if(op->op2 != op_none) {
        TPRINTF(", ");
        print_operand(op->op2);
      }

      if(showcycles) {
        TALIGN(40);
        TPRINTF("; %d", cycles);

        if(cyclesmax) {
          TPRINTF("-%d", cycles+cyclesmax);
        }
      }

      break;
    }

    if(c == (sizeof(m68k_instr) / sizeof(struct instr))) {
      // Not found
      TPRINTF("DW");
      TALIGN(16);
      TPRINTF("$%04X", opcode);
      TALIGN(40);
      TPRINTF("; Illegal opcode");
      ++*suspicious;
    }

    TPRINTF("\n");
  }
}

int64_t d68k(char *_targetdata, int _tsize, const u8 *_code, int _size, int _instructions, u32 _address, int _labels, int _showcycles, int *_suspicious)
{
  targetdata = _targetdata;
  tsize = _tsize;
  code = _code;
  size = _size;
  instructions = _instructions;
  address = _address;
  labels = _labels;
  showcycles = _showcycles;
  suspicious = _suspicious;
  startaddr = address;
  endaddr = address + size;

  *suspicious = 0;

  int errorcode;

  if((errorcode = setjmp(error_jmp))) {
    return errorcode;
  }

  // Do first pass to find labels
  d68k_pass();

  if(!labels) {
    // One pass is enough to disassemble without labels
    return address;
  }

  targetdata = _targetdata;
  tsize = _tsize;
  code = _code;
  size = _size;
  instructions = _instructions;
  address = _address;
  labels = _labels;
  suspicious = _suspicious;
  startaddr = address;
  endaddr = address + size;

  d68k_pass();

  return address;
}

// vim: ts=2 sw=2 sts=2 et
