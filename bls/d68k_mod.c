/* 68000 disassembler.
 * opcode table ripped from asmx2.
 */

#include "bls.h"
#include <stdio.h>
#include <setjmp.h>

typedef uint16_t u16;
typedef int16_t i16;

// Instruction operand type
enum iop
{
	            //  Bits  Description
	op_none,  	//        No operand
  op_ccr,     //        CCR
  op_sr,      //        SR

	ope_imm,    //        Immediate in extension word
	ope_ims,    //        Signed immediate in extension word
  ope_rl,     //        MOVEM register list (A7-A0/D7-D0)
  ope_rld,    //        MOVEM pre-decrement register list (D0-D7/A0-A7)
  ope_dp,     //        Relative pointer in extension word

  opo_dis8v,  // 07..00 Immediate displacement
  opo_dis8p,  // 07..00 Immediate pointer displacement
  opo_vect,   // 03..00 Trap vector

  op2_imm,    // 11..09 Second operand immediate (1..8)
	op1_Dn,			// 02..00 Dn
	op2_Dn,			// 11..09 Dn
	op1_An,			// 02..00 An
	op2_An,			// 11..09 An
  op1_AnPD,		// 02..00 -(An)
  op2_AnPD,		// 11..09 -(An)
  op1_AnPI,		// 02..00 (An)+
  op2_AnPI,		// 11..09 (An)+
  op1_EA,    	// 05..00 Effective address
	op2_EA,     // 11..06 Effective address (reversed mode/register)
  op1e_AnDis  // 02..00 Address register + 16 bits displacement in ext word
};

struct instr
{
  char       name[12];
  u16        code;
  u16        mask;
  enum iop   op1; // First operand
	enum iop   op2; // Second operand
	int        size; // Operation size
};
typedef struct instr *iptr;


// Matching order is important
struct instr m68k_instr[] =
{
#define cc(ipre, ipost, opcode, mask, op1, op2, size) \
  {ipre "T"  ipost, opcode | 0x0000, mask, op1, op2, size}, \
  {ipre "F"  ipost, opcode | 0x0100, mask, op1, op2, size}, \
  {ipre "HI" ipost, opcode | 0x0200, mask, op1, op2, size}, \
  {ipre "LS" ipost, opcode | 0x0300, mask, op1, op2, size}, \
  {ipre "CC" ipost, opcode | 0x0400, mask, op1, op2, size}, \
  {ipre "CS" ipost, opcode | 0x0500, mask, op1, op2, size}, \
  {ipre "NE" ipost, opcode | 0x0600, mask, op1, op2, size}, \
  {ipre "EQ" ipost, opcode | 0x0700, mask, op1, op2, size}, \
  {ipre "VC" ipost, opcode | 0x0800, mask, op1, op2, size}, \
  {ipre "VS" ipost, opcode | 0x0900, mask, op1, op2, size}, \
  {ipre "PL" ipost, opcode | 0x0A00, mask, op1, op2, size}, \
  {ipre "MI" ipost, opcode | 0x0B00, mask, op1, op2, size}, \
  {ipre "GE" ipost, opcode | 0x0C00, mask, op1, op2, size}, \
  {ipre "LT" ipost, opcode | 0x0D00, mask, op1, op2, size}, \
  {ipre "GT" ipost, opcode | 0x0E00, mask, op1, op2, size}, \
  {ipre "LE" ipost, opcode | 0x0F00, mask, op1, op2, size}

  {"ADDX.B", 0xD100, 0xF1F8, op2_Dn, op1_Dn, 1},
  {"ADDX.B", 0xD108, 0xF1F8, op2_AnPD, op1_AnPD, 1},
  {"ADDX.W", 0xD140, 0xF1F8, op2_Dn, op1_Dn, 2},
  {"ADDX.W", 0xD148, 0xF1F8, op2_AnPD, op1_AnPD, 2},
  {"ADDX.L", 0xD180, 0xF1F8, op2_Dn, op1_Dn, 4},
  {"ADDX.L", 0xD188, 0xF1F8, op2_AnPD, op1_AnPD, 4},

  {"ABCD", 0xC100, 0xF1F8, op2_Dn, op1_Dn, 1},
  {"ABCD", 0xC108, 0xF1F8, op2_AnPD, op1_AnPD, 1},

