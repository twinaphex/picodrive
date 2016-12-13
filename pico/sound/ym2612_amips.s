/*
 * PicoDrive
 * (C) notaz, 2006
 * Robson Santana, 2016
 *
 * This work is licensed under the terms of MAME license.
 * See COPYING file in the top-level directory.
 */

# this is a rewrite of MAME's ym2612 code, in particular this is only the main sample-generatin loop.
# it does not seem to give much performance increase (if any at all), so don't use it if it causes trouble.
# - notaz, 2006

# vim:filetype=mipsasm

.set noreorder
.set noat

.text
.align 4

.eqv SLOT1, 0
.eqv SLOT2, 2
.eqv SLOT3, 1
.eqv SLOT4, 3
.eqv SLOT_STRUCT_SIZE, 0x30

.eqv TL_TAB_LEN, 0x1A00

.eqv EG_ATT, 4
.eqv EG_DEC, 3
.eqv EG_SUS, 2
.eqv EG_REL, 1
.eqv EG_OFF, 0

.eqv EG_SH,			  16             # 16.16 fixed point (envelope generator timing)
.eqv EG_TIMER_OVERFLOW, (3*(1<<16)) # envelope generator timer overflows every 3 samples (on real chip)
.eqv LFO_SH,            25  /*  7.25 fixed point (LFO calculations)       */

.eqv ENV_QUIET,		  (2*13*256/8)/2


.macro save_registers
	addiu   $sp, $sp, -40
	sw 		$ra, 36($sp)
	sw 		$t9, 32($sp)
	sw 		$t8, 28($sp)
	sw 		$t7, 24($sp)
	sw 		$t6, 20($sp)
	sw 		$t5, 16($sp)
	sw 		$t4, 12($sp)
	sw 		$a3, 8($sp)
	sw 		$a1, 4($sp)
	sw 		$a0, 0($sp)
.endm

.macro restore_registers
	lw 		$a0, 0($sp)
	lw 		$a1, 4($sp)
	lw 		$a3, 8($sp)
  	lw 		$t4, 12($sp)
  	lw 		$t5, 16($sp)
  	lw 		$t6, 20($sp)
  	lw 		$t7, 24($sp)
  	lw 		$t8, 28($sp)
  	lw 		$t9, 32($sp)
  	lw 		$ra, 36($sp)
  	addiu   $sp, $sp, 40
.endm

# $t5=slot, $a1=eg_cnt, trashes: $a0,$a2,$a3
# writes output to routp, but only if vol_out changes
.macro update_eg_phase_slot slot
    lbu     $a2, 0x17($t5)	     # state
    li      $a3, 1               # 1ci
    subu    $s6, $a2, 1
    bltz    $s6, 5f                   # EG_OFF
	nop
    beqz    $s6, 3f                   # EG_REL
	nop
    subu    $s6, $a2, 3
    bltz    $s6, 2f                   # EG_SUS
	nop
    beqz    $s6, 1f                   # EG_DEC
	nop

0:  # EG_ATT
    lw      $a2, 0x20($t5)       # eg_pack_ar (1ci)
    srl     $a0, $a2, 24
    sllv    $a3, $a3, $a0
    subu    $a3, $a3, 1
    and     $s6, $a1, $a3
    bnez    $s6, 5f                   # do smth for tl problem (set on init?)
	nop
    srlv    $a3, $a1, $a0
    lhu     $a0, 0x1a($t5)	     # volume, unsigned (0-1023)
    and     $a3, $a3, 7
	sll 	$s6, $a3, 1
    addu    $a3, $a3, $s6
    srlv    $a3, $a2, $a3
    and     $a3, $a3, 7           # shift for eg_inc calculation
    not     $a2, $a0
    sllv    $a2, $a2, $a3
	sra		$s6, $a2, 5
    addu    $a0, $a0, $s6
    #subu    $s6, $a0, 0
	bgtz	$a0, 4f					# if (volume <= MIN_ATT_INDEX)
	nop
    li      $a3, EG_DEC
    sb      $a3, 0x17($t5)       # state
    move    $a0, $zero
    b       4f
	nop

1:  # EG_DEC
    lw      $a2, 0x24($t5)       # eg_pack_d1r (1ci)
    srl     $a0, $a2, 24
    sllv    $a3, $a3, $a0
    subu    $a3, $a3, 1
    and     $s6, $a1, $a3
    bnez    $s6, 5f                   # do smth for tl problem (set on init?)
	nop
    srlv    $a3, $a1, $a0
    lhu     $a0, 0x1a($t5)       # volume
    and     $a3, $a3, 7
	sll		$s6, $a3, 1
    addu    $a3, $a3, $s6
    srl     $a3, $a2, $a3
    and     $a3, $a3, 7           # shift for eg_inc calculation
    li      $a2, 1
    sllv    $a3, $a2, $a3
    lw      $a2, 0x1c($t5)       # sl (can be 16bit?)
	sra		$s6, $a3, 1
    addu    $a0, $a0, $s6
    subu    $s6, $a0, $a2               # if ( volume >= (INT32) SLOT->sl )
	bltz	$s6, 4f
	nop
    li      $a3, EG_SUS
    sb      $a3, 0x17($t5)       # state
    b       4f
	nop

2:  # EG_SUS
    lw      $a2, 0x28($t5)       # eg_pack_d2r (1ci)
    srl     $a0, $a2, 24
    sllv    $a3, $a3, $a0
    subu    $a3, $a3, 1
    and     $s6, $a1, $a3
    bnez    $s6, 5f                   # do smth for tl problem (set on init?)
	nop
    srlv    $a3, $a1, $a0
    lhu     $a0, 0x1a($t5)       # volume
    and     $a3, $a3, 7
	sll		$s6, $a3, 1
    addu    $a3, $a3, $s6
    srlv    $a3, $a2, $a3
    and     $a3, $a3, 7           # shift for eg_inc calculation
    li      $a2, 1
    sllv    $a3, $a2, $a3
	sra		$s6, $a3, 1
    addu    $a0, $a0, $s6
    #li      $a2, 1024
    #subu    $a2, $a2, 1            # $a2 = MAX_ATT_INDEX
    li      $a2, 1023               # $a2 = MAX_ATT_INDEX
    subu    $s6, $a0, $a2               # if ( volume >= MAX_ATT_INDEX )
	bltz    $s6, 4f
	nop
    move    $a0, $a2
    b       4f
	nop

