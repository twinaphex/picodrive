
/*
 * Basic macros to emit MIPS instructions and some utils
 * This file has some code from Yabause.
 * Copyright (C) 2008,2009,2010 notaz
 * Copyright 2009 Andrew Church
 * 2016 Robson Santana
 *
 * This work is licensed under the terms of MAME license.
 * See COPYING file in the top-level directory.
 */
#define CONTEXT_REG 11

// XXX: tcache_ptr type for SVP and SH2 compilers differs..
#define EMIT_PTR(ptr, x) \
	do { \
		*(u32 *)ptr = x; \
		ptr = (void *)((u8 *)ptr + sizeof(u32)); \
		COUNT_OP; \
	} while (0)

#define EMIT(x) EMIT_PTR(tcache_ptr, x)

/*************************************************************************/

/* Register names */

#define MIPS_zero  0
#define MIPS_at    1
#define MIPS_v0    2
#define MIPS_v1    3
#define MIPS_a0    4
#define MIPS_a1    5
#define MIPS_a2    6
#define MIPS_a3    7
#define MIPS_t0    8
#define MIPS_t1    9
#define MIPS_t2   10
#define MIPS_t3   11
#define MIPS_t4   12
#define MIPS_t5   13
#define MIPS_t6   14
#define MIPS_t7   15
#define MIPS_s0   16
#define MIPS_s1   17
#define MIPS_s2   18
#define MIPS_s3   19
#define MIPS_s4   20
#define MIPS_s5   21
#define MIPS_s6   22
#define MIPS_s7   23
#define MIPS_t8   24
#define MIPS_t9   25
#define MIPS_k0   26
#define MIPS_k1   27
#define MIPS_gp   28
#define MIPS_sp   29
#define MIPS_s8   30
#define MIPS_ra   31

/*-----------------------------------------------------------------------*/

/* General instruction formats */

