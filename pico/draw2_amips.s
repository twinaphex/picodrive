/*
 * assembly optimized versions of most funtions from draw2.c
 * (C) notaz, 2006-2008
 *
 * This work is licensed under the terms of MAME license.
 * See COPYING file in the top-level directory.
 *
 * this is highly specialized, be careful if changing related C code!
 */

.extern Pico
.extern PicoDraw2FB

# _define these constants in your include file:
 .equiv START_ROW, 		0
 .equiv END_ROW, 		28
# one row means 8 pixels. If above example was used, (27-1)*8=208 lines would be rendered.
#ifndef START_ROW
#define START_ROW 0
#endif
#ifndef END_ROW
#define END_ROW 28
#endif

.set noreorder
.set noat

.text
.align 2

.global BackFillFull # int reg7

BackFillFull:
	#addiu   $sp, $sp, -4     # allocate 1 word on the stack
    #sw   $sp, ($ra)

    lw     $t8, PicoDraw2FB      # lr=PicoDraw2FB
    sll		$a0, $a0, 26
    #lw     $t8, ($t8)
    srl     $a0, $a0, 26
    add     $t8, $t8, (512*8)+8      # 512: line_width in psp, others -> 328

	sll		$t7, $a0, 8
    or     $a0, $a0, $t7
    sll		$t7, $a0, 16
    or     $a0, $a0, $t7

    move     $a1, $a0 # 25 opcodes wasted?
    move     $a2, $a0
    move     $a3, $a0
    move     $t0, $a0
    move     $t1, $a0
    move     $t2, $a0
    move     $t3, $a0
    move     $t4, $a0
    move     $t5, $a0

    li     $t6, (END_ROW-START_ROW)*8

    # go go go!
.bff_loop:
    #add     $ra, $ra, 8
    sub    $t6, $t6, 1   #arm subs

    sw		$a0, ($t8)       # 10*4*8
    sw		$a1, 4($t8)
    sw		$a2, 8($t8)
    sw		$a3, 12($t8)
    sw		$t0, 16($t8)
    sw		$t1, 20($t8)
    sw		$t2, 24($t8)
    sw		$t3, 28($t8)
    sw		$t4, 32($t8)
    sw		$t5, 36($t8)
    addiu   $t8, $t8, 40

    sw		$a0, ($t8)
    sw		$a1, 4($t8)
    sw		$a2, 8($t8)
    sw		$a3, 12($t8)
    sw		$t0, 16($t8)
    sw		$t1, 20($t8)
    sw		$t2, 24($t8)
    sw		$t3, 28($t8)
    sw		$t4, 32($t8)
    sw		$t5, 36($t8)
    addiu   $t8, $t8, 40

    sw		$a0, ($t8)
    sw		$a1, 4($t8)
    sw		$a2, 8($t8)
    sw		$a3, 12($t8)
    sw		$t0, 16($t8)
    sw		$t1, 20($t8)
    sw		$t2, 24($t8)
    sw		$t3, 28($t8)
    sw		$t4, 32($t8)
    sw		$t5, 36($t8)
    addiu   $t8, $t8, 40

    sw		$a0, ($t8)
    sw		$a1, 4($t8)
    sw		$a2, 8($t8)
    sw		$a3, 12($t8)
    sw		$t0, 16($t8)
    sw		$t1, 20($t8)
    sw		$t2, 24($t8)
    sw		$t3, 28($t8)
    sw		$t4, 32($t8)
    sw		$t5, 36($t8)
    addiu   $t8, $t8, 40

    sw		$a0, ($t8)
    sw		$a1, 4($t8)
    sw		$a2, 8($t8)
    sw		$a3, 12($t8)
    sw		$t0, 16($t8)
    sw		$t1, 20($t8)
    sw		$t2, 24($t8)
    sw		$t3, 28($t8)
    sw		$t4, 32($t8)
    sw		$t5, 36($t8)
    addiu   $t8, $t8, 40

    sw		$a0, ($t8)
    sw		$a1, 4($t8)
    sw		$a2, 8($t8)
    sw		$a3, 12($t8)
    sw		$t0, 16($t8)
    sw		$t1, 20($t8)
    sw		$t2, 24($t8)
    sw		$t3, 28($t8)
    sw		$t4, 32($t8)
    sw		$t5, 36($t8)
    addiu   $t8, $t8, 40

    sw		$a0, ($t8)
    sw		$a1, 4($t8)
    sw		$a2, 8($t8)
    sw		$a3, 12($t8)
    sw		$t0, 16($t8)
    sw		$t1, 20($t8)
    sw		$t2, 24($t8)
    sw		$t3, 28($t8)
    sw		$t4, 32($t8)
    sw		$t5, 36($t8)
    addiu   $t8, $t8, 40

    sw		$a0, ($t8)
    sw		$a1, 4($t8)
    sw		$a2, 8($t8)
    sw		$a3, 12($t8)
    sw		$t0, 16($t8)
    sw		$t1, 20($t8)
    sw		$t2, 24($t8)
    sw		$t3, 28($t8)
    sw		$t4, 32($t8)
    sw		$t5, 36($t8)
    addiu   $t8, $t8, 40

    addiu   $t8, $t8, (512-320) # psp screen offset

    bnez    $t6, .bff_loop
    nop

    #ldmfd   sp!, {$t0-$t5,$t6}
    jr      $ra
    nop

#.pool

# -------- some macros --------

# helper
# TileLineSinglecol ($t1=pdest, $t2=pixels8, $t3=pal) $t4: scratch, $t0: pixels8_old
.macro TileLineSinglecol notsinglecol=0
    and     $t2, $t2, 0xf        # #0x0000000f
.if !\notsinglecol
	srl		$s6, $t0, 28
	beq		$t2, $s6, 11f    # if these don't match,
	nop
	li		$s6, 2
	not		$s6, $s6
	and		$t9, $t9, $s6        # it is a sign that whole tile is not singlecolor (only it's lines may be)