3:  # EG_REL
    lw      $a2, 0x2c($t5)       # eg_pack_rr (1ci)
    srl     $a0, $a2, 24
    sllv    $a3, $a3, $a0
    subu    $a3, $a3, 1
    and     $s6, $a1, $a3
    bnez    $s6, 5f                   # do smth for tl problem (set on init?)
	nop
    srlv    $a3, $a1, $a0
    lhu     $a0, 0x1a($t5)       # volume
    and     $a3, $a3, 7
	sll		$s6, $a3, 1
    addu    $a3, $a3, $s6
    srlv    $a3, $a2, $a3
    and     $a3, $a3, 7           # shift for eg_inc calculation
    li      $a2, 1
    sllv    $a3, $a2, $a3
	sra		$s6, $a3, 1
    addu    $a0, $a0, $s6
    #li      $a2, 1024
    #subu    $a2, $a2, 1            # $a2 = MAX_ATT_INDEX
    li      $a2, 1023               # $a2 = MAX_ATT_INDEX
    subu    $s6, $a0, $a2               # if ( volume >= MAX_ATT_INDEX )
	bltz    $s6, 4f
	nop
    move    $a0, $a2
    li      $a3, EG_OFF
    sb      $a3, 0x17($t5)       # state

4:
    lhu     $a3, 0x18($t5)       # tl
    sh      $a0, 0x1a($t5)       # volume
.if     \slot == SLOT1
    srl     $t6, $t6, 16
    addu    $a0, $a0, $a3
	sll		$s6, $t6, 16
    or      $t6, $a0, $s6
.elseif \slot == SLOT2
    sll     $t6, $t6, 16
    addu    $a0, $a0, $a3
    sll     $a0, $a0, 16
	srl		$s6, $t6, 16
    or      $t6, $a0, $s6
.elseif \slot == SLOT3
    srl     $t7, $t7, 16
    addu    $a0, $a0, $a3
	sll		$s6, $t7, 16
    or      $t7, $a0, $s6
.elseif \slot == SLOT4
    sll     $t7, $t7, 16
    addu    $a0, $a0, $a3
    sll     $a0, $a0, 16
	srl     $s6, $t7, 16
    or      $t7, $a0, $s6
.endif

5:
.endm


# $s3=lfo_ampm(31:16), $a1=lfo_cnt_old, $a2=lfo_cnt, $a3=scratch
.macro advance_lfo_m
    srl     $a2, $a2, LFO_SH
	srl		$s6, $a1, LFO_SH
    subu    $s6, $a2, $s6
    beqz    $s6, 1f
	nop
    and     $a3, $a2, 0x3f
    subu    $s6, $a2, 0x40
	bltz	$s6, 0f
	nop
	li		$s6, 0x3f
    subu    $a3, $s6, $a3
0:
	li		$s6, 0xffffff
    and     $s3, $s3, $s6          # lfo_ampm &= 0xff
	sll		$s6, $a3, 1+24
    or      $s3, $s3, $s6

    srl     $a2, $a2, 2
	srl		$s6, $a1, LFO_SH+2
    subu    $s6, $a2, $s6
	beqz	$s6, 1f
	nop
	li		$s6, 0xff00ffff
    and     $s3, $s3, $s6
	sll		$s6, $a2, 16
    or      $s3, $s3, $s6

1:
.endm


# result goes to $a1, trashes $a2
.macro make_eg_out slot
    and     $s6, $s3, 8
	beqz	$s6, 0f
	nop
    and     $s6, $s3, (1<<(\slot+8))
0:
.if     \slot == SLOT1
    sll     $a1, $t6, 16
    srl     $a1, $a1, 17
.elseif \slot == SLOT2
    srl     $a1, $t6, 17
.elseif \slot == SLOT3
    sll     $a1, $t7, 16
    srl     $a1, $a1, 17
.elseif \slot == SLOT4
    srl     $a1, $t7, 17
.endif
	beqz	$s6, 1f
	nop
    and     $a2, $s3, 0xc0
    srl     $a2, $a2, 6
    addu    $a2, $a2, 24
	srlv	$s6, $s3, $a2
    addu    $a1, $a1, $s6
1:
.endm


.macro lookup_tl r
    and     $s6, \r, 0x100
	beqz	$s6, 0f
	nop
    xor     \r, \r, 0xff   # if (sin & 0x100) sin = 0xff - (sin&0xff);
0:
    and     $s6, \r, 0x200
    and     \r, \r, 0xff
	sll		$s5, $a1, 8
    or      \r, \r, $s5
    sll     \r, \r, 1
	add		$s5, \r, $a3
    lhu     \r, ($s5)    # 2ci if ne
	beqz	$s6, 1f
	nop
    subu    \r, $zero, \r
1:
.endm


# lr=context, $s3=pack (stereo, lastchan, disabled, lfo_enabled | pan_r, pan_l, ams(2) | AMmasks(4) | FB(4) | lfo_ampm(16))
# $a0-$a2=scratch, $a3=sin_tab, $t5=scratch, $t6-$t7=vol_out(4), $s1=op1_out
.macro upd_algo0_m

    # SLOT3
    make_eg_out SLOT3
    subu    $s6, $a1, ENV_QUIET
	bltz	$s6, 0f
	nop
    move    $a0, $zero
    b       1f
	nop
0:
    lw      $a2, 0x18($s4)
    lw      $a0, 0x38($s4) # mem (signed)
    srl     $a2, $a2, 16
	srl		$s6, $a0, 1
    addu    $a0, $a2, $s6
    lookup_tl $a0                  # $a0=c2

1:
    # SLOT4
    make_eg_out SLOT4
    subu    $s6, $a1, ENV_QUIET
	bltz	$s6, 2f
	nop
    move    $a0, $zero
    b       3f
	nop
