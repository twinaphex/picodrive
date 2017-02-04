/*
 * SSP1601 to MIPS recompiler
 * (C) notaz, 2008,2009,2010
 *
 * This work is licensed under the terms of MAME license.
 * See COPYING file in the top-level directory.
 */

#include "../../pico_int.h"
#include "../../../cpu/drc/cmn.h"
#include "compiler.h"

// FIXME: asm has these hardcoded
#define SSP_BLOCKTAB_ENTS       (0x5090/2)
#define SSP_BLOCKTAB_IRAM_ONE   (0x800/2) // table entries
#define SSP_BLOCKTAB_IRAM_ENTS  (15*SSP_BLOCKTAB_IRAM_ONE)

static u32 **ssp_block_table; // [0x5090/2];
static u32 **ssp_block_table_iram; // [15][0x800/2];

static u32 *tcache_ptr = NULL;

static int nblocks = 0;
static int n_in_ops = 0;

extern ssp1601_t *ssp;

#define rPC    ssp->gr[SSP_PC].h
#define rPMC   ssp->gr[SSP_PMC]

#define SSP_FLAG_Z (1<<0xd)
#define SSP_FLAG_N (1<<0xf)

#ifndef PSP
//#define DUMP_BLOCK 0x0c9a
void ssp_drc_next(void){}
void ssp_drc_next_patch(void){}
void ssp_drc_end(void){}
#endif

#define COUNT_OP
#include "../../../cpu/drc/emit_mips.c"

// -----------------------------------------------------

static int get_inc(int mode)
{
	int inc = (mode >> 11) & 7;
	if (inc != 0) {
		if (inc != 7) inc--;
		inc = 1 << inc; // 0 1 2 4 8 16 32 128
		if (mode & 0x8000) inc = -inc; // decrement mode
	}
	return inc;
}

u32 ssp_pm_read(int reg)
{
	u32 d = 0, mode;

	if (ssp->emu_status & SSP_PMC_SET)
	{
		ssp->pmac_read[reg] = rPMC.v;
		ssp->emu_status &= ~SSP_PMC_SET;
		return 0;
	}

	// just in case
	ssp->emu_status &= ~SSP_PMC_HAVE_ADDR;

	mode = ssp->pmac_read[reg]>>16;
	if      ((mode & 0xfff0) == 0x0800) // ROM
	{
		d = ((unsigned short *)Pico.rom)[ssp->pmac_read[reg]&0xfffff];
		ssp->pmac_read[reg] += 1;
	}
	else if ((mode & 0x47ff) == 0x0018) // DRAM
	{
		unsigned short *dram = (unsigned short *)svp->dram;
		int inc = get_inc(mode);
		d = dram[ssp->pmac_read[reg]&0xffff];
		ssp->pmac_read[reg] += inc;
	}

	// PMC value corresponds to last PMR accessed
	rPMC.v = ssp->pmac_read[reg];

	return d;
}

#define overwrite_write(dst, d) \
{ \
	if (d & 0xf000) { dst &= ~0xf000; dst |= d & 0xf000; } \
	if (d & 0x0f00) { dst &= ~0x0f00; dst |= d & 0x0f00; } \
	if (d & 0x00f0) { dst &= ~0x00f0; dst |= d & 0x00f0; } \
	if (d & 0x000f) { dst &= ~0x000f; dst |= d & 0x000f; } \
}

void ssp_pm_write(u32 d, int reg)
{
	unsigned short *dram;
	int mode, addr;

	if (ssp->emu_status & SSP_PMC_SET)
	{
		ssp->pmac_write[reg] = rPMC.v;
		ssp->emu_status &= ~SSP_PMC_SET;
		return;
	}

	// just in case
	ssp->emu_status &= ~SSP_PMC_HAVE_ADDR;

	dram = (unsigned short *)svp->dram;
	mode = ssp->pmac_write[reg]>>16;
	addr = ssp->pmac_write[reg]&0xffff;
	if      ((mode & 0x43ff) == 0x0018) // DRAM
	{
		int inc = get_inc(mode);
		if (mode & 0x0400) {
		       overwrite_write(dram[addr], d);
		} else dram[addr] = d;
		ssp->pmac_write[reg] += inc;
	}
	else if ((mode & 0xfbff) == 0x4018) // DRAM, cell inc
	{
		if (mode & 0x0400) {
		       overwrite_write(dram[addr], d);
		} else dram[addr] = d;
		ssp->pmac_write[reg] += (addr&1) ? 0x1f : 1;
	}
	else if ((mode & 0x47ff) == 0x001c) // IRAM
	{
		int inc = get_inc(mode);
		((unsigned short *)svp->iram_rom)[addr&0x3ff] = d;
		ssp->pmac_write[reg] += inc;
		ssp->drc.iram_dirty = 1;
	}

	rPMC.v = ssp->pmac_write[reg];
}


// -----------------------------------------------------

// 14 IRAM blocks
static unsigned char iram_context_map[] =
{
	 0, 0, 0, 0, 1, 0, 0, 0, // 04
	 0, 0, 0, 0, 0, 0, 2, 0, // 0e
	 0, 0, 0, 0, 0, 3, 0, 4, // 15 17
	 5, 0, 0, 6, 0, 7, 0, 0, // 18 1b 1d
	 8, 9, 0, 0, 0,10, 0, 0, // 20 21 25
	 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0,11, 0, 0,12, 0, 0, // 32 35
	13,14, 0, 0, 0, 0, 0, 0  // 38 39
};

int ssp_get_iram_context(void)
{
	unsigned char *ir = (unsigned char *)svp->iram_rom;
	int val1, val = ir[0x083^1] + ir[0x4FA^1] + ir[0x5F7^1] + ir[0x47B^1];
	val1 = iram_context_map[(val>>1)&0x3f];

	if (val1 == 0) {
		elprintf(EL_ANOMALY, "svp: iram ctx val: %02x PC=%04x\n", (val>>1)&0x3f, rPC);
		//debug_dump2file(name, svp->iram_rom, 0x800);
		//exit(1);
	}
	return val1;
}

// -----------------------------------------------------

/* regs with known values */
static struct
{
	ssp_reg_t gr[8];
	unsigned char r[8];
	unsigned int pmac_read[5];
	unsigned int pmac_write[5];
	ssp_reg_t pmc;
	unsigned int emu_status;
} known_regs;

#define KRREG_X     (1 << SSP_X)
#define KRREG_Y     (1 << SSP_Y)
#define KRREG_A     (1 << SSP_A)	/* AH only */
#define KRREG_ST    (1 << SSP_ST)
#define KRREG_STACK (1 << SSP_STACK)
#define KRREG_PC    (1 << SSP_PC)
#define KRREG_P     (1 << SSP_P)
#define KRREG_PR0   (1 << 8)
#define KRREG_PR4   (1 << 12)
#define KRREG_AL    (1 << 16)
#define KRREG_PMCM  (1 << 18)		/* only mode word of PMC */
#define KRREG_PMC   (1 << 19)
#define KRREG_PM0R  (1 << 20)
#define KRREG_PM1R  (1 << 21)
#define KRREG_PM2R  (1 << 22)
#define KRREG_PM3R  (1 << 23)
#define KRREG_PM4R  (1 << 24)
#define KRREG_PM0W  (1 << 25)
#define KRREG_PM1W  (1 << 26)
#define KRREG_PM2W  (1 << 27)
#define KRREG_PM3W  (1 << 28)
#define KRREG_PM4W  (1 << 29)

void emit_save_registers(void) {
	MIPS_ADDIU(MIPS_sp, MIPS_sp, -40);
	MIPS_SW(MIPS_ra, 36,MIPS_sp);
	MIPS_SW(MIPS_t9, 32,MIPS_sp);
	MIPS_SW(MIPS_t8, 28,MIPS_sp);
	MIPS_SW(MIPS_t7, 24,MIPS_sp);
	MIPS_SW(MIPS_t6, 20,MIPS_sp);
	MIPS_SW(MIPS_t5, 16,MIPS_sp);
	MIPS_SW(MIPS_t4, 12,MIPS_sp);
	MIPS_SW(MIPS_a3, 8,MIPS_sp);
	MIPS_SW(MIPS_a1, 4,MIPS_sp);
	MIPS_SW(MIPS_a0, 0,MIPS_sp);
}

void emit_restore_registers(void) {
	MIPS_LW(MIPS_a0, 0,MIPS_sp);
	MIPS_LW(MIPS_a1, 4,MIPS_sp);
	MIPS_LW(MIPS_a3, 8,MIPS_sp);
	MIPS_LW(MIPS_t4, 12,MIPS_sp);
	MIPS_LW(MIPS_t5, 16,MIPS_sp);
	MIPS_LW(MIPS_t6, 20,MIPS_sp);
	MIPS_LW(MIPS_t7, 24,MIPS_sp);
	MIPS_LW(MIPS_t8, 28,MIPS_sp);
	MIPS_LW(MIPS_t9, 32,MIPS_sp);
	MIPS_LW(MIPS_ra, 36,MIPS_sp);
	MIPS_ADDIU(MIPS_sp, MIPS_sp, 40);
}

#define check_generate_c_flag  0
#define check_generate_v_flag  0
#define check_generate_n_flag  1
#define check_generate_z_flag  1

#define reg_n_cache MIPS_t0
#define reg_z_cache MIPS_t1
#define reg_c_cache MIPS_t2
#define reg_v_cache MIPS_t3

#define generate_op_logic_flags(_rd)                                          \
	MIPS_MOVE(MIPS_s7,MIPS_zero);											\
  if(check_generate_n_flag)                                                   \
  {                                                                           \
	  MIPS_SRL(reg_n_cache, _rd, 31);                                      \
	  MIPS_BEQZ(reg_n_cache,3);													\
	  MIPS_NOP();																\
	  MIPS_LUI(MIPS_s6,0x8000);													\
	  MIPS_OR(MIPS_s7,MIPS_s7,MIPS_s6);											\
  }                                                                           \
  if(check_generate_z_flag)                                                   \
  {                                                                           \
	  MIPS_SLTIU(reg_z_cache, _rd, 1);                                     \
	  MIPS_BEQZ(reg_z_cache,3);													\
	  MIPS_NOP();																\
	  MIPS_LUI(MIPS_s6,0x4000);													\
	  MIPS_OR(MIPS_s7,MIPS_s7,MIPS_s6);											\
  }                                                                           \



#define generate_op_sub_flags_prologue(_rn, _rm)                              \
  if(check_generate_c_flag)                                                   \
  {                                                                           \
	  MIPS_SLTU(reg_c_cache, _rn, _rm);                                    \
	  MIPS_XORI(reg_c_cache, reg_c_cache, 1);                              \
  }                                                                           \
  if(check_generate_v_flag)                                                   \
  {                                                                           \
	  MIPS_SLT(reg_v_cache, _rn, _rm);                                     \
  }                                                                           \

#define generate_op_sub_flags_epilogue(_rd)                                   \
  generate_op_logic_flags(_rd);                                               \
  if(check_generate_v_flag)                                                   \
  {                                                                           \
    if(!check_generate_n_flag)                                                \
    {                                                                         \
    	MIPS_SRL(reg_n_cache, _rd, 31);                                    \
    }                                                                         \
    MIPS_XOR(reg_v_cache, reg_v_cache, reg_n_cache);                     \
  }                                                                           \
  if(check_generate_c_flag)                                                   \
  {                                                                           \
    MIPS_BEQZ(reg_c_cache,3);													\
    MIPS_NOP();																\
    MIPS_LUI(MIPS_s6,0x2000);													\
    MIPS_OR(MIPS_s7,MIPS_s7,MIPS_s6);											\
  }                                                                           \
  if(check_generate_v_flag)                                                   \
  {                                                                           \
    MIPS_BEQZ(reg_v_cache,9);													\
    MIPS_NOP();																\
    MIPS_LUI(MIPS_s6,0x1000);													\
    MIPS_OR(MIPS_s7,MIPS_s7,MIPS_s6);											\
    MIPS_BEQZ(reg_n_cache,4);													\
    MIPS_NOP();																\
    MIPS_ADDIU(MIPS_s5,MIPS_zero,1);											\
    MIPS_B(2);																\
    MIPS_NOP();																\
    MIPS_ADDIU(MIPS_s5,MIPS_zero,-1);											\
  }                                                                           \


#define generate_op_add_flags_prologue(_rn, _rm)                                 \
  if(check_generate_c_flag | check_generate_v_flag)                           \
  {                                                                           \
	  MIPS_ADDU(reg_c_cache, _rn, MIPS_zero);                                 \
  }                                                                           \
  if(check_generate_v_flag)                                                   \
  {                                                                           \
	  MIPS_SLT(reg_v_cache, _rn, MIPS_zero);                                \
  }                                                                           \

