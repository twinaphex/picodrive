
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
#define CONTEXT_REG 20

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

#define emith_call_s(target) {\
	MIPS_ADDIU(MIPS_sp, MIPS_sp, -4); \
	MIPS_SW(MIPS_ra, 0,MIPS_sp); \
	emith_call(target); \
	MIPS_LW(MIPS_ra, 0,MIPS_sp); \
	MIPS_ADDIU(MIPS_sp, MIPS_sp, 4); \
}

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
			MIPS_JAL(((uint32_t)target & 0x0FFFFFFC) >> 2);            // jal target
			MIPS_NOP();
		}
		else {
			MIPS_J(((uint32_t)target & 0x0FFFFFFC) >> 2);              // j target
			MIPS_NOP();
		}
	}
	else
	{
		// should never happen
		printf(EL_STATUS|EL_SVP|EL_ANOMALY, "indirect jmp %08x->%08x", target, tcache_ptr);
		exit(1);
	}

	return (u32 *)tcache_ptr - start_ptr;
}


// arm conditions
#define A_COND_AL 0xe
#define A_COND_EQ 0x0
#define A_COND_NE 0x1
#define A_COND_HS 0x2
#define A_COND_LO 0x3
#define A_COND_MI 0x4
#define A_COND_PL 0x5
#define A_COND_VS 0x6
#define A_COND_VC 0x7
#define A_COND_HI 0x8
#define A_COND_LS 0x9
#define A_COND_GE 0xa
#define A_COND_LT 0xb
#define A_COND_GT 0xc
#define A_COND_LE 0xd
#define A_COND_CS A_COND_HS
#define A_COND_CC A_COND_LO

/* unified conditions */
#define DCOND_EQ A_COND_EQ
#define DCOND_NE A_COND_NE
#define DCOND_MI A_COND_MI
#define DCOND_PL A_COND_PL
#define DCOND_HI A_COND_HI
#define DCOND_HS A_COND_HS
#define DCOND_LO A_COND_LO
#define DCOND_GE A_COND_GE
#define DCOND_GT A_COND_GT
#define DCOND_LT A_COND_LT
#define DCOND_LS A_COND_LS
#define DCOND_LE A_COND_LE
#define DCOND_VS A_COND_VS
#define DCOND_VC A_COND_VC

// fake "simple" or "short" jump - using cond insns instead
#define EMITH_NOTHING1(cond) \
	(void)(cond)

#define EMITH_SJMP_START(cond)	EMITH_NOTHING1(cond)
#define EMITH_SJMP_END(cond)	EMITH_NOTHING1(cond)
#define EMITH_SJMP3_START(cond)	EMITH_NOTHING1(cond)
#define EMITH_SJMP3_MID(cond)	EMITH_NOTHING1(cond)
#define EMITH_SJMP3_END()

#define A_OP_MOV 050
#define A_OP_MVN 051
#define A_OP_BIC 052
#define EOP_STMIA 053
#define EOP_LDMIA 054

#define emith_jump_patch(ptr, target) do { \
	u32 *ptr_ = ptr; \
	u32 val_ = (u32 *)(target) - ptr_ - 2; \
	*ptr_ = (*ptr_ & 0xff000000) | (val_ & 0x00ffffff); \
} while (0)

#define emith_move_r_imm_c(cond, r, imm) {\
	switch(cond) {						\
		case A_COND_AL: break;			\
		case A_COND_EQ: MIPS_BNEZ(MIPS_s5,2);MIPS_NOP();break;	\
		case A_COND_NE: MIPS_BEQZ(MIPS_s5,2);MIPS_NOP();break;	\
		case A_COND_LT:											\
		case A_COND_MI: MIPS_BGEZ(MIPS_s5,2);MIPS_NOP();break;	\
		case A_COND_PL: MIPS_BLTZ(MIPS_s5,2);MIPS_NOP();break;	\
		case A_COND_LE: MIPS_BGTZ(MIPS_s5,2);MIPS_NOP();break;	\
		case A_COND_GT: MIPS_BLEZ(MIPS_s5,2);MIPS_NOP();break;	\
		default: break;    										\
	}															\
	emith_move_r_imm(r, imm);									\
}

//void emith_move_r_imm(int r, int imm) {
//	MIPS_LUI(r, imm>>16);
//	MIPS_ORI(r,r,imm);
//}

#define emith_move_r_imm(r, imm) {\
	MIPS_LUI(r, imm>>16);  \
	MIPS_ORI(r,r,imm);		\
}

#define emith_move_r_r(d, s) \
	MIPS_MOVE(d,s);				\

#define emith_mvn_r_r(d, s) \
	MIPS_NOT(d,s);				\

#define emith_neg_r_r(d, s) \
	MIPS_SUBU(d,0,s);			\