2:
    lw      $a2, 0x1c($s4)
    srl     $a0, $a0, 1
	srl		$s6, $a2, 16
    addu    $a0, $a0, $s6
    lookup_tl $a0                  # $a0=output smp

3:
    # SLOT2
    make_eg_out SLOT2
    subu    $s6, $a1, ENV_QUIET
	bltz	$s6, 4f
	nop
    move    $a2, $zero
    b       5f
	nop
4:
    lw      $a2, 0x14($s4)       # 1ci
    srl     $t5, $s1, 17
	srl		$s6, $a2, 16
    addu    $a2, $t5, $s6
    lookup_tl $a2                  # $a2=mem

5:
    sw      $a2, 0x38($s4) # mem
.endm


.macro upd_algo1_m

    # SLOT3
    make_eg_out SLOT3
    subu    $s6, $a1, ENV_QUIET
	bltz	$s6, 0f
	nop
    move    $a0, $zero
    b       1f
	nop
0:
    lw      $a2, 0x18($s4)
    lw      $a0, 0x38($s4) # mem (signed)
    srl     $a2, $a2, 16
	srl		$s6, $a0, 1
    addu    $a0, $a2, $s6
    lookup_tl $a0                 # $a0=c2

1:
    # SLOT4
    make_eg_out SLOT4
    subu    $s6, $a1, ENV_QUIET
	bltz	$s6, 2f
	nop
    move    $a0, $zero
    b       3f
	nop
2:
    lw      $a2, 0x1c($s4)
    srl     $a0, $a0, 1
	srl		$s6, $a2, 16
    addu    $a0, $a0, $s6
    lookup_tl $a0                  # $a0=output smp

3:
    # SLOT2
    make_eg_out SLOT2
    subu    $s6, $a1, ENV_QUIET
	bltz	$s6, 4f
	nop
    move    $a2, $zero
    b       5f
	nop
4:
    lw      $a2, 0x14($s4)       # 1ci
	srl		$a2, $a2, 16
    lookup_tl $a2                  # $a2=mem

5:
	sra		$s6, $s1, 16
    addu    $a2, $a2, $s6
    sw      $a2, 0x38($s4)
.endm


.macro upd_algo2_m

    # SLOT3
    make_eg_out SLOT3
    subu    $s6, $a1, ENV_QUIET
	bltz	$s6, 0f
	nop
    move    $a0, $zero
    b       1f
	nop
0:
    lw      $a2, 0x18($s4)
    lw      $a0, 0x38($s4) # mem (signed)
    srl     $a2, $a2, 16
	srl		$s6, $a0, 1
    addu    $a0, $a2, $s6
    lookup_tl $a0                  # $a0=c2

1:
	sra		$s6, $s1, 16
    addu    $a0, $a0, $s6

    # SLOT4
    make_eg_out SLOT4
    subu    $s6, $a1, ENV_QUIET
	bltz	$s6, 2f
	nop
    move    $a0, $zero
    b       3f
	nop
2:
    lw      $a2, 0x1c($s4)
    srl     $a0, $a0, 1
	srl		$s6, $a2, 16
    addu    $a0, $a0, $s6
    lookup_tl $a0                  # $a0=output smp

3:
    # SLOT2
    make_eg_out SLOT2
    subu    $s6, $a1, ENV_QUIET
	bltz	$s6, 4f
	nop
    move    $a2, $zero
    b       5f
	nop
4:
    lw      $a2, 0x14($s4)
    srl     $a2, $a2, 16      	  # 1ci
    lookup_tl $a2                 # $a2=mem

5:
    sw      $a2, 0x38($s4) # mem
.endm


.macro upd_algo3_m

    # SLOT3
    make_eg_out SLOT3
    subu    $s6, $a1, ENV_QUIET
    lw      $a2, 0x38($s4) # mem (for future)
	bltz	$s6, 0f
	nop
    move    $a0, $a2
    b       1f
	nop
0:
    lw      $a0, 0x18($s4)      # 1ci
    srl     $a0, $a0, 16
    lookup_tl $a0                 # $a0=c2

1:
    addu    $a0, $a0, $a2

    # SLOT4
    make_eg_out SLOT4
    subu    $s6, $a1, ENV_QUIET
	bltz	$s6, 2f
	nop
    move    $a0, $zero
    b       3f
	nop
2:
    lw      $a2, 0x1c($s4)
    srl     $a0, $a0, 1
	srl		$s6, $a2, 16
    addu    $a0, $a0, $s6
    lookup_tl $a0                  # $a0=output smp

3:
    # SLOT2
    make_eg_out SLOT2
    subu    $s6, $a1, ENV_QUIET
	bltz	$s6, 4f
	nop
    move    $a2, $zero
    b       5f
	nop
4:
    lw      $a2, 0x14($s4)
    srl     $t5, $s1, 17
	srl		$s6, $a2, 16
    addu    $a2, $t5, $s6
    lookup_tl $a2                  # $a2=mem

5:
    sw     $a2, 0x38($s4) # mem
.endm


.macro upd_algo4_m

    # SLOT3
    make_eg_out SLOT3
    subu    $s6, $a1, ENV_QUIET
	bltz	$s6, 0f
	nop
    move    $a0, $zero
    b       1f
	nop
0:
    lw      $a0, 0x18($s4)
    srl     $a0, $a0, 16		   # 1ci
    lookup_tl $a0                  # $a0=c2

1:
    # SLOT4
    make_eg_out SLOT4
    subu    $s6, $a1, ENV_QUIET
	bltz	$s6, 2f
	nop
    move    $a0, $zero
    b       3f
	nop
2:
    lw      $a2, 0x1c($s4)
    srl     $a0, $a0, 1
	srl		$s6, $a2, 16
    addu    $a0, $a0, $s6
    lookup_tl $a0                  # $a0=output smp