11:
.endif
    or     	$t4, $t3, $t2
    sll	   	$s6, $t4, 8
    or     	$t4, $t4, $s6

	and		$s6, $t1, 0x1
    bnez	$s6, 12f       # not aligned?
    nop
    sh  	$t4, ($t1)
    sh  	$t4, 2($t1)
    sh  	$t4, 4($t1)
    sh  	$t4, 6($t1)
    b		13f
    nop
12:
    sb  	$t4, ($t1)
    sh  	$t4, 1($t1)
    sh  	$t4, 3($t1)
    sh  	$t4, 5($t1)
    sb  	$t4, 7($t1)
13:
.if !\notsinglecol
    li     	$t0, 0xf
    sll		$s6, $t2, 28
    or     	$t0, $t0, $s6	 # we will need the old palindex later
.endif
.endm

# TileNorm ($t1=pdest, $t2=pixels8, $t3=pal) $t0,$t4: scratch
.macro TileLineNorm
	srl		$s6, $t2, 12		   # #0x0000f000
    and     $t4, $t0, $s6
    beqz	$t4, 21f
    nop
    or      $t4, $t3, $t4
    sb      $t4, ($t1)
21:
	srl		$s6, $t2, 8			   # #0x00000f00
    and     $t4, $t0, $s6
    beqz	$t4, 22f
    nop
    or      $t4, $t3, $t4
    sb      $t4, 1($t1)
22:
	srl		$s6, $t2, 4			   # #0x000000f0
    and     $t4, $t0, $s6
    beqz	$t4, 23f
    nop
    or      $t4, $t3, $t4
    sb      $t4, 2($t1)
23:
    and     $t4, $t0, $t2		   # #0x0000000f
    beqz	$t4, 24f
    nop
    or      $t4, $t3, $t4
    sb      $t4, 3($t1)
24:
	srl		$s6, $t2, 28		   # #0xf0000000
    and     $t4, $t0, $s6
    beqz	$t4, 25f
    nop
    or      $t4, $t3, $t4
    sb      $t4, 4($t1)
25:
	srl		$s6, $t2, 24		   # #0x0f000000
    and     $t4, $t0, $s6
    beqz	$t4, 26f
    nop
    or      $t4, $t3, $t4
    sb      $t4, 5($t1)
26:
	srl		$s6, $t2, 20		   # #0x00f00000
    and     $t4, $t0, $s6
    beqz	$t4, 27f
    nop
    or      $t4, $t3, $t4
    sb      $t4, 6($t1)
27:
	srl		$s6, $t2, 16		   # #0x000f0000
    and     $t4, $t0, $s6
    beqz	$t4, 28f
    nop
    or      $t4, $t3, $t4
    sb      $t4, 7($t1)
28:
.endm

# TileFlip ($t1=pdest, $t2=pixels8, $t3=pal) $t0,$t4: scratch
.macro TileLineFlip
	srl		$s6, $t2, 16		   # #0x000f0000
    and     $t4, $t0, $s6
    beqz	$t4, 31f
    nop
    or      $t4, $t3, $t4
    sb      $t4, ($t1)
31:
	srl		$s6, $t2, 20		   # #0x00f00000
    and     $t4, $t0, $s6
    beqz	$t4, 32f
    nop
    or      $t4, $t3, $t4
    sb      $t4, 1($t1)
32:
	srl		$s6, $t2, 24		   # #0x0f000000
    and     $t4, $t0, $s6
    beqz	$t4, 33f
    nop
    or      $t4, $t3, $t4
    sb      $t4, 2($t1)
33:
    srl		$s6, $t2, 28		   # #0xf0000000
    and     $t4, $t0, $s6
    beqz	$t4, 34f
    nop
    or      $t4, $t3, $t4
    sb      $t4, 3($t1)
34:
    and     $t4, $t0, $t2		   # #0x0000000f
    beqz	$t4, 35f
    nop
    or      $t4, $t3, $t4
    sb      $t4, 4($t1)
35:
	srl		$s6, $t2, 4			   # #0x000000f0
    and     $t4, $t0, $s6
    beqz	$t4, 36f
    nop
    or      $t4, $t3, $t4
    sb      $t4, 5($t1)
36:
	srl		$s6, $t2, 8			   # #0x00000f00
    and     $t4, $t0, $s6
    beqz	$t4, 37f
    nop
    or      $t4, $t3, $t4
    sb      $t4, 6($t1)
37:
	srl		$s6, $t2, 12		   # #0x0000f000
    and     $t4, $t0, $s6
    beqz	$t4, 38f
    nop
    or      $t4, $t3, $t4
    sb      $t4, 7($t1)
38:
.endm

# Tile ($t1=pdest, $t3=pal, $t9=prevcode, $a1=Pico.vram) $t2,$t4,$t7: scratch, $t0=0xf
.macro Tile hflip vflip
	li		$s7, (1<<24)
    sll     $t7, $t9, 13           # $t9=code<<8; addr=(code&0x7ff)<<4;
    srl		$s6, $t7, 16
    add     $t7, $a1, $s6
    or      $t9, $t9, 3            # emptytile=singlecolor=1, $t9 must be <code_16> 00000xxx
.if \vflip
    # we read tilecodes in reverse order if we have vflip
    add     $t7, $t7, 8*4
.endif
    # loop through 8 lines
    li		$s6, (7<<24)
    or      $t9, $t9, $s6
    b       1f # loop_enter
    nop

0:  # singlecol_loop
	#li		$s6, (1<<24)
    sub     $t9, $t9, $s7
    add     $t1, $t1, 512         # set pointer to next line
    bltz    $t9, 8f               # loop_exit with $t0 restore
    nop
1:
.if \vflip
	add		$t7, $t7, -4
    lw      $t2, ($t7)            # pack=*(unsigned int *)(Pico.vram+addr); // Get 8 pixels
.else
    lw	    $t2, ($t7)
    add		$t7, $t7, 4
.endif
    beqz    $t2, 2f                    # empty line
    nop
	li		$s6, 1
	not		$s6, $s6
	and		$t9, $t9, $s6        # fix height
    rotr	$s6, $t2, 4
    bne     $s6, $t2, 3f                    # not singlecolor
    nop
    TileLineSinglecol
    nop
    b       0b
    nop

