#*
#* Compiler helper functions and some SVP HLE code
#* Robson Santana, 2016
#*
#* This work is licensed under the terms of MAME license.
#* See COPYING file in the top-level directory.
#*

#include "../../mips_features.h"

.text
.align 2

#       SSP_G$a0, SSP_X,     SSP_Y,   SSP_A,
#       SSP_ST,  SSP_STACK, SSP_PC,  SSP_P,
#       SSP_PM0, SSP_PM1,   SSP_PM2, SSP_XST,
#       SSP_PM4, SSP_gr13,  SSP_PMC, SSP_AL

# register map:
# $t4:  XXYY
# $t5:  A
# $t6:  STACK and emu flags: sss0 * .uu. .lll NZCV (NZCV is PSR bits from ARM)
# $t7:  SSP context
# $t8:  $a0-$a2 (.210)
# $t9:  $t4-$t6 (.654)
# $s1: P
# $s2: cycles
# $s3: tmp


#define SSP_OFFS_GR         0x400
#define SSP_PC                  6
#define SSP_P                   7
#define SSP_PM0                 8
#define SSP_PMC                14
#define SSP_OFFS_PM_WRITE   0x46c // pmac_write()
#define SSP_OFFS_EMUSTAT    0x484 // emu_status
#define SSP_OFFS_IRAM_ROM   0x48c // ptr_iram_rom
#define SSP_OFFS_DRAM       0x490 // ptr_dram
#define SSP_OFFS_IRAM_DIRTY 0x494
#define SSP_OFFS_IRAM_CTX   0x498 // iram_context
#define SSP_OFFS_BLTAB      0x49c // block_table
#define SSP_OFFS_BLTAB_IRAM 0x4a0
#define SSP_OFFS_TMP0       0x4a4 // for entry PC
#define SSP_OFFS_TMP1       0x4a8
#define SSP_OFFS_TMP2       0x4ac
#define SSP_WAIT_PM0       0x2000


.macro ssp_drc_do_next patch_jump=0
.if \patch_jump
    sw     $s4, SSP_OFFS_TMP2($t7)		# jump insw. (actually call) address + 4
.endif
    sll     $a0, $a0, 16
    srl     $a0, $a0, 16
    sw      $a0, SSP_OFFS_TMP0($t7)
    blt     $a0, 0x400, 1f # ssp_de_iram

    lw     $a2, SSP_OFFS_BLTAB($t7)
    sll		$s5, $a0, 2
    add 	$s5, $a2, $s5
    lw     $a2, ($s5)
    and     $s5, $a2, $a2
.if \patch_jump
    bnez    $s5, ssp_drc_do_patch
    nop
.else
	beqz	$s5, 0f
	nop
    jr    $a2
    nop
.endif
0:
    jal      ssp_translate_block
    nop
    move     $a2, $a0
    lw     $a0, SSP_OFFS_TMP0($t7)		# entry PC
    lw     $a1, SSP_OFFS_BLTAB($t7)
    sll		$s5, $a0, 2
    add 	$s5, $a1, $s5
    sw     $a2, ($s5)
.if \patch_jump
    b       ssp_drc_do_patch
    nop
.else
    jr      $a2
    nop
.endif

1: # ssp_de_iram:
    lw     $a1, SSP_OFFS_IRAM_DIRTY($t7)
    and     $s5, $a1, $a1
    bnez	$s5, 2f
    nop
    lw   $a1, SSP_OFFS_IRAM_CTX($t7)
    b     3f # ssp_de_iram_ctx
    nop

2:
    jal      ssp_get_iram_context
    nop
    li     $a1, 0
    sw     $a1, SSP_OFFS_IRAM_DIRTY($t7)
    move     $a1, $a0
    sw     $a1, SSP_OFFS_IRAM_CTX($t7)
    lw     $a0, SSP_OFFS_TMP0($t7)		# entry PC