  {"ADD.B", 0xD000, 0xF1C0, op1_EA, op2_Dn, 1},
  {"ADD.B", 0xD100, 0xF1C0, op2_Dn, op1_EA, 1},
  {"ADD.W", 0xD040, 0xF1C0, op1_EA, op2_Dn, 2},
  {"ADD.W", 0xD140, 0xF1C0, op2_Dn, op1_EA, 2},
  {"ADD.L", 0xD080, 0xF1C0, op1_EA, op2_Dn, 4},
  {"ADD.L", 0xD180, 0xF1C0, op2_Dn, op1_EA, 4},

  {"ADDA.W", 0xD0C0, 0xF1C0, op1_EA, op2_An, 2},
  {"ADDA.L", 0xD1C0, 0xF1C0, op1_EA, op2_An, 4},

  {"ADDI.B", 0x0600, 0xFFC0, ope_imm, op1_EA, 1},
  {"ADDI.W", 0x0640, 0xFFC0, ope_imm, op1_EA, 2},
  {"ADDI.L", 0x0680, 0xFFC0, ope_imm, op1_EA, 4},

  {"ADDQ.B", 0x5000, 0xF1C0, op2_imm, op1_EA, 1},
  {"ADDQ.W", 0x5040, 0xF1C0, op2_imm, op1_EA, 2},
  {"ADDQ.L", 0x5080, 0xF1C0, op2_imm, op1_EA, 4},

  {"AND.B", 0xC000, 0xF1C0, op1_EA, op2_Dn, 1},
  {"AND.B", 0xC100, 0xF1C0, op2_Dn, op1_EA, 1},
  {"AND.W", 0xC040, 0xF1C0, op1_EA, op2_Dn, 2},
  {"AND.W", 0xC140, 0xF1C0, op2_Dn, op1_EA, 2},
  {"AND.L", 0xC080, 0xF1C0, op1_EA, op2_Dn, 4},
  {"AND.L", 0xC180, 0xF1C0, op2_Dn, op1_EA, 4},

  {"ANDI", 0x023C, 0xFFFF, ope_imm, op_ccr, 1},
  {"ANDI.B", 0x0200, 0xFFC0, ope_imm, op1_EA, 1},
  {"ANDI.W", 0x0240, 0xFFC0, ope_imm, op1_EA, 2},
  {"ANDI.L", 0x0280, 0xFFC0, ope_imm, op1_EA, 4},

  {"ASL.B", 0xE000, 0xF1F8, op2_imm, op1_Dn, 1},
  {"ASL.B", 0xE020, 0xF1F8, op2_Dn, op1_Dn, 1},
  {"ASL.W", 0xE040, 0xF1F8, op2_imm, op1_Dn, 2},
  {"ASL.W", 0xE060, 0xF1F8, op2_Dn, op1_Dn, 2},
  {"ASL.L", 0xE080, 0xF1F8, op2_imm, op1_Dn, 4},
  {"ASL.L", 0xE0A0, 0xF1F8, op2_Dn, op1_Dn, 4},
  {"ASL.B", 0xE0C0, 0xFFC0, op1_EA, op_none, 1},

  {"ASR.B", 0xE100, 0xF1F8, op2_imm, op1_Dn, 1},
  {"ASR.B", 0xE120, 0xF1F8, op2_Dn, op1_Dn, 1},
  {"ASR.W", 0xE140, 0xF1F8, op2_imm, op1_Dn, 2},
  {"ASR.W", 0xE160, 0xF1F8, op2_Dn, op1_Dn, 2},
  {"ASR.L", 0xE180, 0xF1F8, op2_imm, op1_Dn, 4},
  {"ASR.L", 0xE1A0, 0xF1F8, op2_Dn, op1_Dn, 4},
  {"ASR.B", 0xE1C0, 0xFFC0, op1_EA, op_none, 1},

  {"BCC.W", 0x6000, 0xFFFF, ope_dp, op_none, 1},
  {"BCC.B", 0x6000, 0xFF00, ope_dp, op_none, 1},