#define generate_op_add_flags_epilogue(_rd)                                      \
  if(check_generate_v_flag)                                                   \
  {                                                                           \
	  MIPS_SLT(MIPS_s6, _rd, reg_c_cache);                                    \
	  MIPS_XOR(reg_v_cache, reg_v_cache,MIPS_s6);                          \
  }                                                                           \
  if(check_generate_c_flag)                                                   \
  {                                                                           \
	  MIPS_SLTU(reg_c_cache, _rd, reg_c_cache);                            \
  }                                                                           \
  generate_op_logic_flags(_rd)												\
  if(check_generate_c_flag)                                                   \
  {                                                                           \
     MIPS_BEQZ(reg_c_cache,4);													\
     MIPS_NOP();																\
     MIPS_LUI(MIPS_s6,0x2000);													\
     MIPS_OR(MIPS_s7,MIPS_s7,MIPS_s6);											\
     MIPS_ADDIU(MIPS_s5,MIPS_zero,1);											\
}                                                                           \
if(check_generate_v_flag)                                                   \
{                                                                           \
     MIPS_BEQZ(reg_v_cache,3);													\
     MIPS_NOP();																\
     MIPS_LUI(MIPS_s6,0x1000);													\
     MIPS_OR(MIPS_s7,MIPS_s7,MIPS_s6);											\
}                                                                           \

//unsigned short arm_reg_to_mips( unsigned short arm_reg ) {
//	unsigned short mips_reg = MIPS_zero;
//
//	if( arm_reg <= 4 ) {
//		mips_reg = arm_reg + 4;
//	}
//
//	else if( arm_reg <= 7 ) {
//		mips_reg = arm_reg + 8;
//	}
//
//	else if( arm_reg <= 9 ) {
//		mips_reg = arm_reg + 16;
//	}
//
//	else if( ( arm_reg >= 10 ) && ( arm_reg <= 12 ) ) {
//		mips_reg = arm_reg + 7;
//	}
//
//	return mips_reg;
//}

/* bitfield of known register values */
static u32 known_regb = 0;

/* known vals, which need to be flushed
 * (only ST, P, r0-r7, PMCx, PMxR, PMxW)
 * ST means flags are being held in ARM PSR
 * P means that it needs to be recalculated
 */
static u32 dirty_regb = 0;

/* known values of host regs.
 * -1            - unknown
 * 000000-00ffff - 16bit value
 * 100000-10ffff - base reg (r7) + 16bit val
 * 0r0000        - means reg (low) eq gr[r].h, r != AL
 */
static int hostreg_r[4];

static void hostreg_clear(void)
{
	int i;
	for (i = 0; i < 4; i++)
		hostreg_r[i] = -1;
}

static void hostreg_sspreg_changed(int sspreg)
{
	int i;
	for (i = 0; i < 4; i++)
		if (hostreg_r[i] == (sspreg<<16)) hostreg_r[i] = -1;
}


#define PROGRAM(x)   ((unsigned short *)svp->iram_rom)[x]
#define PROGRAM_P(x) ((unsigned short *)svp->iram_rom + (x))

void tr_unhandled(void)
{
	//FILE *f = fopen("tcache.bin", "wb");
	//fwrite(tcache, 1, (tcache_ptr - tcache)*4, f);
	//fclose(f);
	elprintf(EL_ANOMALY, "unhandled @ %04x\n", known_regs.gr[SSP_PC].h<<1);
	//exit(1);
}

/* update P, if needed. Trashes r0 */
static void tr_flush_dirty_P(void)
{
	// TODO: const regs
	if (!(dirty_regb & KRREG_P)) return;
	MIPS_SRA(MIPS_s1,MIPS_t4,16);   // sra  $s1, $t4, 16
	MIPS_SLL(MIPS_a0,MIPS_t4,16);   // sll  $a0, $t4, 16
	MIPS_SRA(MIPS_a0,MIPS_a0,15);   // sra  $a0, $a0, 15
	MIPS_MULT(MIPS_a0,MIPS_s1);     // mult $a0, $s1
	MIPS_MFLO(MIPS_s1);             // mflo $s1
	dirty_regb &= ~KRREG_P;
	hostreg_r[0] = -1;
}

/* write dirty pr to host reg. Nothing is trashed */
static void tr_flush_dirty_pr(int r)
{
	int ror = 0, reg;

	if (!(dirty_regb & (1 << (r+8)))) return;

	switch (r&3) {
		case 0: ror =    0; break;
		case 1: ror = 24; break;
		case 2: ror = 16; break;
	}
	reg = (r < 4) ? 8 : 9;
	MIPS_ADDIU(MIPS_s6,MIPS_zero,0xff);
	MIPS_ROTR(MIPS_s6,MIPS_s6,ror);
	MIPS_NOT(MIPS_s6,MIPS_s6);
	MIPS_AND(arm_reg_to_mips(reg),arm_reg_to_mips(reg),MIPS_s6);
	if (known_regs.r[r] != 0){
		MIPS_ADDIU(MIPS_s6,MIPS_zero,known_regs.r[r]);
		MIPS_ROTR(MIPS_s6,MIPS_s6,ror);
		MIPS_OR(arm_reg_to_mips(reg),arm_reg_to_mips(reg),MIPS_s6);
	}
	dirty_regb &= ~(1 << (r+8));
}

/* write all dirty pr0-pr7 to host regs. Nothing is trashed */
static void tr_flush_dirty_prs(void)
{
	int i, ror = 0, reg;
	int dirty = dirty_regb >> 8;
	if ((dirty&7) == 7) {
		MIPS_LUI(MIPS_t8, ( known_regs.r[0]|(known_regs.r[1]<<8)|(known_regs.r[2]<<16) ) >> 16);
		MIPS_ORI(MIPS_t8,MIPS_t8, known_regs.r[0]|(known_regs.r[1]<<8)|(known_regs.r[2]<<16));
		dirty &= ~7;
	}
	if ((dirty&0x70) == 0x70) {
		MIPS_LUI(MIPS_t9, ( known_regs.r[4]|(known_regs.r[5]<<8)|(known_regs.r[6]<<16)) >> 16);
		MIPS_ORI(MIPS_t9,MIPS_t9, known_regs.r[4]|(known_regs.r[5]<<8)|(known_regs.r[6]<<16));
		dirty &= ~0x70;
	}
	/* r0-r7 */
	for (i = 0; dirty && i < 8; i++, dirty >>= 1)
	{
		if (!(dirty&1)) continue;
		switch (i&3) {
			case 0: ror =    0; break;
			case 1: ror = 24; break;
			case 2: ror = 16; break;
		}
		reg = (i < 4) ? 8 : 9;
		MIPS_ADDIU(MIPS_s6,MIPS_zero,0xff);
		MIPS_ROTR(MIPS_s6,MIPS_s6,ror);
		MIPS_NOT(MIPS_s6,MIPS_s6);
		MIPS_AND(arm_reg_to_mips(reg),arm_reg_to_mips(reg),MIPS_s6);
		if (known_regs.r[i] != 0) {
			MIPS_ADDIU(MIPS_s6,MIPS_zero,known_regs.r[i]);
			MIPS_ROTR(MIPS_s6,MIPS_s6,ror);
			MIPS_OR(arm_reg_to_mips(reg),arm_reg_to_mips(reg),MIPS_s6);
		}
	}
	dirty_regb &= ~0xff00;
}

/* write dirty pr and "forget" it. Nothing is trashed. */
static void tr_release_pr(int r)
{
	tr_flush_dirty_pr(r);
	known_regb &= ~(1 << (r+8));
}

/* flush ARM PSR to r6. Trashes r1 */
static void tr_flush_dirty_ST(void)
{
	if (!(dirty_regb & KRREG_ST)) return;

	MIPS_ADDIU(MIPS_s6,MIPS_zero,0x0f);
	MIPS_NOT(MIPS_s6,MIPS_s6);
	MIPS_AND(MIPS_t6,MIPS_t6,MIPS_s6);
	MIPS_MOVE(MIPS_a1,MIPS_s7);
	MIPS_SRL(MIPS_s6,MIPS_a1,28);
	MIPS_OR(MIPS_t6,MIPS_t6,MIPS_s6);
	dirty_regb &= ~KRREG_ST;
	hostreg_r[1] = -1;
}

/* inverse of above. Trashes r1 */
static void tr_make_dirty_ST(void)
{
	if (dirty_regb & KRREG_ST) return;
	if (known_regb & KRREG_ST) {
		int flags = 0;
		if (known_regs.gr[SSP_ST].h & SSP_FLAG_N) flags |= 8;
		if (known_regs.gr[SSP_ST].h & SSP_FLAG_Z) flags |= 4;
		MIPS_ADDIU(MIPS_s6,MIPS_zero,flags);
		MIPS_ROTR(MIPS_s7,MIPS_s6,4);
	} else {
		MIPS_SLL(MIPS_a1,MIPS_t6,28);
		MIPS_MOVE(MIPS_s7,MIPS_a1);
		hostreg_r[1] = -1;
	}
	dirty_regb |= KRREG_ST;
}

/* load 16bit val into host reg r0-r3. Nothing is trashed */
static void tr_mov16(int r, int val)
{
	if (hostreg_r[r] != val) {
		MIPS_ADDIU(arm_reg_to_mips(r),MIPS_zero, val);
		hostreg_r[r] = val;
	}
}

static void tr_mov16_cond(int cond, int r, int val)
{
	switch(cond) {
		case A_COND_AL: break;
		case A_COND_EQ: MIPS_BNEZ(MIPS_s5,2);MIPS_NOP();break;
		case A_COND_NE: MIPS_BEQZ(MIPS_s5,2);MIPS_NOP();break;
		case A_COND_MI: MIPS_BGEZ(MIPS_s5,2);MIPS_NOP();break;
		case A_COND_PL: MIPS_BLTZ(MIPS_s5,2);MIPS_NOP();break;
		default: break;
	}

	MIPS_ADDIU(arm_reg_to_mips(r),MIPS_zero, val);
	hostreg_r[r] = -1;
}

/* trashes r1 */
static void tr_flush_dirty_pmcrs(void)
{
	u32 i, val = (u32)-1;
	if (!(dirty_regb & 0x3ff80000)) return;

	if (dirty_regb & KRREG_PMC) {
		val = known_regs.pmc.v;
		MIPS_LUI(MIPS_a1, val>>16);
		MIPS_ORI(MIPS_a1,MIPS_a1, val);
		MIPS_SW(MIPS_a1,0x400+SSP_PMC*4,MIPS_t7);
		if (known_regs.emu_status & (SSP_PMC_SET|SSP_PMC_HAVE_ADDR)) {
			elprintf(EL_ANOMALY, "!! SSP_PMC_SET|SSP_PMC_HAVE_ADDR set on flush\n");
			tr_unhandled();
		}
	}
	for (i = 0; i < 5; i++)
	{
		if (dirty_regb & (1 << (20+i))) {
			if (val != known_regs.pmac_read[i]) {
				val = known_regs.pmac_read[i];
				MIPS_LUI(MIPS_a1, val>>16);
				MIPS_ORI(MIPS_a1,MIPS_a1, val);
			}
			MIPS_SW(MIPS_a1,0x454+i*4,MIPS_t7); // pmac_read
		}
		if (dirty_regb & (1 << (25+i))) {
			if (val != known_regs.pmac_write[i]) {
				val = known_regs.pmac_write[i];
				MIPS_LUI(MIPS_a1, val>>16);
				MIPS_ORI(MIPS_a1,MIPS_a1, val);
			}
			MIPS_SW(MIPS_a1,0x46c+i*4,MIPS_t7); // pmac_write
		}
	}
	dirty_regb &= ~0x3ff80000;
	hostreg_r[1] = -1;
}

/* read bank word to r0 (upper bits zero). Thrashes r1. */
static void tr_bank_read(int addr) /* word addr 0-0x1ff */
{
	int breg = 7;
	if (addr > 0x7f) {
		if (hostreg_r[1] != (0x100000|((addr&0x180)<<1))) {
			MIPS_ADDIU(MIPS_s6,MIPS_zero,(addr&0x180)<<1);
			MIPS_ADDU(MIPS_a1,MIPS_t7,MIPS_s6);      // add  $a1, $t7, ((op&0x180)<<1)
			hostreg_r[1] = 0x100000|((addr&0x180)<<1);
		}
		breg = 1;
	}
	MIPS_LHU(MIPS_a0,(addr&0x7f)<<1,arm_reg_to_mips(breg));  // lhu  $t0, (op&0x7f)<<1 ($t1)
	hostreg_r[0] = -1;
}