2:
	li		$s6, 2
	not		$s6, $s6
	and		$t9, $t9, $s6
2:  # empty_loop
	#li		$s6, (1<<24)
    sub     $t9, $t9, $s7
    add     $t1, $t1, 512          # set pointer to next line
    bltz	$t9, 8f # loop_exit with $t0 restore
    nop
.if \vflip
	add		$t7, $t7, -4
    lw      $t2, ($t7)  			# next pack
.else
    lw      $t2, ($t7)
    add		$t7, $t7, 4
.endif
    li      $t0, 0xf              # singlecol_loop might have messed $a0
    beqz    $t2, 2b
    nop

	li		$s6, 3
	not		$s6, $s6
	and		$t9, $t9, $s6         # if we are here, it means we have empty and not empty line
    b       5f
    nop

3:  # not empty, not singlecol
    li      $t0, 0xf
	li		$s6, 3
	not		$s6, $s6
	and		$t9, $t9, $s6
    b       6f
    nop

4:  # not empty, not singlecol loop
	#li		$s6, (1<<24)
    sub     $t9, $t9, $s7
    add     $t1, $t1, 512          # set pointer to next line
    bltz    $t9, 9f # loop_exit
    nop
.if \vflip
	add		$t7, $t7, -4
    lw      $t2, ($t7)  			# next pack
.else
    lw      $t2, ($t7)
    add		$t7, $t7, 4
.endif
    beqz    $t2, 4b                 # empty line
    nop
5:
    rotr	$s6, $t2, 4
    beq     $s6, $t2, 7f			# singlecolor line
    nop
6:
.if \hflip
    TileLineFlip
    nop
.else
    TileLineNorm
    nop
.endif
    b       4b
    nop
7:
    TileLineSinglecol 1
    nop
    b       4b
    nop

8:
    li      $t0, 0xf
9:  # loop_exit
	#li		$s6, (1<<24)
    add     $t9, $t9, $s7          # fix $t9
    sub     $t1, $t1, 512*8        # restore pdest pointer
.endm

# TileLineSinglecolAl ($t1=pdest, $t4,$t7=color)
.macro TileLineSinglecolAl0
    sh		$t4, ($t1)
    sh		$t5, 2($t1)
    sh		$t6, 4($t1)
    sh		$t7, 6($t1)
    add     $t1, $t1, (8+512)
.endm

.macro TileLineSinglecolAl1
    sb      $t0, ($t1)
    sh      $t0, 1($t1)
    sw      $t0, 3($t1)
    sb      $t0, 7($t1)
    add     $t1, $t1, (8+512)
.endm

.macro TileLineSinglecolAl2
    sh      $t0, ($t1)
    sw      $t0, 2($t1)
    sh      $t0, 6($t1)
    add     $t1, $t1, (8+512)
.endm

.macro TileLineSinglecolAl3
    sb      $t0, ($t1)
    sw      $t0, 1($t1)
    sh      $t0, 5($t1)
    sb      $t0, 7($t1)
    add     $t1, $t1, (8+512)
.endm

# TileSinglecol ($t1=pdest, $t2=pixels8, $t3=pal) $t4,$t7: scratch, $t0=0xf
# kaligned==1, if dest is always aligned
.macro TileSinglecol kaligned=0
    and     $t4, $t2, 0xf       # we assume we have good $t2 from previous time
    or      $t4, $t4, $t3
    sll		$s6, $t4, 8
    or      $t4, $t4, $s6
    sll		$s6, $t4, 16
    or      $t4, $t4, $s6
    move    $t7, $t4

.if !\kaligned
	and		$s6, $t1, 2
    bnez    $s6, 42f			# not aligned?
    nop
    and		$s6, $t1, 1
    bnez    $s6, 41f
    nop
.endif

    TileLineSinglecolAl0
    TileLineSinglecolAl0
    TileLineSinglecolAl0
    TileLineSinglecolAl0
    TileLineSinglecolAl0
    TileLineSinglecolAl0
    TileLineSinglecolAl0
    TileLineSinglecolAl0

.if !\kaligned
    b       44f
    nop
41:
    TileLineSinglecolAl1
    TileLineSinglecolAl1
    TileLineSinglecolAl1
    TileLineSinglecolAl1
    TileLineSinglecolAl1
    TileLineSinglecolAl1
    TileLineSinglecolAl1
    TileLineSinglecolAl1
    b       44f
    nop

42:
	and		$s6, $t1, 1
    bnez    $s6, 43f
    nop

    TileLineSinglecolAl2
    TileLineSinglecolAl2
    TileLineSinglecolAl2
    TileLineSinglecolAl2
    TileLineSinglecolAl2
    TileLineSinglecolAl2
    TileLineSinglecolAl2
    TileLineSinglecolAl2
    b       44f
    nop

43:
    TileLineSinglecolAl3
    TileLineSinglecolAl3
    TileLineSinglecolAl3
    TileLineSinglecolAl3
    TileLineSinglecolAl3
    TileLineSinglecolAl3
    TileLineSinglecolAl3
    TileLineSinglecolAl3

44:
.endif
    sub     $t1, $t1, 512*8        # restore pdest pointer
.endm

# DrawLayerTiles(*hcache, *scrpos, (cells<<24)|(nametab<<9)|(vscroll&0x3ff)<<11|(shift(width)<<8)|planeend, (ymask<<24)|(planestart<<16)|(htab||hscroll)

#static void DrawLayerFull(int plane, int *hcache, int planestart, int planeend)

.global DrawLayerFull

