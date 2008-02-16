// 187 blocks, 12072 bytes
// 14 IRAM blocks

#include "../../PicoInt.h"

#define TCACHE_SIZE (256*1024)
static unsigned int *block_table[0x5090/2];
static unsigned int *block_table_iram[15][0x800/2];
static unsigned int *tcache = NULL;
static unsigned int *tcache_ptr = NULL;

static int had_jump = 0;
static int nblocks = 0;
static int iram_context = 0;

#define EMBED_INTERPRETER
#define ssp1601_reset ssp1601_reset_local
#define ssp1601_run ssp1601_run_local

static unsigned int interp_get_pc(void);

#define GET_PC interp_get_pc
#define GET_PPC_OFFS() (interp_get_pc()*2 - 2)
#define SET_PC(d) { had_jump = 1; rPC = d; }		/* must return to dispatcher after this */
//#define GET_PC() (PC - (unsigned short *)svp->iram_rom)
//#define GET_PPC_OFFS() ((unsigned int)PC - (unsigned int)svp->iram_rom - 2)
//#define SET_PC(d) PC = (unsigned short *)svp->iram_rom + d

#include "ssp16.c"

// -----------------------------------------------------

// ld d, s
static void op00(unsigned int op, unsigned int imm)
{
	unsigned int tmpv;
	if (op == 0) return; // nop
	if (op == ((SSP_A<<4)|SSP_P)) { // A <- P
		// not sure. MAME claims that only hi word is transfered.
		read_P(); // update P
		rA32 = rP.v;
	}
	else
	{
		tmpv = REG_READ(op & 0x0f);
		REG_WRITE((op & 0xf0) >> 4, tmpv);
	}
}

// ld d, (ri)
static void op01(unsigned int op, unsigned int imm)
{
	unsigned int tmpv;
	tmpv = ptr1_read(op); REG_WRITE((op & 0xf0) >> 4, tmpv);
}

// ld (ri), s
static void op02(unsigned int op, unsigned int imm)
{
	unsigned int tmpv;
	tmpv = REG_READ((op & 0xf0) >> 4); ptr1_write(op, tmpv);
}

// ldi d, imm
static void op04(unsigned int op, unsigned int imm)
{
	REG_WRITE((op & 0xf0) >> 4, imm);
}

// ld d, ((ri))
static void op05(unsigned int op, unsigned int imm)
{
	unsigned int tmpv;
	tmpv = ptr2_read(op); REG_WRITE((op & 0xf0) >> 4, tmpv);
}

// ldi (ri), imm
static void op06(unsigned int op, unsigned int imm)
{
	ptr1_write(op, imm);
}

// ld adr, a
static void op07(unsigned int op, unsigned int imm)
{
	ssp->RAM[op & 0x1ff] = rA;
}

// ld d, ri
static void op09(unsigned int op, unsigned int imm)
{
	unsigned int tmpv;
	tmpv = rIJ[(op&3)|((op>>6)&4)]; REG_WRITE((op & 0xf0) >> 4, tmpv);
}

// ld ri, s
static void op0a(unsigned int op, unsigned int imm)
{
	rIJ[(op&3)|((op>>6)&4)] = REG_READ((op & 0xf0) >> 4);
}

// ldi ri, simm (also op0d op0e op0f)
static void op0c(unsigned int op, unsigned int imm)
{
	rIJ[(op>>8)&7] = op;
}

// call cond, addr
static void op24(unsigned int op, unsigned int imm)
{
	int cond = 0;
	do {
		COND_CHECK
		if (cond) { int new_PC = imm; write_STACK(GET_PC()); write_PC(new_PC); }
	}
	while (0);
}

// ld d, (a)
static void op25(unsigned int op, unsigned int imm)
{
	unsigned int tmpv;
	tmpv = ((unsigned short *)svp->iram_rom)[rA]; REG_WRITE((op & 0xf0) >> 4, tmpv);
}

// bra cond, addr
static void op26(unsigned int op, unsigned int imm)
{
	do
	{
		int cond = 0;
		COND_CHECK
		if (cond) write_PC(imm);
	}
	while(0);
}