#define emith_lsl(d, s, cnt) \
	MIPS_SLL(d,s,cnt);			\

#define emith_lsr(d, s, cnt) \
	MIPS_SRL(d,s,cnt);			\

#define emith_asr(d, s, cnt) \
	MIPS_SRA(d,s,cnt);			\

#define emith_lslf(d, s, cnt) { \
	MIPS_SLL(d,s,cnt);			\
	MIPS_MOVE(MIPS_s5,d);			\
}

#define emith_lsrf(d, s, cnt) { \
	MIPS_SRL(d,s,cnt);			\
	MIPS_MOVE(MIPS_s5,d);			\
}

#define emith_asrf(d, s, cnt) { \
	MIPS_SRA(d,s,cnt);			\
	MIPS_MOVE(MIPS_s5,d);			\
}

#define emith_rorf(d, s, cnt) { \
	MIPS_ROTR(d,s,cnt);			\
	MIPS_MOVE(MIPS_s5,d);			\
}

#define emith_rolcf(d) \
	emith_adcf_r_r(d, d)

#define emith_rorcf(d) \
	MIPS_AND(MIPS_s5,d,d); /* ROR #0 -> RRX */	\

// note: only C flag updated correctly
#define emith_rolf(d, s, cnt) { \
	MIPS_ROTR(d,s,32-(cnt));			\
	/* we don't have ROL so we shift to get the right carry */ \
	MIPS_SRL(MIPS_s6,d,1);				\
	MIPS_AND(MIPS_s5,d,MIPS_s5);		\
}

#define emith_adcf_r_r(d, s) {\
	emith_add_r_r_r(d, d, s);		\
	MIPS_MOVE(MIPS_s5,d);			\
} //TODO: verificar se adc é addu

#define emith_addf_r_r(d, s) {\
	emith_add_r_r_r(d, d, s);		\
	MIPS_MOVE(MIPS_s5,d);			\
}

#define emith_subf_r_r(d, s) {\
	MIPS_SUBU(d, d, s);		\
	MIPS_MOVE(MIPS_s5,d);			\
}

#define emith_negcf_r_r(d, s) {\
	MIPS_SUBU(d, 0, s);		\
	MIPS_MOVE(MIPS_s5,d);			\
}

#define emith_sbcf_r_r(d, s) {\
	MIPS_SUBU(d, d, s);		\
	MIPS_MOVE(MIPS_s5,d);			\
}

#define emith_eorf_r_r(d, s) {\
	MIPS_XOR(d, d, s);		\
	MIPS_MOVE(MIPS_s5,d);			\
}

#define emith_ror_c(cond, d, s, cnt) { \
	switch(cond) {						\
		case A_COND_AL: break;			\
		case A_COND_EQ: MIPS_BNEZ(MIPS_s5,2);MIPS_NOP();break;	\
		case A_COND_NE: MIPS_BEQZ(MIPS_s5,2);MIPS_NOP();break;	\
		case A_COND_LT:											\
		case A_COND_MI: MIPS_BGEZ(MIPS_s5,2);MIPS_NOP();break;	\
		case A_COND_PL: MIPS_BLTZ(MIPS_s5,2);MIPS_NOP();break;	\
		case A_COND_LE: MIPS_BGTZ(MIPS_s5,2);MIPS_NOP();break;	\
		case A_COND_GT: MIPS_BLEZ(MIPS_s5,2);MIPS_NOP();break;	\
		default: break;    										\
	}															\
	MIPS_ROTR(d,s,cnt);											\
}

static unsigned int _rotr(const unsigned int value, int shift) {
    if ((shift &= sizeof(value)*8 - 1) == 0)
      return value;
    return (value >> shift) | (value << (sizeof(value)*8 - shift));
}

#define __MIPS_INSN_IMM_(op,rs,rt,imm) \
	EMIT(op<<26 | ((rs)&0x1F)<<21 | ((rt)&0x1F)<<16 | ((imm)&0xFFFF))

static void EOP_C_DOP_IMM(int cond, int op, int s, int rn, int rd, int ror2, unsigned int imm) {
	switch(cond) {
		case A_COND_AL: break;
		case A_COND_EQ: MIPS_BNEZ(MIPS_s5,2);MIPS_NOP();break;
		case A_COND_NE: MIPS_BEQZ(MIPS_s5,2);MIPS_NOP();break;
		case A_COND_LT:
		case A_COND_MI: MIPS_BGEZ(MIPS_s5,2);MIPS_NOP();break;
		case A_COND_PL: MIPS_BLTZ(MIPS_s5,2);MIPS_NOP();break;
		case A_COND_LE: MIPS_BGTZ(MIPS_s5,2);MIPS_NOP();break;
		case A_COND_GT: MIPS_BLEZ(MIPS_s5,2);MIPS_NOP();break;
		default: break;    // TODO: completar condições
	}

	int imm_;

	imm_ = _rotr(imm,ror2/2);
	__MIPS_INSN_IMM_(op, (rn), (rd), (imm_));
}