3:

    # SLOT2
    make_eg_out SLOT2
    subu    $s6, $a1, ENV_QUIET
	bgez	$s6, 4f
	nop
    lw      $a2, 0x14($s4)
    srl     $t5, $s1, 17
	srl		$s6, $a2, 16
    addu    $a2, $t5, $s6
    lookup_tl $a2
	addu    $a0, $a0, $a2            # adduto smp
4:
.endm


.macro upd_algo5_m

    # SLOT3
    make_eg_out SLOT3
    subu    $s6, $a1, ENV_QUIET
	bltz	$s6, 0f
	nop
    move    $a0, $zero
    b       1f
	nop
0:
    lw      $a2, 0x18($s4)
    lw      $a0, 0x38($s4) # mem (signed)
    srl     $a2, $a2, 16
	srl		$s6, $a0, 1
    addu    $a0, $a2, $s6
    lookup_tl $a0                  # r0=output smp

1:
    # SLOT4
    make_eg_out SLOT4
    subu    $s6, $a1, ENV_QUIET
	bgez	$s6, 3f
	nop
    lw      $a2, 0x1c($s4)
    srl     $t5, $s1, 17
	srl		$s6, $a2, 16
    addu    $a2, $t5, $s6
    lookup_tl $a2
	addu    $a0, $a0, $a2           # adduto smp

3:
    # SLOT2
    make_eg_out SLOT2
    subu    $s6, $a1, ENV_QUIET
	bgez	$s6, 5f
	nop
    lw      $a2, 0x14($s4)
    srl     $t5, $s1, 17
	srl		$s6, $a2, 16
    addu    $a2, $t5, $s6
    lookup_tl $a2
	addu    $a0, $a0, $a2           # adduto smp

5:
    sra     $a1, $s1, 16
    sw      $a1, 0x38($s4) # mem
.endm


.macro upd_algo6_m

    # SLOT3
    make_eg_out SLOT3
    subu    $s6, $a1, ENV_QUIET
	bltz	$s6, 0f
	nop
    move    $a0, $zero
    b       1f
	nop
0:
    lw      $a0, 0x18($s4)
    srl     $a0, $a0, 16		   # 1ci
    lookup_tl $a0                  # $a0=output smp

1:
    # SLOT4
    make_eg_out SLOT4
    subu    $s6, $a1, ENV_QUIET
	bgez	$s6, 3f
	nop
    lw      $a2, 0x1c($s4)
	srl		$a2, $a2, 16	    	# 1ci
    lookup_tl $a2
	addu    $a0, $a0, $a2           # adduto smp

3:
    # SLOT2
    make_eg_out SLOT2
    subu    $s6, $a1, ENV_QUIET
	bgez	$s6, 5f
	nop
    lw      $a2, 0x14($s4)
    srl     $t5, $s1, 17
	srl		$s6, $a2, 16
    addu    $a2, $t5, $s6
    lookup_tl $a2
	addu    $a0, $a0, $a2           # adduto smp

5:
.endm


.macro upd_algo7_m

    # SLOT3
    make_eg_out SLOT3
    subu    $s6, $a1, ENV_QUIET
	bltz	$s6, 0f
	nop
    move    $a0, $zero
    b       1f
	nop
0:
    lw      $a0, 0x18($s4)
    srl     $a0, $a0, 16		  # 1ci
    lookup_tl $a0                 # $a0=output smp

1:
	sra		$s6, $s1, 16
    addu    $a0, $a0, $s6

    # SLOT4
    make_eg_out SLOT4
    subu    $s6, $a1, ENV_QUIET
	bgez	$s6, 3f
	nop
    lw      $a2, 0x1c($s4)
    srl     $a2, $a2, 16			# 1ci
    lookup_tl $a2
    addu    $a0, $a0, $a2           # adduto smp

3:
    # SLOT2
    make_eg_out SLOT2
    subu    $s6, $a1, ENV_QUIET
	bgez	$s6, 5f
	nop
    lw      $a2, 0x14($s4)
	srl		$a2, $a2, 16			# 1ci
    lookup_tl $a2
    addu    $a0, $a0, $a2           # adduto smp

5:
.endm


.macro upd_slot1_m

    make_eg_out SLOT1
    subu    $s6, $a1, ENV_QUIET
	bltz	$s6, 0f
	nop
    sll     $s1, $s1, 16     # ct->op1_out <<= 16; // op1_out0 = op1_out1; op1_out1 = 0;
    b       0f
	nop
0:
    and     $a2, $s3, 0xf000
    bnez	$a2, 1f
	nop
    move    $a0, $zero
	b		2f
	nop
1:
    srl     $a2, $a2, 12
	sll		$s6, $s1, 16
    addu    $a0, $s1, $s6
    sra     $a0, $a0, 16
    sllv    $a0, $a0, $a2

2:
    lw      $a2, 0x10($s4)
    srl     $a0, $a0, 16
	srl		$s6, $a2, 16
    addu    $a0, $a0, $s6
    lookup_tl $a0
    sll     $s1, $s1, 16     # ct->op1_out <<= 16;
    sll     $a0, $a0, 16
	srl		$s6, $a0, 16
    or      $s1, $s1, $s6

3:
.endm