DrawLayerFull:
    #stmfd   sp!, {$t4-$a2,$s5}
	addiu   $sp, $sp, -24
	sw      $s6, 20($sp)
	sw      $s7, 16($sp)
	sw      $s2, 12($sp)  #lr
	sw      $s5, 8($sp)   #r12
	sw      $s4, 4($sp)   #r11
	sw      $s3, ($sp)    #r10

    move    $t6, $a1        # hcache
    #move	$t1, $a1
	move	$t0, $a0
	#move	$t1, $a1
	move	$t2, $a2
	move	$t3, $a3

    #lw     $s4, (Pico+0x22228)  # Pico.video
    lui     $s4, %hi(Pico+0x22228)
    li		$s7, %lo(Pico+0x22228)
    add     $s4, $s4, $s7         # $s4=Pico.video
    #lw     $s3, (Pico+0x10000)   # $s3=Pico.vram
    lui     $s3, %hi(Pico+0x10000)
    li		$s7, %lo(Pico+0x10000)
    add     $s3, $s3, $s7         # $s3=Pico.vram
    lb      $t5, 13($s4)          # pvid->reg(13)
    lb      $t7, 11($s4)

    sub     $s2, $t3, $t2
    li		$s7, 0x00ff0000
    and     $s2, $s2, $s7         # $s2=cells

    sll     $t5, $t5, 10          # htab=pvid->reg(13)<<9; (halfwords)
    sll		$s7, $t0, 1
    add     $t5, $t5, $s7         # htab+=plane
    li		$s7, 0x00ff0000
    not		$s7, $s7
    and     $t5, $t5, $s7         # just in case

    #tst    $t7, 3                # full screen scroll? (if 0)
    and		$s7, $t7, 3           # full screen scroll? (if 0)
    lb      $t7, 16($s4)          # ??hh??ww
    bnez	$s7, .rtrloop_skip_1
    nop
    add		$s7, $s3, $t5
    lh      $t5, ($s7)
    li		$s7, 0x0000fc00
    not		$s7, $s7
    and     $t5, $t5, $s7         # $t5=hscroll (0-0x3ff)
    b       .rtrloop_skip_2
    nop

.rtrloop_skip_1:
    srl    $t5, $t5, 1
    or     $t5, $t5, 0x8000       # this marks that we have htab pointer, not hscroll here

.rtrloop_skip_2:
    and     $t8, $t7, 3

	sll		$s7, $t7, (1+24)
    or      $t5, $t5, $s7
    li		$s7, 0x1f000000
    or      $t5, $t5, $s7
    sub		$s7, $t8, 1
    bltz	$s7, .rtrloop_skip_3
    nop
    beqz	$s7, .rtrloop_skip_4
    nop
    b       .rtrloop_skip_5
    nop

.rtrloop_skip_3:
	li		$s7, 0x80000000
    b       .rtrloop_skip_6
    nop

.rtrloop_skip_4:
	li		$s7, 0xc0000000
    b       .rtrloop_skip_6
    nop

.rtrloop_skip_5:
	li		$s7, 0xe0000000

.rtrloop_skip_6:
	not		$s7, $s7
	and		$t5, $t5, $s7

    sll     $t9, $t2, 24
    srl		$s7, $t9, 8
    or      $t5, $t5, $s7          # $t5=(ymask<<24)|(trow<<16)|(htab||hscroll)

    add     $t4, $t8, 5
    sub		$s7, $t4, 7
    bltz	$s7, .rtrloop_skip_7
    nop
    sub     $t4, $t4, 1            # $t4=shift(width) (5,6,6,7)

.rtrloop_skip_7:
    or      $s2, $s2, $t4
    sll		$s7, $t3, 24
    or      $s2, $s2, $s7          # $s2=(planeend<<24)|(cells<<16)|shift(width)

    # calculate xmask:
    sll     $t8, $t8, 24+5
    li		$s7, 0x1f000000
    or      $t8, $t8, $s7

    # Find name table:
    #tst     $a0, $a0
    bnez	$t0, .rtrloop_skip_8
    nop
    lb  	$t4, 2($s4)
    srl     $t4, $t4, 3
    b		.rtrloop_skip_9
    nop

.rtrloop_skip_8:
    lb  	$t4, 4($s4)

.rtrloop_skip_9:
    and     $t4, $t4, 7
    sll		$s7, $t4, 13
    or      $s2, $s2, $s7        # $s2|=nametab_bits{3}<<13

    lw      $s4, PicoDraw2FB     # $s4=PicoDraw2FB
    sub     $t4, $t9, (START_ROW<<24)
    #lw     $a2, ($a2)
    sra     $t4, $t4, 24
    li      $t7, 512*8
    mul		$s7, $t4, $t7
    add		$s4, $s7, $s4		   # scrpos+=8*512*(planestart-START_ROW);
    #mla     $a2, $t4, $t7, $a2      # scrpos+=8*328*(planestart-START_ROW);

    # Get vertical scroll value:
    li		$s7, 0x012000
    add     $t7, $s3, $s7
    add     $t7, $t7,  0x000180    # $t7=Pico.vsram (Pico+0x22180)
    lw      $t7, ($t7)
    bnez	$t0, .rtrloop_skip_10
    nop
	sll		$t7, $t7, 22
    b		.rtrloop_skip_11
    nop

.rtrloop_skip_10:
    sll	    $t7, $t7, 6

.rtrloop_skip_11:
    srl     $t7, $t7, 22          # $t7=vscroll (10 bits)

	sll		$s7, $t7, 3
    or      $s2, $s2, $s7
    rotr    $s2, $s2, 24          # packed: cccccccc nnnvvvvv vvvvvsss pppppppp: cells, nametab, vscroll, shift(width), planeend

    and     $t7, $t7, 7
    beqz	$t7, .rtrloop_skip_12
    nop
    add		$s2, $s2, 1            # we have vertically clipped tiles due to vscroll, so we need 1 more row

.rtrloop_skip_12:
	li		$s7, 8
    sub     $t7, $s7, $t7
    sw      $t7, ($t6)             # push y-offset to tilecache
    add		$t6, $t6, 4
    li	    $t4, 512
    mul		$s7, $t4, $t7
    add     $s4, $s7, $s4          # scrpos+=(8-(vscroll&7))*512;

    li      $t9, 0xff000000        # $t9=(prevcode<<8)|flags: 1~tile empty, 2~tile singlecolor