3: # ssp_de_iram_ctx:
    lw     $a2, SSP_OFFS_BLTAB_IRAM($t7)
    sll		$s5, $a1, 12
    add     $a2, $a2, $s5			# block_tab_iram + iram_context * 0x800/2*4
    sll		$s5, $a0, 2
    add     $a1, $a2, $s5
    lw     $a2, ($a1)
    and     $s5, $a2, $a2
.if \patch_jump
    bnez    $s5, ssp_drc_do_patch
    nop
.else
	beqz	$s5, 4f
	nop
    jr    $a2
    nop
.endif
4:
    sw     $a1, SSP_OFFS_TMP1($t7)
    jal      ssp_translate_block
    nop
    move     $a2, $a0
    lw     $a0, SSP_OFFS_TMP0($t7)		# entry PC
    lw     $a1, SSP_OFFS_TMP1($t7)		# &block_table_iram(iram_context)(rPC)
    sw     $a2, ($a1)
.if \patch_jump
    b       ssp_drc_do_patch
    nop
.else
    jr      $a2
    nop
.endif
.endm # ssp_drc_do_next


FUNCTION(ssp_drc_entry):
	addiu   $sp, $sp, -24
	sw      $s6, 20($sp)
	sw      $s5, 16($sp)
	sw      $s4, 12($sp)
	sw      $s3, 8($sp)
	sw      $s2, 4($sp)
	sw      $s1, ($sp)

    move     $t7, $a0                      # ssp
    move     $s2, $a1
ssp_regfile_load:
    add     $a2, $t7, 0x400
    add     $a2, $a2, 4

    lw		$a3, ($a2)
    lw		$t4, 4($a2)
    lw		$t5, 8($a2)
    lw		$t6, 12($a2)
    lw		$t8, 16($a2)
    addiu   $a2, $a2, 20

    srl     $a3, $a3, 16
    srl     $a3, $a3, 16
    srl		$s5, $t4, 16
    or     $t4, $a3, $s5         # XXYY

    and     $t8, $t8, 0x0f0000
    sll     $t8, $t8, 13             # sss0 *
    and     $t9, $t6, 0x670000
    and     $s5, $t6,     0x80000000
    beqz	$s5, skip_1
    nop
    or   $t8, $t8, 0x8
skip_1:
    and     $s5, $t6,     0x20000000
    beqz	$s5, skip_2
    nop
    or   $t8, $t8, 0x4                # sss0 *           NZ..
skip_2:
	srl		$s5, $t9, 12
    or     $t6, $t8, $s5         # sss0 * .uu. .lll NZ..

    lw     $t8, 0x440($t7)            # $a0-$a2
    lw     $t9, 0x444($t7)            # $t4-$t6
    lw     $s1, (0x400+SSP_P*4)($t7)  # P

    lw     $a0, (SSP_OFFS_GR+SSP_PC*4)($t7)
    srl     $a0, $a0, 16


FUNCTION(ssp_drc_next):
    ssp_drc_do_next 0


FUNCTION(ssp_drc_next_patch):
    ssp_drc_do_next 1

ssp_drc_do_patch:
    lw     $a1, SSP_OFFS_TMP2($t7)	# jump insw. (actually call) address + 4
    sub    $s3,$a2, $a1
    bnez	$s5, skip_3
    nop
    li   $a3,     0xe1000000
    or   $a3, $a3, 0x00a00000		# nop
    sw   $a3, -4($a1)
    b     ssp_drc_dp_end
    nop

skip_3:
    bne     $s3,4, skip_4
    lw   $a3, ($a1)
    add   $a3, $a3, 1
    sw   $a3, -4($a1)               # movee the other cond up
    li   $a3,     0xe1000000
    or   $a3, $a3, 0x00a00000
    sw   $a3, ($a1)                    # fill it's place with nop
    b     ssp_drc_dp_end
    nop