  {"BRA.W", 0x6000, 0xFFFF, ope_dp, op_none, 2},
  {"BRA.B", 0x6000, 0xFF00, ope_dp, op_none, 2},
  {"BSR.W", 0x6100, 0xFFFF, ope_dp, op_none, 2},
  {"BSR.B", 0x6100, 0xFF00, ope_dp, op_none, 2},
  cc("B", ".W", 0x6000, 0xFFFF, ope_dp, op_none, 2),
  cc("B", ".B", 0x6000, 0xFF00, ope_dp, op_none, 1),

  {"BCHG.L", 0x0140, 0xF1C0, op2_Dn, op1_EA, 4},
  {"BCHG.B", 0x0840, 0xFFC0, ope_imm, op1_EA, 1},
  {"BCLR.L", 0x0180, 0xF1C0, op2_Dn, op1_EA, 4},
  {"BCLR.B", 0x0880, 0xFFC0, ope_imm, op1_EA, 1},
  {"BSET.L", 0x01C0, 0xF1C0, op2_Dn, op1_EA, 4},
  {"BSET.B", 0x08C0, 0xFFC0, ope_imm, op1_EA, 1},
  {"BTST.L", 0x0100, 0xF1C0, op2_Dn, op1_EA, 4},
  {"BTST.B", 0x0800, 0xFFC0, ope_imm, op1_EA, 1},
  
  {"CHK.W", 0x4180, 0xF1C0, op1_EA, op2_Dn, 2},

  {"CLR.B", 0x4200, 0xFFC0, op1_EA, op_none, 1},
  {"CLR.W", 0x4240, 0xFFC0, op1_EA, op_none, 2},
  {"CLR.L", 0x4280, 0xFFC0, op1_EA, op_none, 4},

  {"CMP.B", 0xB000, 0xF1C0, op1_EA, op2_Dn, 1},
  {"CMP.W", 0xB040, 0xF1C0, op1_EA, op2_Dn, 2},
  {"CMP.L", 0xB080, 0xF1C0, op1_EA, op2_Dn, 4},

  {"CMPA.W", 0xB0C0, 0xF1C0, op1_EA, op2_An, 2},
  {"CMPA.L", 0xB1C0, 0xF1C0, op1_EA, op2_An, 4},

  {"CMPI.B", 0x0C00, 0xFFC0, ope_imm, op1_EA, 1},
  {"CMPI.W", 0x0C40, 0xFFC0, ope_imm, op1_EA, 2},
  {"CMPI.L", 0x0C80, 0xFFC0, ope_imm, op1_EA, 4},

  {"CMPM.B", 0xB108, 0xF1F8, op2_AnPI, op1_AnPI, 1},
  {"CMPM.W", 0xB148, 0xF1F8, op2_AnPI, op1_AnPI, 2},
  {"CMPM.L", 0xB188, 0xF1F8, op2_AnPI, op1_AnPI, 4},

  {"DBRA", 0x51C8, 0xFFF8, op1_Dn, ope_dp, 2},
  cc("DB", "", 0x50C8, 0xFFF8, op1_Dn, ope_dp, 2),

  {"DIVS", 0x81C0, 0xF1C0, op1_EA, op2_Dn, 2},
  {"DIVU", 0x80C0, 0xF1C0, op1_EA, op2_Dn, 2},
  {"MULS", 0xC1C0, 0xF1C0, op1_EA, op2_Dn, 2},
  {"MULU", 0xC0C0, 0xF1C0, op1_EA, op2_Dn, 2},

  {"EOR.B", 0xB100, 0xF1C0, op2_Dn, op1_EA, 1},
  {"EOR.W", 0xB140, 0xF1C0, op2_Dn, op1_EA, 2},
  {"EOR.L", 0xB180, 0xF1C0, op2_Dn, op1_EA, 4},

  {"EORI", 0x0A3C, 0xFFFF, ope_imm, op_ccr, 1},
  {"EORI.B", 0x0A00, 0xFFC0, ope_imm, op1_EA, 1},
  {"EORI.W", 0x0A00, 0xFFC0, ope_imm, op1_EA, 2},
  {"EORI.L", 0x0A00, 0xFFC0, ope_imm, op1_EA, 4},

  {"EXG", 0xC140, 0xF1F8, op2_Dn, op1_Dn, 4},
  {"EXG", 0xC144, 0xF1F8, op2_An, op1_An, 4},
  {"EXG", 0xC184, 0xF1F8, op2_Dn, op1_Dn, 4},