.rtrloop_outer:
    sll     $t4, $s2, 11
    srl     $t4, $t4, 25           # $t4=vscroll>>3 (7 bits)
    srl		$s7, $t5, 16
    add     $t4, $t4, $s7          # +trow
    srl		$s7, $t5, 24
    and     $t4, $t4, $s7          # &=ymask
    srl     $t7, $s2, 8
    and     $t7, $t7, 7            # shift(width)
    srl     $t0, $s2, 9
    and     $t0, $t0, 0x7000       # nametab
    sll		$s7, $t4, $t7
    add     $s5, $t0, $s7          # nametab_row = nametab + (((trow+(vscroll>>3))&ymask)<<shift(width));

    srl     $t4, $s2, 24
    sll		$s7, $t4, 23
    or      $s5, $s5, $s7
    sll     $s5, $s5, 1             # (nametab_row|(cells<<24)) (halfword compliant)

    # htab?
    and		$s7, $t5, 0x8000
    bnez	$s7, .rtrloop_skip_13
    nop
    sll		$t7, $t5, 22 			# hscroll (0-3FFh)
    srl	    $t7, $t7, 22
    b       .rtr_hscroll_done
    nop

.rtrloop_skip_13:
    # get hscroll from htab
    sll     $t7, $t5, 17
    li		$s7, 0x00ff0000
    and     $t4, $t5, $s7
    sll		$s7, $t4, 5
    add     $t7, $t7, $s7         # +=trow<<4
    beqz	$t4, .rtrloop_skip_14
    nop
    and     $t4, $s2, 0x3800
    sll		$s7, $t4, 7
    sub     $t7, $t7, $s7         # if(trow) htaddr-(vscroll&7)<<1;
.rtrloop_skip_14:
    srl     $t7, $t7, 16          # halfwords
    add		$s7, $s3, $t7
    lh      $t7, ($s7)

.rtr_hscroll_done:
	li		$s7, 0xff000000
    and     $t8, $t8, $s7
    li		$s7, 0
    sub     $t4, $s7, $t7         # $t4=tilex(-ts->hscroll)>>3
    sra     $t4, $t4, 3
    and     $t4, $t4, 0xff
    or      $t8, $t8, $t4         # $t8=(xmask<<24)|tilex

    sub     $t7, $t7, 1
    and     $t7, $t7, 7
    add     $t7, $t7, 1           # $t7=dx((ts->hscroll-1)&7)+1

	li		$s7, 8
    bne		$t7, $s7, .rtrloop_skip_15
    nop
    li		$s7, 0x01000000
    sub     $s5, $s5, $s7          # we will loop cells+1 times, so loop less when there is no hscroll

.rtrloop_skip_15:
    add     $t1, $s4, $t7          # $t1=pdest
    li	    $t0, 0xf
    b       .rtrloop_enter
    nop

    # $t4 & $t7 are scratch in this loop
.rtrloop: # 40-41 times
    add     $t1, $t1, 8
    li		$s7, 0x01000000
    sub     $s5, $s5, $s7
    add     $t8, $t8, 1
    bltz	$s5, .rtrloop_exit
    nop

.rtrloop_enter:
	srl		$s7, $t8, 24
    and     $t7, $t8, $s7
    sll		$s7, $t7, 1
    add     $t7, $s3, $s7
    li		$s7, 0xff000000
    not		$s7, $s7
    and     $t4, $s5, $s7          # Pico.vram(nametab_row+(tilex&xmask));
    add		$s7, $t7, $t4
    lh      $t7, ($s7)             # $t7=code (int, but from unsigned, no sign extend)
    and		$t7, $t7, 0xffff       # trick: only lower bits

    and     $s7, $t7, 0x8000
    bnez    $s7, .rtr_hiprio
    nop

	srl		$s7, $t9, 8
    bne     $t7, $s7, .rtr_notsamecode
    nop
    # we know stuff about this tile already
    and     $s7, $t9, 1
    bnez    $s7, .rtrloop         # empty tile
    nop
    and     $s7, $t9, 2
    bnez    $s7, .rtr_singlecolor # singlecolor tile
    nop
    b       .rtr_samecode
    nop

.rtr_notsamecode:
	li		$s7, 0x600000
    and     $t4, $t9, $s7
    sll     $t9, $t7, 8           # remember new code

    # update cram
    and     $t7, $t7, 0x6000
    sra     $t3, $t7,  9      # $t3=pal=((code&0x6000)>>9);

.rtr_samecode:

	move	$a1, $s3
	li		$s7, 0x100000
    and     $s7, $t9, $s7       # vflip?
    bnez    $s7, .rtr_vflip
    nop

	li		$s7, 0x080000
    and     $s7, $t9, $s7       # hflip?
    bnez    $s7, .rtr_hflip
    nop

    # Tile ($t1=pdest, $t3=pal, $t9=prevcode, $a1=Pico.vram) $t2,$t4,$t7: scratch, $t0=0xf
    Tile 0, 0
    b       .rtrloop
    nop

.rtr_hflip:
    Tile 1, 0
    b       .rtrloop
    nop

.rtr_vflip:
	li		$s7, 0x080000
    and     $s7, $t9, $s7       # hflip?
    bnez    $s7, .rtr_vflip_hflip
    nop

    Tile 0, 1
    b       .rtrloop
    nop

.rtr_vflip_hflip:
    Tile 1, 1
    b       .rtrloop
    nop

.rtr_singlecolor:
    TileSinglecol
    b       .rtrloop
    nop

.rtr_hiprio:
    # *(*hcache)++  code|(dx<<16)|(trow<<27);
    sub     $t4, $t1, $s4
    sll		$s7, $t4, 16
    or      $t7, $t7, $s7
    li		$s7, 0x00ff0000
    and     $t4, $t5, $s7
    sll		$s7, $t4, 11
    or      $t7, $t7, $s7 # (trow<<27)
    sw      $t7, ($t6)    # cache hi priority tile
    add		$t6, $t6, 4
    b       .rtrloop
    nop

.rtrloop_exit:
	li		$s7, 0x00010000
    add     $t5, $t5, $s7
    sll     $t4, $t5, 8
    sll		$s7, $s2, 24
    sub		$s7, $t4, $s7
    bgez    $s7, .rtrloop_outer_exit
    nop
    add     $s4, $s4, 512*8
    b       .rtrloop_outer
    nop