skip_4:
    lw     $a3, -4($a1)
    sub     $s3,$s3,4
    srl     $a3, $a3, 24
    and     $a3, $a3, -2			# L bit
    sll		$s5, $s3, 6
    or     $a3, $a3, $s5
    rotr     $a3, $a3, 8              # patched branch inswuction
    sw     $a3, -4($a1)               # patch the bl/b to jump directly to another handler

ssp_drc_dp_end:
    sw     $a2, SSP_OFFS_TMP1($t7)
    sub     $a0, $a1, 4
    add     $a1, $a1, 4
    jal      cache_flush_d_inval_i
    nop
    lw     $a2, SSP_OFFS_TMP1($t7)
    lw     $a0, SSP_OFFS_TMP0($t7)
    jr      $a2
    nop


FUNCTION(ssp_drc_end):
    sll     $a0, $a0, 16
    sw     $a0, (SSP_OFFS_GR+SSP_PC*4)($t7)

ssp_regfile_store:
    sw     $s1, (0x400+SSP_P*4)($t7)  # P
    sw     $t8, 0x440($t7)            # $a0-$a2
    sw     $t9, 0x444($t7)            # $t4-$t6

    srl     $t9, $t6, 13
    and     $t9, $t9, (7<<16)            # STACK
    sll     $a3, $t6, 28
    #msr     cpsr_f, r3                  @ to to ARM PSR
    and     $t6, $t6, 0x670
    sll     $t6, $t6, 12
    and		$s5, $a3, $t6
    bgez	$s5, skip_5
    nop
    or   $t6, $t6, 0x80000000         # N
skip_5:
    and		$s5, $a3, $t6
    bnez	$s5, skip_6
    nop
    or   $t6, $t6, 0x20000000         # Z
skip_6:
    sll     $a3, $t4, 16             # Y
    srl     $a2, $t4, 16
    sll     $a2, $a2, 16             # X
    add     $t8, $t7, 0x400
    add     $t8, $t8, 4
    #stmia   $t8, {$a2,$a3,$t5,$t6,$t9}

    sw		$a2, ($t8)
    sw		$a3, 4($t8)
    sw		$t5, 8($t8)
    sw		$t6, 12($t8)
    sw		$t9, 16($t8)
    addiu   $t8, $t8, 20

    move     $a0, $s2

	lw      $s1, ($sp)
	lw      $s2, 4($sp)
	lw      $s3, 8($sp)
	lw      $s4, 12($sp)
	lw      $s5, 16($sp)
	lw      $s6, 20($sp)
	addiu   $sp, $sp, 24
	jr		$ra
	nop



# ld      A, PM0
# andi    2
# bra     z=1, gloc_0800
FUNCTION(ssp_hle_800):
    lw     $a0, (SSP_OFFS_GR+SSP_PM0*4)($t7)
    lw     $a1, SSP_OFFS_EMUSTAT($t7)
    and    $s5, $a0, 0x20000
    bnez	$s5, skip_7
    nop
    or   $a1, $a1,  SSP_WAIT_PM0
    sub   $s2,$s2, 1024
    sw   $a1, SSP_OFFS_EMUSTAT($t7)
    li     $a0,     0x400
    b     ssp_drc_end
    nop

skip_7:
    or   $a0, $a0, 0x004
    b       ssp_drc_next
    nop


.macro hle_flushflags
    and     $t6, $t6, -16
    mrs     $a1, cpsr
    srl		$s5, $a1, 28
    or     $t6, $t6, $s5
.endm

.macro hle_popstack
    sub     $t6, $t6, 0x20000000
    add     $a1, $t7, 0x400
    add     $a1, $a1, 0x048			# stack
    srl		$s5, $t6, 28
    add     $a1, $a1, $s5
    lhu    $a0, ($a1)
.endm

FUNCTION(ssp_hle_902):
    ble     $s2, 0, ssp_drc_end

    add     $a1, $t7, 0x200
    lhu    $a0, ($a1)
    lw     $a3, SSP_OFFS_IRAM_ROM($t7)
    sll		$s5, $a0, 1
    add     $a2, $a3, $s5			# ($t7|00)
    lhu    $a0, ($a2)
    add		$a2, $a2, 2
    sll     $t5, $t5, 16
    srl     $t5, $t5, 16
    and     $a0, $a0, -64513
    sll		$s5, $a0, 1
    add     $a3, $a3, $s5 			# IRAM dest
    lhu    $s3,($a2)			# length
    add		$a2, $a2, 2
    and     $a3, $a3, -4				# always seen aligned
#    or     $t5, $t5, 0x08000000
#    or     $t5, $t5, 0x00880000
#    sub     $t5, $t5, $s3, lsl 16
    and     $t6, $t6, -16
    add     $s3,$s3,1
    li     $a0, 1
    sw     $a0, SSP_OFFS_IRAM_DIRTY($t7)
    sll		$s5, $s3, 1
    sub     $s2,$s2,$s5
    sub     $s2,$s2,$s3				# -= length*3

ssp_hle_902_loop:
    lhu    $a0, ($a2)
    add		$a2, $a2, 2
    lhu    $a1, ($a2)
    add		$a2, $a2, 2
    sub    $s3,$s3,2
    sll		$s5, $a1, 16
    or     $a0, $a0, $s5
    sw     $a0, ($a3)
    add		$a3, $a3, 4
    bgtz    $s3, ssp_hle_902_loop
    nop

    and		$s5, $s3, 1
    beqz	$s5, skip_8
    nop
    lhu     $a0, ($a2)
    add		$a2, $a2, 2
    sh      $a0, ($a3)
    add		$a3, $a3, 2

skip_8:
    lw     $a0, SSP_OFFS_IRAM_ROM($t7)
    add     $a1, $t7, 0x200
    sub     $a2, $a2, $a0
    srl     $a2, $a2, 1
    sh    $a2, ($a1)				# ($t7|00)

    sub     $a0, $a3, $a0
    srl     $a0, $a0, 1
    or     $a0, $a0, 0x08000000
    or     $a0, $a0, 0x001c8000
    sw     $a0, (SSP_OFFS_GR+SSP_PMC*4)($t7)
    sw     $a0, (SSP_OFFS_PM_WRITE+4*4)($t7)

    hle_popstack
    sub    $s2,$s2,16				# timeslice is likely to end
    blez   $s2, ssp_drc_end
    nop
    b       ssp_drc_next
    nop


# this one is car rendering related
.macro hle_11_12c_mla offs_in
    lh   $t5, (\offs_in+0)($t7)
    lh   $a0, (\offs_in+2)($t7)
    lh   $a1, (\offs_in+4)($t7)
    mul     $t5, $a2, $t5
    lh   $s3, (\offs_in+6)($t7)
    mul		$s5, $a0, $a3
    add     $t5, $s5, $t5
    mul		$s5, $t4, $a1
    add     $t5, $s5, $t5
    sll		$s5, $s3, 11
    add     $t5, $t5, $s5

    srl     $t5, $t5, 13
    srl		$s5, $t8, 23
    add     $a1, $t7, $s5
    sh      $t5, ($a1)
    add     $t8, $t8, (1<<24)
.endm

FUNCTION(ssp_hle_11_12c):
    ble     $s2, 0, ssp_drc_end
    nop

    li     $a0, 0
    jal      ssp_pm_read
    nop
    move     $t4, $v0

    li     $a0, 0
    jal      ssp_pm_read
    nop
    move     $t5, $v0

    li     $a0, 0
    jal      ssp_pm_read
    nop

    sll     $a2, $t4, 16
    sra     $a2, $a2, 15			# ($t7|00) << 1
    sll     $a3, $t5, 16
    sra     $a3, $a3, 15			# ($t7|01) << 1
    sll     $t4, $a0, 16
    sra     $t4, $t4, 15			# ($t7|10) << 1

    and     $t8, $t8, -256
    rotr     $t8, $t8, 16

    hle_11_12c_mla 0x20
    hle_11_12c_mla 0x28
    hle_11_12c_mla 0x30

    rotr     $t8, $t8, 16
    or     $t8, $t8, 0x1c
#    hle_flushflags
    hle_popstack
    sub     $s2,$s2,33
    b       ssp_drc_next


FUNCTION(ssp_hle_11_384):
    li     $a3, 2
    b       ssp_hle_11_38x

FUNCTION(ssp_hle_11_38a):
    li     $a3, 3		# $t5

ssp_hle_11_38x:
    ble     $s2, 0, ssp_drc_end
    nop

    li     $a2, 0		# EFh, EEh
    li     $a1, 1		# $t4
    add     $a0, $t7, 0x1c0	# $a0 (based)

ssp_hle_11_38x_loop:
    lhu    $t5, ($a0)
    add		$a2, $a2, 2
    lw     $s3, 0x224($t7)
    sll     $t5, $t5, 16
    sra		$s5, $t5, 31
    xor     $t5, $t5, $s5
    srl		$s5, $t5, 31
    add     $t5, $t5, $s5	# abs($t5)
    sll		$s5, $s3, 16
    blt     $t5, $s5, skip_9
    nop
    sll		$s5, $a1, 16
    or   $a2, $a2, $s5	# EFh |= $t4

skip_9:
	add		$a0, $a0, 2
    lhu    $t5, ($a0)
    lw     $s3, 0x220($t7)
    srl		$s5, $s3, 16
    blt     $t5, $s5, skip_10
    nop
    sll		$s5, $a1, 16
    or   $a2, $a2, $s5	# EFh |= $t4

skip_10:
    lw     $s3, 0x1e8($t7)
    add     $a0, $a0, 2
    sll     $s3,$s3,16
    srl		$s5, $s3, 16
    bge		$t5, $s5, skip_11
    nop
    or   $a2, $a2, $a1

skip_11:
    sll     $a1, $a1, 1
    sub    $a3, $a3, 1
    bgez	$a3, ssp_hle_11_38x_loop
    nop

    sw     $a2, 0x1dc($t7)
    sub     $a0, $a0, $t7
    and     $t8, $t8, -256
    srl		$s5, $a0, 1
    or     $t8, $t8, $s5
    and     $t9, $t9, -256
    or     $t9, $t9, $a1

#    hle_flushflags
    hle_popstack
    sub     $s2,$s2,(9+30*4)
    b       ssp_drc_next
    nop


FUNCTION(ssp_hle_07_6d6):
    ble     $s2, 0,ssp_drc_end
    nop

    lw     $a1, 0x20c($t7)
    and     $a0, $t8, 0xff	# assuming alignment
    sll		$s5, $a0, 1
    add     $a0, $t7, $s5
    srl     $a2, $a1, 16
    sll     $a1, $a1, 16	# 106h << 16
    sll     $a2, $a2, 16	# 107h << 16

ssp_hle_07_6d6_loop:
    lw     $t5, ($a0)
    add		$a0, $a0, 4
    and     $s5, $t5, $t5
    bltz	$s5, ssp_hle_07_6d6_end
    nop
    sll     $t5, $t5, 16
    bge     $t5, $a1, skip_12
    nop
    move   $a1, $t5
skip_12:
    sub     $s2,$s2,16
    blt     $t5, $a2, ssp_hle_07_6d6_loop
    move     $a2, $t5
    b       ssp_hle_07_6d6_loop
    nop

ssp_hle_07_6d6_end:
    sub     $a0, $a0, $t7
    srl     $a0, $a0,  1
    and     $t8, $t8, -256
    or     $t8, $t8, $a0
    srl		$s5, $a1, 16
    or     $a1, $a2, $s5
    sw     $a1, 0x20c($t7)
    hle_popstack
    sub     $s2,$s2,6
    b       ssp_drc_next
    nop


FUNCTION(ssp_hle_07_030):
    lhu    $a0, ($t7)
    sll     $a0, $a0, 4
    srl		$s5, $a0, 16
    or     $a0, $a0, $s5
    sh    $a0, ($t7)
    sub     $s2,$s2,3

FUNCTION(ssp_hle_07_036):
    lw     $a1, 0x1e0($t7)	# F1h F0h
    srl		$s5, $a1, 16
    sub     $t5, $s5, $a1
    sll     $t5, $t5, 16	# AL not needed
    sub     $s2,$s2,5
    blt     $t5, (4<<16), hle_07_036_ending2
    nop
    lw     $a1, 0x1dc($t7)	# EEh
    sll		$s5, $a1, 16
    sub     $s2,$s2,5
    bge     $t5, $s5, hle_07_036_ret
    nop

    srl     $a0, $t5, 16
    add     $a1, $t7, 0x100
    sh    $a0, 0xea($a1)	# F5h
    lw     $a0, 0x1e0($t7)	# F0h
    and     $a0, $a0, 3
    sh    $a0, 0xf0($a1)	# F8h
    add     $a2, $a0, 0xc0	# $a2
    sll		$s5, $a2, 1
    add     $a2, $t7, $s5
    lhu    $a2, ($a2)
    lw     $a0, ($t7)
    li     $a1, 4
    and     $a0, $a0, $a2
    jal      ssp_pm_write
    nop
    # will handle PMC later
    lw     $a0, 0x1e8($t7)	# F5h << 16
    lw     $a1, 0x1f0($t7)	# F8h
    lw     $a2, 0x1d4($t7)	# EAh
    sub     $a0, $a0, (3<<16)
    sll		$s5, $a1, 16
    add     $a0, $a0, $s5
    sra		$s5, $a0, 18
    sub     $a0, $a2, $s5
    and     $a0, $a0, 0x7f
    li		$s5, 0x78
    sub    $a0, $s5, $a0	# length
    blez    $a0, hle_07_036_ending1
    nop

    sub     $s2,$s2,$a0

    # copy part
    lw     $a1, (SSP_OFFS_GR+SSP_PMC*4)($t7)
    lw     $a2, SSP_OFFS_DRAM($t7)
    sll     $a1, $a1,  16
    srl		$s5, $a1, 15
    add     $a1, $a2, $s5	# addr (based)
    lhu    $a2, ($t7)	# pattern
    lhu    $a3, 6($t7)	# mode

    li     $s3,    0x4000
    or     $s3,$s3,0x0018
    sub    $s3,$a3, $s3
    beqz	$s3, skip_13
    nop
    sub  $s3,$s3,0x0400
    beqz	$s3, skip_12
    nop
    jal    tr_unhandled
    nop

skip_13:
	sll		$s5, $a2, 16
    or     $a2, $a2, $s5
    and     $s5, $a3, 0x400
    bnez    $s5, hle_07_036_ovrwr
    nop

hle_07_036_no_ovrwr:
    and		$s5, $a1, 2
    beqz	$s5, skip_14
    nop
    sh  $a2, ($a1)	# align
    add		$a1, $a1, 0x3e
    sub   $a0, $a0, 1
skip_14:
    sub    $a0, $a0, 4
    bltz   $a0,  hle_07_036_l2
    nop

hle_07_036_l1:
    sub    $a0, $a0, 4
    sw     $a2, ($a1)
	add		$a1, $a1, 0x40
    sw     $a2, ($a1)
    add		$a1, $a1, 0x40
    bgez   $a0,  hle_07_036_l1
    nop

hle_07_036_l2:
    and    $s5, $a0, 2
    beqz	$s5, skip_15
    nop
    sw   $a2, ($a1)
    add		$a1, $a1, 0x40