  {"EXT.W", 0x4480, 0xFFF8, op1_Dn, op_none, 2},
  {"EXT.L", 0x44C0, 0xFFF8, op1_Dn, op_none, 4},

  {"ILLEGAL", 0x4AFC, 0xFFFF, op_none, op_none, 0},

  {"JMP", 0x4EC0, 0xFFC0, op1_EA, op_none, 4},
  {"JSR", 0x4E80, 0xFFC0, op1_EA, op_none, 4},

  {"PEA", 0x4840, 0xFFC0, op1_EA, op_none, 4},
  {"LEA", 0x41C0, 0xF1C0, op1_EA, op2_An, 4},

  {"LINK", 0x4E50, 0xFFF8, op1_An, ope_ims, 2},

  {"LSR.B", 0xE008, 0xF1F8, op2_imm, op1_Dn, 1},
  {"LSR.B", 0xE028, 0xF1F8, op2_Dn, op1_Dn, 1},
  {"LSR.W", 0xE048, 0xF1F8, op2_imm, op1_Dn, 2},
  {"LSR.W", 0xE068, 0xF1F8, op2_Dn, op1_Dn, 2},
  {"LSR.L", 0xE088, 0xF1F8, op2_imm, op1_Dn, 4},
  {"LSR.L", 0xE0A8, 0xF1F8, op2_Dn, op1_Dn, 4},
  {"LSR.B", 0xE2C0, 0xFFC0, op1_EA, op_none, 1},

  {"LSL.B", 0xE108, 0xF1F8, op2_imm, op1_Dn, 1},
  {"LSL.B", 0xE128, 0xF1F8, op2_Dn, op1_Dn, 1},
  {"LSL.W", 0xE148, 0xF1F8, op2_imm, op1_Dn, 2},
  {"LSL.W", 0xE168, 0xF1F8, op2_Dn, op1_Dn, 2},
  {"LSL.L", 0xE188, 0xF1F8, op2_imm, op1_Dn, 4},
  {"LSL.L", 0xE1A8, 0xF1F8, op2_Dn, op1_Dn, 4},
  {"LSL.B", 0xE3C0, 0xFFC0, op1_EA, op_none, 1},

  {"MOVEA.W", 0x3040, 0xF1C0, op1_EA, op2_An, 2},
  {"MOVEA.L", 0x2040, 0xF1C0, op1_EA, op2_An, 2},

  {"MOVE.B", 0x1000, 0x3000, op1_EA, op2_EA, 1},
  {"MOVE.W", 0x3000, 0x3000, op1_EA, op2_EA, 2},
  {"MOVE.L", 0x2000, 0x3000, op1_EA, op2_EA, 4},

  {"MOVE", 0x42C0, 0xFFC0, op_ccr, op1_EA, 2},
  {"MOVE", 0x44C0, 0xFFC0, op1_EA, op_ccr, 2},
  {"MOVE", 0x40C0, 0xFFC0, op_sr, op1_EA, 2},

  // Pre-decrement modes
  {"MOVEM.W", 0x48A0, 0xFFF8, op1_EA, ope_rld, 2},
  {"MOVEM.L", 0x48E0, 0xFFF8, op1_EA, ope_rld, 4},
  // Other MOVEM modes
  {"MOVEM.W", 0x4880, 0xFFC0, op1_EA, ope_rl, 2},
  {"MOVEM.L", 0x48C0, 0xFFC0, op1_EA, ope_rl, 4},
  {"MOVEM.W", 0x4C80, 0xFFC0, ope_rl, op1_EA, 2},
  {"MOVEM.L", 0x4CC0, 0xFFC0, ope_rl, op1_EA, 4},

  {"MOVEP.W", 0x0108, 0xF1F8, op1e_AnDis, op2_Dn, 2},
  {"MOVEP.L", 0x0148, 0xF1F8, op1e_AnDis, op2_Dn, 4},
  {"MOVEP.W", 0x0188, 0xF1F8, op2_Dn, op1e_AnDis, 2},
  {"MOVEP.L", 0x01C8, 0xF1F8, op2_Dn, op1e_AnDis, 4},

  {"MOVEQ", 0x7000, 0xF100, opo_dis8v, op2_Dn, 4},

  {"NBCD", 0x4800, 0xFFC0, op1_EA, op_none, 1},