static void emith_op_imm2(int cond, int s, int op, int rd, int rn, int imm)
{
	int ror2;
	u32 v;

	switch (op) {
	case A_OP_MOV:
		rn = 0;
		if (~imm < 0x10000) {
			imm = ~imm;
			op = A_OP_MVN;
		}
		break;

	case __SP_XOR:
	case __SP_SUBU:
	case __SP_ADDU:
	case __SP_OR:
		if (s == 0 && imm == 0)
			return;
		break;
	}

	for (v = imm, ror2 = 0; ; ror2 -= 8/2) {
		/* shift down to get 'best' rot2 */
		for (; v && !(v & 3); v >>= 2)
			ror2--;

		EOP_C_DOP_IMM(cond, op, s, rn, rd, ror2 & 0x0f, v & 0xff);

		v >>= 8;
		if (v == 0)
			break;
		if (op == A_OP_MOV)
			op = __SP_OR;
		if (op == A_OP_MVN)
			op = A_OP_BIC;
		rn = rd;
	}
}

#define emith_ror(d, s, cnt) \
	emith_ror_c(A_COND_AL, d, s, cnt)

#define emith_rol(d, s, cnt) \
	emith_ror_c(A_COND_AL, d, s, 32-(cnt))

#define emith_and_r_r_imm(d, s, imm) \
	emith_op_imm2(A_COND_AL, 0, __OP_ANDI, d, s, imm)

#define emith_add_r_r_imm(d, s, imm) \
	emith_op_imm2(A_COND_AL, 0, __OP_ADDIU, d, s, imm)

#define emith_sub_r_r_imm(d, s, imm) \
	emith_op_imm2(A_COND_AL, 0, __OP_ADDIU, d, s, -imm)

#define emith_move_r_imm_s8(r, imm) { \
	if ((imm) & 0x80) \
		MIPS_NOT(r,((imm) ^ 0xff)); \
	else \
		MIPS_ADDIU(r, 0, imm); \
}

#define emith_add_r_imm(r, imm) \
	emith_op_imm(A_COND_AL, 0, __OP_ADDIU, r, imm)

//#define emith_adc_r_imm(r, imm) \
//	emith_op_imm(A_COND_AL, 0, A_OP_ADC, r, imm)

#define emith_sub_r_imm(r, imm) \
	emith_op_imm(A_COND_AL, 0, __OP_ADDIU, r, -imm)

#define emith_bic_r_imm(r, imm) { \
	int imm_;					\
	imm_ = ~imm;				\
	emith_op_imm(A_COND_AL, 0, __OP_ANDI, r, imm_);  \
}

#define emith_and_r_imm(r, imm) \
	emith_op_imm(A_COND_AL, 0, __OP_ANDI, r, imm)

#define emith_or_r_imm(r, imm) \
	emith_op_imm(A_COND_AL, 0, __OP_ORI, r, imm)

#define emith_eor_r_imm(r, imm) \
	emith_op_imm(A_COND_AL, 0, __OP_XORI, r, imm)

#define emith_tst_r_imm(r, imm) \
	MIPS_ANDI(MIPS_s5,r,imm)

#define emith_add_r_r_r_lsl(d, s1, s2, lslimm) { \
	MIPS_SLL(MIPS_s6,s2,lslimm);				\
	MIPS_ADDU(d,s1,MIPS_s6);					\
}

#define emith_or_r_r_r_lsl(d, s1, s2, lslimm) { \
	MIPS_SLL(MIPS_s6,s2,lslimm);				\
	MIPS_OR(d,s1,MIPS_s6);	    				\
}

#define emith_eor_r_r_r_lsl(d, s1, s2, lslimm) { \
	MIPS_SLL(MIPS_s6,s2,lslimm);				\
	MIPS_XOR(d,s1,MIPS_s6);	    				\
}

#define emith_eor_r_r_r_lsr(d, s1, s2, lsrimm) { \
	MIPS_SRL(MIPS_s6,s2,lsrimm);				\
	MIPS_XOR(d,s1,MIPS_s6);	    				\
}

#define emith_or_r_r_lsl(d, s, lslimm) \
	emith_or_r_r_r_lsl(d, d, s, lslimm)

#define emith_eor_r_r_lsr(d, s, lsrimm) \
	emith_eor_r_r_r_lsr(d, d, s, lsrimm)

#define emith_add_r_r_r(d, s1, s2) \
	MIPS_ADDU(d,s1,s2);				\

#define emith_or_r_r_r(d, s1, s2) \
	MIPS_OR(d,s1,s2);				\