skip_15:
    and    $s5, $a0, 1
    beqz	$s5, skip_16
    nop
    sh  $a2, ($a1)
    add		$a1, $a1, 2
skip_16:
    b       hle_07_036_end_copy
    nop

hle_07_036_ovrwr:
    and    $s5, $a2,     0x000f
    bnez	$s5, skip_17
    nop
    or   $s3,$s3,0x000f
skip_17:
    #tst     $a2,     0x00f0
    and    $s5, $a2,     0x00f0
    bnez	$s5, skip_18
    nop
    or   $s3,$s3,0x00f0
skip_18:
    #tst     $a2,     0x0f00
    and    $s5, $a2,     0x0f00
    bnez	$s5, skip_19
    nop
    or   $s3,$s3,0x0f00
skip_19:
    #tst     $a2,     0xf000
    and    $s5, $a2,     0xf000
    bnez	$s5, skip_20
    nop
    or   $s3,$s3,0xf000
skip_20:
	sll		$s5, $s3, 16
    or    $s3,$s3,$s5
    beqz     $s3, hle_07_036_no_ovrwr
    nop

    and		$s5, $a1, 2
    beqz     $s5, hle_07_036_ol0
    nop
    lhu    $a3, ($a1)
    and     $a3, $a3, $s3
    or     $a3, $a3, $a2
    sh    $a3, ($a1)	# align
    add		$a1, $a1, 0x3e
    sub     $a0, $a0, 1

hle_07_036_ol0:
    sub    $a0, $a0, 2
    bltz   $a0,  hle_07_036_ol2
    nop

hle_07_036_ol1:
    sub    $a0, $a0, 2
    lw     $a3, ($a1)
    and     $a3, $a3, $s3
    or     $a3, $a3, $a2
    sw     $a3, ($a1)
    add		$a1, $a1, 0x40
    bgez    $a0, hle_07_036_ol1
    nop

hle_07_036_ol2:
    and		$s5, $a0, 1
    beqz	$s5, hle_07_036_end_copy
    nop
    lhu  $a3, ($a1)
    and   $a3, $a3, $s3
    or   $a3, $a3, $a2
    sh  $a3, ($a1)
    add		$a1, $a1, 2

hle_07_036_end_copy:
    lw     $a2, SSP_OFFS_DRAM($t7)
    add     $a3, $t7, 0x400
    sub     $a0, $a1, $a2          # new addr
    srl     $a0, $a0, 1
    sh    $a0, (0x6c+4*4)($a3) # SSP_OFFS_PM_WRITE+4*4 (low)

hle_07_036_ending1:
    lw     $a0, 0x1e0($t7)	# F1h << 16
    add     $a0, $a0, (1<<16)
    and     $a0, $a0, (3<<16)
    add     $a0, $a0, (0xc4<<16)
    and     $t8, $t8, -16711681
    or     $t8, $t8, $a0		# $a2
    srl		$s5, $a0, 15
    add     $a0, $t7, $s5
    lhu    $a0, ($a0)
    lw     $a2, ($t7)
    and     $a0, $a0, $a2
    sll    $t5, $a0, 16

    lw     $a1, 4($t7)	# new mode
    add     $a2, $t7, 0x400
    sh    $a1, (0x6c+4*4+2)($a2) # SSP_OFFS_PM_WRITE+4*4 (high)
    li     $a1, 4
    jal      ssp_pm_write
    nop
    sub     $s2,$s2,35

hle_07_036_ret:
    hle_popstack
    b       ssp_drc_next
    nop

hle_07_036_ending2:
    sub     $s2,$s2,3
    sll    $t5, $t5, 1
    bltz     $t5, hle_07_036_ret
    nop
    li     $a0, 0x87
    b       ssp_drc_next	# let the dispatcher finish this
    nop

# vim:filetype=mipsasm