/* write r0 to bank. Trashes r1. */
static void tr_bank_write(int addr)
{
	int breg = 7;
	if (addr > 0x7f) {
		if (hostreg_r[1] != (0x100000|((addr&0x180)<<1))) {
			MIPS_ADDIU(MIPS_s6,MIPS_zero,(addr&0x180)<<1);
			MIPS_ADDU(MIPS_a1,MIPS_t7,MIPS_s6);      // add  $a1, $t7, ((op&0x180)<<1)
			hostreg_r[1] = 0x100000|((addr&0x180)<<1);
		}
		breg = 1;
	}
	MIPS_SH(MIPS_a0,(addr&0x7f)<<1,arm_reg_to_mips(breg)); // sh   $a0, (op&0x7f)<<1 ($a1)
}

/* handle RAM bank pointer modifiers. if need_modulo, trash r1-r3, else nothing */
static void tr_ptrr_mod(int r, int mod, int need_modulo, int count)
{
	int modulo_shift = -1;	/* unknown */

	if (mod == 0) return;

	if (!need_modulo || mod == 1) // +!
		modulo_shift = 8;
	else if (need_modulo && (known_regb & KRREG_ST)) {
		modulo_shift = known_regs.gr[SSP_ST].h & 7;
		if (modulo_shift == 0) modulo_shift = 8;
	}

	if (modulo_shift == -1)
	{
		int reg = (r < 4) ? 8 : 9;
		tr_release_pr(r);
		if (dirty_regb & KRREG_ST) {
			// avoid flushing ARM flags
			MIPS_ANDI(MIPS_a1,MIPS_t6,0x70);
			MIPS_ADDIU(MIPS_a1,MIPS_a1,-0x10);
			MIPS_ANDI(MIPS_a1,MIPS_a1,0x70);
			MIPS_ADDI(MIPS_a1,MIPS_a1,0x10);
		} else {
			MIPS_ANDI(MIPS_a1,MIPS_t6,0x70); // andi  $a1, $t6, 0x70
			MIPS_MOVE(MIPS_s5,MIPS_a1);
			MIPS_BNEZ(MIPS_s5,2);
			MIPS_NOP();
			MIPS_ADDIU(MIPS_a1,MIPS_zero,0x80); // li $a1, 0x80
		}

		MIPS_SRL(MIPS_a1,MIPS_a1,4); // srl $a1, $a1, 4
		MIPS_ADDIU(MIPS_s6,MIPS_zero,8);
		MIPS_SUBU(MIPS_a2,MIPS_s6,MIPS_a1); // sub $a2, 8, $a1
		MIPS_ADDIU(MIPS_s6,MIPS_zero,count);
		MIPS_ROTR(MIPS_a3,MIPS_s6,8);    // li $a3, 0x01000000
		if (r&3) {
			MIPS_ADDI(MIPS_a1,MIPS_a1,(r&3)*8); // add $a1, $a1, (r&3)*8
		}
		MIPS_ROTRV(arm_reg_to_mips(reg),arm_reg_to_mips(reg),MIPS_a1); // rotr $mreg, $mreg, $a1
		if (mod == 2){
			MIPS_SLLV(MIPS_s6,MIPS_a3,MIPS_a2);
			MIPS_SUBU(arm_reg_to_mips(reg),arm_reg_to_mips(reg),MIPS_s6); // sub $mreg, $mreg, 0x01000000 << $a2
		}
		else {
			MIPS_SLLV(MIPS_s6,MIPS_a3,MIPS_a2);
			MIPS_ADDU(arm_reg_to_mips(reg),arm_reg_to_mips(reg),MIPS_s6); // sub $mreg, $mreg, 0x01000000 << $a2
		}
		MIPS_ADDIU(MIPS_s6,MIPS_zero,32);
		MIPS_SUBU(MIPS_a1,MIPS_s6,MIPS_a1); // sub $a1, 32, $a1
		MIPS_ROTRV(arm_reg_to_mips(reg),arm_reg_to_mips(reg),MIPS_a1); // rotrv $mreg, $mreg, $a1
		hostreg_r[1] = hostreg_r[2] = hostreg_r[3] = -1;
	}
	else if (known_regb & (1 << (r + 8)))
	{
		int modulo = (1 << modulo_shift) - 1;
		if (mod == 2)
		     known_regs.r[r] = (known_regs.r[r] & ~modulo) | ((known_regs.r[r] - count) & modulo);
		else known_regs.r[r] = (known_regs.r[r] & ~modulo) | ((known_regs.r[r] + count) & modulo);
	}
	else
	{
		int reg = (r < 4) ? 8 : 9;
		int ror = ((r&3) + 1)*8 - (8 - modulo_shift);
		MIPS_ROTR(arm_reg_to_mips(reg),arm_reg_to_mips(reg),ror);
		// {add|sub} $mreg, $mreg, #1<<shift
		MIPS_ADDIU(MIPS_s6,MIPS_zero,count << (8 - modulo_shift));
		MIPS_ROTR(MIPS_s6,MIPS_s6,8);
		if(mod==2)
			MIPS_SUBU(arm_reg_to_mips(reg),arm_reg_to_mips(reg),MIPS_s6);
		else
			MIPS_ADDU(arm_reg_to_mips(reg),arm_reg_to_mips(reg),MIPS_s6);
		MIPS_ROTR(arm_reg_to_mips(reg),arm_reg_to_mips(reg),32-ror);
	}
}

/* handle writes r0 to (rX). Trashes r1.
 * fortunately we can ignore modulo increment modes for writes. */
static void tr_rX_write(int op)
{
	if ((op&3) == 3)
	{
		int mod = (op>>2) & 3; // direct addressing
		tr_bank_write((op & 0x100) + mod);
	}
	else
	{
		int r = (op&3) | ((op>>6)&4);
		if (known_regb & (1 << (r + 8))) {
			tr_bank_write((op&0x100) | known_regs.r[r]);
		} else {
			int reg = (r < 4) ? 8 : 9;
			int ror = ((4 - (r&3))*8) & 0x1f;
			MIPS_ADDIU(MIPS_s6,MIPS_zero,0xff);
			MIPS_ROTR(MIPS_s6,MIPS_s6,ror);
			MIPS_AND(MIPS_a1,arm_reg_to_mips(reg),MIPS_s6); // and $a1, $t{7,8}, <mask>
			if (r >= 4) {
				MIPS_ADDIU(MIPS_s6,MIPS_zero,1);
				MIPS_ROTR(MIPS_s6,MIPS_s6,((ror-8)&0x1f));
				MIPS_OR(MIPS_a1,MIPS_a1,MIPS_s6); // or $a1, $a1, 1<<shift
			}
			if (r&3) {
				MIPS_SRL(MIPS_s6,MIPS_a1,(r&3)*8-1);  // srl $s5, $a1, <lsr>
				MIPS_ADDU(MIPS_a1,MIPS_t7,MIPS_s6);  // add $a1, $t7, $s5
			}
			else {
				MIPS_SLL(MIPS_s6,MIPS_a1,1);  // sll $s5, $a1, 1
				MIPS_ADDU(MIPS_a1,MIPS_t7,MIPS_s6);  // add $a1, $t7, $s5
			}
			MIPS_SH(MIPS_a0,0,MIPS_a1);			// sh $a0, ($a1)
			hostreg_r[1] = -1;
		}
		tr_ptrr_mod(r, (op>>2) & 3, 0, 1);
	}
}

/* read (rX) to r0. Trashes r1-r3. */
static void tr_rX_read(int r, int mod)
{
	if ((r&3) == 3)
	{
		tr_bank_read(((r << 6) & 0x100) + mod); // direct addressing
	}
	else
	{
		if (known_regb & (1 << (r + 8))) {
			tr_bank_read(((r << 6) & 0x100) | known_regs.r[r]);
		} else {
			int reg = (r < 4) ? 8 : 9;
			int ror = ((4 - (r&3))*8) & 0x1f;
			MIPS_ADDIU(MIPS_s6,MIPS_zero,0xff);
			MIPS_ROTR(MIPS_s6,MIPS_s6,ror);
			MIPS_AND(MIPS_a1,arm_reg_to_mips(reg),MIPS_s6); // and $a1, $t{7,8}, <mask>
			if (r >= 4) {
				MIPS_ADDIU(MIPS_s6,MIPS_zero,1);
				MIPS_ROTR(MIPS_s6,MIPS_s6,((ror-8)&0x1f));
				MIPS_OR(MIPS_a1,MIPS_a1,MIPS_s6); // or $a1, $a1, 1<<shift
			}
			if (r&3) {
				MIPS_SRL(MIPS_s6,MIPS_a1,(r&3)*8-1);  // srl $s5, $a1, <lsr>
				MIPS_ADDU(MIPS_a1,MIPS_t7,MIPS_s6);  // add $a1, $t7, $s5
			}
			else {
				MIPS_SLL(MIPS_s6,MIPS_a1,1);  // sll $s5, $a1, 1
				MIPS_ADDU(MIPS_a1,MIPS_t7,MIPS_s6);  // add $a1, $t7, $s5
			}
			MIPS_LHU(MIPS_a0,0,MIPS_a1);		// lhu $t0, ($t1)
			hostreg_r[0] = hostreg_r[1] = -1;
		}
		tr_ptrr_mod(r, mod, 1, 1);
	}
}

/* read ((rX)) to r0. Trashes r1,r2. */
static void tr_rX_read2(int op)
{
	int r = (op&3) | ((op>>6)&4); // src

	if ((r&3) == 3) {
		tr_bank_read((op&0x100) | ((op>>2)&3));
	} else if (known_regb & (1 << (r+8))) {
		tr_bank_read((op&0x100) | known_regs.r[r]);
	} else {
		int reg = (r < 4) ? 8 : 9;
		int ror = ((4 - (r&3))*8) & 0x1f;
		MIPS_ADDIU(MIPS_s6,MIPS_zero,0xff);
		MIPS_ROTR(MIPS_s6,MIPS_s6,ror);
		MIPS_AND(MIPS_a1,arm_reg_to_mips(reg),MIPS_s6); // and $a1, $t{7,8}, <mask>
		if (r >= 4) {
			MIPS_ADDIU(MIPS_s6,MIPS_zero,1);
			MIPS_ROTR(MIPS_s6,MIPS_s6,((ror-8)&0x1f));
			MIPS_OR(MIPS_a1,MIPS_a1,MIPS_s6); // or $a1, $a1, 1<<shift
		}
		if (r&3) {
			MIPS_SRL(MIPS_s6,MIPS_a1,(r&3)*8-1);  // srl $s6, $a1, <lsr>
			MIPS_ADDU(MIPS_a1,MIPS_t7,MIPS_s6);  // add $a1, $t7, $s6
		}
		else {
			MIPS_SLL(MIPS_s6,MIPS_a1,1);  // sll $s5, $a1, 1
			MIPS_ADDU(MIPS_a1,MIPS_t7,MIPS_s6);  // add $a1, $t7, $s5
		}
		MIPS_LHU(MIPS_a0,0,MIPS_a1);		// lhu $a0, ($a1)
	}
	MIPS_LW(MIPS_a2,0x48c,MIPS_t7);        // ptr_iram_rom
	MIPS_SLL(MIPS_s6,MIPS_a0,1);            // sll $s6, $a0, 1
	MIPS_ADDU(MIPS_a2,MIPS_a2,MIPS_s6);      // add $a2, $a2, $s6
	MIPS_ADDI(MIPS_a0,MIPS_a0,1);			// addi $a0, $a0, 1
	if ((r&3) == 3) {
		tr_bank_write((op&0x100) | ((op>>2)&3));
	} else if (known_regb & (1 << (r+8))) {
		tr_bank_write((op&0x100) | known_regs.r[r]);
	} else {
		MIPS_SH(MIPS_a0,0,MIPS_a1);			// sh $a0, ($a1)
		hostreg_r[1] = -1;
	}
	MIPS_LHU(MIPS_a0,0,MIPS_a2);		    // lhu $a0, ($a2)
	hostreg_r[0] = hostreg_r[2] = -1;
}