.rtrloop_outer_exit:

    # terminate cache list
    li	    $t0, 0
    sw      $t0, ($t6)    # save cache pointer

	lw      $s3, ($sp)
	lw      $s4, 4($sp)
	lw      $s5, 8($sp)
	lw      $s2, 12($sp)
	lw      $s7, 16($sp)
	lw      $s6, 20($sp)
	addiu   $sp, $sp, 24
    #ldmfd   sp!, {$t4-$a2,$s5}
    jr      $ra
    nop

#.pool

.global DrawTilesFromCacheF # int *hc

DrawTilesFromCacheF:
	addiu   $sp, $sp, -16
	sw      $s4, 12($sp)
	sw      $s7, 8($sp)
	sw      $s6, 4($sp)
	sw      $s5, ($sp)
    #stmfd   sp!, {$t4-$t10,lr}

    move	$t0, $a0
    #move	$t1, $a1
    #move	$t2, $a2
    #move	$t3, $a3

    li      $t9, 0xff000000 # $t9=prevcode=-1
    li      $t6, -1          # $t6=prevy=-1

    lw      $t4, PicoDraw2FB  # $t4=PicoDraw2FB
    lw      $t1, ($t0)    # read y offset
    add		$t0, $t0, 4
    li      $t7, 512
    mul     $s6, $t7, $t1
    add		$t1, $t4, $s6
    sub     $s5, $t1, (512*8*START_ROW) # $t3=scrpos

    lui     $a1, %hi(Pico+0x10000)
    li		$s6, %lo(Pico+0x10000)
    add     $a1, $a1, $s6         # $a1=Pico.vram
    move    $t8, $t0               # hc
    li      $t0, 0xf

    # scratch: $t4, $t7
	# *hcache++ = code|(dx<<16)|(trow<<27); // cache it

.dtfcf_loop:
    lw      $t7, ($t8)    # read code
    add		$t8, $t8, 4
    srl		$t1, $t7, 16   # $t1=dx;
    beqz    $t1, .dtfcf_exit   # dx is never zero, this must be a terminator, return
    nop
    #ldmeqfd sp!, {$t4-$t10,pc} # dx is never zero, this must be a terminator, return

    # row changed?
    srl		$s6, $t7, 27
    beq		$t6, $s6, .dtfcf_skip_1
    nop
    move    $t6, $s6
    li      $t4, 512*8
    mul     $s6, $t4, $t6
    add		$t5, $s5, $s6     # $t5=pd = scrpos + prevy*512*8

.dtfcf_skip_1:
	li		$s6, 0xf800
	not		$s6, $s6
	and		$t1, $t1, $s6
    add     $t1, $t5, $t1      # $t1=pdest (halfwords)

    sll     $t7, $t7, 16
    srl     $t7, $t7, 16

	srl		$s6, $t9, 8
    bne     $t7, $s6, .dtfcf_notsamecode
    nop
    # we know stuff about this tile already
    and		$s6, $t9, 1
    bnez    $s6, .dtfcf_loop         # empty tile
    nop
    and		$s6, $t9, 2
    bnez    $s6, .dtfcf_singlecolor  # singlecolor tile
    nop
    b       .dtfcf_samecode
    nop

.dtfcf_notsamecode:
	li		$s6, 0x600000
    and     $t4, $t9, $s6
    sll     $t9, $t7, 8      # remember new code

    # update cram val
    and     $t7, $t7, 0x6000
    sra     $t3, $t7, 9         # $t3=pal=((code&0x6000)>>9);

.dtfcf_samecode:

	li		$s6, 0x100000
    and	    $s6, $t9, $s6          # vflip?
    bnez    $s6, .dtfcf_vflip

	li		$s6, 0x080000
    and     $s6, $t9, $s6	       # hflip?
    bnez    $s6, .dtfcf_hflip

    # Tile ($t1=pdest, $t3=pal, $t9=prevcode, $a1=Pico.vram) $t2,$t4,$t7: scratch, $t0=0xf
    Tile 0, 0
    b       .dtfcf_loop

.dtfcf_hflip:
    Tile 1, 0
    b       .dtfcf_loop

.dtfcf_vflip:
	li		$s6, 0x080000
    and     $s6, $t9, $s6	       # hflip?
    bnez    $s6, .dtfcf_vflip_hflip

    Tile 0, 1
    b       .dtfcf_loop

.dtfcf_vflip_hflip:
    Tile 1, 1
    b       .dtfcf_loop

.dtfcf_singlecolor:
    TileSinglecol
    b       .dtfcf_loop

.dtfcf_exit:
	lw      $s5, ($sp)
	lw      $s6, 4($sp)
	lw      $s7, 8($sp)
	lw      $s4, 12($sp)
	addiu   $sp, $sp, 16

	jr		$ra
	nop

#.pool


# ###############

# (tile_start<<16)|row_start
.global DrawWindowFull # int tstart, int tend, int prio

DrawWindowFull:
    #stmfd   sp!, {$t4-$s4,lr}
	addiu   $sp, $sp, -20
	sw      $s7, 16($sp)
	sw      $s2, 12($sp)  #lr
	sw      $s5, 8($sp)   #r12
	sw      $s4, 4($sp)   #r11
	sw      $s3, ($sp)    #r10

	move	$t0, $a0
	move	$t1, $a1
	move	$t2, $a2
	move	$t3, $a3

    lui     $s4, %hi(Pico+0x22228)
    li		$s7, %lo(Pico+0x22228)
    add     $s4, $s4, $s7         # $s4=Pico.video
    lb 		$s5, 3($s4)        # pvid->reg(3)
    sll     $s5, $s5, 10

    lw      $t4, 12($s4)
    li      $t5, 1                # nametab_step
    and     $s7, $t4, 1           # 40 cell mode?
    bnez	$s7, .dwfskip_1
    nop
    and     $s5, $s5, 0xf800
    sll     $t5, $t5, 6        # nametab_step
    b		.dwfskip_2
    nop

.dwfskip_1:
	and     $s5, $s5, 0xf000     # 0x3c<<10
	sll     $t5, $t5, 7