#define emith_eor_r_r_r(d, s1, s2) \
	MIPS_XOR(d,s1,s2);				\

#define emith_mul(d, s1, s2) { \
	MIPS_MULT(s1,s2);     \
	MIPS_MFLO(d);             \
}

// TODO: verificar diferenças entre add e adc; sub e sbc

#define emith_add_r_r(d, s) \
	emith_add_r_r_r(d, d, s)

#define emith_sub_r_r(d, s) \
	MIPS_SUBU(d,d,s);			\

#define emith_adc_r_r(d, s) \
	emith_add_r_r_r(d, d, s)

#define emith_sbc_r_r(d, s) \
	MIPS_SUBU(d,d,s);			\

#define emith_and_r_r(d, s) \
	MIPS_AND(d,d,s);			\

#define emith_or_r_r(d, s) \
	MIPS_OR(d,d,s);				\

#define emith_eor_r_r(d, s) \
	MIPS_XOR(d,d,s);			\

#define emith_tst_r_r(d, s) \
	MIPS_AND(MIPS_s5,d,s);		\

#define emith_teq_r_r(d, s) \
	MIPS_XOR(MIPS_s5,d,s);		\

#define emith_cmp_r_r(d, s) \
	MIPS_SUBU(MIPS_s5,d,s);		\

#define emith_add_r_imm_c(cond, r, imm) \
	emith_op_imm(cond, 0, __OP_ADDIU, r, imm)

static void emith_sub_r_imm_c( int cond, int r, int imm) {
	emith_op_imm2( cond, 0, __OP_ADDIU, r, r, -imm);
}

//#define emith_sub_r_imm_c(cond, r, imm) \
//	emith_op_imm(cond, 0, __OP_ADDIU, r, -imm)

#define emith_or_r_imm_c(cond, r, imm) \
	emith_op_imm(cond, 0, __OP_ORI, r, imm)

#define emith_eor_r_imm_c(cond, r, imm) \
	emith_op_imm(cond, 0, __OP_XORI, r, imm)

#define emith_bic_r_imm_c(cond, r, imm) \
	int imm_;					\
	imm_ = ~imm;				\
	emith_op_imm(cond, 0, __OP_ANDI, r, imm_)

#define emith_subf_r_imm(r, imm) { \
	MIPS_ADDIU(r, r, -imm);		\
	MIPS_MOVE(MIPS_s5,r);			\
}

#define emith_ctx_write(r, offs) \
	MIPS_SW(r,offs,CONTEXT_REG);	\

#define emith_ctx_read(r, offs) \
	MIPS_LW(r,offs,CONTEXT_REG);	\

#define host_arg2reg(rd, arg) \
	rd = arg

#define emith_cmp_r_imm(r, imm) \
	MIPS_ADDIU(MIPS_s5,r,-imm);	\

//	u32 op = A_OP_CMP, imm_ = imm; \
//	if (~imm_ < 0x100) { \
//		imm_ = ~imm_; \
//		op = A_OP_CMN; \
//	} \
//	emith_top_imm(A_COND_AL, op, r, imm); \
//}

#define emith_call_ctx(offs) { \
	emith_move_r_r(14, 15); \
	emith_jump_ctx(offs); \
}

#define emith_jump_ctx(offs) \
	emith_jump_ctx_c(A_COND_AL, offs)

#define emith_jump_ctx_c(cond, offs) {\
	switch(cond) {						\
		case A_COND_AL: break;			\
		case A_COND_EQ: MIPS_BNEZ(MIPS_s5,2);MIPS_NOP();break;	\
		case A_COND_NE: MIPS_BEQZ(MIPS_s5,2);MIPS_NOP();break;	\
		case A_COND_LT:											\
		case A_COND_MI: MIPS_BGEZ(MIPS_s5,2);MIPS_NOP();break;	\
		case A_COND_PL: MIPS_BLTZ(MIPS_s5,2);MIPS_NOP();break;	\
		case A_COND_LE: MIPS_BGTZ(MIPS_s5,2);MIPS_NOP();break;	\
		case A_COND_GT: MIPS_BLEZ(MIPS_s5,2);MIPS_NOP();break;	\
		default: break;    										\
	}															\
	MIPS_LW(16,offs,CONTEXT_REG);								\
}

#define emith_jump_patchable(target) \
	emith_jump(target)

#define emith_jump_cond_patchable(cond, target) \
	emith_jump_cond(cond, target)