// mod cond, op
static void op48(unsigned int op, unsigned int imm)
{
	do
	{
		int cond = 0;
		COND_CHECK
		if (cond) {
			switch (op & 7) {
				case 2: rA32 = (signed int)rA32 >> 1; break; // shr (arithmetic)
				case 3: rA32 <<= 1; break; // shl
				case 6: rA32 = -(signed int)rA32; break; // neg
				case 7: if ((int)rA32 < 0) rA32 = -(signed int)rA32; break; // abs
				default: elprintf(EL_SVP|EL_ANOMALY, "ssp FIXME: unhandled mod %i @ %04x",
							 op&7, GET_PPC_OFFS());
			}
			UPD_ACC_ZN // ?
		}
	}
	while(0);
}

// mpys?
static void op1b(unsigned int op, unsigned int imm)
{
	read_P(); // update P
	rA32 -= rP.v;			// maybe only upper word?
	UPD_ACC_ZN			// there checking flags after this
	rX = ptr1_read_(op&3, 0, (op<<1)&0x18); // ri (maybe rj?)
	rY = ptr1_read_((op>>4)&3, 4, (op>>3)&0x18); // rj
}

// mpya (rj), (ri), b
static void op4b(unsigned int op, unsigned int imm)
{
	read_P(); // update P
	rA32 += rP.v; // confirmed to be 32bit
	UPD_ACC_ZN // ?
	rX = ptr1_read_(op&3, 0, (op<<1)&0x18); // ri (maybe rj?)
	rY = ptr1_read_((op>>4)&3, 4, (op>>3)&0x18); // rj
}

// mld (rj), (ri), b
static void op5b(unsigned int op, unsigned int imm)
{
	rA32 = 0;
	rST &= 0x0fff; // ?
	rX = ptr1_read_(op&3, 0, (op<<1)&0x18); // ri (maybe rj?)
	rY = ptr1_read_((op>>4)&3, 4, (op>>3)&0x18); // rj
}

// OP a, s
static void op10(unsigned int op, unsigned int imm)
{
	do
	{
		unsigned int tmpv;
		OP_CHECK32(OP_SUBA32); tmpv = REG_READ(op & 0x0f); OP_SUBA(tmpv);
	}
	while(0);
}

static void op30(unsigned int op, unsigned int imm)
{
	do
	{
		unsigned int tmpv;
		OP_CHECK32(OP_CMPA32); tmpv = REG_READ(op & 0x0f); OP_CMPA(tmpv);
	}
	while(0);
}

static void op40(unsigned int op, unsigned int imm)
{
	do
	{
		unsigned int tmpv;
		OP_CHECK32(OP_ADDA32); tmpv = REG_READ(op & 0x0f); OP_ADDA(tmpv);
	}
	while(0);
}

static void op50(unsigned int op, unsigned int imm)
{
	do
	{
		unsigned int tmpv;
		OP_CHECK32(OP_ANDA32); tmpv = REG_READ(op & 0x0f); OP_ANDA(tmpv);
	}
	while(0);
}

static void op60(unsigned int op, unsigned int imm)
{
	do
	{
		unsigned int tmpv;
		OP_CHECK32(OP_ORA32 ); tmpv = REG_READ(op & 0x0f); OP_ORA (tmpv);
	}
	while(0);
}

static void op70(unsigned int op, unsigned int imm)
{
	do
	{
		unsigned int tmpv;
		OP_CHECK32(OP_EORA32); tmpv = REG_READ(op & 0x0f); OP_EORA(tmpv);
	}
	while(0);
}

// OP a, (ri)
static void op11(unsigned int op, unsigned int imm)
{
	unsigned int tmpv;
	tmpv = ptr1_read(op); OP_SUBA(tmpv);
}

static void op31(unsigned int op, unsigned int imm)
{
	unsigned int tmpv;
	tmpv = ptr1_read(op); OP_CMPA(tmpv);
}

static void op41(unsigned int op, unsigned int imm)
{
	unsigned int tmpv;
	tmpv = ptr1_read(op); OP_ADDA(tmpv);
}

static void op51(unsigned int op, unsigned int imm)
{
	unsigned int tmpv;
	tmpv = ptr1_read(op); OP_ANDA(tmpv);
}