.dwfskip_2:
    and     $t4, $t0, 0xff
    mul		$s7, $t5, $t4
    add     $s5, $s7, $s5      # nametab += nametab_step*start;

    srl     $t4, $t0, 16       # $t4=start_cell_h
    sll		$s7, $t4, 1
    add     $t7, $s5, $s7

    # fetch the first code now
    #ldr     $s3, =(Pico+0x10000)  # lr=Pico.vram
    lui     $s3, %hi(Pico+0x10000)
    li		$s7, %lo(Pico+0x10000)
    add     $s3, $s3, $s7         # $s3=Pico.vram
    add		$s7, $s3, $t7
    lh      $t7, ($s7)
    srl		$s7, $t7, 15
    bne		$t2, $s7, .dwfexit
    nop

	srl		$s7, $t1, 16
    sub     $t8, $s7, $t4         # cells (h)
    sll		$s7, $t4, 8
    or      $t8, $t8, $s7
    sll     $t4, $t1, 24
    sll		$s7, $t0, 24
    sub     $t4, $t4, $s7
    srl		$s7, $t4, 8
    or      $t8, $t8, $s7          # $t8=cells_h|(start_cell_h<<8)|(cells_v<<16)
    li		$s7, 0x010000
    sub     $t8, $t8, $s7     # adjust for algo

    li      $t9, 0xff000000       # $t9=prevcode=-1

    lw      $s4, PicoDraw2FB     # $s4=scrpos
    and     $t4, $t0, 0xff
    #ldr     $s4, ($s4)
    sub     $t4, $t4, START_ROW
    add     $s4, $s4, 512*8
    add     $s4, $s4, 8

    li      $t7, 512*8
    mul		$s7, $t7, $t4
    add     $s4, $s7, $s4      # scrpos+=8*512*(start-START_ROW);
    li      $t0, 0xf

.dwfloop_outer:
    and     $t6, $t8, 0xff00       # $t6=tilex
    srl		$s7, $t6, 5
    add     $t1, $s4, $s7          # $t1=pdest
    srl		$s7, $t6, 7
    add     $t6, $s5, $s7
    add     $t6, $s3, $t6          # $t6=Pico.vram+nametab+tilex
    sll		$s7, $t8, 24
    or      $t8, $t8, $s7
    li		$s7, 0x01000000
    sub     $t8, $t8, $s7   # cell loop counter
    b       .dwfloop_enter
    nop

    # $t4 & $t7 are scratch in this loop
.dwfloop:
    add     $t1, $t1, 8
    li		$s7, 0x01000000
    sub     $t8, $t8, $s7
    bltz    $t8, .dwfloop_exit
    nop

.dwfloop_enter:
    lh      $t7, ($t6)      # $t7=code
    add		$t6, $t6, 2

	srl		$t7, $t9, 8
    bne     $t7, $t9, .dwf_notsamecode
    nop
    # we know stuff about this tile already
    and     $s7, $t9, 1
    bnez    $s7, .dwfloop         # empty tile
    nop
    and     $s7, $t9, 2
    bnez    $s7, .dwf_singlecolor # singlecolor tile
    nop
    b       .dwf_samecode
    nop

.dwf_notsamecode:
	li		$s7, 0x600000
    and     $t4, $t9, $s7
    sll     $t9, $t7, 8      # remember new code

    # update cram val
    and     $t7, $t7, 0x6000
    sra     $t3, $t7, 9      # $t3=pal=((code&0x6000)>>9);

.dwf_samecode:

	li		$s7, 0x100000
    and     $s7, $t9, $s7       # vflip?
    bnez    $s7, .dwf_vflip
    nop

    li		$s7, 0x080000
    and     $s7, $t9, $s7       # hflip?
    bnez    $s7, .dwf_hflip
    nop

    # Tile ($t1=pdest, $t3=pal, $t9=prevcode, $s3=Pico.vram) $t2,$t4,$t7: scratch, $t0=0xf
    Tile 0, 0
    b       .dwfloop
    nop

.dwf_hflip:
    Tile 1, 0
    b       .dwfloop
    nop

.dwf_vflip:
    li		$s7, 0x080000
    and     $s7, $t9, $s7       # hflip?
    bnez    $s7, .dwf_vflip_hflip
    nop

    Tile 0, 1
    b       .dwfloop
    nop

.dwf_vflip_hflip:
    Tile 1, 1
    b       .dwfloop
    nop

.dwf_singlecolor:
    TileSinglecol 1
    b       .dwfloop
    nop

.dwfloop_exit:
	li		$s7, 0xff000000
	not		$s7, $s7
    and     $t8, $t8, $s7  # fix $t8
    li		$s7, 0x010000
    sub     $t8, $t8, $s7
    bltz	$t8, .dwfexit
    nop
    add     $s4, $s4, 328*8
    add     $s5, $s5, $t5         # nametab+=nametab_step
    b       .dwfloop_outer
    nop

.dwfexit:
	lw      $s3, ($sp)
	lw      $s4, 4($sp)
	lw      $s5, 8($sp)
	lw      $s2, 12($sp)
	lw      $s7, 16($sp)
	addiu   $sp, $sp, 20
    #ldmfd   sp!, {$t4-$a2,$s5}
    jr      $ra
    nop
	#ldmnefd sp!, {$t4-$s4,pc}      # hack: simply assume that whole window uses same priority

#.pool


# ---------------- sprites ---------------

.macro SpriteLoop hflip vflip
.if \vflip
	srl		$t1, $t5, 24			# height
    li      $t0, 512*8
    mul     $s6, $t1, $t0
    add		$a2, $a2, $s6			# scrpos+=height*512*8;
    sll		$s6, $t1, 3
    add     $a3, $a3, $s6           # sy+=height*8
.endif
    li      $t0,  0xf
.if \hflip
    and     $t1, $t5, 0xff
    sll		$s6, $t1, 3
    add     $t8, $t8, $s6           # sx+=width*8