#define emith_jump_cond(cond, target) { \
	switch(cond) {						\
		case A_COND_AL: break;			\
		case A_COND_EQ: MIPS_BNEZ(MIPS_s5,3);MIPS_NOP();break;	\
		case A_COND_NE: MIPS_BEQZ(MIPS_s5,3);MIPS_NOP();break;	\
		case A_COND_LT:											\
		case A_COND_MI: MIPS_BGEZ(MIPS_s5,3);MIPS_NOP();break;	\
		case A_COND_PL: MIPS_BLTZ(MIPS_s5,3);MIPS_NOP();break;	\
		case A_COND_LE: MIPS_BGTZ(MIPS_s5,3);MIPS_NOP();break;	\
		case A_COND_GT: MIPS_BLEZ(MIPS_s5,3);MIPS_NOP();break;	\
		default: break;    										\
	}															\
	emith_xjump(target, 0);										\
}

#define emith_op_imm(cond, s, op, r, imm) \
	emith_op_imm2(cond, s, op, r, r, imm)

#define emith_sext(d, s, bits) { \
	MIPS_SLL(d,s,32 - (bits)); \
	MIPS_SRA(d,d,32 - (bits)); \
}

#define emith_jump_reg_c(cond, r) {\
	switch(cond) {						\
		case A_COND_AL: break;			\
		case A_COND_EQ: MIPS_BNEZ(MIPS_s5,3);MIPS_NOP();break;	\
		case A_COND_NE: MIPS_BEQZ(MIPS_s5,3);MIPS_NOP();break;	\
		case A_COND_LT:											\
		case A_COND_MI: MIPS_BGEZ(MIPS_s5,3);MIPS_NOP();break;	\
		case A_COND_PL: MIPS_BLTZ(MIPS_s5,3);MIPS_NOP();break;	\
		case A_COND_LE: MIPS_BGTZ(MIPS_s5,3);MIPS_NOP();break;	\
		case A_COND_GT: MIPS_BLEZ(MIPS_s5,3);MIPS_NOP();break;	\
		default: break;    										\
	}															\
	MIPS_JR(r);													\
	MIPS_NOP();													\
}

#define emith_jump_reg(r) \
	emith_jump_reg_c(A_COND_AL, r)

#define emith_ret_c(cond) \
	emith_jump_reg_c(cond, MIPS_ra)

static unsigned short arm_reg_to_mips( unsigned short arm_reg ) {
	unsigned short mips_reg = MIPS_zero;

	if( arm_reg <= 3 ) {
		mips_reg = arm_reg + 4;
	}

	else if( arm_reg <= 7 ) {
		mips_reg = arm_reg + 8;
	}

	else if( arm_reg <= 9 ) {
		mips_reg = arm_reg + 16;
	}

	else if( ( arm_reg >= 10 ) && ( arm_reg <= 12 ) ) {
		mips_reg = arm_reg + 7;
	}

	else if( ( arm_reg >= 14 ) && ( arm_reg <= 14 ) ) {
		mips_reg = arm_reg - 6;
	}

	if( mips_reg == MIPS_zero ) {
		mips_reg = -1;
	}

	return mips_reg;
}

#define emith_ctx_do_multiple(op, r, offs, count, tmpr) do { \
	int v_, r_ = r, c_ = count, b_ = CONTEXT_REG;        \
	for (v_ = 0; c_; c_--, r_++)                         \
		v_ |= 1 << r_;                               \
	if ((offs) != 0) {                                   \
		MIPS_ADDI(tmpr,CONTEXT_REG,offs);				\
		b_ = tmpr;                                   \
	}                                                    \
	int i;												\
	for(i = 0; i < count; i++) {						\
		if( op == EOP_LDMIA) {                          \
			MIPS_LW( arm_reg_to_mips(r+i),i*4,b_);		\
		}												\
		else {											\
			MIPS_SW( arm_reg_to_mips(r+i),i*4,b_);		\
		}												\
	}													\
} while(0)
//	op(b_,v_);                                           \ //	TODO: não terminado
//} while(0)

//#define emith_ctx_do_multiple(op, r, offs, count, tmpr) do { \
//	int v_, r_ = r, c_ = count, b_ = CONTEXT_REG;        \
//	for (v_ = 0; c_; c_--, r_++)                         \
//		v_ |= 1 << r_;                               \
//	if ((offs) != 0) {                                   \
//		emith_move_r_imm(MIPS_s6,offs);					\
//		MIPS_SLL(MIPS_s6,MIPS_s6,2);					\
//		MIPS_ROTR(MIPS_s6,MIPS_s6,30);					\
//		MIPS_ADD(tmpr,CONTEXT_REG,MIPS_s6);				\
//		b_ = tmpr;                                   \
//	}                                                    \
//} while(0)
//	//op(b_,v_);                                           \
////} while(0)

#define emith_ctx_read_multiple(r, offs, count, tmpr) \
	emith_ctx_do_multiple(EOP_LDMIA, r, offs, count, tmpr)