/*
.global update_eg_phase # FM_SLOT *SLOT, UINT32 eg_cnt

update_eg_phase:
	addiu   $sp, $sp, -8
	sw 		$t6, 4($sp)
	sw 		$t5, ($sp)
    move    $t5, $a0             # slot
    lhu     $a3, 0x18($t5)       # tl
    lhu     $t6, 0x1a($t5)       # volume
    addu    $t6, $t6, $a3
    update_eg_phase_slot SLOT1
    move    $a0, $t6
	lw 		$t5, ($sp)
	lw 		$t6, 4($sp)
	addiu   $sp, $sp, -8

	#save_registers
    #jal     breakpoint0
    #nop
    #restore_registers

    jr      $ra
	nop
#.pool


.global advance_lfo # int lfo_ampm, UINT32 lfo_cnt_old, UINT32 lfo_cnt

advance_lfo:
    sll     $s3, $a0, 16
    advance_lfo_m
    srl     $a0, $s3, 16

	#save_registers
    #jal     breakpoint1
    #nop
    #restore_registers

    jr      $ra
	nop
#.pool


.global upd_algo0 # chan_rend_context *c
upd_algo0:
    #stmfd   sp!, {$t4-$s1,lr}
	addiu   $sp, $sp, -32
	sw 		$ra, 28($sp)
	sw 		$s1, 24($sp)
	sw 		$t9, 20($sp)
	sw 		$t8, 16($sp)
	sw 		$t7, 12($sp)
	sw 		$t6, 8($sp)
	sw 		$t5, 4($sp)
	sw 		$t4, ($sp)

    move    $s4, $a0

    lui     $a3, %hi(ym_sin_tab)
    li	    $s6, %lo(ym_sin_tab)
    addu    $a3, $a3, $s6
    lui     $t5, %hi(ym_tl_tab)
    li	    $s6, %lo(ym_tl_tab)
    addu    $t5, $t5, $s6
    #ldmia   lr, {$t6-$t7}
	lw		$t6, ($s4)
	lw		$t7, 4($s4)
    lw      $s1, 0x54($s4)
    lw      $s3, 0x4c($s4)

    upd_algo0_m

  	lw 		$t4, ($sp)
  	lw 		$t5, 4($sp)
  	lw 		$t6, 8($sp)
  	lw 		$t7, 12($sp)
  	lw 		$t8, 16($sp)
  	lw 		$t9, 20($sp)
  	lw 		$s1, 24($sp)
  	lw 		$ra, 28($sp)
  	addiu   $sp, $sp, 32
    #ldmfd   sp!, {$t4-$s1,pc}

	#save_registers
    #jal     breakpoint2
    #nop
    #restore_registers

	jr		$ra
	nop
#.pool


.global upd_algo1 # chan_rend_context *c
upd_algo1:
	addiu   $sp, $sp, -32
	sw 		$ra, 28($sp)
	sw 		$s1, 24($sp)
	sw 		$t9, 20($sp)
	sw 		$t8, 16($sp)
	sw 		$t7, 12($sp)
	sw 		$t6, 8($sp)
	sw 		$t5, 4($sp)
	sw 		$t4, ($sp)

    move    $s4, $a0

    lui     $a3, %hi(ym_sin_tab)
    li	    $s6, %lo(ym_sin_tab)
    addu    $a3, $a3, $s6
    lui     $t5, %hi(ym_tl_tab)
    li	    $s6, %lo(ym_tl_tab)
    addu    $t5, $t5, $s6
    #ldmia   lr, {$t6-$t7}
	lw		$t6, ($s4)
	lw		$t7, 4($s4)
    lw      $s1, 0x54($s4)
    lw      $s3, 0x4c($s4)

    upd_algo1_m

  	lw 		$t4, ($sp)
  	lw 		$t5, 4($sp)
  	lw 		$t6, 8($sp)
  	lw 		$t7, 12($sp)
  	lw 		$t8, 16($sp)
  	lw 		$t9, 20($sp)
  	lw 		$s1, 24($sp)
  	lw 		$ra, 28($sp)
  	addiu   $sp, $sp, 32

	#save_registers
    #jal     breakpoint2
    #nop
    #restore_registers

	jr		$ra
	nop
#.pool


.global upd_algo2 # chan_rend_context *c
upd_algo2:
	addiu   $sp, $sp, -32
	sw 		$ra, 28($sp)
	sw 		$s1, 24($sp)
	sw 		$t9, 20($sp)
	sw 		$t8, 16($sp)
	sw 		$t7, 12($sp)
	sw 		$t6, 8($sp)
	sw 		$t5, 4($sp)
	sw 		$t4, ($sp)

    move    $s4, $a0

    lui     $a3, %hi(ym_sin_tab)
    li	    $s6, %lo(ym_sin_tab)
    addu    $a3, $a3, $s6
    lui     $t5, %hi(ym_tl_tab)
    li	    $s6, %lo(ym_tl_tab)
    addu    $t5, $t5, $s6
    #ldmia   lr, {$t6-$t7}
	lw		$t6, ($s4)
	lw		$t7, 4($s4)
    lw      $s1, 0x54($s4)
    lw      $s3, 0x4c($s4)

    upd_algo2_m

  	lw 		$t4, ($sp)
  	lw 		$t5, 4($sp)
  	lw 		$t6, 8($sp)
  	lw 		$t7, 12($sp)
  	lw 		$t8, 16($sp)
  	lw 		$t9, 20($sp)
  	lw 		$s1, 24($sp)
  	lw 		$ra, 28($sp)
  	addiu   $sp, $sp, 32

	#save_registers
    #jal     breakpoint2
    #nop
    #restore_registers

	jr		$ra
	nop
#.pool


.global upd_algo3 # chan_rend_context *c
upd_algo3:
	addiu   $sp, $sp, -32
	sw 		$ra, 28($sp)
	sw 		$s1, 24($sp)
	sw 		$t9, 20($sp)
	sw 		$t8, 16($sp)
	sw 		$t7, 12($sp)
	sw 		$t6, 8($sp)
	sw 		$t5, 4($sp)
	sw 		$t4, ($sp)

    move    $s4, $a0

    lui     $a3, %hi(ym_sin_tab)
    li	    $s6, %lo(ym_sin_tab)
    addu    $a3, $a3, $s6
    lui     $t5, %hi(ym_tl_tab)
    li	    $s6, %lo(ym_tl_tab)
    addu    $t5, $t5, $s6
    #ldmia   lr, {$t6-$t7}
	lw		$t6, ($s4)
	lw		$t7, 4($s4)
    lw      $s1, 0x54($s4)
    lw      $s3, 0x4c($s4)

    upd_algo3_m

  	lw 		$t4, ($sp)
  	lw 		$t5, 4($sp)
  	lw 		$t6, 8($sp)
  	lw 		$t7, 12($sp)
  	lw 		$t8, 16($sp)
  	lw 		$t9, 20($sp)
  	lw 		$s1, 24($sp)
  	lw 		$ra, 28($sp)
  	addiu   $sp, $sp, 32

	#save_registers
    #jal     breakpoint2
    #nop
    #restore_registers

	jr		$ra
	nop
#.pool


.global upd_algo4 # chan_rend_context *c
upd_algo4:
	addiu   $sp, $sp, -32
	sw 		$ra, 28($sp)
	sw 		$s1, 24($sp)
	sw 		$t9, 20($sp)
	sw 		$t8, 16($sp)
	sw 		$t7, 12($sp)
	sw 		$t6, 8($sp)
	sw 		$t5, 4($sp)
	sw 		$t4, ($sp)

    move    $s4, $a0

    lui     $a3, %hi(ym_sin_tab)
    li	    $s6, %lo(ym_sin_tab)
    addu    $a3, $a3, $s6
    lui     $t5, %hi(ym_tl_tab)
    li	    $s6, %lo(ym_tl_tab)
    addu    $t5, $t5, $s6
    #ldmia   lr, {$t6-$t7}
	lw		$t6, ($s4)
	lw		$t7, 4($s4)
    lw      $s1, 0x54($s4)
    lw      $s3, 0x4c($s4)

    upd_algo4_m

  	lw 		$t4, ($sp)
  	lw 		$t5, 4($sp)
  	lw 		$t6, 8($sp)
  	lw 		$t7, 12($sp)
  	lw 		$t8, 16($sp)
  	lw 		$t9, 20($sp)
  	lw 		$s1, 24($sp)
  	lw 		$ra, 28($sp)
  	addiu   $sp, $sp, 32

	#save_registers
    #jal     breakpoint2
    #nop
    #restore_registers

	jr		$ra
	nop
#.pool


.global upd_algo5 # chan_rend_context *c
upd_algo5:
	addiu   $sp, $sp, -32
	sw 		$ra, 28($sp)
	sw 		$s1, 24($sp)
	sw 		$t9, 20($sp)
	sw 		$t8, 16($sp)
	sw 		$t7, 12($sp)
	sw 		$t6, 8($sp)
	sw 		$t5, 4($sp)
	sw 		$t4, ($sp)

    move    $s4, $a0

    lui     $a3, %hi(ym_sin_tab)
    li	    $s6, %lo(ym_sin_tab)
    addu    $a3, $a3, $s6
    lui     $t5, %hi(ym_tl_tab)
    li	    $s6, %lo(ym_tl_tab)
    addu    $t5, $t5, $s6
    #ldmia   lr, {$t6-$t7}
	lw		$t6, ($s4)
	lw		$t7, 4($s4)
    lw      $s1, 0x54($s4)
    lw      $s3, 0x4c($s4)

    upd_algo5_m

  	lw 		$t4, ($sp)
  	lw 		$t5, 4($sp)
  	lw 		$t6, 8($sp)
  	lw 		$t7, 12($sp)
  	lw 		$t8, 16($sp)
  	lw 		$t9, 20($sp)
  	lw 		$s1, 24($sp)
  	lw 		$ra, 28($sp)
  	addiu   $sp, $sp, 32

	#save_registers
    #jal     breakpoint2
    #nop
    #restore_registers

	jr		$ra
	nop
#.pool


.global upd_algo6 # chan_rend_context *c
upd_algo6:
	addiu   $sp, $sp, -32
	sw 		$ra, 28($sp)
	sw 		$s1, 24($sp)
	sw 		$t9, 20($sp)
	sw 		$t8, 16($sp)
	sw 		$t7, 12($sp)
	sw 		$t6, 8($sp)
	sw 		$t5, 4($sp)
	sw 		$t4, ($sp)

    move    $s4, $a0

    lui     $a3, %hi(ym_sin_tab)
    li	    $s6, %lo(ym_sin_tab)
    addu    $a3, $a3, $s6
    lui     $t5, %hi(ym_tl_tab)
    li	    $s6, %lo(ym_tl_tab)
    addu    $t5, $t5, $s6
    #ldmia   lr, {$t6-$t7}
	lw		$t6, ($s4)
	lw		$t7, 4($s4)
    lw      $s1, 0x54($s4)
    lw      $s3, 0x4c($s4)

    upd_algo6_m

  	lw 		$t4, ($sp)
  	lw 		$t5, 4($sp)
  	lw 		$t6, 8($sp)
  	lw 		$t7, 12($sp)
  	lw 		$t8, 16($sp)
  	lw 		$t9, 20($sp)
  	lw 		$s1, 24($sp)
  	lw 		$ra, 28($sp)
  	addiu   $sp, $sp, 32

	#save_registers
    #jal     breakpoint2
    #nop
    #restore_registers

	jr		$ra
	nop
#.pool


.global upd_algo7 # chan_rend_context *c
upd_algo7:
	addiu   $sp, $sp, -32
	sw 		$ra, 28($sp)
	sw 		$s1, 24($sp)
	sw 		$t9, 20($sp)
	sw 		$t8, 16($sp)
	sw 		$t7, 12($sp)
	sw 		$t6, 8($sp)
	sw 		$t5, 4($sp)
	sw 		$t4, ($sp)

    move    $s4, $a0

    lui     $a3, %hi(ym_sin_tab)
    li	    $s6, %lo(ym_sin_tab)
    addu    $a3, $a3, $s6
    lui     $t5, %hi(ym_tl_tab)
    li	    $s6, %lo(ym_tl_tab)
    addu    $t5, $t5, $s6
    #ldmia   lr, {$t6-$t7}
	lw		$t6, ($s4)
	lw		$t7, 4($s4)
    lw      $s1, 0x54($s4)
    lw      $s3, 0x4c($s4)

    upd_algo7_m

  	lw 		$t4, ($sp)
  	lw 		$t5, 4($sp)
  	lw 		$t6, 8($sp)
  	lw 		$t7, 12($sp)
  	lw 		$t8, 16($sp)
  	lw 		$t9, 20($sp)
  	lw 		$s1, 24($sp)
  	lw 		$ra, 28($sp)
  	addiu   $sp, $sp, 32

	#save_registers
    #jal     breakpoint2
    #nop
    #restore_registers

	jr		$ra
	nop
#.pool


.global upd_slot1 # chan_rend_context *c
upd_slot1:
	addiu   $sp, $sp, -32
	sw 		$ra, 28($sp)
	sw 		$s1, 24($sp)
	sw 		$t9, 20($sp)
	sw 		$t8, 16($sp)
	sw 		$t7, 12($sp)
	sw 		$t6, 8($sp)
	sw 		$t5, 4($sp)
	sw 		$t4, ($sp)

    move    $s4, $a0

    lui     $a3, %hi(ym_sin_tab)
    li	    $s6, %lo(ym_sin_tab)
    addu    $a3, $a3, $s6
    lui     $t5, %hi(ym_tl_tab)
    li	    $s6, %lo(ym_tl_tab)
    addu    $t5, $t5, $s6
    #ldmia   lr, {$t6-$t7}
	lw		$t6, ($s4)
	lw		$t7, 4($s4)
    lw      $s1, 0x54($s4)
    lw      $s3, 0x4c($s4)

    upd_slot1_m
    sw      $s1, 0x38($s4)

  	lw 		$t4, ($sp)
  	lw 		$t5, 4($sp)
  	lw 		$t6, 8($sp)
  	lw 		$t7, 12($sp)
  	lw 		$t8, 16($sp)
  	lw 		$t9, 20($sp)
  	lw 		$s1, 24($sp)
  	lw 		$ra, 28($sp)
  	addiu   $sp, $sp, 32

	#save_registers
    #jal     breakpoint2
    #nop
    #restore_registers

	jr		$ra
	nop
#.pool
*/