// check if AL is going to be used later in block
static int tr_predict_al_need(void)
{
	int tmpv, tmpv2, op, pc = known_regs.gr[SSP_PC].h;

	while (1)
	{
		op = PROGRAM(pc);
		switch (op >> 9)
		{
			// ld d, s
			case 0x00:
				tmpv2 = (op >> 4) & 0xf; // dst
				tmpv  = op & 0xf; // src
				if ((tmpv2 == SSP_A && tmpv == SSP_P) || tmpv2 == SSP_AL) // ld A, P; ld AL, *
					return 0;
				break;

			// ld (ri), s
			case 0x02:
			// ld ri, s
			case 0x0a:
			// OP a, s
			case 0x10: case 0x30: case 0x40: case 0x60: case 0x70:
				tmpv  = op & 0xf; // src
				if (tmpv == SSP_AL) // OP *, AL
					return 1;
				break;

			case 0x04:
			case 0x06:
			case 0x14:
			case 0x34:
			case 0x44:
			case 0x64:
			case 0x74: pc++; break;

			// call cond, addr
			case 0x24:
			// bra cond, addr
			case 0x26:
			// mod cond, op
			case 0x48:
			// mpys?
			case 0x1b:
			// mpya (rj), (ri), b
			case 0x4b: return 1;

			// mld (rj), (ri), b
			case 0x5b: return 0; // cleared anyway

			// and A, *
			case 0x50:
				tmpv  = op & 0xf; // src
				if (tmpv == SSP_AL) return 1;
			case 0x51: case 0x53: case 0x54: case 0x55: case 0x59: case 0x5c:
				return 0;
		}
		pc++;
	}
}


/* get ARM cond which would mean that SSP cond is satisfied. No trash. */
static int tr_cond_check(int op)
{
	int f = (op & 0x100) >> 8;
	switch (op&0xf0) {
		case 0x00: return A_COND_AL;	/* always true */
		case 0x50:			/* Z matches f(?) bit */
			if (dirty_regb & KRREG_ST) return f ? A_COND_EQ : A_COND_NE;
			MIPS_ANDI(MIPS_s5,MIPS_t6,4);
			return f ? A_COND_NE : A_COND_EQ;
		case 0x70:			/* N matches f(?) bit */
			if (dirty_regb & KRREG_ST) return f ? A_COND_MI : A_COND_PL;
			MIPS_ANDI(MIPS_s5,MIPS_t6,8);
			return f ? A_COND_NE : A_COND_EQ;
		default:
			elprintf(EL_ANOMALY, "unimplemented cond?\n");
			tr_unhandled();
			return 0;
	}
}

static int tr_neg_cond(int cond)
{
	switch (cond) {
		case A_COND_AL: elprintf(EL_ANOMALY, "neg for AL?\n"); exit(1);
		case A_COND_EQ: return A_COND_NE;
		case A_COND_NE: return A_COND_EQ;
		case A_COND_MI: return A_COND_PL;
		case A_COND_PL: return A_COND_MI;
		default:        elprintf(EL_ANOMALY, "bad cond for neg\n"); exit(1);
	}
	return 0;
}

static int tr_aop_ssp2mips(int op)
{
	switch (op) {
		case 1: return __SP_SUBU;
		case 3: return -1; // arm cmp
		case 4: return __SP_ADDU;
		case 5: return __SP_AND;
		case 6: return __SP_OR;
		case 7: return __SP_XOR;
	}

	tr_unhandled();
	return 0;
}

#define emith_call_c_func emith_call

// -----------------------------------------------------

//@ r4:  XXYY
//@ r5:  A
//@ r6:  STACK and emu flags
//@ r7:  SSP context
//@ r10: P

// read general reg to r0. Trashes r1
static void tr_GR0_to_r0(int op)
{
	tr_mov16(0, 0xffff);
}

static void tr_X_to_r0(int op)
{
	if (hostreg_r[0] != (SSP_X<<16)) {
		MIPS_SRL(MIPS_a0,MIPS_t4,16); // srl  $a0, $t4, 16
		hostreg_r[0] = SSP_X<<16;
	}
}

static void tr_Y_to_r0(int op)
{
	if (hostreg_r[0] != (SSP_Y<<16)) {
		MIPS_MOVE(MIPS_a0,MIPS_t4);
		hostreg_r[0] = SSP_Y<<16;
	}
}

static void tr_A_to_r0(int op)
{
	if (hostreg_r[0] != (SSP_A<<16)) {
		MIPS_SRL(MIPS_a0,MIPS_t5,16); // srl  $a0, $t5, 16
		hostreg_r[0] = SSP_A<<16;
	}
}

static void tr_ST_to_r0(int op)
{
	// VR doesn't need much accuracy here..
	MIPS_SRL(MIPS_a0,MIPS_t6,4); // srl  $a0, $t6, 4
	MIPS_ANDI(MIPS_a0,MIPS_a0,0x67); // andi  $a0, $a0, 0x67
	hostreg_r[0] = -1;
}

static void tr_STACK_to_r0(int op)
{
	// 448
	MIPS_LUI(MIPS_s6,0x2000);
	MIPS_SUBU(MIPS_t6,MIPS_t6,MIPS_s6);  // sub  $t6, $t6, 1<<29
	MIPS_ADDIU(MIPS_s6,MIPS_zero,0x400);
	MIPS_ADDU(MIPS_a1,MIPS_t7,MIPS_s6);   // add  $a1, $t7, 0x400
	MIPS_ADDI(MIPS_a1,MIPS_a1,0x48);    // add  $a1, $a1, 0x048
	MIPS_SRL(MIPS_s6,MIPS_t6,28);       // srl	$s5, $t6, 28
	MIPS_ADDU(MIPS_a1,MIPS_a1,MIPS_s6);  // add  $a1, $a1, $s5
	MIPS_LHU(MIPS_a0,0,MIPS_a1);		// lhu  $a0, ($a1)
	hostreg_r[0] = hostreg_r[1] = -1;
}

static void tr_PC_to_r0(int op)
{
	tr_mov16(0, known_regs.gr[SSP_PC].h);
}

static void tr_P_to_r0(int op)
{
	tr_flush_dirty_P();
	MIPS_SRL(MIPS_a0,MIPS_s1,16);
	hostreg_r[0] = -1;
}

static void tr_AL_to_r0(int op)
{
	if (op == 0x000f) {
		if (known_regb & KRREG_PMC) {
			known_regs.emu_status &= ~(SSP_PMC_SET|SSP_PMC_HAVE_ADDR);
		} else {
			MIPS_LW(MIPS_a0,0x484,MIPS_t7);  // lw $a1, 0x484 ($t7)
			MIPS_ADDIU(MIPS_s6,MIPS_s6,SSP_PMC_SET|SSP_PMC_HAVE_ADDR);
			MIPS_NOT(MIPS_s6,MIPS_s6);
			MIPS_AND(MIPS_a0,MIPS_a0,MIPS_s6);
			MIPS_SW(MIPS_a0,0x484,MIPS_t7);
		}
	}

	if (hostreg_r[0] != (SSP_AL<<16)) {
		MIPS_MOVE(MIPS_a0,MIPS_t5);  // move  $a0, $t5
		hostreg_r[0] = SSP_AL<<16;
	}
}

static void tr_PMX_to_r0(int reg)
{
	if ((known_regb & KRREG_PMC) && (known_regs.emu_status & SSP_PMC_SET))
	{
		known_regs.pmac_read[reg] = known_regs.pmc.v;
		known_regs.emu_status &= ~SSP_PMC_SET;
		known_regb |= 1 << (20+reg);
		dirty_regb |= 1 << (20+reg);
		return;
	}

	if ((known_regb & KRREG_PMC) && (known_regb & (1 << (20+reg))))
	{
		u32 pmcv = known_regs.pmac_read[reg];
		int mode = pmcv>>16;
		known_regs.emu_status &= ~SSP_PMC_HAVE_ADDR;

		if      ((mode & 0xfff0) == 0x0800)
		{
			MIPS_LW(MIPS_a1,0x488,MIPS_t7);   // rom_ptr
			MIPS_LUI(MIPS_a0,((pmcv&0xfffff)<<1)>>16);
			MIPS_ORI(MIPS_a0,MIPS_a0,(pmcv&0xfffff)<<1);
			MIPS_ADDU(MIPS_s6,MIPS_a1,MIPS_a0);  // add $s5, $a1, $a0
			MIPS_LHU(MIPS_a0,0,MIPS_s6);		// lhu $a0,($s6)
			known_regs.pmac_read[reg] += 1;
		}
		else if ((mode & 0x47ff) == 0x0018) // DRAM
		{
			int inc = get_inc(mode);
			MIPS_LW(MIPS_a1,0x490,MIPS_t7);   // dram_ptr
			MIPS_LUI(MIPS_a0,((pmcv&0xffff)<<1)>>16);
			MIPS_ORI(MIPS_a0,MIPS_a0,(pmcv&0xffff)<<1);
			MIPS_ADDU(MIPS_s6,MIPS_a1,MIPS_a0);  // add $s6, $a1, $a0
			MIPS_LHU(MIPS_a0,0,MIPS_s6);		// lhu $a0,($s6)
			if (reg == 4 && (pmcv == 0x187f03 || pmcv == 0x187f04)) // wait loop detection
			{
				int flag = (pmcv == 0x187f03) ? SSP_WAIT_30FE06 : SSP_WAIT_30FE08;
				tr_flush_dirty_ST();
				MIPS_LW(MIPS_a1,0x484,MIPS_t7);   // lw $a1, 0x484($t7) // emu_status
				MIPS_MOVE(MIPS_s5,MIPS_a0);
				MIPS_BNEZ(MIPS_s5,7);
				MIPS_NOP();
				MIPS_ADDIU(MIPS_s2,MIPS_s2,-0x400);  // sub $s2, $s2, 1024
				MIPS_LUI(MIPS_s6,(flag>>8)>>16);
				MIPS_ORI(MIPS_s6,MIPS_s6,flag>>8);
				MIPS_ROTR(MIPS_s6,MIPS_s6,24);
				MIPS_OR(MIPS_a1,MIPS_a1,MIPS_s6);   // or $a1, $a1, SSP_WAIT_30FE08
				MIPS_SW(MIPS_a1,0x484,MIPS_t7);     // sw $a1, 0x484($t7) // emu_status
			}
			known_regs.pmac_read[reg] += inc;
		}
		else
		{
			tr_unhandled();
		}
		known_regs.pmc.v = known_regs.pmac_read[reg];
		//known_regb |= KRREG_PMC;
		dirty_regb |= KRREG_PMC;
		dirty_regb |= 1 << (20+reg);
		hostreg_r[0] = hostreg_r[1] = -1;
		return;
	}

	known_regb &= ~KRREG_PMC;
	dirty_regb &= ~KRREG_PMC;
	known_regb &= ~(1 << (20+reg));
	dirty_regb &= ~(1 << (20+reg));

	// call the C code to handle this
	tr_flush_dirty_ST();
	//tr_flush_dirty_pmcrs();
	tr_mov16(0, reg);
	emit_save_registers();
	emith_call(ssp_pm_read_asm);
	emit_restore_registers();
  	MIPS_MOVE(MIPS_a0,MIPS_v0);
	hostreg_clear();
}

static void tr_PM0_to_r0(int op)
{
	tr_PMX_to_r0(0);
}

static void tr_PM1_to_r0(int op)
{
	tr_PMX_to_r0(1);
}

static void tr_PM2_to_r0(int op)
{
	tr_PMX_to_r0(2);
}

static void tr_XST_to_r0(int op)
{
	MIPS_ADDI(MIPS_a0,MIPS_t7,0x400); // add $a0, $t7, 0x400
	MIPS_LHU(MIPS_a0,SSP_XST*4+2,MIPS_a0);
}

static void tr_PM4_to_r0(int op)
{
	tr_PMX_to_r0(4);
}

static void tr_PMC_to_r0(int op)
{
	if (known_regb & KRREG_PMC)
	{
		if (known_regs.emu_status & SSP_PMC_HAVE_ADDR) {
			known_regs.emu_status |= SSP_PMC_SET;
			known_regs.emu_status &= ~SSP_PMC_HAVE_ADDR;
			// do nothing - this is handled elsewhere
		} else {
			tr_mov16(0, known_regs.pmc.l);
			known_regs.emu_status |= SSP_PMC_HAVE_ADDR;
		}
	}
	else
	{
		MIPS_LW(MIPS_a1,0x484,MIPS_t7);    // lw $a1, 0x484($t7) // emu_status
		tr_flush_dirty_ST();
		if (op != 0x000e)
			MIPS_LW(MIPS_a0,0x400+SSP_PMC*4,MIPS_t7);
		MIPS_ANDI(MIPS_s5,MIPS_a1, SSP_PMC_HAVE_ADDR);
		MIPS_BNEZ(MIPS_s5,4);
		MIPS_NOP();
		MIPS_ORI(MIPS_a1,MIPS_a1,SSP_PMC_HAVE_ADDR);  // or $a1, $a1, ..
		MIPS_B(5);
		MIPS_NOP();
		MIPS_ADDIU(MIPS_s6,MIPS_zero,SSP_PMC_HAVE_ADDR);
		MIPS_NOT(MIPS_s6,MIPS_s6);
		MIPS_AND(MIPS_a1,MIPS_a1,MIPS_s6);  // and $a1, $a1, not SSP_PMC_HAVE_ADDR
		MIPS_ORI(MIPS_a1,MIPS_a1,SSP_PMC_SET);  // or $a1, $a1, ..
		MIPS_SW(MIPS_a1,0x484,MIPS_t7);
		hostreg_r[0] = hostreg_r[1] = -1;
	}
}


typedef void (tr_read_func)(int op);

static tr_read_func *tr_read_funcs[16] =
{
	tr_GR0_to_r0,
	tr_X_to_r0,
	tr_Y_to_r0,
	tr_A_to_r0,
	tr_ST_to_r0,
	tr_STACK_to_r0,
	tr_PC_to_r0,
	tr_P_to_r0,
	tr_PM0_to_r0,
	tr_PM1_to_r0,
	tr_PM2_to_r0,
	tr_XST_to_r0,
	tr_PM4_to_r0,
	(tr_read_func *)tr_unhandled,
	tr_PMC_to_r0,
	tr_AL_to_r0
};


// write r0 to general reg handlers. Trashes r1
#define TR_WRITE_R0_TO_REG(reg) \
{ \
	hostreg_sspreg_changed(reg); \
	hostreg_r[0] = (reg)<<16; \
	if (const_val != -1) { \
		known_regs.gr[reg].h = const_val; \
		known_regb |= 1 << (reg); \
	} else { \
		known_regb &= ~(1 << (reg)); \
	} \
}

static void tr_r0_to_GR0(int const_val)
{
	// do nothing
}

static void tr_r0_to_X(int const_val)
{
	MIPS_SLL(MIPS_t4,MIPS_t4,16);		// sll  $t4, $t4, 16
	MIPS_SRL(MIPS_t4,MIPS_t4,16);		// srl  $t4, $t4, 16
	MIPS_SLL(MIPS_s6,MIPS_a0,16);		// sll  $s5, $a0, 16
	MIPS_OR(MIPS_t4,MIPS_t4,MIPS_s6);	// or  $t4, $t4, $s5
	dirty_regb |= KRREG_P;			// touching X or Y makes P dirty.
	TR_WRITE_R0_TO_REG(SSP_X);
}

static void tr_r0_to_Y(int const_val)
{
	MIPS_SRL(MIPS_t4,MIPS_t4,16);		// srl  $t4, $t4, 16
	MIPS_SLL(MIPS_s6,MIPS_a0,16);		// sll  $s5, $a0, 16
	MIPS_OR(MIPS_t4,MIPS_t4,MIPS_s6);	// or  $t4, $t4, $s5
	MIPS_ROTR(MIPS_t4,MIPS_t4,16);		// rotr  $t4, $t4, 16
	dirty_regb |= KRREG_P;
	TR_WRITE_R0_TO_REG(SSP_Y);
}

static void tr_r0_to_A(int const_val)
{
	if (tr_predict_al_need()) {
		MIPS_SLL(MIPS_t5,MIPS_t5,16);		// sll  $t5, $t5, 16
		MIPS_SRL(MIPS_t5,MIPS_t5,16);		// srl  $t5, $t5, 16  @ AL
		MIPS_SLL(MIPS_s6,MIPS_a0,16);		// sll  $s5, $a0, 16
		MIPS_OR(MIPS_t5,MIPS_t5,MIPS_s6);	// or  $t5, $t5, $s5
	}
	else
		MIPS_SLL(MIPS_t5,MIPS_a0,16);		// sll  $t5, $a0, 16  @ AL
	TR_WRITE_R0_TO_REG(SSP_A);
}

static void tr_r0_to_ST(int const_val)
{
	// VR doesn't need much accuracy here..
	MIPS_ANDI(MIPS_a1,MIPS_a0,0x67);    // and   $a1, $a0, 0x67
	MIPS_LUI(MIPS_s6,0xe000);
	MIPS_AND(MIPS_t6,MIPS_t6,MIPS_s6);  // and $t6, $t6, (7<<29)
	MIPS_SLL(MIPS_s6,MIPS_a1,4);		// sll	 $s6, $a1, 4
	MIPS_OR(MIPS_t6,MIPS_t6,MIPS_s6);   // or    $t6, $t6, $s6
	TR_WRITE_R0_TO_REG(SSP_ST);
	hostreg_r[1] = -1;
	dirty_regb &= ~KRREG_ST;
}

static void tr_r0_to_STACK(int const_val)
{
	// 448
	MIPS_ADDI(MIPS_a1,MIPS_t7,0x400);		// add $a1, $t7, 0x400
	MIPS_ADDI(MIPS_a1,MIPS_a1,0x48);		// add $a1, $a1, 0x048
	MIPS_SRL(MIPS_s6,MIPS_t6,28);			// srl $s6, $t6, 28
	MIPS_ADDU(MIPS_a1,MIPS_a1,MIPS_s6);		// add $a1, $a1, $s6
	MIPS_SH(MIPS_a0,0,MIPS_a1);				// sh  $a0, ($a1)
	MIPS_LUI(MIPS_s6,0x2000);
	MIPS_ADDU(MIPS_t6,MIPS_t6,MIPS_s6);     // add $t6, $t6, (1<<29)
	hostreg_r[1] = -1;
}

static void tr_r0_to_PC(int const_val)
{
/*
 * do nothing - dispatcher will take care of this
	EOP_MOV_REG_LSL(1, 0, 16);		// mov  r1, r0, lsl #16
	EOP_STR_IMM(1,7,0x400+6*4);		// str  r1, [r7, #(0x400+6*8)]
	hostreg_r[1] = -1;
*/
}

static void tr_r0_to_AL(int const_val)
{
	MIPS_SRL(MIPS_t5,MIPS_t5,16);		// srl  $t5, $t5, 16
	MIPS_SLL(MIPS_s6,MIPS_a0,16);		// sll  $s5, $a0, 16
	MIPS_OR(MIPS_t5,MIPS_t5,MIPS_s6);	// or  $t5, $t5, $s5
	MIPS_ROTR(MIPS_t5,MIPS_t5,16);		// rotr  $t5, $t5, 16
	hostreg_sspreg_changed(SSP_AL);
	if (const_val != -1) {
		known_regs.gr[SSP_A].l = const_val;
		known_regb |= 1 << SSP_AL;
	} else
		known_regb &= ~(1 << SSP_AL);
}

static void tr_r0_to_PMX(int reg)
{
	if ((known_regb & KRREG_PMC) && (known_regs.emu_status & SSP_PMC_SET))
	{
		known_regs.pmac_write[reg] = known_regs.pmc.v;
		known_regs.emu_status &= ~SSP_PMC_SET;
		known_regb |= 1 << (25+reg);
		dirty_regb |= 1 << (25+reg);
		return;
	}

	if ((known_regb & KRREG_PMC) && (known_regb & (1 << (25+reg))))
	{
		int mode, addr;

		known_regs.emu_status &= ~SSP_PMC_HAVE_ADDR;

		mode = known_regs.pmac_write[reg]>>16;
		addr = known_regs.pmac_write[reg]&0xffff;
		if      ((mode & 0x43ff) == 0x0018) // DRAM
		{
			int inc = get_inc(mode);
			if (mode & 0x0400) tr_unhandled();
			MIPS_LW(MIPS_a1,0x490,MIPS_t7);  		// dram_ptr
			MIPS_LUI(MIPS_a2,(addr << 1)>>16);
			MIPS_ORI(MIPS_a2,MIPS_a2,addr << 1);
			MIPS_ADDU(MIPS_s6,MIPS_a1,MIPS_a2);
			MIPS_SH(MIPS_a0,0,MIPS_s6);  		// sh $a0, ($s5)
			known_regs.pmac_write[reg] += inc;
		}
		else if ((mode & 0xfbff) == 0x4018) // DRAM, cell inc
		{
			if (mode & 0x0400) tr_unhandled();
			MIPS_LW(MIPS_a1,0x490,MIPS_t7);  		// dram_ptr
			MIPS_LUI(MIPS_a2,(addr << 1)>>16);
			MIPS_ORI(MIPS_a2,MIPS_a2,addr << 1);
			MIPS_ADDU(MIPS_s6,MIPS_a1,MIPS_a2);
			MIPS_SH(MIPS_a0,0,MIPS_s6);  		// sh $a0, ($s5)
			known_regs.pmac_write[reg] += (addr&1) ? 31 : 1;
		}
		else if ((mode & 0x47ff) == 0x001c) // IRAM
		{
			int inc = get_inc(mode);
			MIPS_LW(MIPS_a1,0x48c,MIPS_t7);  		// iram_ptr
			MIPS_LUI(MIPS_a2,((addr&0x3ff) << 1)>>16);
			MIPS_ORI(MIPS_a2,MIPS_a2,(addr&0x3ff) << 1);
			MIPS_ADDU(MIPS_s6,MIPS_a1,MIPS_a2);
			MIPS_SH(MIPS_a0,0,MIPS_s6);  		// sh $a0, ($s5)
			MIPS_ADDIU(MIPS_a1,MIPS_zero,1);
			MIPS_SW(MIPS_a1,0x494,MIPS_t7);     // iram_dirty
			known_regs.pmac_write[reg] += inc;
		}
		else
			tr_unhandled();

		known_regs.pmc.v = known_regs.pmac_write[reg];
		//known_regb |= KRREG_PMC;
		dirty_regb |= KRREG_PMC;
		dirty_regb |= 1 << (25+reg);
		hostreg_r[1] = hostreg_r[2] = -1;
		return;
	}

	known_regb &= ~KRREG_PMC;
	dirty_regb &= ~KRREG_PMC;
	known_regb &= ~(1 << (25+reg));
	dirty_regb &= ~(1 << (25+reg));

	// call the C code to handle this
	tr_flush_dirty_ST();
	//tr_flush_dirty_pmcrs();
	tr_mov16(1, reg);
	emit_save_registers();
	emith_call(ssp_pm_write_asm);
	emit_restore_registers();
	hostreg_clear();
}

static void tr_r0_to_PM0(int const_val)
{
	tr_r0_to_PMX(0);
}

static void tr_r0_to_PM1(int const_val)
{
	tr_r0_to_PMX(1);
}

static void tr_r0_to_PM2(int const_val)
{
	tr_r0_to_PMX(2);
}

static void tr_r0_to_PM4(int const_val)
{
	tr_r0_to_PMX(4);
}

static void tr_r0_to_PMC(int const_val)
{
	if ((known_regb & KRREG_PMC) && const_val != -1)
	{
		if (known_regs.emu_status & SSP_PMC_HAVE_ADDR) {
			known_regs.emu_status |= SSP_PMC_SET;
			known_regs.emu_status &= ~SSP_PMC_HAVE_ADDR;
			known_regs.pmc.h = const_val;
		} else {
			known_regs.emu_status |= SSP_PMC_HAVE_ADDR;
			known_regs.pmc.l = const_val;
		}
	}
	else
	{
		tr_flush_dirty_ST();
		if (known_regb & KRREG_PMC) {
			MIPS_ADDIU(MIPS_a1,MIPS_zero,known_regs.pmc.v);
			MIPS_SW(MIPS_a1,0x400+SSP_PMC*4,MIPS_t7);
			known_regb &= ~KRREG_PMC;
			dirty_regb &= ~KRREG_PMC;
		}
		MIPS_LW(MIPS_a1,0x484,MIPS_t7);  		// lw $a1, 0x484($t7) // emu_status
		MIPS_ADDIU(MIPS_a2,MIPS_t7,0x400);		// add $aS, $t7, 0x400
		MIPS_ANDI(MIPS_s5,MIPS_a1,SSP_PMC_HAVE_ADDR);   // TODO: fixme, use this line
		MIPS_BNEZ(MIPS_s5,5);
		MIPS_NOP();
		MIPS_SH(MIPS_a0,SSP_PMC*4,MIPS_a2);		      // sw $a0, SSP_PMC($a2)
		MIPS_ORI(MIPS_a1,MIPS_a1,SSP_PMC_HAVE_ADDR);  // or $a1, $a1, ..
		MIPS_B(6);
		MIPS_NOP();
		MIPS_SH(MIPS_a0,SSP_PMC*4+2,MIPS_a2);
		MIPS_ADDIU(MIPS_s6,MIPS_zero,SSP_PMC_HAVE_ADDR);
		MIPS_NOT(MIPS_s6,MIPS_s6);
		MIPS_AND(MIPS_a1,MIPS_a1,MIPS_s6);  // and $a1, $a1, not SSP_PMC_HAVE_ADDR
		MIPS_ORI(MIPS_a1,MIPS_a1,SSP_PMC_SET);  // or $a1, $a1, ..
		MIPS_SW(MIPS_a1,0x484,MIPS_t7);
		hostreg_r[1] = hostreg_r[2] = -1;
	}
}

typedef void (tr_write_func)(int const_val);

static tr_write_func *tr_write_funcs[16] =
{
	tr_r0_to_GR0,
	tr_r0_to_X,
	tr_r0_to_Y,
	tr_r0_to_A,
	tr_r0_to_ST,
	tr_r0_to_STACK,
	tr_r0_to_PC,
	(tr_write_func *)tr_unhandled,
	tr_r0_to_PM0,
	tr_r0_to_PM1,
	tr_r0_to_PM2,
	(tr_write_func *)tr_unhandled,
	tr_r0_to_PM4,
	(tr_write_func *)tr_unhandled,
	tr_r0_to_PMC,
	tr_r0_to_AL
};

static void tr_mac_load_XY(int op)
{
	tr_rX_read(op&3, (op>>2)&3); // X
	MIPS_SLL(MIPS_t4,MIPS_a0,16);
	tr_rX_read(((op>>4)&3)|4, (op>>6)&3); // Y
	MIPS_OR(MIPS_t4,MIPS_t4,MIPS_a0);
	dirty_regb |= KRREG_P;
	hostreg_sspreg_changed(SSP_X);
	hostreg_sspreg_changed(SSP_Y);
	known_regb &= ~KRREG_X;
	known_regb &= ~KRREG_Y;
}

// -----------------------------------------------------

static int tr_detect_set_pm(unsigned int op, int *pc, int imm)
{
	u32 pmcv, tmpv;
	if (!((op&0xfef0) == 0x08e0 && (PROGRAM(*pc)&0xfef0) == 0x08e0)) return 0;

	// programming PMC:
	// ldi PMC, imm1
	// ldi PMC, imm2
	(*pc)++;
	pmcv = imm | (PROGRAM((*pc)++) << 16);
	known_regs.pmc.v = pmcv;
	known_regb |= KRREG_PMC;
	dirty_regb |= KRREG_PMC;
	known_regs.emu_status |= SSP_PMC_SET;
	n_in_ops++;

	// check for possible reg programming
	tmpv = PROGRAM(*pc);
	if ((tmpv & 0xfff8) == 0x08 || (tmpv & 0xff8f) == 0x80)
	{
		int is_write = (tmpv & 0xff8f) == 0x80;
		int reg = is_write ? ((tmpv>>4)&0x7) : (tmpv&0x7);
		if (reg > 4) tr_unhandled();
		if ((tmpv & 0x0f) != 0 && (tmpv & 0xf0) != 0) tr_unhandled();
		if (is_write)
			known_regs.pmac_write[reg] = pmcv;
		else
			known_regs.pmac_read[reg] = pmcv;
		known_regb |= is_write ? (1 << (reg+25)) : (1 << (reg+20));
		dirty_regb |= is_write ? (1 << (reg+25)) : (1 << (reg+20));
		known_regs.emu_status &= ~SSP_PMC_SET;
		(*pc)++;
		n_in_ops++;
		return 5;
	}

	tr_unhandled();
	return 4;
}

static const short pm0_block_seq[] = { 0x0880, 0, 0x0880, 0, 0x0840, 0x60 };

static int tr_detect_pm0_block(unsigned int op, int *pc, int imm)
{
	// ldi ST, 0
	// ldi PM0, 0
	// ldi PM0, 0
	// ldi ST, 60h
	unsigned short *pp;
	if (op != 0x0840 || imm != 0) return 0;
	pp = PROGRAM_P(*pc);
	if (memcmp(pp, pm0_block_seq, sizeof(pm0_block_seq)) != 0) return 0;
	MIPS_LUI(MIPS_s6,0xe000);
	MIPS_AND(MIPS_t6,MIPS_t6,MIPS_s6);     // and   $t6, $t6, 7<<29     @ preserve STACK
	MIPS_ORI(MIPS_t6,MIPS_t6,0x600);     // or    $t6, $t6, 0x600
	hostreg_sspreg_changed(SSP_ST);
	known_regs.gr[SSP_ST].h = 0x60;
	known_regb |= 1 << SSP_ST;
	dirty_regb &= ~KRREG_ST;
	(*pc) += 3*2;
	n_in_ops += 3;
	return 4*2;
}

static int tr_detect_rotate(unsigned int op, int *pc, int imm)
{
	// @ 3DA2 and 426A
	// ld PMC, (r3|00)
	// ld (r3|00), PMC
	// ld -, AL
	if (op != 0x02e3 || PROGRAM(*pc) != 0x04e3 || PROGRAM(*pc + 1) != 0x000f) return 0;

	tr_bank_read(0);
	MIPS_SLL(MIPS_a0,MIPS_a0,4);
	MIPS_SRL(MIPS_s6,MIPS_a0,16);
	MIPS_OR(MIPS_a0,MIPS_a0,MIPS_s6);
	tr_bank_write(0);
	(*pc) += 2;
	n_in_ops += 2;
	return 3;
}

// -----------------------------------------------------