static void op61(unsigned int op, unsigned int imm)
{
	unsigned int tmpv;
	tmpv = ptr1_read(op); OP_ORA (tmpv);
}

static void op71(unsigned int op, unsigned int imm)
{
	unsigned int tmpv;
	tmpv = ptr1_read(op); OP_EORA(tmpv);
}

// OP a, adr
static void op03(unsigned int op, unsigned int imm)
{
	unsigned int tmpv;
	tmpv = ssp->RAM[op & 0x1ff]; OP_LDA (tmpv);
}

static void op13(unsigned int op, unsigned int imm)
{
	unsigned int tmpv;
	tmpv = ssp->RAM[op & 0x1ff]; OP_SUBA(tmpv);
}

static void op33(unsigned int op, unsigned int imm)
{
	unsigned int tmpv;
	tmpv = ssp->RAM[op & 0x1ff]; OP_CMPA(tmpv);
}

static void op43(unsigned int op, unsigned int imm)
{
	unsigned int tmpv;
	tmpv = ssp->RAM[op & 0x1ff]; OP_ADDA(tmpv);
}

static void op53(unsigned int op, unsigned int imm)
{
	unsigned int tmpv;
	tmpv = ssp->RAM[op & 0x1ff]; OP_ANDA(tmpv);
}

static void op63(unsigned int op, unsigned int imm)
{
	unsigned int tmpv;
	tmpv = ssp->RAM[op & 0x1ff]; OP_ORA (tmpv);
}

static void op73(unsigned int op, unsigned int imm)
{
	unsigned int tmpv;
	tmpv = ssp->RAM[op & 0x1ff]; OP_EORA(tmpv);
}

// OP a, imm
static void op14(unsigned int op, unsigned int imm)
{
	OP_SUBA(imm);
}

static void op34(unsigned int op, unsigned int imm)
{
	OP_CMPA(imm);
}

static void op44(unsigned int op, unsigned int imm)
{
	OP_ADDA(imm);
}

static void op54(unsigned int op, unsigned int imm)
{
	OP_ANDA(imm);
}

static void op64(unsigned int op, unsigned int imm)
{
	OP_ORA (imm);
}

static void op74(unsigned int op, unsigned int imm)
{
	OP_EORA(imm);
}

// OP a, ((ri))
static void op15(unsigned int op, unsigned int imm)
{
	unsigned int tmpv;
	tmpv = ptr2_read(op); OP_SUBA(tmpv);
}

static void op35(unsigned int op, unsigned int imm)
{
	unsigned int tmpv;
	tmpv = ptr2_read(op); OP_CMPA(tmpv);
}

static void op45(unsigned int op, unsigned int imm)
{
	unsigned int tmpv;
	tmpv = ptr2_read(op); OP_ADDA(tmpv);
}

static void op55(unsigned int op, unsigned int imm)
{
	unsigned int tmpv;
	tmpv = ptr2_read(op); OP_ANDA(tmpv);
}

static void op65(unsigned int op, unsigned int imm)
{
	unsigned int tmpv;
	tmpv = ptr2_read(op); OP_ORA (tmpv);
}

static void op75(unsigned int op, unsigned int imm)
{
	unsigned int tmpv;
	tmpv = ptr2_read(op); OP_EORA(tmpv);
}

// OP a, ri
static void op19(unsigned int op, unsigned int imm)
{
	unsigned int tmpv;
	tmpv = rIJ[IJind]; OP_SUBA(tmpv);
}

static void op39(unsigned int op, unsigned int imm)
{
	unsigned int tmpv;
	tmpv = rIJ[IJind]; OP_CMPA(tmpv);
}

static void op49(unsigned int op, unsigned int imm)
{
	unsigned int tmpv;
	tmpv = rIJ[IJind]; OP_ADDA(tmpv);
}

static void op59(unsigned int op, unsigned int imm)
{
	unsigned int tmpv;
	tmpv = rIJ[IJind]; OP_ANDA(tmpv);
}

static void op69(unsigned int op, unsigned int imm)
{
	unsigned int tmpv;
	tmpv = rIJ[IJind]; OP_ORA (tmpv);
}