#define __MIPS_INSN_IMM(op,rs,rt,imm) \
	EMIT(__OP_##op<<26 | ((rs)&0x1F)<<21 | ((rt)&0x1F)<<16 | ((imm)&0xFFFF))

#define __MIPS_INSN_JUMP(op,target) \
	EMIT(__OP_##op<<26 | ((target)&0x03FFFFFF))

#define __MIPS_INSN_REG(op,rs,rt,rd,shamt,funct) \
	EMIT(__OP_##op<<26 | ((rs)&0x1F)<<21 | ((rt)&0x1F)<<16 | ((rd)&0x1F)<<11 | \
	 ((shamt)&0x3F)<<6 | ((funct)&0x3F))

/*-----------------------------------------------------------------------*/

/* Instruction groups ("op" field) -- some groups not used for SH-2
 * emulation are omitted */

#define __OP_SPECIAL  000
#define __OP_REGIMM   001
#define __OP_J        002
#define __OP_JAL      003
#define __OP_BEQ      004
#define __OP_BNE      005
#define __OP_BLEZ     006
#define __OP_BGTZ     007

#define __OP_ADDI     010
#define __OP_ADDIU    011
#define __OP_SLTI     012
#define __OP_SLTIU    013
#define __OP_ANDI     014
#define __OP_ORI      015
#define __OP_XORI     016
#define __OP_LUI      017

#define __OP_BEQL     024
#define __OP_BNEL     025
#define __OP_BLEZL    026
#define __OP_BGTZL    027

#define __OP_ALLEGREX 037  // Allegrex-specific instruction group

#define __OP_LB       040
#define __OP_LH       041
#define __OP_LWL      042
#define __OP_LW       043
#define __OP_LBU      044
#define __OP_LHU      045
#define __OP_LWR      046

#define __OP_SB       050
#define __OP_SH       051
#define __OP_SWL      052
#define __OP_SW       053
#define __OP_SWR      056

/* SPECIAL functions ("funct" field) */

#define __SP_SLL      000
#define __SP_SRL      002
#define __SP_SRA      003
#define __SP_SLLV     004
#define __SP_SRLV     006
#define __SP_SRAV     007

#define __SP_JR       010
#define __SP_JALR     011
#define __SP_MOVZ     012
#define __SP_MOVN     013

#define __SP_MFHI     020
#define __SP_MTHI     021
#define __SP_MFLO     022
#define __SP_MTLO     023

#define __SP_MULT     030
#define __SP_MULTU    031
#define __SP_DIV      032
#define __SP_DIVU     033

#define __SP_ADD      040
#define __SP_ADDU     041
#define __SP_SUB      042
#define __SP_SUBU     043
#define __SP_AND      044
#define __SP_OR       045
#define __SP_XOR      046
#define __SP_NOR      047

#define __SP_SLT      052
#define __SP_SLTU     053

/* REGIMM functions ("rt" field) */

#define __RI_BLTZ     000
#define __RI_BGEZ     001
#define __RI_BLTZL    002
#define __RI_BGEZL    003

#define __RI_BLTZAL   020
#define __RI_BGEZAL   021
#define __RI_BLTZALL  022
#define __RI_BGEZALL  023

/* Allegrex-specific instructions ("funct" field) */

#define __AL_EXT      000
#define __AL_INS      004

#define __AL_SEX      040  // Sign EXtend (SEB/SEH depending on shamt)

/*-----------------------------------------------------------------------*/

/* Shortcuts for SPECIAL, REGIMM, and ALLEGREX instructions */

#define __MIPS_SPECIAL(rs,rt,rd,shamt,funct) \
	__MIPS_INSN_REG(SPECIAL, rs, rt, rd, shamt, __SP_##funct)

#define __MIPS_REGIMM(rs,rt,imm) \
	__MIPS_INSN_IMM(REGIMM, rs, __RI_##rt, imm)

#define __MIPS_ALLEGREX(rs,rt,rd,shamt,funct) \
	__MIPS_INSN_REG(ALLEGREX, rs, rt, rd, shamt, __AL_##funct)

/*-----------------------------------------------------------------------*/

/* Instruction definitions.  Note that branch offsets and jump targets must
 * be specified as in the opcode (shifted right two bits, relative to the
 * delay slot for relative branches). */

#define MIPS_ADD(rd,rs,rt)      __MIPS_SPECIAL((rs), (rt), (rd), 0, ADD)
#define MIPS_ADDI(rt,rs,imm)    __MIPS_INSN_IMM(ADDI, (rs), (rt), (imm))
#define MIPS_ADDIU(rt,rs,imm)   __MIPS_INSN_IMM(ADDIU, (rs), (rt), (imm))
#define MIPS_ADDU(rd,rs,rt)     __MIPS_SPECIAL((rs), (rt), (rd), 0, ADDU)
#define MIPS_AND(rd,rs,rt)      __MIPS_SPECIAL((rs), (rt), (rd), 0, AND)
#define MIPS_ANDI(rt,rs,imm)    __MIPS_INSN_IMM(ANDI, (rs), (rt), (imm))

#define MIPS_BEQ(rs,rt,offset)  __MIPS_INSN_IMM(BEQ, (rs), (rt), (offset))
#define MIPS_BEQL(rs,rt,offset) __MIPS_INSN_IMM(BEQL, (rs), (rt), (offset))
#define MIPS_BGEZ(rs,offset)    __MIPS_REGIMM((rs), BGEZ, (offset))
#define MIPS_BGEZAL(rs,offset)  __MIPS_REGIMM((rs), BGEZAL, (offset))
#define MIPS_BGEZALL(rs,offset) __MIPS_REGIMM((rs), BGEZALL, (offset))
#define MIPS_BGEZL(rs,offset)   __MIPS_REGIMM((rs), BGEZL, (offset))
#define MIPS_BGTZ(rs,offset)    __MIPS_INSN_IMM(BGTZ, (rs), 0, (offset))
#define MIPS_BGTZL(rs,offset)   __MIPS_INSN_IMM(BGTZL, (rs), 0, (offset))
#define MIPS_BLEZ(rs,offset)    __MIPS_INSN_IMM(BLEZ, (rs), 0, (offset))
#define MIPS_BLEZL(rs,offset)   __MIPS_INSN_IMM(BLEZL, (rs), 0, (offset))
#define MIPS_BLTZ(rs,offset)    __MIPS_REGIMM((rs), BLTZ, (offset))
#define MIPS_BLTZAL(rs,offset)  __MIPS_REGIMM((rs), BLTZAL, (offset))
#define MIPS_BLTZALL(rs,offset) __MIPS_REGIMM((rs), BLTZALL, (offset))
#define MIPS_BLTZL(rs,offset)   __MIPS_REGIMM((rs), BLTZL, (offset))
#define MIPS_BNE(rs,rt,offset)  __MIPS_INSN_IMM(BNE, (rs), (rt),  (offset))
#define MIPS_BNEL(rs,rt,offset) __MIPS_INSN_IMM(BNEL, (rs), (rt),  (offset))

#define MIPS_DIV(rs,rt)         __MIPS_SPECIAL((rs), (rt), 0, 0, DIV)
#define MIPS_DIVU(rs,rt)        __MIPS_SPECIAL((rs), (rt), 0, 0, DIVU)

#define MIPS_EXT(rt,rs,first,count) \
	__MIPS_ALLEGREX((rs), (rt), (count)-1, (first), EXT)

#define MIPS_INS(rt,rs,first,count) \
	__MIPS_ALLEGREX((rs), (rt), (first)+(count)-1, (first), INS)

#define MIPS_J(target)          __MIPS_INSN_JUMP(J, (target))
#define MIPS_JAL(target)        __MIPS_INSN_JUMP(JAL, (target))
#define MIPS_JALR(rs)           __MIPS_SPECIAL((rs), 0, MIPS_ra, 0, JALR)
#define MIPS_JR(rs)             __MIPS_SPECIAL((rs), 0, 0, 0, JR)

#define MIPS_LB(rt,offset,base) __MIPS_INSN_IMM(LB, (base), (rt), (offset))
#define MIPS_LBU(rt,offset,base) __MIPS_INSN_IMM(LBU, (base), (rt), (offset))
#define MIPS_LH(rt,offset,base) __MIPS_INSN_IMM(LH, (base), (rt), (offset))
#define MIPS_LHU(rt,offset,base) __MIPS_INSN_IMM(LHU, (base), (rt), (offset))
#define MIPS_LUI(rt,imm)        __MIPS_INSN_IMM(LUI, 0, (rt), (imm))
#define MIPS_LW(rt,offset,base) __MIPS_INSN_IMM(LW, (base), (rt), (offset))
#define MIPS_LWL(rt,offset,base) __MIPS_INSN_IMM(LWL, (base), (rt), (offset))
#define MIPS_LWR(rt,offset,base) __MIPS_INSN_IMM(LWR, (base), (rt), (offset))

#define MIPS_MFHI(rd)           __MIPS_SPECIAL(0, 0, (rd), 0, MFHI)
#define MIPS_MFLO(rd)           __MIPS_SPECIAL(0, 0, (rd), 0, MFLO)
#define MIPS_MOVN(rd,rs,rt)     __MIPS_SPECIAL((rs), (rt), (rd), 0, MOVN)
#define MIPS_MOVZ(rd,rs,rt)     __MIPS_SPECIAL((rs), (rt), (rd), 0, MOVZ)
#define MIPS_MTHI(rs)           __MIPS_SPECIAL((rs), 0, 0, 0, MTHI)
#define MIPS_MTLO(rs)           __MIPS_SPECIAL((rs), 0, 0, 0, MTLO)
#define MIPS_MULT(rs,rt)        __MIPS_SPECIAL((rs), (rt), 0, 0, MULT)
#define MIPS_MULTU(rs,rt)       __MIPS_SPECIAL((rs), (rt), 0, 0, MULTU)

#define MIPS_NOR(rd,rs,rt)      __MIPS_SPECIAL((rs), (rt), (rd), 0, NOR)

#define MIPS_OR(rd,rs,rt)       __MIPS_SPECIAL((rs), (rt), (rd), 0, OR)
#define MIPS_ORI(rt,rs,imm)     __MIPS_INSN_IMM(ORI, (rs), (rt), (imm))

#define MIPS_SB(rt,offset,base) __MIPS_INSN_IMM(SB, (base), (rt), (offset))
#define MIPS_SEB(rd,rt)         __MIPS_ALLEGREX(0, (rt), (rd), 16, SEX)
#define MIPS_SEH(rd,rt)         __MIPS_ALLEGREX(0, (rt), (rd), 24, SEX)
#define MIPS_SH(rt,offset,base) __MIPS_INSN_IMM(SH, (base), (rt), (offset))
#define MIPS_SLL(rd,rt,sa)      __MIPS_SPECIAL(0, (rt), (rd), (sa), SLL)
#define MIPS_SLLV(rd,rt,rs)     __MIPS_SPECIAL((rs), (rt), (rd), 0, SLLV)
#define MIPS_SLT(rd,rs,rt)      __MIPS_SPECIAL((rs), (rt), (rd), 0, SLT)
#define MIPS_SLTI(rt,rs,imm)    __MIPS_INSN_IMM(SLTI, (rs), (rt), (imm))
#define MIPS_SLTIU(rt,rs,imm)   __MIPS_INSN_IMM(SLTIU, (rs), (rt), (imm))
#define MIPS_SLTU(rd,rs,rt)     __MIPS_SPECIAL((rs), (rt), (rd), 0, SLTU)
#define MIPS_SRA(rd,rt,sa)      __MIPS_SPECIAL(0, (rt), (rd), (sa), SRA)
#define MIPS_SRAV(rd,rt,rs)     __MIPS_SPECIAL((rs), (rt), (rd), 0, SRAV)
#define MIPS_SRL(rd,rt,sa)      __MIPS_SPECIAL(0, (rt), (rd), (sa), SRL)
#define MIPS_SRLV(rd,rt,rs)     __MIPS_SPECIAL((rs), (rt), (rd), 0, SRLV)
#define MIPS_SUB(rd,rs,rt)      __MIPS_SPECIAL((rs), (rt), (rd), 0, SUB)
#define MIPS_SUBU(rd,rs,rt)     __MIPS_SPECIAL((rs), (rt), (rd), 0, SUBU)
#define MIPS_SW(rt,offset,base) __MIPS_INSN_IMM(SW, (base), (rt), (offset))
#define MIPS_SWL(rt,offset,base) __MIPS_INSN_IMM(SWL, (base), (rt), (offset))
#define MIPS_SWR(rt,offset,base) __MIPS_INSN_IMM(SWR, (base), (rt), (offset))
#define MIPS_ROTR(rd,rt,sa)      __MIPS_SPECIAL(1, (rt), (rd), (sa), SRL)
#define MIPS_ROTRV(rd,rt,rs)     __MIPS_SPECIAL((rs), (rt), (rd), 1, SRLV)

#define MIPS_XOR(rd,rs,rt)      __MIPS_SPECIAL((rs), (rt), (rd), 0, XOR)
#define MIPS_XORI(rt,rs,imm)    __MIPS_INSN_IMM(XORI, (rs), (rt), (imm))

/*-----------------------------------------------------------------------*/

/* Macro instructions */

#define MIPS_B(offset)          MIPS_BEQ(MIPS_zero, MIPS_zero, (offset))
#define MIPS_BAL(offset)        MIPS_BGEZAL(MIPS_zero, (offset))
#define MIPS_BEQZ(rs,offset)    MIPS_BEQ((rs), MIPS_zero, (offset))
#define MIPS_BEQZL(rs,offset)   MIPS_BEQL((rs), MIPS_zero, (offset))
#define MIPS_BNEZ(rs,offset)    MIPS_BNE((rs), MIPS_zero, (offset))
#define MIPS_BNEZL(rs,offset)   MIPS_BNEL((rs), MIPS_zero, (offset))

#define MIPS_MOVE(rd,rs)        MIPS_ADDU((rd), (rs), 0)

#define MIPS_NEG(rd,rs)         MIPS_SUBU((rd), MIPS_zero, (rs))
#define MIPS_NOP()              MIPS_SLL(MIPS_zero, MIPS_zero, 0)  // 0x0000000
#define MIPS_NOT(rd,rs)         MIPS_NOR((rd), (rs), MIPS_zero)

/*************************************************************************/

#define is_offset_24(val) \
	((val) >= (int)0xff000000 && (val) <= 0x00ffffff)

#define emith_call(target) \
	emith_xjump(target, 1)

#define emith_jump(target) \
	emith_xjump(target, 0)

static int emith_xjump(void *target, int is_call)
{
	int val = (u32 *)target - (u32 *)tcache_ptr - 2;
	int direct = is_offset_24(val);
	u32 *start_ptr = (u32 *)tcache_ptr;

	if (direct)
	{
		if(is_call) {
			//MIPS_ADDIU(MIPS_sp,MIPS_sp,-12);
			//MIPS_SW(MIPS_ra,8,MIPS_sp);
			//MIPS_SW(MIPS_a1,4,MIPS_sp);
			//MIPS_SW(MIPS_a0,0,MIPS_sp);

//			MIPS_ADDIU(MIPS_sp, MIPS_sp, -72);
//			MIPS_SW(MIPS_ra, 68,MIPS_sp);
//			MIPS_SW(MIPS_s7, 64,MIPS_sp);
//			MIPS_SW(MIPS_s6, 60,MIPS_sp);
//			MIPS_SW(MIPS_s5, 56,MIPS_sp);
//			MIPS_SW(MIPS_s4, 52,MIPS_sp);
//			MIPS_SW(MIPS_s3, 48,MIPS_sp);
//			MIPS_SW(MIPS_s2, 44,MIPS_sp);
//			MIPS_SW(MIPS_s1, 40,MIPS_sp);
//			MIPS_SW(MIPS_t9, 36,MIPS_sp);
//			MIPS_SW(MIPS_t8, 32,MIPS_sp);
//			MIPS_SW(MIPS_t7, 28,MIPS_sp);
//			MIPS_SW(MIPS_t6, 24,MIPS_sp);
//			MIPS_SW(MIPS_t5, 20,MIPS_sp);
//			MIPS_SW(MIPS_t4, 16,MIPS_sp);
//			MIPS_SW(MIPS_a3, 12,MIPS_sp);
//			MIPS_SW(MIPS_a2, 8,MIPS_sp);
//		  	MIPS_SW(MIPS_a1, 4,MIPS_sp);
//		  	MIPS_SW(MIPS_a0, 0,MIPS_sp);
			MIPS_JAL(((uint32_t)target & 0x0FFFFFFC) >> 2);            // jal target
			MIPS_NOP();
//		  	MIPS_LW(MIPS_a0, 0,MIPS_sp);
//		  	MIPS_LW(MIPS_a1, 4,MIPS_sp);
//		  	MIPS_LW(MIPS_a2, 8,MIPS_sp);
//		  	MIPS_LW(MIPS_a3, 12,MIPS_sp);
//		  	MIPS_LW(MIPS_t4, 16,MIPS_sp);
//		  	MIPS_LW(MIPS_t5, 20,MIPS_sp);
//		  	MIPS_LW(MIPS_t6, 24,MIPS_sp);
//		  	MIPS_LW(MIPS_t7, 28,MIPS_sp);
//		  	MIPS_LW(MIPS_t8, 32,MIPS_sp);
//		  	MIPS_LW(MIPS_t9, 36,MIPS_sp);
//		  	MIPS_LW(MIPS_s1, 40,MIPS_sp);
//		  	MIPS_LW(MIPS_s2, 44,MIPS_sp);
//		  	MIPS_LW(MIPS_s3, 48,MIPS_sp);
//		  	MIPS_LW(MIPS_s4, 52,MIPS_sp);
//		  	MIPS_LW(MIPS_s5, 56,MIPS_sp);
//		  	MIPS_LW(MIPS_s6, 60,MIPS_sp);
//		  	MIPS_LW(MIPS_s7, 64,MIPS_sp);
//		  	MIPS_LW(MIPS_ra, 68,MIPS_sp);
//		  	MIPS_ADDIU(MIPS_sp, MIPS_sp, 72);

			//MIPS_LW(MIPS_a0,0,MIPS_sp);
			//MIPS_LW(MIPS_a1,4,MIPS_sp);
			//MIPS_LW(MIPS_ra,8,MIPS_sp);
			//MIPS_ADDIU(MIPS_sp,MIPS_sp,12);
		}
		else {
			MIPS_J(((uint32_t)target & 0x0FFFFFFC) >> 2);              // j target
			MIPS_NOP();
		}
	}
	else
	{
#ifdef __EPOC32__
//		elprintf(EL_SVP, "emitting indirect jmp %08x->%08x", tcache_ptr, target);
		if (is_call)
			EOP_ADD_IMM(14,15,0,8);			// add lr,pc,#8
		EOP_C_AM2_IMM(cond,1,0,1,15,15,0);		// ldrcc pc,[pc]
		EOP_MOV_REG_SIMPLE(15,15);			// mov pc, pc
		EMIT((u32)target);
#else
		// should never happen
		printf(EL_STATUS|EL_SVP|EL_ANOMALY, "indirect jmp %08x->%08x", target, tcache_ptr);
		exit(1);
#endif
	}

	return (u32 *)tcache_ptr - start_ptr;
}

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */

