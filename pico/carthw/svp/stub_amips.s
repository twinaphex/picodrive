#*
#* Compiler helper functions and some SVP HLE code
#* Robson Santana, 2016
#*
#* This work is licensed under the terms of MAME license.
#* See COPYING file in the top-level directory.
#*

.set noreorder
.set noat

.text
.align 4

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


.eqv SSP_OFFS_GR,         0x400
.eqv SSP_PC,                  6
.eqv SSP_P,                   7
.eqv SSP_PM0,                 8
.eqv SSP_PMC,                14
.eqv SSP_OFFS_PM_WRITE,   0x46c # pmac_write()
.eqv SSP_OFFS_EMUSTAT,    0x484 # emu_status
.eqv SSP_OFFS_IRAM_ROM,   0x48c # ptr_iram_rom
.eqv SSP_OFFS_DRAM,       0x490 # ptr_dram
.eqv SSP_OFFS_IRAM_DIRTY, 0x494
.eqv SSP_OFFS_IRAM_CTX,   0x498 # iram_context
.eqv SSP_OFFS_BLTAB,      0x49c # block_table
.eqv SSP_OFFS_BLTAB_IRAM, 0x4a0
.eqv SSP_OFFS_TMP0,       0x4a4 # for entry PC
.eqv SSP_OFFS_TMP1,       0x4a8
.eqv SSP_OFFS_TMP2,       0x4ac
.eqv SSP_WAIT_PM0,       0x2000


.macro save_registers
	addiu   $sp, $sp, -72
	sw 		$ra, 68($sp)
	sw 		$s7, 64($sp)
	sw 		$s6, 60($sp)
	sw 		$s5, 56($sp)
	sw 		$s4, 52($sp)
	sw 		$s3, 48($sp)
	sw 		$s2, 44($sp)
	sw 		$s1, 40($sp)
	sw 		$t9, 36($sp)
	sw 		$t8, 32($sp)
	sw 		$t7, 28($sp)
	sw 		$t6, 24($sp)
	sw 		$t5, 20($sp)
	sw 		$t4, 16($sp)
	sw 		$a3, 12($sp)
	sw 		$a2, 8($sp)
  	sw 		$a1, 4($sp)
  	sw 		$a0, ($sp)
.endm

.macro restore_registers
  	lw 		$a0, ($sp)
  	lw 		$a1, 4($sp)
  	lw 		$a2, 8($sp)
  	lw 		$a3, 12($sp)
  	lw 		$t4, 16($sp)
  	lw 		$t5, 20($sp)
  	lw 		$t6, 24($sp)
  	lw 		$t7, 28($sp)
  	lw 		$t8, 32($sp)
  	lw 		$t9, 36($sp)
  	lw 		$s1, 40($sp)
  	lw 		$s2, 44($sp)
  	lw 		$s3, 48($sp)
  	lw 		$s4, 52($sp)
  	lw 		$s5, 56($sp)
  	lw 		$s6, 60($sp)
  	lw 		$s7, 64($sp)
  	lw 		$ra, 68($sp)
  	addiu   $sp, $sp, 72
.endm


.macro ssp_drc_do_next patch_jump=0
.if \patch_jump
    sw      $ra, SSP_OFFS_TMP2($t7)		# jump insw. (actually call) address + 4
.endif
    sll     $a0, $a0, 16
    srl     $a0, $a0, 16
    sw      $a0, SSP_OFFS_TMP0($t7)
    sub		$s6, $a0, 0x400
    bltz    $s6, 1f # ssp_de_iram
    nop

    lw      $a2, SSP_OFFS_BLTAB($t7)
    sll		$s6, $a0, 2
    addu	$s6, $a2, $s6
    lw      $a2, ($s6)
.if \patch_jump
    bnez    $a2, ssp_drc_do_patch
    nop
.else
	beqz	$a2, 0f
	nop
	#li		$s6, 0x08c00000
	#subu 	$s6, $s6, $a2
	#bgez	$s6, 88
	#nop
	#restore_registers
	#jr		$ra
	#nop
	jr		$a2
    nop
.endif
0:
	#addiu   $sp, $sp, -8
	#sw      $a3, 16($sp)
	#sw      $a2, 12($sp)
	#sw      $a1, 8($sp)
	#sw      $a0, 4($sp)
	#sw      $t7, 4($sp)
	#sw      $ra, ($sp)
	save_registers
    jal     ssp_translate_block
    nop
    restore_registers
    #lw      $ra, ($sp)
    #lw      $t7, 4($sp)
    #lw      $a0, 4($sp)
    #lw      $a1, 8($sp)
    #lw      $a2, 12($sp)
    #lw      $a3, 16($sp)
	#addiu   $sp, $sp, 8

    move    $a2, $v0
    lw      $a0, SSP_OFFS_TMP0($t7)		# entry PC
    lw      $a1, SSP_OFFS_BLTAB($t7)
    sll		$s6, $a0, 2
    addu	$s6, $a1, $s6
    sw      $a2, ($s6)
.if \patch_jump
    b       ssp_drc_do_patch
    nop
.else
	#li		$s6, 0x08c00000
	#subu 	$s6, $s6, $a2
	#bgez	$s6, 88
	#nop
	#restore_registers
	#jr		$ra
	#nop
	jr		$a2
    nop
.endif

1: # ssp_de_iram:
    lw      $a1, SSP_OFFS_IRAM_DIRTY($t7)
    and     $s6, $a1, $a1
    bnez	$s6, 2f
    nop
    lw      $a1, SSP_OFFS_IRAM_CTX($t7)
    b       3f # ssp_de_iram_ctx
    nop

2:
	addiu   $sp, $sp, -4
	#sw      $a1, 8($sp)
	#sw      $a0, 4($sp)
	sw      $ra, ($sp)
	#save_registers
    jal     ssp_get_iram_context
    nop
    #restore_registers
    lw      $ra, ($sp)
    #lw      $a0, 4($sp)
    #lw      $a1, 8($sp)
	addiu   $sp, $sp, 4
    li      $a1, 0
    sw      $a1, SSP_OFFS_IRAM_DIRTY($t7)
    move    $a1, $v0
    sw      $a1, SSP_OFFS_IRAM_CTX($t7)
    lw      $a0, SSP_OFFS_TMP0($t7)		# entry PC

3: # ssp_de_iram_ctx:
    lw      $a2, SSP_OFFS_BLTAB_IRAM($t7)
    sll		$s6, $a1, 12
    addu    $a2, $a2, $s6			# block_tab_iram + iram_context * 0x800/2*4
    sll		$s6, $a0, 2
    addu    $a1, $a2, $s6
    lw      $a2, ($a1)
.if \patch_jump
	bnez    $a2, ssp_drc_do_patch
    nop
.else
	#beqz	$s6, 4f
	beqz	$a2, 4f
	nop
	#li		$s6, 0x08c00000
	#subu 	$s6, $s6, $a2
	#bgez	$s6, 88
	#nop
	#restore_registers
	#jr		$ra
	#nop
    jr      $a2
    nop
.endif
4:
    sw      $a1, SSP_OFFS_TMP1($t7)
	#addiu   $sp, $sp, -8
	#sw      $a3, 16($sp)
	#sw      $a2, 12($sp)
	#sw      $a1, 8($sp)
	#sw      $a0, 4($sp)
	#sw      $t7, 4($sp)
	#sw      $ra, ($sp)
	save_registers
    jal     ssp_translate_block
    nop
    restore_registers
    #lw      $ra, ($sp)
    #lw      $t7, 4($sp)
    #lw      $a0, 4($sp)
    #lw      $a1, 8($sp)
    #lw      $a2, 12($sp)
    #lw      $a3, 16($sp)
	#addiu   $sp, $sp, 8

    move    $a2, $v0
    lw      $a0, SSP_OFFS_TMP0($t7)		# entry PC
    lw      $a1, SSP_OFFS_TMP1($t7)		# &block_table_iram(iram_context)(rPC)
    sw      $a2, ($a1)
.if \patch_jump
    b       ssp_drc_do_patch
    nop
.else
	#li		$s6, 0x08c00000
	#subu 	$s6, $s6, $a2
	#bgez	$s6, 88
	#nop
	#restore_registers
	#jr		$ra
	#nop
    jr      $a2
    nop
.endif
.endm # ssp_drc_do_next


.global ssp_drc_entry
ssp_drc_entry:
	#addiu   $sp, $sp, -36
	#sw      $ra, 32($sp)
	#sw      $s8, 28($sp)
	#sw      $s7, 24($sp)
	#sw      $s6, 20($sp)
	#sw      $s6, 16($sp)
	#sw      $s4, 12($sp)
	#sw      $s3, 8($sp)
	#sw      $s2, 4($sp)
	#sw      $s1, ($sp)
	save_registers

    move     $t7, $a0                      # ssp
    move     $s2, $a1
ssp_regfile_load:
    addu    $a2, $t7, 0x400
    addu    $a2, $a2, 4

    lw		$a3, ($a2)
    lw		$t4, 4($a2)
    lw		$t5, 8($a2)
    lw		$t6, 12($a2)
    lw		$t8, 16($a2)
    #addiu   $a2, $a2, 20
    lw		$s7, 0x4b0($t7)

    srl     $a3, $a3, 16
    srl     $a3, $a3, 16
    srl		$s6, $t4, 16
    or      $t4, $a3, $s6         # XXYY

	li		$s6, 0x0f0000
    and     $t8, $t8, $s6
    sll     $t8, $t8, 13             # sss0 *
    li		$s6, 0x670000
    and     $t9, $t6, $s6
    li		$s6, 0x80000000
    and     $s6, $t6, $s6
    beqz	$s6, skip_1
    nop
    or      $t8, $t8, 0x8
skip_1:
	li		$s6, 0x20000000
    and     $s6, $t6, $s6
    beqz	$s6, skip_2
    nop
    or      $t8, $t8, 0x4                # sss0 *           NZ..
skip_2:
	srl		$s6, $t9, 12
    or      $t6, $t8, $s6         # sss0 * .uu. .lll NZ..

    lw      $t8, 0x440($t7)            # $a0-$a2
    lw      $t9, 0x444($t7)            # $t4-$t6
    lw      $s1, (0x400+SSP_P*4)($t7)  # P

    lw      $a0, (SSP_OFFS_GR+SSP_PC*4)($t7)
    srl     $a0, $a0, 16


.global ssp_drc_next
ssp_drc_next:
    ssp_drc_do_next 0


.global ssp_drc_next_patch
ssp_drc_next_patch:
    ssp_drc_do_next 1

ssp_drc_do_patch:
    lw      $a1, SSP_OFFS_TMP2($t7)	# jump insw. (actually call) address + 4
    subu    $s3, $a2, $a1
    bnez	$s3, skip_3
    nop
    #li      $a3, 0x1			# nop
    #li      $s6, 0x00a00000
    #or      $a3, $a3, $s6		# nop
    #sw      $a3, -16($a1)		# nop
	#sw      $a3, -8($a1)		# nop

    sw      $zero, -16($a1)		# nop
	sw      $zero, -8($a1)		# nop
    b       ssp_drc_dp_end
    nop

skip_3:
	subu    $s6, $s3, 16
    bnez    $s6, skip_4
    nop
    lw      $a3, ($a1)
    #addu    $a3, $a3, 4  # need verify, maybe 4
	#li      $a3, 0x3
    sw      $a3, -16($a1)               # move the other cond up
	#li      $a3, 0x2			# nop
	#sw      $a3, ($a1)
    #li      $a3, 0xe1000000
    #li      $s6, 0x00a00000
    #or      $a3, $a3, $s6

    sw      $zero, ($a1)                    # fill it's place with nop
	sw      $zero, 8($a1)                    # fill it's place with nop
    b       ssp_drc_dp_end
    nop

skip_4:
    #lw      $a3, -8($a1)
    addu    $s3, $s3, 4
    #srl     $a3, $a3, 26
    #li		$s6, -2
    #and     $a3, $a3, $s6			# L bit
    #sll		$s6, $s3, 6
    #or      $a3, $a3, $s6
    li		$a3, 4					 # BEQ opcode
    rotr    $a3, $a3, 6              # patched branch instruction
    #sll		$s3, $s3, 6
    #srl		$s3, $s3, 6

    srl     $s3, $s3, 2
    andi	$s3, $s3, 0xffff		 # 16 bits offset
    or		$a3, $a3, $s3

    #ori		$a3, $a3, 4
    sw      $a3, -8($a1)             # patch the bl/b to jump directly to another handler

ssp_drc_dp_end:
    sw      $a2, SSP_OFFS_TMP1($t7)
    subu    $a0, $a1, 4
    addu    $a1, $a1, 4
	#addiu   $sp, $sp, -12
	#sw      $a1, 8($sp)
	#sw      $a0, 4($sp)
	#sw      $ra, ($sp)
	save_registers
    jal     cache_flush_d_inval_i
    nop
    restore_registers
    #lw      $ra, ($sp)
    #lw      $a0, 4($sp)
    #lw      $a1, 8($sp)
	#addiu   $sp, $sp, 12
    lw      $a2, SSP_OFFS_TMP1($t7)
    lw      $a0, SSP_OFFS_TMP0($t7)
    jr      $a2
    nop


.global ssp_drc_end
ssp_drc_end:
#addiu $sp,$sp,8  #only debug
    sll     $a0, $a0, 16
    sw      $a0, (SSP_OFFS_GR+SSP_PC*4)($t7)

ssp_regfile_store:
    sw      $s1, (0x400+SSP_P*4)($t7)  # P
    sw      $t8, 0x440($t7)            # $a0-$a2
    sw      $t9, 0x444($t7)            # $t4-$t6

    srl     $t9, $t6, 13
    li		$s6, (7<<16)
    and     $t9, $t9, $s6            # STACK
    sll     $a3, $t6, 28
    move	$s7, $a3
    #msr     cpsr_f, r3                  @ to to ARM PSR
    and     $t6, $t6, 0x670
    sll     $t6, $t6, 12
    and		$s6, $a3, $t6
    bgez	$s6, skip_5
    nop
    li		$s6, 0x80000000
    or      $t6, $t6, $s6         # N
skip_5:
    and		$s6, $a3, $t6
    bnez	$s6, skip_6
    nop
    li		$s6, 0x20000000
    or      $t6, $t6, $s6         # Z
skip_6:
    sll     $a3, $t4, 16             # Y
    srl     $a2, $t4, 16
    sll     $a2, $a2, 16             # X
    addu    $t8, $t7, 0x400
    addu    $t8, $t8, 4
    #stmia   $t8, {$a2,$a3,$t5,$t6,$t9}

    sw		$a2, ($t8)
    sw		$a3, 4($t8)
    sw		$t5, 8($t8)
    sw		$t6, 12($t8)
    sw		$t9, 16($t8)
    #addiu   $t8, $t8, 20
    sw		$s7, 0x4b0($t7)

    move    $v0, $s2

	#lw      $s1, ($sp)
	#lw      $s2, 4($sp)
	#lw      $s3, 8($sp)
	#lw      $s4, 12($sp)
	#lw      $s6, 16($sp)
	#lw      $s6, 20($sp)
	#lw      $s7, 24($sp)
	#lw      $s8, 28($sp)
	#lw      $ra, 32($sp)
	#addiu   $sp, $sp, 36
    restore_registers
	jr		$ra
	nop

# ld      A, PM0
# andi    2
# bra     z=1, gloc_0800
.global ssp_hle_800
ssp_hle_800:
    lw      $a0, (SSP_OFFS_GR+SSP_PM0*4)($t7)
    lw      $a1, SSP_OFFS_EMUSTAT($t7)
    li		$s6, 0x20000
    and     $s6, $a0, $s6
    bnez	$s6, skip_7
    nop
    or      $a1, $a1, SSP_WAIT_PM0
    subu    $s2, $s2, 1024
    sw      $a1, SSP_OFFS_EMUSTAT($t7)
    li      $a0,     0x400
    j       ssp_drc_end
    nop

skip_7:
	li      $a0, 0x400
    or      $a0, $a0, 0x004
    j       ssp_drc_next
    nop


.macro hle_flushflags
    and     $t6, $t6, -16
    move	$a1, $s7
    srl		$s6, $a1, 28
    or      $t6, $t6, $s6
.endm

.macro hle_popstack
	li		$s6, 0x20000000
    subu    $t6, $t6, $s6
    addu    $a1, $t7, 0x400
    addu    $a1, $a1, 0x048			# stack
    srl		$s6, $t6, 28
    addu    $a1, $a1, $s6
    lhu     $a0, ($a1)
.endm

.global	ssp_hle_902
ssp_hle_902:
    bgt     $s2, 0, skip_7_2
    nop
    j       ssp_drc_end
    nop

skip_7_2:
    addu    $a1, $t7, 0x200
    lhu     $a0, ($a1)
    lw      $a3, SSP_OFFS_IRAM_ROM($t7)
    sll		$s6, $a0, 1
    addu    $a2, $a3, $s6			# ($t7|00)
    lhu     $a0, ($a2)
    add		$a2, $a2, 2
    sll     $t5, $t5, 16
    srl     $t5, $t5, 16
    li		$s6, -64513
    and     $a0, $a0, $s6
    sll		$s6, $a0, 1
    addu    $a3, $a3, $s6 			# IRAM dest
    lhu     $s3,($a2)			# length
    add		$a2, $a2, 2
    li		$s6, -4
    and     $a3, $a3, $s6				# always seen aligned
#    or     $t5, $t5, 0x08000000
#    or     $t5, $t5, 0x00880000
#    subu   $t5, $t5, $s3, lsl 16
	li		$s6, -16
    and     $t6, $t6, $s6
    addu    $s3,$s3,1
    li      $a0, 1
    sw      $a0, SSP_OFFS_IRAM_DIRTY($t7)
    sll		$s6, $s3, 1
    subu    $s2,$s2,$s6
    subu    $s2,$s2,$s3				# -= length*3

ssp_hle_902_loop:
    lhu     $a0, ($a2)
    add		$a2, $a2, 2
    lhu     $a1, ($a2)
    add		$a2, $a2, 2
    subu    $s3,$s3,2
    sll		$s6, $a1, 16
    or      $a0, $a0, $s6
    sw      $a0, ($a3)
    add		$a3, $a3, 4
    bgtz    $s3, ssp_hle_902_loop
    nop

    and		$s6, $s3, 1
    beqz	$s6, skip_8
    nop
    lhu     $a0, ($a2)
    add		$a2, $a2, 2
    sh      $a0, ($a3)
    add		$a3, $a3, 2

skip_8:
    lw      $a0, SSP_OFFS_IRAM_ROM($t7)
    addu    $a1, $t7, 0x200
    subu    $a2, $a2, $a0
    srl     $a2, $a2, 1
    sh      $a2, ($a1)				# ($t7|00)

    subu    $a0, $a3, $a0
    srl     $a0, $a0, 1
    li		$s6, 0x08000000
    or      $a0, $a0, $s6
    li		$s6, 0x001c8000
    or      $a0, $a0, $s6
    sw      $a0, (SSP_OFFS_GR+SSP_PMC*4)($t7)
    sw      $a0, (SSP_OFFS_PM_WRITE+4*4)($t7)

    hle_popstack
    subu    $s2,$s2,16				# timeslice is likely to end
    bgtz    $s2, skip_8_2
    nop
    j       ssp_drc_end
    nop
skip_8_2:
    j       ssp_drc_next
    nop


# this one is car rendering related
.macro hle_11_12c_mla offs_in
    lh   	$t5, (\offs_in+0)($t7)
    lh   	$a0, (\offs_in+2)($t7)
    lh   	$a1, (\offs_in+4)($t7)
    mul     $t5, $a2, $t5
    lh   	$s3, (\offs_in+6)($t7)
    mul		$s6, $a0, $a3
    addu    $t5, $s6, $t5
    mul		$s6, $t4, $a1
    addu    $t5, $s6, $t5
    sll		$s6, $s3, 11
    addu    $t5, $t5, $s6

    srl     $t5, $t5, 13
	move    $s5, $t5

	blez	$s5, 10f
	nop
	lui		$s7, 0x0
	b		12f
	nop
10:
	bnez    $s5, 11f
	nop
	lui     $s7, 0x6000
	b		12f
	nop
11:
	lui 	$s7, 0x8000

12:
    srl		$s6, $t8, 23
    addu    $a1, $t7, $s6
    sh      $t5, ($a1)
    li		$s6, (1<<24)
    addu    $t8, $t8, $s6
.endm

.global ssp_hle_11_12c
ssp_hle_11_12c:
    bgt     $s2, 0, skip_8_3
    nop
    j       ssp_drc_end
    nop

skip_8_3:
    li      $a0, 0

	#save_registers
	addiu   $sp, $sp, -12
	sw      $a1, 8($sp)
	sw      $a0, 4($sp)
	sw      $ra, ($sp)
    jal     ssp_pm_read
    nop
    lw      $ra, ($sp)
    lw      $a0, 4($sp)
    lw      $a1, 8($sp)
	addiu   $sp, $sp, 12
    move    $t4, $v0

    li      $a0, 0
	addiu   $sp, $sp, -12
	sw      $a1, 8($sp)
	sw      $a0, 4($sp)
	sw      $ra, ($sp)
    jal     ssp_pm_read
    nop
    lw      $ra, ($sp)
    lw      $a0, 4($sp)
    lw      $a1, 8($sp)
	addiu   $sp, $sp, 12
	move    $t5, $v0

    li      $a0, 0
	addiu   $sp, $sp, -12
	sw      $a1, 8($sp)
	sw      $a0, 4($sp)
	sw      $ra, ($sp)
    jal     ssp_pm_read
    nop
    #restore_registers
    lw      $ra, ($sp)
    lw      $a0, 4($sp)
    lw      $a1, 8($sp)
	addiu   $sp, $sp, 12
	move	$a0, $v0

    sll     $a2, $t4, 16
    sra     $a2, $a2, 15			# ($t7|00) << 1
    sll     $a3, $t5, 16
    sra     $a3, $a3, 15			# ($t7|01) << 1
    sll     $t4, $a0, 16
    sra     $t4, $t4, 15			# ($t7|10) << 1

	li		$s6, -256
    and     $t8, $t8, $s6
    rotr    $t8, $t8, 16

    hle_11_12c_mla 0x20
    hle_11_12c_mla 0x28
    hle_11_12c_mla 0x30

    rotr    $t8, $t8, 16
    or      $t8, $t8, 0x1c
#    hle_flushflags
    hle_popstack
    subu    $s2,$s2,33
    j       ssp_drc_next
    nop


.global ssp_hle_11_384
ssp_hle_11_384:
    li      $a3, 2
    b       ssp_hle_11_38x
    nop

.global ssp_hle_11_38a
ssp_hle_11_38a:
    li     $a3, 3		# $t5

ssp_hle_11_38x:
    bgt     $s2, 0, skip_8_4
    nop
    j       ssp_drc_end
    nop

skip_8_4:
    li      $a2, 0		# EFh, EEh
    li      $a1, 1		# $t4
    addu    $a0, $t7, 0x1c0	# $a0 (based)

ssp_hle_11_38x_loop:
    lhu     $t5, ($a0)
    add		$a2, $a2, 2
    lw      $s3, 0x224($t7)
    sll     $t5, $t5, 16
    sra		$s6, $t5, 31
    xor     $t5, $t5, $s6
    srl		$s6, $t5, 31
    addu    $t5, $t5, $s6	# abs($t5)
    sll		$s6, $s3, 16
    sub		$s6, $t5, $s6
    bltz    $s6, skip_9
    nop
    sll		$s6, $a1, 16
    or      $a2, $a2, $s6	# EFh |= $t4

skip_9:
	add		$a0, $a0, 2
    lhu     $t5, ($a0)
    lw      $s3, 0x220($t7)
    srl		$s6, $s3, 16
    sub		$s6, $t5, $s6
    bltz    $s6, skip_10
    nop
    sll		$s6, $a1, 16
    or      $a2, $a2, $s6	# EFh |= $t4

skip_10:
    lw      $s3, 0x1e8($t7)
    addu    $a0, $a0, 2
    sll     $s3, $s3, 16
    srl		$s6, $s3, 16
    sub		$s6, $t5, $s6
    bgez    $s6, skip_11
    nop
    or      $a2, $a2, $a1

skip_11:
    sll     $a1, $a1, 1
    subu    $a3, $a3, 1
    bgez	$a3, ssp_hle_11_38x_loop
    nop

    sw      $a2, 0x1dc($t7)
    subu    $a0, $a0, $t7
    li		$s6, -256
    and     $t8, $t8, $s6
    srl		$s6, $a0, 1
    or      $t8, $t8, $s6
    li		$s6, -256
    and     $t9, $t9, $s6
    or      $t9, $t9, $a1

#    hle_flushflags
    hle_popstack
    subu    $s2,$s2,(9+30*4)
    j       ssp_drc_next
    nop


.global ssp_hle_07_6d6
ssp_hle_07_6d6:
    bgt     $s2, 0, skip_11_2
    nop
    j       ssp_drc_end
    nop

skip_11_2:
    lw      $a1, 0x20c($t7)
    and     $a0, $t8, 0xff	# assuming alignment
    sll		$s6, $a0, 1
    addu    $a0, $t7, $s6
    srl     $a2, $a1, 16
    sll     $a1, $a1, 16	# 106h << 16
    sll     $a2, $a2, 16	# 107h << 16

ssp_hle_07_6d6_loop:
    lw      $t5, ($a0)
    add		$a0, $a0, 4
    and     $s6, $t5, $t5
    bltz	$s6, ssp_hle_07_6d6_end
    nop
    sll     $t5, $t5, 16
    sub		$s6, $t5, $a1
    bgez    $s6, skip_12
    nop
    move    $a1, $t5
skip_12:
    subu    $s2,$s2,16
    sub		$s6, $t5, $a2
    bltz    $s6, ssp_hle_07_6d6_loop
    nop
    move    $a2, $t5
    b       ssp_hle_07_6d6_loop
    nop

ssp_hle_07_6d6_end:
    subu    $a0, $a0, $t7
    srl     $a0, $a0,  1
    li		$s6, -256
    and     $t8, $t8, $s6
    or      $t8, $t8, $a0
    srl		$s6, $a1, 16
    or      $a1, $a2, $s6
    sw      $a1, 0x20c($t7)
    hle_popstack
    subu    $s2,$s2,6
    j       ssp_drc_next
    nop


.global ssp_hle_07_030
ssp_hle_07_030:
    lhu     $a0, ($t7)
    sll     $a0, $a0, 4
    srl		$s6, $a0, 16
    or      $a0, $a0, $s6
    sh      $a0, ($t7)
    subu    $s2,$s2,3

.global ssp_hle_07_036
ssp_hle_07_036:
    lw      $a1, 0x1e0($t7)	# F1h F0h
    srl		$s6, $a1, 16
    subu    $t5, $s6, $a1
    sll     $t5, $t5, 16	# AL not needed
    subu    $s2,$s2,5
    li		$s6, (4<<16)
    sub		$s6, $t5, $s6
    bltz    $s6, hle_07_036_ending2
    nop
    lw      $a1, 0x1dc($t7)	# EEh
    sll		$s6, $a1, 16
    subu    $s2,$s2,5
    sub		$s6, $t5, $s6
    bgez    $s6, hle_07_036_ret
    nop

    srl     $a0, $t5, 16
    addu    $a1, $t7, 0x100
    sh      $a0, 0xea($a1)	# F5h
    lw      $a0, 0x1e0($t7)	# F0h
    and     $a0, $a0, 3
    sh      $a0, 0xf0($a1)	# F8h
    addu    $a2, $a0, 0xc0	# $a2
    sll		$s6, $a2, 1
    addu    $a2, $t7, $s6
    lhu     $a2, ($a2)
    lw      $a0, ($t7)
    li      $a1, 4
    and     $a0, $a0, $a2
	addiu   $sp, $sp, -12
	sw      $a1, 8($sp)
	sw      $a0, 4($sp)
	sw      $ra, ($sp)
	#save_registers
    jal     ssp_pm_write
    nop
    #restore_registers
    lw      $ra, ($sp)
    lw      $a0, 4($sp)
    lw      $a1, 8($sp)
	addiu   $sp, $sp, 12

    # will handle PMC later
    lw      $a0, 0x1e8($t7)	# F5h << 16
    lw      $a1, 0x1f0($t7)	# F8h
    lw      $a2, 0x1d4($t7)	# EAh
    li		$s6, (3<<16)
    subu    $a0, $a0, $s6
    sll		$s6, $a1, 16
    addu    $a0, $a0, $s6
    sra		$s6, $a0, 18
    subu    $a0, $a2, $s6
    and     $a0, $a0, 0x7f
    li		$s6, 0x78
    subu    $a0, $s6, $a0	# length
    blez    $a0, hle_07_036_ending1
    nop

    subu    $s2,$s2,$a0

    # copy part
    lw      $a1, (SSP_OFFS_GR+SSP_PMC*4)($t7)
    lw      $a2, SSP_OFFS_DRAM($t7)
    sll     $a1, $a1, 16
    srl		$s6, $a1, 15
    addu    $a1, $a2, $s6	# addr (based)
    lhu     $a2, ($t7)	# pattern
    lhu     $a3, 6($t7)	# mode

    li      $s3,    0x4000
    or      $s3, $s3,0x0018
    subu    $s3, $a3, $s3
    beqz 	$s3, skip_13
    nop
    subu    $s3,$s3,0x0400
    beqz	$s3, skip_13
    nop

	addiu   $sp, $sp, -12
	sw      $a1, 8($sp)
	sw      $a0, 4($sp)
	sw      $ra, ($sp)
	#save_registers
    jal     tr_unhandled
    nop
    #restore_registers
    lw      $ra, ($sp)
    lw      $a0, 4($sp)
    lw      $a1, 8($sp)
	addiu   $sp, $sp, 12

skip_13:
	sll		$s6, $a2, 16
    or      $a2, $a2, $s6
    and     $s6, $a3, 0x400
    bnez    $s6, hle_07_036_ovrwr
    nop

hle_07_036_no_ovrwr:
    and		$s6, $a1, 2
    beqz	$s6, skip_14
    nop
    sh      $a2, ($a1)	# align
    add		$a1, $a1, 0x3e
    subu    $a0, $a0, 1
skip_14:
    subu    $a0, $a0, 4
    bltz    $a0,  hle_07_036_l2
    nop

hle_07_036_l1:
    subu    $a0, $a0, 4
    sw      $a2, ($a1)
	add		$a1, $a1, 0x40
    sw      $a2, ($a1)
    add		$a1, $a1, 0x40
    bgez    $a0,  hle_07_036_l1
    nop

hle_07_036_l2:
    and     $s6, $a0, 2
    beqz	$s6, skip_15
    nop
    sw      $a2, ($a1)
    add		$a1, $a1, 0x40
skip_15:
    and     $s6, $a0, 1
    beqz	$s6, skip_16
    nop
    sh      $a2, ($a1)
    add		$a1, $a1, 2
skip_16:
    b       hle_07_036_end_copy
    nop

hle_07_036_ovrwr:
    and     $s6, $a2, 0x000f
    bnez	$s6, skip_17
    nop
    or      $s3, $s3, 0x000f
skip_17:
    and     $s6, $a2, 0x00f0
    bnez	$s6, skip_18
    nop
    or      $s3, $s3, 0x00f0
skip_18:
    and     $s6, $a2, 0x0f00
    bnez	$s6, skip_19
    nop
    or      $s3, $s3, 0x0f00
skip_19:
    and     $s6, $a2, 0xf000
    bnez	$s6, skip_20
    nop
    or      $s3, $s3, 0xf000
skip_20:
	sll		$s6, $s3, 16
    or      $s3, $s3, $s6
    beqz    $s3, hle_07_036_no_ovrwr
    nop

    and		$s6, $a1, 2
    beqz    $s6, hle_07_036_ol0
    nop
    lhu     $a3, ($a1)
    and     $a3, $a3, $s3
    or      $a3, $a3, $a2
    sh      $a3, ($a1)	# align
    add		$a1, $a1, 0x3e
    subu    $a0, $a0, 1

hle_07_036_ol0:
    subu    $a0, $a0, 2
    bltz    $a0, hle_07_036_ol2
    nop

hle_07_036_ol1:
    subu    $a0, $a0, 2
    lw      $a3, ($a1)
    and     $a3, $a3, $s3
    or      $a3, $a3, $a2
    sw      $a3, ($a1)
    add		$a1, $a1, 0x40
    bgez    $a0, hle_07_036_ol1
    nop

hle_07_036_ol2:
    and		$s6, $a0, 1
    beqz	$s6, hle_07_036_end_copy
    nop
    lhu     $a3, ($a1)
    and     $a3, $a3, $s3
    or      $a3, $a3, $a2
    sh      $a3, ($a1)
    add		$a1, $a1, 2

hle_07_036_end_copy:
    lw      $a2, SSP_OFFS_DRAM($t7)
    addu    $a3, $t7, 0x400
    subu    $a0, $a1, $a2          # new addr
    srl     $a0, $a0, 1
    sh      $a0, (0x6c+4*4)($a3) # SSP_OFFS_PM_WRITE+4*4 (low)

hle_07_036_ending1:
    lw      $a0, 0x1e0($t7)	# F1h << 16
    li		$s6, (1<<16)
    addu    $a0, $a0, $s6
    li		$s6, (3<<16)
    and     $a0, $a0, $s6
    li		$s6, (0xc4<<16)
    addu    $a0, $a0, $s6
    li		$s6, -16711681
    and     $t8, $t8, $s6
    or      $t8, $t8, $a0		# $a2
    srl		$s6, $a0, 15
    addu    $a0, $t7, $s6
    lhu     $a0, ($a0)
    lw      $a2, ($t7)
    and     $a0, $a0, $a2
    sll     $t5, $a0, 16
	move    $s5, $t5

	blez	$s5, skip21
	nop
	lui		$s7, 0x0
	b		skip23
	nop
skip21:
	bnez    $s5, skip22
	nop
	lui     $s7, 0x6000
	b		skip23
	nop
skip22:
	lui 	$s7, 0x8000

skip23:	
    lw      $a1, 4($t7)	# new mode
    addu    $a2, $t7, 0x400
    sh      $a1, (0x6c+4*4+2)($a2) # SSP_OFFS_PM_WRITE+4*4 (high)
    li      $a1, 4
	addiu   $sp, $sp, -12
	sw      $a1, 8($sp)
	sw      $a0, 4($sp)
	sw      $ra, ($sp)
	#save_registers
    jal     ssp_pm_write
    nop
    #restore_registers
    lw      $ra, ($sp)
    lw      $a0, 4($sp)
    lw      $a1, 8($sp)
	addiu   $sp, $sp, 12
    subu    $s2, $s2, 35

hle_07_036_ret:
    hle_popstack
    j       ssp_drc_next
    nop

hle_07_036_ending2:
    subu    $s2, $s2, 3
    sll     $t5, $t5, 1
	move    $s5, $t5

	blez	$s5, skip24
	nop
	lui		$s7, 0x0
	b		skip26
	nop
skip24:
	bnez    $s5, skip25
	nop
	lui     $s7, 0x6000
	b		skip26
	nop
skip25:
	lui 	$s7, 0x8000

skip26:	
    bltz    $s5, hle_07_036_ret
    nop
    li      $a0, 0x87
    j       ssp_drc_next	# let the dispatcher finish this
    nop

# vim:filetype=mipsasm