static void op79(unsigned int op, unsigned int imm)
{
	unsigned int tmpv;
	tmpv = rIJ[IJind]; OP_EORA(tmpv);
}

// OP simm
static void op1c(unsigned int op, unsigned int imm)
{
	OP_SUBA(op & 0xff);
}

static void op3c(unsigned int op, unsigned int imm)
{
	OP_CMPA(op & 0xff);
}

static void op4c(unsigned int op, unsigned int imm)
{
	OP_ADDA(op & 0xff);
}

static void op5c(unsigned int op, unsigned int imm)
{
	OP_ANDA(op & 0xff);
}

static void op6c(unsigned int op, unsigned int imm)
{
	OP_ORA (op & 0xff);
}

static void op7c(unsigned int op, unsigned int imm)
{
	OP_EORA(op & 0xff);
}

typedef void (in_func)(unsigned int op, unsigned int imm);

static in_func *in_funcs[0x80] =
{
	op00, op01, op02, op03, op04, op05, op06, op07,
	NULL, op09, op0a, NULL, op0c, op0c, op0c, op0c,
	op10, op11, NULL, op13, op14, op15, NULL, NULL,
	NULL, op19, NULL, op1b, op1c, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, op24, op25, op26, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	op30, op31, NULL, op33, op34, op35, NULL, NULL,
	NULL, op39, NULL, NULL, op3c, NULL, NULL, NULL,
	op40, op41, NULL, op43, op44, op45, NULL, NULL,
	op48, op49, NULL, op4b, op4c, NULL, NULL, NULL,
	op50, op51, NULL, op53, op54, op55, NULL, NULL,
	NULL, op59, NULL, op5b, op5c, NULL, NULL, NULL,
	op60, op61, NULL, op63, op64, op65, NULL, NULL,
	NULL, op69, NULL, NULL, op6c, NULL, NULL, NULL,
	op70, op71, NULL, op73, op74, op75, NULL, NULL,
	NULL, op79, NULL, NULL, op7c, NULL, NULL, NULL,
};

// -----------------------------------------------------