#define emith_ctx_write_multiple(r, offs, count, tmpr) \
	emith_ctx_do_multiple(EOP_STMIA, r, offs, count, tmpr)

//TODO: verificar diferentes tipos de multiplicação em MIPS

#define emith_mul_u64(dlo, dhi, s1, s2) { \
	MIPS_MULT(s1,s2);     \
	MIPS_MFLO(dlo);             \
	MIPS_MFHI(dhi);             \
}

#define emith_mul_s64(dlo, dhi, s1, s2) { \
	MIPS_MULT(s1,s2);     \
	MIPS_MFLO(dlo);             \
	MIPS_MFHI(dhi);             \
}

#define emith_mula_s64(dlo, dhi, s1, s2) { \
	MIPS_MULT(s1,s2);     \
	MIPS_MFLO(dlo);             \
	MIPS_MFHI(dhi);             \
}

// misc
#define emith_read_r_r_offs_c(cond, r, rs, offs) {\
	switch(cond) {						\
		case A_COND_AL: break;			\
		case A_COND_EQ: MIPS_BNEZ(MIPS_s5,2);MIPS_NOP();break;	\
		case A_COND_NE: MIPS_BEQZ(MIPS_s5,2);MIPS_NOP();break;	\
		case A_COND_LT:											\
		case A_COND_MI: MIPS_BGEZ(MIPS_s5,2);MIPS_NOP();break;	\
		case A_COND_PL: MIPS_BLTZ(MIPS_s5,2);MIPS_NOP();break;	\
		case A_COND_LE: MIPS_BGTZ(MIPS_s5,2);MIPS_NOP();break;	\
		case A_COND_GT: MIPS_BLEZ(MIPS_s5,2);MIPS_NOP();break;	\
		default: break;    										\
	}															\
	MIPS_LW(r,offs,rs);											\
}

#define emith_read8_r_r_offs_c(cond, r, rs, offs) {\
	switch(cond) {						\
		case A_COND_AL: break;			\
		case A_COND_EQ: MIPS_BNEZ(MIPS_s5,2);MIPS_NOP();break;	\
		case A_COND_NE: MIPS_BEQZ(MIPS_s5,2);MIPS_NOP();break;	\
		case A_COND_LT:											\
		case A_COND_MI: MIPS_BGEZ(MIPS_s5,2);MIPS_NOP();break;	\
		case A_COND_PL: MIPS_BLTZ(MIPS_s5,2);MIPS_NOP();break;	\
		case A_COND_LE: MIPS_BGTZ(MIPS_s5,2);MIPS_NOP();break;	\
		case A_COND_GT: MIPS_BLEZ(MIPS_s5,2);MIPS_NOP();break;	\
		default: break;    										\
	}															\
	MIPS_LBU(r,offs,rs);										\
}

#define emith_read16_r_r_offs_c(cond, r, rs, offs) {\
	switch(cond) {						\
		case A_COND_AL: break;			\
		case A_COND_EQ: MIPS_BNEZ(MIPS_s5,2);MIPS_NOP();break;	\
		case A_COND_NE: MIPS_BEQZ(MIPS_s5,2);MIPS_NOP();break;	\
		case A_COND_LT:											\
		case A_COND_MI: MIPS_BGEZ(MIPS_s5,2);MIPS_NOP();break;	\
		case A_COND_PL: MIPS_BLTZ(MIPS_s5,2);MIPS_NOP();break;	\
		case A_COND_LE: MIPS_BGTZ(MIPS_s5,2);MIPS_NOP();break;	\
		case A_COND_GT: MIPS_BLEZ(MIPS_s5,2);MIPS_NOP();break;	\
		default: break;    										\
	}															\
	MIPS_LHU(r,offs,rs);										\
}

#define emith_read_r_r_offs(r, rs, offs) \
	emith_read_r_r_offs_c(A_COND_AL, r, rs, offs)

#define emith_read8_r_r_offs(r, rs, offs) \
	emith_read8_r_r_offs_c(A_COND_AL, r, rs, offs)

#define emith_read16_r_r_offs(r, rs, offs) \
	emith_read16_r_r_offs_c(A_COND_AL, r, rs, offs)

#define emith_write_sr(sr, srcr) { \
	emith_lsr(sr, sr, 10); \
	emith_or_r_r_r_lsl(sr, sr, srcr, 22); \
	emith_ror(sr, sr, 22); \
}