  {"NEG.B", 0x4400, 0xFFC0, op1_EA, op_none, 1},
  {"NEG.W", 0x4440, 0xFFC0, op1_EA, op_none, 2},
  {"NEG.L", 0x4480, 0xFFC0, op1_EA, op_none, 4},

  {"NEGX.B", 0x4000, 0xFFC0, op1_EA, op_none, 1},
  {"NEGX.W", 0x4040, 0xFFC0, op1_EA, op_none, 2},
  {"NEGX.L", 0x4080, 0xFFC0, op1_EA, op_none, 4},

  {"NOP", 0x4E71, 0xFFFF, op_none, op_none, 0},

  {"NOT.B", 0x4600, 0xFFC0, op1_EA, op_none, 1},
  {"NOT.W", 0x4640, 0xFFC0, op1_EA, op_none, 2},
  {"NOT.L", 0x4680, 0xFFC0, op1_EA, op_none, 4},

  {"OR.B", 0x8000, 0xF1C0, op1_EA, op2_Dn, 1},
  {"OR.B", 0x8100, 0xF1C0, op2_Dn, op1_EA, 1},
  {"OR.W", 0x8040, 0xF1C0, op1_EA, op2_Dn, 2},
  {"OR.W", 0x8140, 0xF1C0, op2_Dn, op1_EA, 2},
  {"OR.L", 0x8080, 0xF1C0, op1_EA, op2_Dn, 4},
  {"OR.L", 0x8180, 0xF1C0, op2_Dn, op1_EA, 4},

  {"ORI", 0x003C, 0xFFFF, ope_imm, op_ccr, 1},
  {"ORI.B", 0x0000, 0xFFC0, ope_imm, op1_EA, 1},
  {"ORI.W", 0x0040, 0xFFC0, ope_imm, op1_EA, 2},
  {"ORI.L", 0x0080, 0xFFC0, ope_imm, op1_EA, 4},

  {"ROR.B", 0xE018, 0xF1F8, op2_imm, op1_Dn, 1},
  {"ROR.B", 0xE038, 0xF1F8, op2_Dn, op1_Dn, 1},
  {"ROR.W", 0xE058, 0xF1F8, op2_imm, op1_Dn, 2},
  {"ROR.W", 0xE078, 0xF1F8, op2_Dn, op1_Dn, 2},
  {"ROR.L", 0xE098, 0xF1F8, op2_imm, op1_Dn, 4},
  {"ROR.L", 0xE0B8, 0xF1F8, op2_Dn, op1_Dn, 4},
  {"ROR.B", 0xE6C0, 0xFFC0, op1_EA, op_none, 1},

  {"ROL.B", 0xE118, 0xF1F8, op2_imm, op1_Dn, 1},
  {"ROL.B", 0xE138, 0xF1F8, op2_Dn, op1_Dn, 1},
  {"ROL.W", 0xE158, 0xF1F8, op2_imm, op1_Dn, 2},
  {"ROL.W", 0xE178, 0xF1F8, op2_Dn, op1_Dn, 2},
  {"ROL.L", 0xE198, 0xF1F8, op2_imm, op1_Dn, 4},
  {"ROL.L", 0xE1B8, 0xF1F8, op2_Dn, op1_Dn, 4},
  {"ROL.B", 0xE7C0, 0xFFC0, op1_EA, op_none, 1},

  {"ROXR.B", 0xE010, 0xF1F8, op2_imm, op1_Dn, 1},
  {"ROXR.B", 0xE030, 0xF1F8, op2_Dn, op1_Dn, 1},
  {"ROXR.W", 0xE050, 0xF1F8, op2_imm, op1_Dn, 2},
  {"ROXR.W", 0xE070, 0xF1F8, op2_Dn, op1_Dn, 2},
  {"ROXR.L", 0xE090, 0xF1F8, op2_imm, op1_Dn, 4},
  {"ROXR.L", 0xE0B0, 0xF1F8, op2_Dn, op1_Dn, 4},
  {"ROXR.B", 0xE4C0, 0xFFC0, op1_EA, op_none, 1},