static unsigned int crctable[256] =
{
 0x00000000L, 0x77073096L, 0xEE0E612CL, 0x990951BAL,
 0x076DC419L, 0x706AF48FL, 0xE963A535L, 0x9E6495A3L,
 0x0EDB8832L, 0x79DCB8A4L, 0xE0D5E91EL, 0x97D2D988L,
 0x09B64C2BL, 0x7EB17CBDL, 0xE7B82D07L, 0x90BF1D91L,
 0x1DB71064L, 0x6AB020F2L, 0xF3B97148L, 0x84BE41DEL,
 0x1ADAD47DL, 0x6DDDE4EBL, 0xF4D4B551L, 0x83D385C7L,
 0x136C9856L, 0x646BA8C0L, 0xFD62F97AL, 0x8A65C9ECL,
 0x14015C4FL, 0x63066CD9L, 0xFA0F3D63L, 0x8D080DF5L,
 0x3B6E20C8L, 0x4C69105EL, 0xD56041E4L, 0xA2677172L,
 0x3C03E4D1L, 0x4B04D447L, 0xD20D85FDL, 0xA50AB56BL,
 0x35B5A8FAL, 0x42B2986CL, 0xDBBBC9D6L, 0xACBCF940L,
 0x32D86CE3L, 0x45DF5C75L, 0xDCD60DCFL, 0xABD13D59L,
 0x26D930ACL, 0x51DE003AL, 0xC8D75180L, 0xBFD06116L,
 0x21B4F4B5L, 0x56B3C423L, 0xCFBA9599L, 0xB8BDA50FL,
 0x2802B89EL, 0x5F058808L, 0xC60CD9B2L, 0xB10BE924L,
 0x2F6F7C87L, 0x58684C11L, 0xC1611DABL, 0xB6662D3DL,
 0x76DC4190L, 0x01DB7106L, 0x98D220BCL, 0xEFD5102AL,
 0x71B18589L, 0x06B6B51FL, 0x9FBFE4A5L, 0xE8B8D433L,
 0x7807C9A2L, 0x0F00F934L, 0x9609A88EL, 0xE10E9818L,
 0x7F6A0DBBL, 0x086D3D2DL, 0x91646C97L, 0xE6635C01L,
 0x6B6B51F4L, 0x1C6C6162L, 0x856530D8L, 0xF262004EL,
 0x6C0695EDL, 0x1B01A57BL, 0x8208F4C1L, 0xF50FC457L,
 0x65B0D9C6L, 0x12B7E950L, 0x8BBEB8EAL, 0xFCB9887CL,
 0x62DD1DDFL, 0x15DA2D49L, 0x8CD37CF3L, 0xFBD44C65L,
 0x4DB26158L, 0x3AB551CEL, 0xA3BC0074L, 0xD4BB30E2L,
 0x4ADFA541L, 0x3DD895D7L, 0xA4D1C46DL, 0xD3D6F4FBL,
 0x4369E96AL, 0x346ED9FCL, 0xAD678846L, 0xDA60B8D0L,
 0x44042D73L, 0x33031DE5L, 0xAA0A4C5FL, 0xDD0D7CC9L,
 0x5005713CL, 0x270241AAL, 0xBE0B1010L, 0xC90C2086L,
 0x5768B525L, 0x206F85B3L, 0xB966D409L, 0xCE61E49FL,
 0x5EDEF90EL, 0x29D9C998L, 0xB0D09822L, 0xC7D7A8B4L,
 0x59B33D17L, 0x2EB40D81L, 0xB7BD5C3BL, 0xC0BA6CADL,
 0xEDB88320L, 0x9ABFB3B6L, 0x03B6E20CL, 0x74B1D29AL,
 0xEAD54739L, 0x9DD277AFL, 0x04DB2615L, 0x73DC1683L,
 0xE3630B12L, 0x94643B84L, 0x0D6D6A3EL, 0x7A6A5AA8L,
 0xE40ECF0BL, 0x9309FF9DL, 0x0A00AE27L, 0x7D079EB1L,
 0xF00F9344L, 0x8708A3D2L, 0x1E01F268L, 0x6906C2FEL,
 0xF762575DL, 0x806567CBL, 0x196C3671L, 0x6E6B06E7L,
 0xFED41B76L, 0x89D32BE0L, 0x10DA7A5AL, 0x67DD4ACCL,
 0xF9B9DF6FL, 0x8EBEEFF9L, 0x17B7BE43L, 0x60B08ED5L,
 0xD6D6A3E8L, 0xA1D1937EL, 0x38D8C2C4L, 0x4FDFF252L,
 0xD1BB67F1L, 0xA6BC5767L, 0x3FB506DDL, 0x48B2364BL,
 0xD80D2BDAL, 0xAF0A1B4CL, 0x36034AF6L, 0x41047A60L,
 0xDF60EFC3L, 0xA867DF55L, 0x316E8EEFL, 0x4669BE79L,
 0xCB61B38CL, 0xBC66831AL, 0x256FD2A0L, 0x5268E236L,
 0xCC0C7795L, 0xBB0B4703L, 0x220216B9L, 0x5505262FL,
 0xC5BA3BBEL, 0xB2BD0B28L, 0x2BB45A92L, 0x5CB36A04L,
 0xC2D7FFA7L, 0xB5D0CF31L, 0x2CD99E8BL, 0x5BDEAE1DL,
 0x9B64C2B0L, 0xEC63F226L, 0x756AA39CL, 0x026D930AL,
 0x9C0906A9L, 0xEB0E363FL, 0x72076785L, 0x05005713L,
 0x95BF4A82L, 0xE2B87A14L, 0x7BB12BAEL, 0x0CB61B38L,
 0x92D28E9BL, 0xE5D5BE0DL, 0x7CDCEFB7L, 0x0BDBDF21L,
 0x86D3D2D4L, 0xF1D4E242L, 0x68DDB3F8L, 0x1FDA836EL,
 0x81BE16CDL, 0xF6B9265BL, 0x6FB077E1L, 0x18B74777L,
 0x88085AE6L, 0xFF0F6A70L, 0x66063BCAL, 0x11010B5CL,
 0x8F659EFFL, 0xF862AE69L, 0x616BFFD3L, 0x166CCF45L,
 0xA00AE278L, 0xD70DD2EEL, 0x4E048354L, 0x3903B3C2L,
 0xA7672661L, 0xD06016F7L, 0x4969474DL, 0x3E6E77DBL,
 0xAED16A4AL, 0xD9D65ADCL, 0x40DF0B66L, 0x37D83BF0L,
 0xA9BCAE53L, 0xDEBB9EC5L, 0x47B2CF7FL, 0x30B5FFE9L,
 0xBDBDF21CL, 0xCABAC28AL, 0x53B39330L, 0x24B4A3A6L,
 0xBAD03605L, 0xCDD70693L, 0x54DE5729L, 0x23D967BFL,
 0xB3667A2EL, 0xC4614AB8L, 0x5D681B02L, 0x2A6F2B94L,
 0xB40BBE37L, 0xC30C8EA1L, 0x5A05DF1BL, 0x2D02EF8DL
};