58:
	sub		$s6, $t8, (8+512+8)
    bltz    $s6, 51f
    nop
    srl		$s6, $t5, 16
    add     $t9, $t9, $s6
    sub     $t5, $t5, 1             # sub width
    sub     $t8, $t8, 8
    b       58b
    nop
.else
    bgtz    $t8, 51f             # skip tiles hidden on the left of screen
    nop
58:
	srl		$s6, $t5, 16
    add     $t9, $t9, $s6
    sub     $t5, $t5, 1
    add     $t8, $t8, 8
    blez    $t8, 58b
    nop
    b       51f
    nop
.endif

50: # outer
.if !\hflip
    add     $t8, $t8, 8          # sx+=8
.endif
	li		$s6, 0xff000000
	not		$s6, $s6
	and		$t5, $t5, $s6        # fix height
    sll		$s6, $t5, 16
    or      $t5, $t5, $s6

51: # outer_enter
    sub     $t5, $t5, 1          # width--
    sll		$t1, $t5, 24
    bltz	$t1, 54f			 # end of tile
    nop
    #ldmmifd sp!, {$t0-$t9,pc}    # end of tile
.if \hflip
	move	$s6, $t8
    sub     $t8, $t8, 8          # sx-=8
    sub		$s6, $s6, $t8
    blez	$s6, 54f       # tile offscreen
    nop
    #ldmlefd sp!, {$t0-$t9,pc}    # tile offscreen
.else
    #cmp     $t4, 512
    sub		$s6, $t8, 328
    bgez	$s6, 54f              # tile offscreen
    nop
    #ldmgefd sp!, {$t0-$t9,pc}    # tile offscreen
.endif
    move    $t6, $a3            # $t2=sy
    add     $t1, $a2, $t8         # pdest=scrpos+sx
    b       53f
    nop

52: # inner
    add     $t9, $t9, (1<<8)       # tile++
.if !\vflip
    add     $t6, $t6, 8          # sy+=8
    add     $t1, $t1, 512*8
.endif

53: # inner_enter
    # end of sprite?
    li		$s6, 0x01000000
    sub     $t5, $t5, $s6
    bltz    $t5, 50b                 # ->outer
    nop
.if \vflip
    sub     $t6, $t6, 8          # sy-=8
    sub     $t1, $t1, 512*8
.endif

    # offscreen?
    #cmp     $t2, (START_ROW*8)
    ble     $t6, (START_ROW*8), 52b
    nop

    #cmp     $t2, (END_ROW*8+8)
    sub		$s6, $t6, (END_ROW*8+8)
    bgez     $s6, 52b
    nop

    # Tile ($a1=pdest, $a3=pal, $t5=prevcode, $t3=Pico.vram) $a2,$t0,$t3: scratch, $a0=0xf
    Tile \hflip, \vflip
    nop
    b       52b
    nop

54:
	lw      $s5, ($sp)
	lw      $s6, 4($sp)
	lw      $s7, 8($sp)
	addiu   $sp, $sp, 12

	jr		$ra
	nop

.endm



.global DrawSpriteFull # unsigned int *sprite

DrawSpriteFull:
    #stmfd   sp!, {$t0-$t9,lr}

	addiu   $sp, $sp, -12
	sw      $s7, 8($sp)
	sw      $s6, 4($sp)
	sw      $s5, ($sp)

    lw      $t3, ($a0)        # sprite[0]
    sll		$t5, $t3, 4
    srl		$t6, $t5, 30
    add     $t6, $t6, 1       # $t6=width
    sll		$t5, $t5, 2
    srl		$t5, $t5, 30
    add     $t5, $t5, 1       # $t5=height

    sll     $a3, $t3, 23
    srl     $a3, $a3, 23

    lw      $s5, 4($a0)       # $s5=code
    sub     $a3, $a3, 0x78    # $a3=sy
    sll     $t8, $s5, 7
    srl     $t8, $t8, 23
    sub     $t8, $t8, 0x78    # $t8=sx

    sll     $t9, $s5, 21
    srl     $t9, $t9, 13      # $t9=tile<<8

    and     $t3, $s5, 0x6000
    srl     $t3, $t3, 9       # $t3=pal=((code>>9)&0x30);

	lw		$a2, (PicoDraw2FB)     # $a2=scrpos
    #lui     $a2, %hi(PicoDraw2FB)
    #li		$s6, %lo(PicoDraw2FB)
    #add     $a2, $a2, $s6         # $a2=scrpos
    lui     $a1, %hi(Pico+0x10000)
    li		$s6, %lo(Pico+0x10000)
    add     $a1, $a1, $s6         # $a1=Pico.vram
    #lw      $a2, ($a2)
    sub     $t1, $a3, (START_ROW*8)
    li      $t0, 512
    mul     $s6, $t1, $t0
    add		$a2, $a2, $s6		# scrpos+=(sy-START_ROW*8)*512;

	sll     $s6, $t5, 16
    or      $t5, $t5, $s6
    sll     $s6, $t5, 8
    or      $t5, $t6, $s6        # $t5=width|(height<<8)|(height<<24)

    #tst    $ra, #0x1000
    #li		$s6, 0x1000
    #bne    $s5, $s6,  .dsf_vflip   # vflip?
    and		$s6, $s5, 0x1000
    bnez    $s6, .dsf_vflip   # vflip?
    nop

    #tst     $ra, #0x0800         # hflip?
    #li		$s6, 0x0800
    #bne     $s5, $s6,  .dsf_hflip   # hflip?
    and		$s6, $s5, 0x0800
    bnez    $s6, .dsf_hflip   # hflip?
    nop

    SpriteLoop 0, 0
    nop

.dsf_hflip:
    SpriteLoop 1, 0
    nop

.dsf_vflip:
    #tst     $ra, #0x0800
    #li		$s6, 0x0800
    #bne     $t8, $s6,  .dsf_vflip_hflip   # hflip?
    and		$s6, $s5, 0x0800
    bnez    $s6, .dsf_vflip_hflip   # hflip?
    nop

    SpriteLoop 0, 1
    nop

.dsf_vflip_hflip:
    SpriteLoop 1, 1
    nop

#.pool

# vim:filetype=armasm