#define emith_clear_msb_c(cond, d, s, count) { \
	u32 t; \
	if ((count) <= 8) { \
		t = (count) - 8; \
		t = (0xff << t) & 0xff; \
		emith_move_r_imm(MIPS_s6,t); \
		MIPS_ROTR(MIPS_s6,MIPS_s6,8);	\
		MIPS_NOT(MIPS_s6,MIPS_s6);		\
		MIPS_AND(d,s,MIPS_s6);			\
		switch(cond) {						\
			case A_COND_AL: break;			\
			case A_COND_EQ: MIPS_BNEZ(MIPS_s5,6);MIPS_NOP();break;	\
			case A_COND_NE: MIPS_BEQZ(MIPS_s5,6);MIPS_NOP();break;	\
			case A_COND_LT:											\
			case A_COND_MI: MIPS_BGEZ(MIPS_s5,6);MIPS_NOP();break;	\
			case A_COND_PL: MIPS_BLTZ(MIPS_s5,6);MIPS_NOP();break;	\
			case A_COND_LE: MIPS_BGTZ(MIPS_s5,6);MIPS_NOP();break;	\
			case A_COND_GT: MIPS_BLEZ(MIPS_s5,6);MIPS_NOP();break;	\
			default: break;    										\
		}															\
		emith_move_r_imm(MIPS_s6,t); \
		MIPS_ROTR(MIPS_s6,MIPS_s6,8);	\
		MIPS_NOT(MIPS_s6,MIPS_s6);		\
		MIPS_AND(d,s,MIPS_s6);			\
	} else if ((count) >= 24) { \
		t = (count) - 24; \
		t = 0xff >> t; \
		MIPS_ANDI(d,s,t); \
		switch(cond) {						\
			case A_COND_AL: break;			\
			case A_COND_EQ: MIPS_BNEZ(MIPS_s5,2);MIPS_NOP();break;	\
			case A_COND_NE: MIPS_BEQZ(MIPS_s5,2);MIPS_NOP();break;	\
			case A_COND_LT:											\
			case A_COND_MI: MIPS_BGEZ(MIPS_s5,2);MIPS_NOP();break;	\
			case A_COND_PL: MIPS_BLTZ(MIPS_s5,2);MIPS_NOP();break;	\
			case A_COND_LE: MIPS_BGTZ(MIPS_s5,2);MIPS_NOP();break;	\
			case A_COND_GT: MIPS_BLEZ(MIPS_s5,2);MIPS_NOP();break;	\
			default: break;    										\
		}															\
		MIPS_ANDI(d,s,t); \
	} else { \
		switch(cond) {						\
			case A_COND_AL: break;			\
			case A_COND_EQ: MIPS_BNEZ(MIPS_s5,3);MIPS_NOP();break;	\
			case A_COND_NE: MIPS_BEQZ(MIPS_s5,3);MIPS_NOP();break;	\
			case A_COND_LT:											\
			case A_COND_MI: MIPS_BGEZ(MIPS_s5,3);MIPS_NOP();break;	\
			case A_COND_PL: MIPS_BLTZ(MIPS_s5,3);MIPS_NOP();break;	\
			case A_COND_LE: MIPS_BGTZ(MIPS_s5,3);MIPS_NOP();break;	\
			case A_COND_GT: MIPS_BLEZ(MIPS_s5,3);MIPS_NOP();break;	\
			default: break;    										\
		}															\
		MIPS_SLL(d,s,count); \
		MIPS_SRL(d,s,count); \
	} \
}

#define emith_clear_msb(d, s, count) \
	emith_clear_msb_c(A_COND_AL, d, s, count)

#define JMP_POS(ptr) \
	ptr = tcache_ptr; \
	tcache_ptr += sizeof(u32)

#define JMP_EMIT(cond, ptr) { \
	u32 val_ = (u32 *)tcache_ptr - (u32 *)(ptr) - 2; \
	EOP_C_B_PTR(ptr, cond, 0, val_ & 0xffffff); \
}

#define EMITH_JMP_START(cond) { \
	void *cond_ptr; \
	JMP_POS(cond_ptr)

#define EMITH_JMP_END(cond) \
	JMP_EMIT(cond, cond_ptr); \
}

#define EOP_C_B_PTR(ptr,cond,l,signed_immed_24) \
	EMIT_PTR(ptr,__OP_BEQ<<26 | ((MIPS_zero)&0x1F)<<21 | ((MIPS_zero)&0x1F)<<16 | ((signed_immed_24)&0xFFFF))

#define emith_tpop_carry(sr, is_sub) {  \
	if (is_sub)                     \
		emith_eor_r_imm(sr, 1); \
	emith_lsrf(sr, sr, 1);          \
}

#define emith_tpush_carry(sr, is_sub) { \
	emith_adc_r_r(sr, sr);          \
	if (is_sub)                     \
		emith_eor_r_imm(sr, 1); \
}


/* SH2 drc specific */
/* pushes r12 for eabi alignment */