# lr=context, $s3=pack (stereo, lastchan, disabled, lfo_enabled | pan_r, pan_l, ams(2) | AMmasks(4) | FB(4) | lfo_ampm(16))
# $a0-$a2=scratch, $a3=sin_tab/scratch, $t4=(length<<8)|unused(4),was_update,algo(3), $t5=tl_tab/slot,
# $t6-$t7=vol_out(4), $t8=eg_timer, $t9=eg_timer_add(31:16), $s1=op1_out, $s2=buffer
.global chan_render_loop # chan_rend_context *ct, int *buffer, int length

chan_render_loop:

	addiu   $sp, $sp, -68
	sw 		$ra, 64($sp)
	sw 		$a3, 60($sp)
	sw 		$a2, 56($sp)
	sw 		$a1, 52($sp)
	sw 		$a0, 48($sp)
	sw 		$s6, 44($sp)
	sw 		$s5, 40($sp)
	sw 		$s4, 36($sp)
	sw 		$s3, 32($sp)
	sw 		$s2, 28($sp)
	sw 		$s1, 24($sp)
	sw 		$t9, 20($sp)
	sw 		$t8, 16($sp)
	sw 		$t7, 12($sp)
	sw 		$t6, 8($sp)
	sw 		$t5, 4($sp)
	sw 		$t4, ($sp)
    move    $s4,  $a0
    sll     $t4,  $a2, 8      # no more 24 bits here
    lw      $s3, 0x4c($s4)
    lw      $a0, 0x50($s4)
    move    $s2, $a1
    andi    $a0,  $a0, 7
    or      $t4,  $t4, $a0          # (length<<8)|algo
    addu    $a0,  $s4, 0x44
	lw		$t8, ($a0)				# eg_timer, eg_timer_add
	lw		$t9, 4($a0)
    lw      $s1, 0x54($s4)     # op1_out
	lw		$t6, ($s4)				# load volumes
	lw		$t7, 4($s4)

    andi    $s6, $s3, 8              # lfo?
    beqz    $s6, crl_loop
	nop