  {"ROXL.B", 0xE110, 0xF1F8, op2_imm, op1_Dn, 1},
  {"ROXL.B", 0xE130, 0xF1F8, op2_Dn, op1_Dn, 1},
  {"ROXL.W", 0xE150, 0xF1F8, op2_imm, op1_Dn, 2},
  {"ROXL.W", 0xE170, 0xF1F8, op2_Dn, op1_Dn, 2},
  {"ROXL.L", 0xE190, 0xF1F8, op2_imm, op1_Dn, 4},
  {"ROXL.L", 0xE1B0, 0xF1F8, op2_Dn, op1_Dn, 4},
  {"ROXL.B", 0xE5C0, 0xFFC0, op1_EA, op_none, 1},

  {"RTR", 0x4E77, 0xFFFF, op_none, op_none, 0},
  {"RTS", 0x4E75, 0xFFFF, op_none, op_none, 0},

  cc("S", "", 0x50C0, 0xF0C0, op1_EA, op_none, 1),

  {"SUBX.B", 0x9100, 0xF1F8, op2_Dn, op1_Dn, 1},
  {"SUBX.B", 0x9108, 0xF1F8, op2_AnPD, op1_AnPD, 1},
  {"SUBX.W", 0x9140, 0xF1F8, op2_Dn, op1_Dn, 2},
  {"SUBX.W", 0x9148, 0xF1F8, op2_AnPD, op1_AnPD, 2},
  {"SUBX.L", 0x9180, 0xF1F8, op2_Dn, op1_Dn, 4},
  {"SUBX.L", 0x9188, 0xF1F8, op2_AnPD, op1_AnPD, 4},

  {"SBCD", 0x8100, 0xF1F8, op2_Dn, op1_Dn, 1},
  {"SBCD", 0x8108, 0xF1F8, op2_AnPD, op1_AnPD, 1},

  {"SUB.B", 0x9000, 0xF1C0, op1_EA, op2_Dn, 1},
  {"SUB.B", 0x9100, 0xF1C0, op2_Dn, op1_EA, 1},
  {"SUB.W", 0x9040, 0xF1C0, op1_EA, op2_Dn, 2},
  {"SUB.W", 0x9140, 0xF1C0, op2_Dn, op1_EA, 2},
  {"SUB.L", 0x9080, 0xF1C0, op1_EA, op2_Dn, 4},
  {"SUB.L", 0x9180, 0xF1C0, op2_Dn, op1_EA, 4},

  {"SUBA.W", 0x90C0, 0xF1C0, op1_EA, op2_An, 2},
  {"SUBA.L", 0x91C0, 0xF1C0, op1_EA, op2_An, 4},

  {"SUBI.B", 0x0400, 0xFFC0, ope_imm, op1_EA, 1},
  {"SUBI.W", 0x0440, 0xFFC0, ope_imm, op1_EA, 2},
  {"SUBI.L", 0x0480, 0xFFC0, ope_imm, op1_EA, 4},

  {"SUBQ.B", 0x5100, 0xF1C0, op2_imm, op1_EA, 1},
  {"SUBQ.W", 0x5140, 0xF1C0, op2_imm, op1_EA, 2},
  {"SUBQ.L", 0x5180, 0xF1C0, op2_imm, op1_EA, 4},
  
  {"SWAP", 0x4840, 0xFFF8, op1_Dn, op_none, 2},

  {"TAS", 0x4AC0, 0xFFC0, op1_EA, op_none, 1},

  {"TRAP", 0x4E40, 0xFFF0, opo_vect, op_none, 0},
  {"TRAPV", 0x4E76, 0xFFFF, op_none, op_none, 0},

  {"TST.B", 0x4500, 0xFFC0, op1_EA, op_none, 1},
  {"TST.W", 0x4540, 0xFFC0, op1_EA, op_none, 2},
  {"TST.L", 0x4580, 0xFFC0, op1_EA, op_none, 4},

  {"UNLK", 0x4E58, 0xFFF8, op1_An, op_none, 0},
#undef cc
};

static char *target;
static int tsize;
static const u8 *code;
static int size;
static u32 address;
static int labels;
static int *suspicious;

static u32 startaddr;
static u32 endaddr;
static u32 opcode;
static u32 baseaddr;
static iptr op;

static jmp_buf error_jmp;
// Errors returned by longjmp
const char * d68k_error(int r)
{
  switch(r)
  {
    case 1 : return "Fetched past the end of code";
    case 2 : return "Not enough space to store disassembled string";
  }
  return "Invalid error code";
}