static u32 chksum_crc32 (unsigned char *block, unsigned int length)
{
   register u32 crc;
   unsigned long i;

   crc = 0xFFFFFFFF;
   for (i = 0; i < length; i++)
   {
      crc = ((crc >> 8) & 0x00FFFFFF) ^ crctable[(crc ^ *block++) & 0xFF];
   }
   return (crc ^ 0xFFFFFFFF);
}

//static int iram_crcs[32] = { 0, };

// -----------------------------------------------------

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

static unsigned int checksums[] =
{
	0,
	0xfa9ddfb2,
	0x229c80b6,
	0x3af0c3d3,
	0x98fc4552,
	0x5ecacdbc,
	0xa6931962,
	0x53930b10,
	0x69524552,
	0xcb1ccdaf,
	0x995068c7,
	0x48b97f4d,
	0xe8c61b74,
	0xafa2e81a,
	0x4e3e071a
};


static int get_iram_context(void)
{
	unsigned char *ir = (unsigned char *)svp->iram_rom;
	int val1, val = ir[0x083^1] + ir[0x4FA^1] + ir[0x5F7^1] + ir[0x47B^1];
	int crc = chksum_crc32(svp->iram_rom, 0x800);
	val1 = iram_context_map[(val>>1)&0x3f];

	if (crc != checksums[val1] || val1 == 0) {
		printf("val: %02x PC=%04x\n", (val>>1)&0x3f, rPC);
		elprintf(EL_ANOMALY, "bad crc: %08x vs %08x", crc, checksums[val1]);
		//debug_dump2file(name, svp->iram_rom, 0x800);
		exit(1);
	}
	elprintf(EL_ANOMALY, "iram_context: %02i", val1);
	return val1;
}

#define PROGRAM(x) ((unsigned short *)svp->iram_rom)[x]

static u32 interp_get_pc(void)
{
#if 0
	unsigned short *pc1 = PC;
	int i;

	while (pc1[-1] != 0xfe01) pc1--;		// goto current block start

	if (rPC >= 0x800/2)
	{
		for (i = 0; i < 0x5090/2; i++)
			if (block_table[i] == pc1) break;
		if (i == 0x5090/2) goto fail;
	}
	else
	{
		for (i = 0; i < 0x800/2; i++)
			if (block_table_iram[iram_context][i] == pc1) break;
		if (i == 0x800/2) goto fail;
	}

	return i + (PC - pc1);
fail:
	printf("block not found!\n");
	exit(1);
#else
	return rPC;
#endif
}