static int translate_op(unsigned int op, int *pc, int imm, int *end_cond, int *jump_pc)
{
	u32 tmpv, tmpv2;
	int ret = 0;
	known_regs.gr[SSP_PC].h = *pc;

	switch (op >> 9)
	{
		// ld d, s
		case 0x00:
			if (op == 0) { ret++; break; } // nop
			tmpv  = op & 0xf; // src
			tmpv2 = (op >> 4) & 0xf; // dst
			if (tmpv2 == SSP_A && tmpv == SSP_P) { // ld A, P
				tr_flush_dirty_P();
				MIPS_MOVE(MIPS_t5,MIPS_s1);
				hostreg_sspreg_changed(SSP_A);
				known_regb &= ~(KRREG_A|KRREG_AL);
				ret++; break;
			}
			tr_read_funcs[tmpv](op);
			tr_write_funcs[tmpv2]((known_regb & (1 << tmpv)) ? known_regs.gr[tmpv].h : -1);
			if (tmpv2 == SSP_PC) {
				ret |= 0x10000;
				*end_cond = -A_COND_AL;
			}
			ret++; break;

		// ld d, (ri)
		case 0x01: {
			int r = (op&3) | ((op>>6)&4);
			int mod = (op>>2)&3;
			tmpv = (op >> 4) & 0xf; // dst
			ret = tr_detect_rotate(op, pc, imm);
			if (ret > 0) break;
			if (tmpv != 0)
				tr_rX_read(r, mod);
			else {
				int cnt = 1;
				while (PROGRAM(*pc) == op) {
					(*pc)++; cnt++; ret++;
					n_in_ops++;
				}
				tr_ptrr_mod(r, mod, 1, cnt); // skip
			}
			tr_write_funcs[tmpv](-1);
			if (tmpv == SSP_PC) {
				ret |= 0x10000;
				*end_cond = -A_COND_AL;
			}
			ret++; break;
		}

		// ld (ri), s
		case 0x02:
			tmpv = (op >> 4) & 0xf; // src
			tr_read_funcs[tmpv](op);
			tr_rX_write(op);
			ret++; break;

		// ld a, adr
		case 0x03:
			tr_bank_read(op&0x1ff);
			tr_r0_to_A(-1);
			ret++; break;

		// ldi d, imm
		case 0x04:
			tmpv = (op & 0xf0) >> 4; // dst
			ret = tr_detect_pm0_block(op, pc, imm);
			if (ret > 0) break;
			ret = tr_detect_set_pm(op, pc, imm);
			if (ret > 0) break;
			tr_mov16(0, imm);
			tr_write_funcs[tmpv](imm);
			if (tmpv == SSP_PC) {
				ret |= 0x10000;
				*jump_pc = imm;
			}
			ret += 2; break;

		// ld d, ((ri))
		case 0x05:
			tmpv2 = (op >> 4) & 0xf;  // dst
			tr_rX_read2(op);
			tr_write_funcs[tmpv2](-1);
			if (tmpv2 == SSP_PC) {
				ret |= 0x10000;
				*end_cond = -A_COND_AL;
			}
			ret += 3; break;

		// ldi (ri), imm
		case 0x06:
			tr_mov16(0, imm);
			tr_rX_write(op);
			ret += 2; break;

		// ld adr, a
		case 0x07:
			tr_A_to_r0(op);
			tr_bank_write(op&0x1ff);
			ret++; break;

		// ld d, ri
		case 0x09: {
			int r;
			r = (op&3) | ((op>>6)&4); // src
			tmpv2 = (op >> 4) & 0xf;  // dst
			if ((r&3) == 3) tr_unhandled();

			if (known_regb & (1 << (r+8))) {
				tr_mov16(0, known_regs.r[r]);
				tr_write_funcs[tmpv2](known_regs.r[r]);
			} else {
				int reg = (r < 4) ? 8 : 9;
				if (r&3)
					MIPS_SRL(MIPS_a0,arm_reg_to_mips(reg),(r&3)*8); // srl $a0, $t{7,8}, <lsr>
				MIPS_ANDI(MIPS_a0,(r&3)?MIPS_a0:arm_reg_to_mips(reg),0xff);    // and $a0, $t{7,8}, <mask>
				hostreg_r[0] = -1;
				tr_write_funcs[tmpv2](-1);
			}
			ret++; break;
		}

		// ld ri, s
		case 0x0a: {
			int r;
			r = (op&3) | ((op>>6)&4); // dst
			tmpv = (op >> 4) & 0xf;   // src
			if ((r&3) == 3) tr_unhandled();

			if (known_regb & (1 << tmpv)) {
				known_regs.r[r] = known_regs.gr[tmpv].h;
				known_regb |= 1 << (r + 8);
				dirty_regb |= 1 << (r + 8);
			} else {
				int reg = (r < 4) ? 8 : 9;
				int ror = ((4 - (r&3))*8) & 0x1f;
				tr_read_funcs[tmpv](op);
				MIPS_ADDIU(MIPS_s6,MIPS_zero,0xff);
				MIPS_ROTR(MIPS_s6,MIPS_s6,ror);
				MIPS_NOT(MIPS_s6,MIPS_s6);
				MIPS_AND(arm_reg_to_mips(reg),arm_reg_to_mips(reg),MIPS_s6);    // and   $t{7,8}, $t{7,8}, not 0xff
				MIPS_ANDI(MIPS_a0,MIPS_a0,0xff);								// and $a0, $a0, 0xff
				MIPS_SLL(MIPS_s6,MIPS_a0,(r&3)*8);								// sll $s5, $a0, <lsl>
				MIPS_OR(arm_reg_to_mips(reg),arm_reg_to_mips(reg),MIPS_s6);		// orr $t{7,8}, $t{7,8}, $s5
				hostreg_r[0] = -1;
				known_regb &= ~(1 << (r+8));
				dirty_regb &= ~(1 << (r+8));
			}
			ret++; break;
		}

		// ldi ri, simm
		case 0x0c: case 0x0d: case 0x0e: case 0x0f:
			tmpv = (op>>8)&7;
			known_regs.r[tmpv] = op;
			known_regb |= 1 << (tmpv + 8);
			dirty_regb |= 1 << (tmpv + 8);
			ret++; break;

		// call cond, addr
		case 0x24: {
			u32 *jump_op = NULL;
			tmpv = tr_cond_check(op);
			if (tmpv != A_COND_AL) {
				jump_op = tcache_ptr;
				MIPS_MOVE(MIPS_a0,MIPS_zero);  // placeholder for branch
				MIPS_NOP();                    // mips need one instruction slot after branch instruction
			}
			tr_mov16(0, *pc);
			tr_r0_to_STACK(*pc);
			if (tmpv != A_COND_AL) {
				u32 *real_ptr = tcache_ptr;
				tcache_ptr = jump_op;
				switch(tr_neg_cond(tmpv)) {  // MIPS need skip one more instruction
					case A_COND_EQ: MIPS_BEQZ(MIPS_s5,real_ptr - jump_op - 1);MIPS_NOP();break;
					case A_COND_NE: MIPS_BNEZ(MIPS_s5,real_ptr - jump_op - 1);MIPS_NOP();break;
					case A_COND_MI: MIPS_BLTZ(MIPS_s5,real_ptr - jump_op - 1);MIPS_NOP();break;
					case A_COND_PL: MIPS_BGEZ(MIPS_s5,real_ptr - jump_op - 1);MIPS_NOP();break;
					default: break;
				}
				tcache_ptr = real_ptr;
			}
			tr_mov16_cond(tmpv, 0, imm);
			if (tmpv != A_COND_AL)
				tr_mov16_cond(tr_neg_cond(tmpv), 0, *pc);
			tr_r0_to_PC(tmpv == A_COND_AL ? imm : -1);
			ret |= 0x10000;
			*end_cond = tmpv;
			*jump_pc = imm;
			ret += 2; break;
		}

		// ld d, (a)
		case 0x25:
			tmpv2 = (op >> 4) & 0xf;  // dst
			tr_A_to_r0(op);
			MIPS_LW(MIPS_a1,0x48c,MIPS_t7);			// ptr_iram_rom
			MIPS_SLL(MIPS_s6,MIPS_a0,1);
			MIPS_ADDU(MIPS_a0,MIPS_a1,MIPS_s6);
			MIPS_LHU(MIPS_a0,0,MIPS_a0);
			hostreg_r[0] = hostreg_r[1] = -1;
			tr_write_funcs[tmpv2](-1);
			if (tmpv2 == SSP_PC) {
				ret |= 0x10000;
				*end_cond = -A_COND_AL;
			}
			ret += 3; break;

		// bra cond, addr
		case 0x26:
			tmpv = tr_cond_check(op);
			tr_mov16_cond(tmpv, 0, imm);
			if (tmpv != A_COND_AL)
				tr_mov16_cond(tr_neg_cond(tmpv), 0, *pc);
			tr_r0_to_PC(tmpv == A_COND_AL ? imm : -1);
			ret |= 0x10000;
			*end_cond = tmpv;
			*jump_pc = imm;
			ret += 2; break;

		// mod cond, op
		case 0x48: {
			// check for repeats of this op
			tmpv = 1; // count
			while (PROGRAM(*pc) == op && (op & 7) != 6) {
				(*pc)++; tmpv++;
				n_in_ops++;
			}
			if ((op&0xf0) != 0) // !always
				tr_make_dirty_ST();

			tmpv2 = tr_cond_check(op);

			switch(tmpv2) {
				case A_COND_AL: break;
				case A_COND_EQ: MIPS_BNEZ(MIPS_s5,6);MIPS_NOP();break;
				case A_COND_NE: MIPS_BEQZ(MIPS_s5,6);MIPS_NOP();break;
				case A_COND_MI: MIPS_BGEZ(MIPS_s5,6);MIPS_NOP();break;
				case A_COND_PL: MIPS_BLTZ(MIPS_s5,6);MIPS_NOP();break;
				default: break;
			}

			switch (op & 7) {
				case 2: MIPS_SRA(MIPS_t5,MIPS_t5,tmpv);MIPS_MOVE(MIPS_s5,MIPS_t5);MIPS_NOP();MIPS_NOP();MIPS_NOP();break; // shr (arithmetic)
				case 3: MIPS_SLL(MIPS_t5,MIPS_t5,tmpv);MIPS_MOVE(MIPS_s5,MIPS_t5);MIPS_NOP();MIPS_NOP();MIPS_NOP();break; // shl
				case 6: MIPS_SUBU(MIPS_t5,MIPS_zero,MIPS_t5);MIPS_MOVE(MIPS_s5,MIPS_t5);MIPS_NOP();MIPS_NOP();MIPS_NOP();break; // neg
				case 7: MIPS_SRA(MIPS_s6,MIPS_t5,31);MIPS_XOR(MIPS_a1,MIPS_t5,MIPS_s6); // eor  r1, r5, r5, asr #31
					MIPS_SRL(MIPS_s6,MIPS_t5,31);MIPS_ADDU(MIPS_t5,MIPS_a1,MIPS_s6); // adds r5, r1, r5, lsr #31
					MIPS_MOVE(MIPS_s5,MIPS_t5);
					hostreg_r[1] = -1; break; // abs
				default: tr_unhandled();
			}

			hostreg_sspreg_changed(SSP_A);
			dirty_regb |=  KRREG_ST;
			known_regb &= ~KRREG_ST;
			known_regb &= ~(KRREG_A|KRREG_AL);
			ret += tmpv; break;
		}

		// mpys?
		case 0x1b:
			tr_flush_dirty_P();
			tr_mac_load_XY(op);
			tr_make_dirty_ST();
			generate_op_sub_flags_prologue(MIPS_t5,MIPS_s1);
			MIPS_SUBU(MIPS_t5,MIPS_t5,MIPS_s1);
			MIPS_MOVE(MIPS_s5,MIPS_t5);
			generate_op_sub_flags_epilogue(MIPS_t5);
			hostreg_sspreg_changed(SSP_A);
			known_regb &= ~(KRREG_A|KRREG_AL);
			dirty_regb |= KRREG_ST;
			ret++; break;

		// mpya (rj), (ri), b
		case 0x4b:
			tr_flush_dirty_P();
			tr_mac_load_XY(op);
			tr_make_dirty_ST();
			MIPS_ADDU(MIPS_t5,MIPS_t5,MIPS_s1);
			MIPS_MOVE(MIPS_s5,MIPS_t5);
			hostreg_sspreg_changed(SSP_A);
			known_regb &= ~(KRREG_A|KRREG_AL);
			dirty_regb |= KRREG_ST;
			ret++; break;

		// mld (rj), (ri), b
		case 0x5b:
			MIPS_MOVE(MIPS_t5,MIPS_zero);
			MIPS_MOVE(MIPS_s5,MIPS_t5);
			generate_op_logic_flags(MIPS_s5);
			hostreg_sspreg_changed(SSP_A);
			known_regs.gr[SSP_A].v = 0;
			known_regb |= (KRREG_A|KRREG_AL);
			dirty_regb |= KRREG_ST;
			tr_mac_load_XY(op);
			ret++; break;

		// OP a, s
		case 0x10:
		case 0x30:
		case 0x40:
		case 0x50:
		case 0x60:
		case 0x70:
			tmpv = op & 0xf; // src
			tmpv2 = tr_aop_ssp2mips(op>>13); // op
			if (tmpv == SSP_P) {
				tr_flush_dirty_P();
				if( tmpv2 != -1 ) {
					__MIPS_INSN_REG(SPECIAL,MIPS_t5,MIPS_s1,MIPS_t5,0,tmpv2);  // OPs $t5, $t5, $s1
					MIPS_MOVE(MIPS_s5,MIPS_t5);
				}
				else {
					MIPS_SUBU(MIPS_s5,MIPS_t5,MIPS_s1);
				}
			} else if (tmpv == SSP_A) {
				if( tmpv2 != -1 ) {
					__MIPS_INSN_REG(SPECIAL,MIPS_t5,MIPS_t5,MIPS_t5,0,tmpv2);  // OPs $t5, $t5, $t5
					MIPS_MOVE(MIPS_s5,MIPS_t5);
				}
				else {
					MIPS_MOVE(MIPS_s5,MIPS_zero);
				}
			} else {
				tr_read_funcs[tmpv](op);
				MIPS_SLL(MIPS_s6,MIPS_a0,16);
				if( tmpv2 != -1 ) {

					if( tmpv2 == __SP_SUBU ){
						generate_op_sub_flags_prologue(MIPS_t5,MIPS_s6);
					}

					__MIPS_INSN_REG(SPECIAL,MIPS_t5,MIPS_s6,MIPS_t5,0,tmpv2);  // OPs $t5, $t5, $s6
					MIPS_MOVE(MIPS_s5,MIPS_t5);

					if( tmpv2 == __SP_SUBU ){
						generate_op_sub_flags_epilogue(MIPS_t5);
					}
				}
				else {
					generate_op_sub_flags_prologue(MIPS_t5,MIPS_s6);
					MIPS_SUBU(MIPS_s5,MIPS_t5,MIPS_s6);
					generate_op_sub_flags_epilogue(MIPS_s5);
				}
			}
			hostreg_sspreg_changed(SSP_A);
			known_regb &= ~(KRREG_A|KRREG_AL|KRREG_ST);
			dirty_regb |= KRREG_ST;
			ret++; break;

		// OP a, (ri)
		case 0x11:
		case 0x31:
		case 0x41:
		case 0x51:
		case 0x61:
		case 0x71:
			tmpv2 = tr_aop_ssp2mips(op>>13); // op
			tr_rX_read((op&3)|((op>>6)&4), (op>>2)&3);
			MIPS_SLL(MIPS_s6,MIPS_a0,16);
			if( tmpv2 != -1 ) {
				__MIPS_INSN_REG(SPECIAL,MIPS_t5,MIPS_s6,MIPS_t5,0,tmpv2);  // OPs $t5, $t5, $s5
				MIPS_MOVE(MIPS_s5,MIPS_t5);
			}
			else {
				generate_op_sub_flags_prologue(MIPS_t5,MIPS_s6);
				MIPS_SUBU(MIPS_s5,MIPS_t5,MIPS_s6);
				generate_op_sub_flags_epilogue(MIPS_s5);
			}
			hostreg_sspreg_changed(SSP_A);
			known_regb &= ~(KRREG_A|KRREG_AL|KRREG_ST);
			dirty_regb |= KRREG_ST;
			ret++; break;

		// OP a, adr
		case 0x13:
		case 0x33:
		case 0x43:
		case 0x53:
		case 0x63:
		case 0x73:
			tmpv2 = tr_aop_ssp2mips(op>>13); // op
			tr_bank_read(op&0x1ff);
			MIPS_SLL(MIPS_s6,MIPS_a0,16);
			if( tmpv2 != -1 ) {
				__MIPS_INSN_REG(SPECIAL,MIPS_t5,MIPS_s6,MIPS_t5,0,tmpv2);  // OPs $t5, $t5, $s5
				MIPS_MOVE(MIPS_s5,MIPS_t5);
			}
			else {
				MIPS_SUBU(MIPS_s5,MIPS_t5,MIPS_s6);
			}
			hostreg_sspreg_changed(SSP_A);
			known_regb &= ~(KRREG_A|KRREG_AL|KRREG_ST);
			dirty_regb |= KRREG_ST;
			ret++; break;

		// OP a, imm
		case 0x14:
		case 0x34:
		case 0x44:
		case 0x54:
		case 0x64:
		case 0x74:
			tmpv = (op & 0xf0) >> 4;
			tmpv2 = tr_aop_ssp2mips(op>>13); // op
			tr_mov16(0, imm);
			MIPS_SLL(MIPS_s6,MIPS_a0,16);
			if( tmpv2 != -1 ) {
				__MIPS_INSN_REG(SPECIAL,MIPS_t5,MIPS_s6,MIPS_t5,0,tmpv2);  // OPs $t5, $t5, $s5
				MIPS_MOVE(MIPS_s5,MIPS_t5);

				if( ( tmpv2 == __SP_AND ) || ( tmpv2 == __SP_OR ) || ( tmpv2 == __SP_XOR ) ) {
					generate_op_logic_flags(MIPS_t5);
				}
			}
			else {
				MIPS_SUBU(MIPS_s5,MIPS_t5,MIPS_s6);
			}
			hostreg_sspreg_changed(SSP_A);
			known_regb &= ~(KRREG_A|KRREG_AL|KRREG_ST);
			dirty_regb |= KRREG_ST;
			ret += 2; break;

		// OP a, ((ri))
		case 0x15:
		case 0x35:
		case 0x45:
		case 0x55:
		case 0x65:
		case 0x75:
			tmpv2 = tr_aop_ssp2mips(op>>13); // op
			tr_rX_read2(op);
			MIPS_SLL(MIPS_s6,MIPS_a0,16);
			if( tmpv2 != -1 ) {
				__MIPS_INSN_REG(SPECIAL,MIPS_t5,MIPS_s6,MIPS_t5,0,tmpv2);  // OPs $t5, $t5, $s5
				MIPS_MOVE(MIPS_s5,MIPS_t5);
			}
			else {
				generate_op_sub_flags_prologue(MIPS_t5,MIPS_s6);
				MIPS_SUBU(MIPS_s5,MIPS_t5,MIPS_s6);
				generate_op_sub_flags_epilogue(MIPS_s5);
			}
			hostreg_sspreg_changed(SSP_A);
			known_regb &= ~(KRREG_A|KRREG_AL|KRREG_ST);
			dirty_regb |= KRREG_ST;
			ret += 3; break;

		// OP a, ri
		case 0x19:
		case 0x39:
		case 0x49:
		case 0x59:
		case 0x69:
		case 0x79: {
			int r;
			tmpv2 = tr_aop_ssp2mips(op>>13); // op
			r = (op&3) | ((op>>6)&4); // src
			if ((r&3) == 3) tr_unhandled();

			if (known_regb & (1 << (r+8))) {
				MIPS_LUI(MIPS_s6,known_regs.r[r]);
				if( tmpv2 != -1 ) {

					if( tmpv2 == __SP_SUBU ){
						generate_op_sub_flags_prologue(MIPS_t5,MIPS_s6);
					}

					__MIPS_INSN_REG(SPECIAL,MIPS_t5,MIPS_s6,MIPS_t5,0,tmpv2);  // OPs $t5, $t5, $s5
					MIPS_MOVE(MIPS_s5,MIPS_t5);

					if( tmpv2 == __SP_SUBU ){
						generate_op_sub_flags_epilogue(MIPS_t5);
					}
				}
				else {
					MIPS_SUBU(MIPS_s5,MIPS_t5,MIPS_s6);
				}
			} else {
				int reg = (r < 4) ? 8 : 9;
				if (r&3)
					MIPS_SRL(MIPS_a0,arm_reg_to_mips(reg),(r&3)*8);  // srl  $a0, $t{7,8}, <lsr>
				MIPS_ANDI(MIPS_a0,(r&3)?MIPS_a0:arm_reg_to_mips(reg),0xff);  // and $a0, $t{7,8}, <mask>
				MIPS_SLL(MIPS_s6,MIPS_a0,16);

				if( tmpv2 != -1 ) {

					__MIPS_INSN_REG(SPECIAL,MIPS_t5,MIPS_s6,MIPS_t5,0,tmpv2);  // OPs $t5, $t5, $s5
					MIPS_MOVE(MIPS_s5,MIPS_t5);
				}
				else {
					MIPS_SUBU(MIPS_s5,MIPS_t5,MIPS_s6);
				}
				hostreg_r[0] = -1;
			}
			hostreg_sspreg_changed(SSP_A);
			known_regb &= ~(KRREG_A|KRREG_AL|KRREG_ST);
			dirty_regb |= KRREG_ST;
			ret++; break;
		}

		// OP simm
		case 0x1c:
		case 0x3c:
		case 0x4c:
		case 0x5c:
		case 0x6c:
		case 0x7c:
			tmpv2 = tr_aop_ssp2mips(op>>13); // op
			MIPS_LUI(MIPS_s6,(op & 0xff));

			if( tmpv2 != -1 ) {
				__MIPS_INSN_REG(SPECIAL,MIPS_t5,MIPS_s6,MIPS_t5,0,tmpv2);  // OPs $t5, $t5, $s5
				MIPS_MOVE(MIPS_s5,MIPS_t5);
			}
			else {

				generate_op_sub_flags_prologue(MIPS_t5,MIPS_s6);
				MIPS_SUBU(MIPS_s5,MIPS_t5,MIPS_s6);
				generate_op_sub_flags_epilogue(MIPS_s5);
			}
			hostreg_sspreg_changed(SSP_A);
			known_regb &= ~(KRREG_A|KRREG_AL|KRREG_ST);
			dirty_regb |= KRREG_ST;
			ret++; break;
	}

	n_in_ops++;

	return ret;
}