crl_loop_lfo:
    addu    $a0, $s4, 0x30
	lw		$a1, ($a0)
	lw		$a2, 4($a0)
    addu    $a2, $a2, $a1
    sw      $a2, 0x30($s4)
    # $s3=lfo_ampm(31:16), $a1=lfo_cnt_old, $a2=lfo_cnt
    advance_lfo_m

crl_loop:
    subu    $t4, $t4, 0x100
    bltz    $t4, crl_loop_end
	nop

    # -- EG --
    addu    $t8, $t8, $t9
	lui		$s6, EG_TIMER_OVERFLOW>>16
    subu    $s6, $t8, $s6
    bltz    $s6, eg_done
	nop
    addu    $a0, $s4, 0x3c
	lw		$a1, ($a0)				# eg_cnt, CH
	lw		$t5, 4($a0)
eg_loop:
    #addi    $t8, $t8, -EG_TIMER_OVERFLOW
	#li		$s6, EG_TIMER_OVERFLOW
	lui		$s6, EG_TIMER_OVERFLOW>>16
    subu    $t8, $t8, $s6
    addu    $a1, $a1, 1
                                        # SLOT1 (0)
    # $t5=slot, $a1=eg_cnt, trashes: $a0,$a2,$a3
    update_eg_phase_slot SLOT1
    #update_eg_phase_slot 1 0 0 0 #SLOT1
    addu    $t5, $t5, SLOT_STRUCT_SIZE*2 # SLOT2 (2)
    update_eg_phase_slot SLOT2
    #update_eg_phase_slot 0 1 0 0 #SLOT2
    subu    $t5, $t5, SLOT_STRUCT_SIZE   # SLOT3 (1)
    update_eg_phase_slot SLOT3
    #update_eg_phase_slot 0 0 1 0 #SLOT3
    addu    $t5, $t5, SLOT_STRUCT_SIZE*2 # SLOT4 (3)
    update_eg_phase_slot SLOT4
    #update_eg_phase_slot 0 0 0 1 #SLOT4

	lui		$s6, EG_TIMER_OVERFLOW>>16
    subu    $s6, $t8, $s6
	bltz	$s6, skip0
	nop
    subu    $t5, $t5, SLOT_STRUCT_SIZE*3
    b       eg_loop
	nop
skip0:
    sw     $a1, 0x3c($s4)