u32 fetch(int bytes)
{
  if(size < bytes)
  {
    longjmp(error_jmp, 1);
  }
  u32 v = getint(code, bytes);
  code += bytes;
  size -= bytes;
  address += bytes;
  return v;
}

#define TPRINTF(...) do { int TPRINTF_sz;\
  if(tsize) TPRINTF_sz = snprintf(target, tsize, __VA_ARGS__); else longjmp(error_jmp, 2); \
  tsize -= TPRINTF_sz; \
  target += TPRINTF_sz; } while(0)

void print_label(u32 addr)
{
  const char *name = getdsymat(addr);
  if(name)
  {
    TPRINTF("%s", name);
    return;
  }

  char n[8];
  snprintf(n, 8, "a%06X", addr);
  TPRINTF("%s", n);
  setdsym(n, addr);
}

void print_pc_offset(int offset)
{
  u32 a = baseaddr + offset;
  if(labels && a >= startaddr && a < endaddr)
  {
    print_label(a);
  }
  else
  {
    TPRINTF("%d", offset);
  }
}

void print_rl(u32 list)
{
  if(!list)
  {
    ++*suspicious;
    TPRINTF("none");
    return;
  }
  int fr = 1; // First register
  int ls = 0; // Last set
  int sc = 0; // Set count
  char rt; // Register type
  int rn; // Register number
  for(rt = 'D'; rt >= 'A'; rt -= 'D' - 'A')
  {
    for(rn = 0; rn < 7; ++rn)
    {
      int r = list & 1;
      if(r && !ls)
      {
        ls = 1;
        sc = 1;
        if(fr)
        {
          fr = 0;
        }
        else
        {
          TPRINTF("/");
        }
        TPRINTF("%c%d", rt, rn);
      }
      else if(r && ls)
      {
        ++sc;
      }
      else if(!r && ls)
      {
        if(sc >= 2)
        {
          TPRINTF("-%c%d", rt, rn);
        }
      }

      list >>= 1;
    }
    if(ls)
    {
      ls = 0;
      if(sc >= 2)
      {
        TPRINTF("-%c%d", rt, rn);
      }
    }
  }
}