static void emit_block_prologue(void)
{
	// check if there are enough cycles..
	// note: r0 must contain PC of current block
	MIPS_BGTZ(MIPS_s2,3);
	MIPS_NOP();
	emith_jump(ssp_drc_end);
}

/* cond:
 * >0: direct (un)conditional jump
 * <0: indirect jump
 */
static void *emit_block_epilogue(int cycles, int cond, int pc, int end_pc)
{
	void *end_ptr = NULL;

	if (cycles > 0xff) {
		elprintf(EL_ANOMALY, "large cycle count: %i\n", cycles);
		cycles = 0xff;
	}
	MIPS_ADDIU(MIPS_s6,MIPS_zero,cycles);
	MIPS_SUBU(MIPS_s2,MIPS_s2,MIPS_s6);   // sub $s2, $s2, cycles

	if (cond < 0 || (end_pc >= 0x400 && pc < 0x400)) {
		// indirect jump, or rom -> iram jump, must use dispatcher
		emith_jump(ssp_drc_next);
	}
	else if (cond == A_COND_AL) {
		u32 *target = (pc < 0x400) ?
			ssp_block_table_iram[ssp->drc.iram_context * SSP_BLOCKTAB_IRAM_ONE + pc] :
			ssp_block_table[pc];
		if (target != NULL)
			emith_jump(target);
		else {
			int ops = emith_jump(ssp_drc_next);
			end_ptr = tcache_ptr;
			// cause the next block to be emitted over jump instruction
			tcache_ptr -= ops;
		}
	}
	else {
		u32 *target1 = (pc     < 0x400) ?
			ssp_block_table_iram[ssp->drc.iram_context * SSP_BLOCKTAB_IRAM_ONE + pc] :
			ssp_block_table[pc];
		u32 *target2 = (end_pc < 0x400) ?
			ssp_block_table_iram[ssp->drc.iram_context * SSP_BLOCKTAB_IRAM_ONE + end_pc] :
			ssp_block_table[end_pc];

		if (target1 != NULL) {
			switch(cond) {
				case A_COND_AL: break;
				case A_COND_EQ: MIPS_BNEZ(MIPS_s5,3);MIPS_NOP();break;
				case A_COND_NE: MIPS_BEQZ(MIPS_s5,3);MIPS_NOP();break;
				case A_COND_MI: MIPS_BGEZ(MIPS_s5,3);MIPS_NOP();break;
				case A_COND_PL: MIPS_BLTZ(MIPS_s5,3);MIPS_NOP();break;
				default: break;
			}
		     emith_jump(target1);
		}

		if (target2 != NULL) {
			switch(tr_neg_cond(cond)) {
				case A_COND_AL: break;
				case A_COND_EQ: MIPS_BNEZ(MIPS_s5,3);MIPS_NOP();break;
				case A_COND_NE: MIPS_BEQZ(MIPS_s5,3);MIPS_NOP();break;
				case A_COND_MI: MIPS_BGEZ(MIPS_s5,3);MIPS_NOP();break;
				case A_COND_PL: MIPS_BLTZ(MIPS_s5,3);MIPS_NOP();break;
				default: break;
			}
		     emith_jump(target2); // neg_cond, to be able to swap jumps if needed
		}

		// emit patchable branches
		if (target1 == NULL) {
			switch(cond) {
				case A_COND_AL: break;
				case A_COND_EQ: MIPS_BNEZ(MIPS_s5,3);MIPS_NOP();break;
				case A_COND_NE: MIPS_BEQZ(MIPS_s5,3);MIPS_NOP();break;
				case A_COND_MI: MIPS_BGEZ(MIPS_s5,3);MIPS_NOP();break;
				case A_COND_PL: MIPS_BLTZ(MIPS_s5,3);MIPS_NOP();break;
				default: break;
			}
			emith_call(ssp_drc_next_patch);
		}
		if (target2 == NULL) {
			switch(tr_neg_cond(cond)) {
				case A_COND_AL: break;
				case A_COND_EQ: MIPS_BNEZ(MIPS_s5,3);MIPS_NOP();break;
				case A_COND_NE: MIPS_BEQZ(MIPS_s5,3);MIPS_NOP();break;
				case A_COND_MI: MIPS_BGEZ(MIPS_s5,3);MIPS_NOP();break;
				case A_COND_PL: MIPS_BLTZ(MIPS_s5,3);MIPS_NOP();break;
				default: break;
			}
			emith_call(ssp_drc_next_patch);
		}
	}

	if (end_ptr == NULL)
		end_ptr = tcache_ptr;

	return end_ptr;
}

void *ssp_translate_block(int pc)
{
	unsigned int op, op1, imm, ccount = 0;
	unsigned int *block_start, *block_end;
	int ret, end_cond = A_COND_AL, jump_pc = -1;

	//printf("translate %04x -> %04x\n", pc<<1, (tcache_ptr-tcache)<<2);

	block_start = tcache_ptr;
	known_regb = 0;
	dirty_regb = KRREG_P;
	known_regs.emu_status = 0;
	hostreg_clear();

	emit_block_prologue();

	for (; ccount < 100;)
	{
		op = PROGRAM(pc++);
		op1 = op >> 9;
		imm = (u32)-1;

		if ((op1 & 0xf) == 4 || (op1 & 0xf) == 6)
			imm = PROGRAM(pc++); // immediate

		ret = translate_op(op, &pc, imm, &end_cond, &jump_pc);
		if (ret <= 0)
		{
			elprintf(EL_ANOMALY, "NULL func! op=%08x (%02x)\n", op, op1);
			//exit(1);
		}

		ccount += ret & 0xffff;
		if (ret & 0x10000) break;
	}

	if (ccount >= 100) {
		end_cond = A_COND_AL;
		jump_pc = pc;
		MIPS_ADDIU(MIPS_a0,MIPS_zero,pc);
	}

	tr_flush_dirty_prs();
	tr_flush_dirty_ST();
	tr_flush_dirty_pmcrs();
	block_end = emit_block_epilogue(ccount, end_cond, jump_pc, pc);

	if (tcache_ptr - (u32 *)tcache > DRC_TCACHE_SIZE/4) {
		elprintf(EL_ANOMALY|EL_STATUS|EL_SVP, "tcache overflow!\n");
		exit(1);
	}

	// stats
	nblocks++;
#if 0
	printf("%i blocks, %i bytes, k=%.3f\n", nblocks, ((void *)tcache_ptr - (void *)tcache)*4,
		(double)((void *)tcache_ptr - (void *)tcache) / (double)n_in_ops);
#endif

#ifdef DUMP_BLOCK
	{
		FILE *f = fopen("tcache.bin", "wb");
		fwrite((void *)tcache, 1, ((void *)tcache_ptr - (void *)tcache)*4, f);
		fclose(f);
	}
	printf("dumped tcache.bin\n");
	exit(0);
#endif

#ifdef PSP
	cache_flush_d_inval_i(block_start, block_end);
#endif

	return block_start;
}



// -----------------------------------------------------

static void ssp1601_state_load(void)
{
	ssp->drc.iram_dirty = 1;
	ssp->drc.iram_context = 0;
}

void ssp1601_dyn_exit(void)
{
	free(ssp_block_table);
	free(ssp_block_table_iram);
	ssp_block_table = ssp_block_table_iram = NULL;

	drc_cmn_cleanup();
}

int ssp1601_dyn_startup(void)
{
	drc_cmn_init();

	ssp_block_table = calloc(sizeof(ssp_block_table[0]), SSP_BLOCKTAB_ENTS);
	if (ssp_block_table == NULL)
		return -1;
	ssp_block_table_iram = calloc(sizeof(ssp_block_table_iram[0]), SSP_BLOCKTAB_IRAM_ENTS);
	if (ssp_block_table_iram == NULL) {
		free(ssp_block_table);
		return -1;
	}

	memset(tcache, 0, DRC_TCACHE_SIZE);
	tcache_ptr = (void *)tcache;

	PicoLoadStateHook = ssp1601_state_load;

	n_in_ops = 0;
#ifdef PSP
	// hle'd blocks
	ssp_block_table[0x800/2] = (void *) ssp_hle_800;
	ssp_block_table[0x902/2] = (void *) ssp_hle_902;
	ssp_block_table_iram[ 7 * SSP_BLOCKTAB_IRAM_ONE + 0x030/2] = (void *) ssp_hle_07_030;
	ssp_block_table_iram[ 7 * SSP_BLOCKTAB_IRAM_ONE + 0x036/2] = (void *) ssp_hle_07_036;
	ssp_block_table_iram[ 7 * SSP_BLOCKTAB_IRAM_ONE + 0x6d6/2] = (void *) ssp_hle_07_6d6;
	ssp_block_table_iram[11 * SSP_BLOCKTAB_IRAM_ONE + 0x12c/2] = (void *) ssp_hle_11_12c;
	ssp_block_table_iram[11 * SSP_BLOCKTAB_IRAM_ONE + 0x384/2] = (void *) ssp_hle_11_384;
	ssp_block_table_iram[11 * SSP_BLOCKTAB_IRAM_ONE + 0x38a/2] = (void *) ssp_hle_11_38a;
#endif

	return 0;
}


void ssp1601_dyn_reset(ssp1601_t *ssp)
{
	ssp1601_reset(ssp);
	ssp->drc.iram_dirty = 1;
	ssp->drc.iram_context = 0;
	// must do this here because ssp is not available @ startup()
	ssp->drc.ptr_rom = (u32) Pico.rom;
	ssp->drc.ptr_iram_rom = (u32) svp->iram_rom;
	ssp->drc.ptr_dram = (u32) svp->dram;
	ssp->drc.ptr_btable = (u32) ssp_block_table;
	ssp->drc.ptr_btable_iram = (u32) ssp_block_table_iram;

	// prevent new versions of IRAM from appearing
	memset(svp->iram_rom, 0, 0x800);
}


void ssp1601_dyn_run(int cycles)
{
	if (ssp->emu_status & SSP_WAIT_MASK) return;

#ifdef DUMP_BLOCK
	ssp_translate_block(DUMP_BLOCK >> 1);
#endif
#ifdef PSP
	ssp_drc_entry(ssp, cycles);
#endif
}