static void emit_save_registers_(void) {
	MIPS_ADDIU(MIPS_sp, MIPS_sp, -88);
	MIPS_SW(MIPS_ra, 84,MIPS_sp);
	MIPS_SW(MIPS_s7, 80,MIPS_sp);
	MIPS_SW(MIPS_s6, 76,MIPS_sp);
	MIPS_SW(MIPS_s5, 72,MIPS_sp);
	MIPS_SW(MIPS_s4, 68,MIPS_sp);
	MIPS_SW(MIPS_s3, 64,MIPS_sp);
	MIPS_SW(MIPS_s2, 60,MIPS_sp);
	MIPS_SW(MIPS_s1, 56,MIPS_sp);
	MIPS_SW(MIPS_t9, 52,MIPS_sp);
	MIPS_SW(MIPS_t8, 48,MIPS_sp);
	MIPS_SW(MIPS_t7, 44,MIPS_sp);
	MIPS_SW(MIPS_t6, 40,MIPS_sp);
	MIPS_SW(MIPS_t5, 36,MIPS_sp);
	MIPS_SW(MIPS_t4, 32,MIPS_sp);
	MIPS_SW(MIPS_t3, 28,MIPS_sp);
	MIPS_SW(MIPS_t2, 24,MIPS_sp);
	MIPS_SW(MIPS_t1, 20,MIPS_sp);
	MIPS_SW(MIPS_t0, 16,MIPS_sp);
	MIPS_SW(MIPS_a3, 12,MIPS_sp);
	MIPS_SW(MIPS_a2, 8,MIPS_sp);
	MIPS_SW(MIPS_a1, 4,MIPS_sp);
	MIPS_SW(MIPS_a0, 0,MIPS_sp);
}

static void emit_restore_registers_(void) {
	MIPS_LW(MIPS_a0, 0,MIPS_sp);
	MIPS_LW(MIPS_a1, 4,MIPS_sp);
	MIPS_LW(MIPS_a2, 8,MIPS_sp);
	MIPS_LW(MIPS_a3, 12,MIPS_sp);
	MIPS_LW(MIPS_t0, 16,MIPS_sp);
	MIPS_LW(MIPS_t1, 20,MIPS_sp);
	MIPS_LW(MIPS_t2, 24,MIPS_sp);
	MIPS_LW(MIPS_t3, 28,MIPS_sp);
	MIPS_LW(MIPS_t4, 32,MIPS_sp);
	MIPS_LW(MIPS_t5, 36,MIPS_sp);
	MIPS_LW(MIPS_t6, 40,MIPS_sp);
	MIPS_LW(MIPS_t7, 44,MIPS_sp);
	MIPS_LW(MIPS_t8, 48,MIPS_sp);
	MIPS_LW(MIPS_t9, 52,MIPS_sp);
	MIPS_LW(MIPS_s1, 56,MIPS_sp);
	MIPS_LW(MIPS_s2, 60,MIPS_sp);
	MIPS_LW(MIPS_s3, 64,MIPS_sp);
	MIPS_LW(MIPS_s4, 68,MIPS_sp);
	MIPS_LW(MIPS_s5, 72,MIPS_sp);
	MIPS_LW(MIPS_s6, 76,MIPS_sp);
	MIPS_LW(MIPS_s7, 80,MIPS_sp);
	MIPS_LW(MIPS_ra, 84,MIPS_sp);
	MIPS_ADDIU(MIPS_sp, MIPS_sp, 88);
}

#define emith_sh2_drc_entry() {\
	emit_save_registers_(); \
}

#define emith_sh2_drc_exit() {\
	emit_restore_registers_(); \
	MIPS_JR(MIPS_ra);			\
	MIPS_NOP();					\
}

#define host_instructions_updated(base, end) \
	cache_flush_d_inval_i(base, end)

#define emith_sh2_wcall(a, tab) { \
	MIPS_SRL(12, a, SH2_WRITE_SHIFT); \
	MIPS_SLL(MIPS_s6,12,2);				\
	MIPS_ADDU(MIPS_s6,tab,MIPS_s6);		\
	MIPS_LW(12,0,MIPS_s6);				\
	emith_move_r_r(2, CONTEXT_REG); \
	emith_jump_reg(12); \
}

#define emith_sh2_div1_step(rn, rm, sr) {         \
	void *jmp0, *jmp1;                        \
	emith_tst_r_imm(sr, Q);  /* if (Q ^ M) */ \
	JMP_POS(jmp0);           /* beq do_sub */ \
	emith_addf_r_r(rn, rm);                   \
	emith_eor_r_imm_c(A_COND_CS, sr, T);      \
	JMP_POS(jmp1);           /* b done */     \
	JMP_EMIT(A_COND_EQ, jmp0); /* do_sub: */  \
	emith_subf_r_r(rn, rm);                   \
	emith_eor_r_imm_c(A_COND_CC, sr, T);      \
	JMP_EMIT(A_COND_AL, jmp1); /* done: */    \
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