void print_ea(int mode, int reg)
{
  switch(mode) {
    case 0: { /* Dn */
      TPRINTF("D%d", (reg));
      break;
    }
    case 1: { /* An */
      TPRINTF("A%d", (reg));
      break;
    }
    case 2: { /* (An) */
      TPRINTF("(A%d)", (reg));
      break;
    }
    case 3: { /* (An)+ */
      TPRINTF("(A%d)+", (reg));
      break;
    }
    case 4: { /* -(An) */
      TPRINTF("-(A%d)", (reg));
      break;
    }
    case 5: { /* d16(An) */
      int offset = (i16)fetch(2);
      TPRINTF("%d(A%d)", offset, (reg));
      break;
    }
    case 6: { /* d8(An,Xn) */
      u32 flags = fetch(1);
      int offset = (int)(char)(fetch(1));
      TPRINTF("%d(A%d,%c%d.%c)", offset, (reg), (flags & 0x80) ? 'A':'D', (flags >> 4) & 7, (flags & 0x08) ? 'W' : 'L');
      if(flags & 0x07) { TPRINTF("\t; Illegal"); ++*suspicious; }
      break;
    }
    case 7: { /* Other modes */
      switch(reg) {
        case 0: { /* xxx.W */
          u32 addr = fetch(2); if(addr >= 0x8000) addr |= 0xFF0000;
          TPRINTF("$%06X.W", addr);
          break;
        }
        case 1: { /* xxx.L */
          u32 addr = fetch(4);
          TPRINTF("$%06X.L", addr);
          break;
        }
        case 2: { /* d16(PC) */
          int offset = (int)(i16)fetch(2);
          print_pc_offset(offset);
          TPRINTF("(PC)");
          break;
        }
				case 3: { /* d8(PC,Xn) */
					u32 flags = fetch(1);
					int offset = (int)(char)fetch(1);
					TPRINTF("%d(PC,%c%d.%c)", offset, (flags & 0x80) ? 'A':'D', (flags >> 4) & 7, (flags & 0x08) ? 'W' : 'L');
					if(flags & 0x07) { TPRINTF("\t; Illegal"); ++*suspicious; }
					break;
        }
				case 4: { /* #xxx */
					u32 data = fetch(op->size);
					TPRINTF("#$%X", data);
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
  switch(ot)
  {
    case op_none: break;
    case op_ccr: TPRINTF("CCR"); break;
    case op_sr: TPRINTF("SR"); break;

    case ope_imm:
      u = fetch(op->size);
      TPRINTF("#$%X", u);
      break;

    case ope_ims:
      s = fetch(op->size);
      signext(&s, op->size * 8);
      TPRINTF("#%d", s);

    case ope_rl:
      u = fetch(2);
      print_rl(u);

    case ope_rld:
      u = fetch(2);
      {
        // Inverse bits and display normally
        u32 inverse = 0;
        int i = 0;
        for(i = 0; i < 16; ++i)
        {
          inverse |= u & 1;
          u >>= 1;
          inverse <<= 1;
        }
        print_rl(inverse);
      }

    case ope_dp:
      s = fetch(2);
      signext(&s, 16);
      print_pc_offset(s);
      break;
    
    case opo_dis8v:
      s = opcode;
      signext(&s, 8);
      TPRINTF("#%d", s);
      break;

    case opo_dis8p:
      s = opcode;
      signext(&s, 8);
      print_pc_offset(s);
      break;

    case opo_vect:
      TPRINTF("#%d", opcode & 0xF);
      break;

    case op2_imm:
      u = (opcode >> 9) & 7;
      if(!u)
      {
        u = 8;
      }
      TPRINTF("#%u", u);
      break;

    case op1_Dn:
      TPRINTF("D%d", opcode & 7);
      break;

    case op2_Dn:
      TPRINTF("D%d", (opcode >> 9) & 7);
      break;

    case op1_An:
      TPRINTF("A%d", opcode & 7);
      break;

    case op2_An:
      TPRINTF("A%d", (opcode >> 9) & 7);
      break;

    case op1_AnPD:
      TPRINTF("-(A%d)", opcode & 7);
      break;

    case op2_AnPD:
      TPRINTF("-(A%d)", (opcode >> 9) & 7);
      break;

    case op1_AnPI:
      TPRINTF("(A%d)+", opcode & 7);
      break;

    case op2_AnPI:
      TPRINTF("(A%d)+", (opcode >> 9) & 7);
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
      TPRINTF("%d(A%d)", s, opcode & 7);
      break;
  }

}

static void d68k_pass()
{
  while(size && tsize)
  {
    if(labels)
    {
      const char *l = getdsymat(address);
      if(l)
        TPRINTF("%s", l);
    }
    TPRINTF("\t");

    opcode = fetch(2);
    baseaddr = address;

    u32 c;
    for(c = 0; c < (sizeof(m68k_instr) / sizeof(struct instr)); ++c)
    {
      op = &m68k_instr[c];
      if((opcode & op->mask) != op->code)
      {
        continue;
      }
      TPRINTF("%s\t", op->name);

      if(op->op1 != op_none)
      {
        print_operand(op->op1);
      }
      if(op->op2 != op_none)
      {
        TPRINTF(", ");
        print_operand(op->op2);
      }

      break;
    }
    if(c == (sizeof(m68k_instr) / sizeof(struct instr)))
    {
      // Not found
      TPRINTF("DW\t$%04X\t; Illegal opcode", opcode);
      ++*suspicious;
    }
    TPRINTF("\n");
  }
}

int d68k(char *_target, int _tsize, const u8 *_code, int _size, sv_t _address, int _labels, int *_suspicious)
{
  target = _target;
  tsize = _tsize;
  code = _code;
  size = _size;
  address = _address;
  labels = _labels;
  suspicious = _suspicious;
  startaddr = address;
  endaddr = address + size;

  *suspicious = 0;

  int errorcode;
  if((errorcode = setjmp(error_jmp)))
  {
    return errorcode;
  }

  // Do first pass to find labels
  d68k_pass();

  if(!labels)
  {
    // One pass is enough to disassemble without labels
    return 0;
  }

  target = _target;
  tsize = _tsize;
  code = _code;
  size = _size;
  address = _address;
  labels = _labels;
  suspicious = _suspicious;
  startaddr = address;
  endaddr = address + size;

  d68k_pass();

  return 0;
}