eg_done:

    # -- disabled? --
    and     $a0, $s3, 0xC
    subu    $s6, $a0, 0xC
    beqz    $s6, crl_loop_lfo
	nop
    subu    $s6, $a0, 0x4
    beqz    $s6, crl_loop
	nop

    # -- SLOT1 --
    lui     $a3, %hi(ym_tl_tab)
    li	    $s6, %lo(ym_tl_tab)
    addu    $a3, $a3, $s6

    # lr=context, $s3=pack (stereo, lastchan, disabled, lfo_enabled | pan_r, pan_l, ams(2) | AMmasks(4) | FB(4) | lfo_ampm(16))
    # $a0-$a2=scratch, $a3=tl_tab, $t5=scratch, $t6-$t7=vol_out(4), $s1=op1_out
    upd_slot1_m

    # -- SLOT2+ --
    and     $a0, $t4, 7
	beqz	$a0, crl_algo0
	nop
	sub		$s6, $a0, 1
	beqz	$s6, crl_algo1
	nop
	sub		$s6, $a0, 2
	beqz	$s6, crl_algo2
	nop
	sub		$s6, $a0, 3
	beqz	$s6, crl_algo3
	nop
	sub		$s6, $a0, 4
	beqz	$s6, crl_algo4
	nop
	sub		$s6, $a0, 5
	beqz	$s6, crl_algo5
	nop
	sub		$s6, $a0, 6
	beqz	$s6, crl_algo6
	nop
	sub		$s6, $a0, 7
	beqz	$s6, crl_algo7
	nop

crl_algo0:
    upd_algo0_m
    b       crl_algo_done
	nop
    #.pool

crl_algo1:
    upd_algo1_m
    b       crl_algo_done
	nop
    #.pool

crl_algo2:
    upd_algo2_m
    b       crl_algo_done
	nop
    #.pool

crl_algo3:
    upd_algo3_m
    b       crl_algo_done
	nop
    #.pool

crl_algo4:
    upd_algo4_m
    b       crl_algo_done
	nop
    #.pool

crl_algo5:
    upd_algo5_m
    b       crl_algo_done
    #.pool

crl_algo6:
    upd_algo6_m
    b       crl_algo_done
    #.pool

crl_algo7:
    upd_algo7_m
    #.pool


crl_algo_done:
    # -- WRITE SAMPLE --
    and     $s6, $a0, $a0
    beqz    $s6, ctl_sample_skip
	nop
    or      $t4, $t4, 8              # have_output
    and     $s6, $s3, 1
    beqz    $s6, ctl_sample_mono
	nop

    and     $s6, $s3, 0x20              # L
	beqz    $s6, skip1
	nop
    lw      $a1, ($s2)
    addu    $a1, $a0, $a1
    sw      $a1, ($s2)
	add		$s2, $s2, 4
	b		skip2
	nop
skip1:
	addu    $s2, $s2, 4
skip2:
    and     $s6, $s3, 0x10              # R
	beqz    $s6, skip3
	nop
    lw      $a1, ($s2)
    addu    $a1, $a0, $a1
    sw      $a1, ($s2)
	add		$s2, $s2, 4
	b		skip4
	nop
skip3:
	addu    $s2, $s2, 4
skip4:
    b       crl_do_phase
	nop

ctl_sample_skip:
    and     $a1, $s3, 1
    addu    $a1, $a1, 1
	sll		$s6, $a1, 2
    addu    $s2, $s2, $s6
    b       crl_do_phase
	nop

ctl_sample_mono:
    lw      $a1, ($s2)
    addu    $a1, $a0, $a1
    sw      $a1, ($s2)
	add		$s2, $s2, 4

crl_do_phase:
    # -- PHASE UPDATE --
    addu    $t5, $s4, 0x10
    #ldmia   $t5, {$a0-$a1}
	lw		$a0, ($t5)
	lw		$a1, 4($t5)
    addu    $t5, $s4, 0x20
    #ldmia   $t5, {$a2-$a3}
	lw		$a2, ($t5)
	lw		$a3, 4($t5)
    addu    $t5, $s4, 0x10
    addu    $a0, $a0, $a2
    addu    $a1, $a1, $a3
    #stmia   $t5!,{$a0-$a1}
	sw		$a0, ($t5)
	sw		$a1, 4($t5)
	add		$t5, $t5, 8
    #ldmia   $t5, {$a0-$a1}
	lw		$a0, ($t5)
	lw		$a1, 4($t5)
    addu    $t5, $s4, 0x28
    #ldmia   $t5, {$a2-$a3}
	lw		$a2, ($t5)
	lw		$a3, 4($t5)
    addu    $t5, $s4, 0x18
    addu    $a0, $a0, $a2
    addu    $a1, $a1, $a3
    #stmia   $t5, {$a0-$a1}
	sw		$a0, ($t5)
	sw		$a1, 4($t5)

    and     $s6, $s3, 8
    bnez    $s6, crl_loop_lfo
	nop
    b       crl_loop
	nop


crl_loop_end:
    sw     $t8, 0x44($s4)     # eg_timer
    sw     $s3, 0x4c($s4)     # pack (for lfo_ampm)
    sw     $t4, 0x50($s4)     # was_update
    sw     $s1, 0x54($s4)     # op1_out
    #ldmfd   sp!, {$t4-$s2,pc}
  	lw 		$t4, ($sp)
  	lw 		$t5, 4($sp)
  	lw 		$t6, 8($sp)
  	lw 		$t7, 12($sp)
  	lw 		$t8, 16($sp)
  	lw 		$t9, 20($sp)
  	lw 		$s1, 24($sp)
	lw 		$s2, 28($sp)
	lw 		$s3, 32($sp)
	lw 		$s4, 36($sp)
	lw 		$s5, 40($sp)
	lw 		$s6, 44($sp)
	lw 		$a0, 48($sp)
	lw 		$a1, 52($sp)
	lw 		$a2, 56($sp)
	lw 		$a3, 60($sp)
  	lw 		$ra, 64($sp)
  	addiu   $sp, $sp, 68

	#save_registers
    #jal     breakpoint3
    #nop
    #restore_registers

	jr		$ra
	nop

#.pool