static void *translate_block(int pc)
{
	unsigned int op, op1, icount = 0;
	void *ret;

	ret = tcache_ptr;

	//printf("translate %04x -> %04x\n", pc<<1, (tcache_ptr-tcache)<<1);
	for (;;)
	{
		icount++;
		op = PROGRAM(pc++);
		op1 = op >> 9;
		// need immediate?
		if ((op1 & 0xf) == 4 || (op1 & 0xf) == 6) {
			op |= PROGRAM(pc++) << 16; // immediate
		}
		*tcache_ptr++ = op;
		*tcache_ptr = (unsigned int) in_funcs[op1];
		if (*tcache_ptr == 0) {
			printf("NULL func! op=%08x (%02x)\n", op, op1);
			exit(1);
		}
		tcache_ptr++;
		if (op1 == 0x24 || op1 == 0x26 || // call, bra
			((op1 == 0 || op1 == 1 || op1 == 4 || op1 == 5 || op1 == 9 || op1 == 0x25) &&
				(op & 0xf0) == 0x60)) { // ld PC
			break;
		}
	}
	*tcache_ptr++ = 0xfe01;
	*tcache_ptr++ = 0xfe01; // end of block
	//printf("  %i inst\n", icount);

	if (tcache_ptr - tcache > TCACHE_SIZE/4) {
		printf("tcache overflow!\n");
		fflush(stdout);
		exit(1);
	}

	// stats
	nblocks++;
	//if (pc >= 0x400)
	printf("%i blocks, %i bytes\n", nblocks, (tcache_ptr - tcache)*4);

	return ret;
}



// -----------------------------------------------------

int ssp1601_dyn_init(void)
{
	tcache = tcache_ptr = malloc(TCACHE_SIZE);
	memset(tcache, 0, sizeof(TCACHE_SIZE));
	memset(block_table, 0, sizeof(block_table));
	memset(block_table_iram, 0, sizeof(block_table_iram));
	*tcache_ptr++ = 0xfe01;
	*tcache_ptr++ = 0xfe01;

	return 0;
}


void ssp1601_dyn_reset(ssp1601_t *ssp)
{
	ssp1601_reset_local(ssp);
}


static void ssp1601_run2(unsigned int *iPC)
{
	in_func *func;
	unsigned int op, op1, imm;
	while (*iPC != 0xfe01)
	{
		rPC++;
		op = *iPC & 0xffff;
		imm = *iPC++ >> 16;
		op1 = op >> 9;
		if ((op1 & 0xf) == 4 || (op1 & 0xf) == 6) rPC++;
		PC = ((unsigned short *)&op) + 1; /* needed for interpreter */

		func = (in_func *) *iPC++;
		func(op, imm);
		g_cycles--;
	}
}


void ssp1601_dyn_run(int cycles)
{
	while (cycles > 0)
	{
		unsigned int *iPC;
		//int pc_old = rPC;
		if (rPC < 0x800/2)
		{
			if (iram_dirty) {
				iram_context = get_iram_context();
				iram_dirty--;
			}
			if (block_table_iram[iram_context][rPC] == NULL)
				block_table_iram[iram_context][rPC] = translate_block(rPC);
			iPC = block_table_iram[iram_context][rPC];
		}
		else
		{
			if (block_table[rPC] == NULL)
				block_table[rPC] = translate_block(rPC);
			iPC = block_table[rPC];
		}

		had_jump = 0;

		//printf("enter @ %04x, PC=%04x\n", (PC - tcache)<<1, rPC<<1);
		g_cycles = 0;
		ssp1601_run2(iPC);
		cycles += g_cycles;
/*
		if (!had_jump) {
			// no jumps
			if (pc_old < 0x800/2)
				rPC += (PC - block_table_iram[iram_context][pc_old]) - 1;
			else
				rPC += (PC - block_table[pc_old]) - 1;
		}
*/
		//printf("end   @ %04x, PC=%04x\n", (PC - tcache)<<1, rPC<<1);
/*
		if (pc_old < 0x400) {
			// flush IRAM cache
			tcache_ptr = block_table[pc_old];
			block_table[pc_old] = NULL;
			nblocks--;
		}
		if (pc_old >= 0x400 && rPC < 0x400)
		{
			int i, crc = chksum_crc32(svp->iram_rom, 0x800);
			for (i = 0; i < 32; i++)
				if (iram_crcs[i] == crc) break;
			if (i == 32) {
				char name[32];
				for (i = 0; i < 32 && iram_crcs[i]; i++);
				iram_crcs[i] = crc;
				printf("%i IRAMs\n", i+1);
				sprintf(name, "ir%08x.bin", crc);
				debug_dump2file(name, svp->iram_rom, 0x800);
			}
			printf("CRC %08x %08x\n", crc, iram_id);
		}
*/
	}
//	debug_dump2file("tcache.bin", tcache, (tcache_ptr - tcache) << 1);
//	exit(1);
}
