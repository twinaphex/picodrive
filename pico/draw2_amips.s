/*
 * assembly optimized versions of most funtions from draw2.c
 * Robson Santana, 2016
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

.global BackFillFull_asm # int reg7

BackFillFull_asm:
	#addiu  $sp, $sp, -4     # allocate 1 word on the stack
    #sw     $sp, ($ra)

	move    $t0, $a0
    lw      $a0, PicoDraw2FB      # lr=PicoDraw2FB
    sll		$t0, $t0, 26
    #lw     $a0, ($a0)
    srl     $t0, $t0, 26
    add     $a0, $a0, (512*8)+8      # 512: line_width in psp, others -> 328

	sll		$t7, $t0, 8
    or      $t0, $t0, $t7
    sll		$t7, $t0, 16
    or      $t0, $t0, $t7

    move    $t1, $t0 # 25 opcodes wasted?
    move    $t2, $t0
    move    $t3, $t0
    move    $t4, $t0
    move    $t5, $t0
    move    $t6, $t0
    move    $t7, $t0
    move    $t8, $t0
    move    $t9, $t0

    li      $a1, (END_ROW-START_ROW)*8

    # go go go!
.bff_loop:
    #add    $ra, $ra, 8
    sub     $a1, $a1, 1   #arm subs

    sw		$t0, ($a0)       # 10*4*8
    sw		$t1, 4($a0)
    sw		$t2, 8($a0)
    sw		$t3, 12($a0)
    sw		$t4, 16($a0)
    sw		$t5, 20($a0)
    sw		$t6, 24($a0)
    sw		$t7, 28($a0)
    sw		$t8, 32($a0)
    sw		$t9, 36($a0)
    addiu   $a0, $a0, 40

    sw		$t0, ($a0)
    sw		$t1, 4($a0)
    sw		$t2, 8($a0)
    sw		$t3, 12($a0)
    sw		$t4, 16($a0)
    sw		$t5, 20($a0)
    sw		$t6, 24($a0)
    sw		$t7, 28($a0)
    sw		$t8, 32($a0)
    sw		$t9, 36($a0)
    addiu   $a0, $a0, 40

    sw		$t0, ($a0)
    sw		$t1, 4($a0)
    sw		$t2, 8($a0)
    sw		$t3, 12($a0)
    sw		$t4, 16($a0)
    sw		$t5, 20($a0)
    sw		$t6, 24($a0)
    sw		$t7, 28($a0)
    sw		$t8, 32($a0)
    sw		$t9, 36($a0)
    addiu   $a0, $a0, 40

    sw		$t0, ($a0)
    sw		$t1, 4($a0)
    sw		$t2, 8($a0)
    sw		$t3, 12($a0)
    sw		$t4, 16($a0)
    sw		$t5, 20($a0)
    sw		$t6, 24($a0)
    sw		$t7, 28($a0)
    sw		$t8, 32($a0)
    sw		$t9, 36($a0)
    addiu   $a0, $a0, 40

    sw		$t0, ($a0)
    sw		$t1, 4($a0)
    sw		$t2, 8($a0)
    sw		$t3, 12($a0)
    sw		$t4, 16($a0)
    sw		$t5, 20($a0)
    sw		$t6, 24($a0)
    sw		$t7, 28($a0)
    sw		$t8, 32($a0)
    sw		$t9, 36($a0)
    addiu   $a0, $a0, 40

    sw		$t0, ($a0)
    sw		$t1, 4($a0)
    sw		$t2, 8($a0)
    sw		$t3, 12($a0)
    sw		$t4, 16($a0)
    sw		$t5, 20($a0)
    sw		$t6, 24($a0)
    sw		$t7, 28($a0)
    sw		$t8, 32($a0)
    sw		$t9, 36($a0)
    addiu   $a0, $a0, 40

    sw		$t0, ($a0)
    sw		$t1, 4($a0)
    sw		$t2, 8($a0)
    sw		$t3, 12($a0)
    sw		$t4, 16($a0)
    sw		$t5, 20($a0)
    sw		$t6, 24($a0)
    sw		$t7, 28($a0)
    sw		$t8, 32($a0)
    sw		$t9, 36($a0)
    addiu   $a0, $a0, 40

    sw		$t0, ($a0)
    sw		$t1, 4($a0)
    sw		$t2, 8($a0)
    sw		$t3, 12($a0)
    sw		$t4, 16($a0)
    sw		$t5, 20($a0)
    sw		$t6, 24($a0)
    sw		$t7, 28($a0)
    sw		$t8, 32($a0)
    sw		$t9, 36($a0)
    addiu   $a0, $a0, 40

    addiu   $a0, $a0, (512-320) # psp screen offset

    bnez    $a1, .bff_loop
    nop

    #ldmfd  sp!, {$t4-$t9,$a1}
    jr      $ra
    nop

#.pool

# -------- some macros --------

# helper
# TileLineSinglecol ($a1=pdest, $a2=pixels8, $a3=pal) $t4: scratch, $a0: pixels8_old
.macro TileLineSinglecol notsinglecol=0
    and     $a2, $a2, 0xf        # #0x0000000f
.if !\notsinglecol
	srl		$s5, $a0, 28
	beq		$a2, $s5, 11f    # if these don't match,
	nop
	li		$s5, 2
	not		$s5, $s5
	and		$t9, $t9, $s5        # it is a sign that whole tile is not singlecolor (only it's lines may be)
11:
.endif
    or     	$t4, $a3, $a2
    sll	   	$s5, $t4, 8
    or     	$t4, $t4, $s5

	and		$s5, $a1, 0x1
    bnez	$s5, 12f       # not aligned?
    nop
    sh  	$t4, ($a1)
    sh  	$t4, 2($a1)
    sh  	$t4, 4($a1)
    sh  	$t4, 6($a1)
    b		13f
    nop
12:
    sb  	$t4, ($a1)
    sh  	$t4, 1($a1)
    sh  	$t4, 3($a1)
    sh  	$t4, 5($a1)
    sb  	$t4, 7($a1)
13:
.if !\notsinglecol
    li     	$a0, 0xf
    sll		$s5, $a2, 28
    or     	$a0, $a0, $s5	 # we will need the old palindex later
.endif
.endm

# TileNorm ($a1=pdest, $a2=pixels8, $a3=pal) $a0,$t4: scratch
.macro TileLineNorm
	srl		$s5, $a2, 12		   # #0x0000f000
    and     $t4, $a0, $s5
    beqz	$t4, 21f
    or      $t4, $a3, $t4
    sb      $t4, ($a1)
21:
	srl		$s5, $a2, 8			   # #0x00000f00
    and     $t4, $a0, $s5
    beqz	$t4, 22f
    or      $t4, $a3, $t4
    sb      $t4, 1($a1)
22:
	srl		$s5, $a2, 4			   # #0x000000f0
    and     $t4, $a0, $s5
    beqz	$t4, 23f
    or      $t4, $a3, $t4
    sb      $t4, 2($a1)
23:
    and     $t4, $a0, $a2		   # #0x0000000f
    beqz	$t4, 24f
    or      $t4, $a3, $t4
    sb      $t4, 3($a1)
24:
	srl		$s5, $a2, 28		   # #0xf0000000
    and     $t4, $a0, $s5
    beqz	$t4, 25f
    or      $t4, $a3, $t4
    sb      $t4, 4($a1)
25:
	srl		$s5, $a2, 24		   # #0x0f000000
    and     $t4, $a0, $s5
    beqz	$t4, 26f
    or      $t4, $a3, $t4
    sb      $t4, 5($a1)
26:
	srl		$s5, $a2, 20		   # #0x00f00000
    and     $t4, $a0, $s5
    beqz	$t4, 27f
    or      $t4, $a3, $t4
    sb      $t4, 6($a1)
27:
	srl		$s5, $a2, 16		   # #0x000f0000
    and     $t4, $a0, $s5
    beqz	$t4, 28f
    or      $t4, $a3, $t4
    sb      $t4, 7($a1)
28:
.endm

# TileFlip ($a1=pdest, $a2=pixels8, $a3=pal) $a0,$t4: scratch
.macro TileLineFlip
	srl		$s5, $a2, 16		   # #0x000f0000
    and     $t4, $a0, $s5
    beqz	$t4, 31f
    or      $t4, $a3, $t4
    sb      $t4, ($a1)
31:
	srl		$s5, $a2, 20		   # #0x00f00000
    and     $t4, $a0, $s5
    beqz	$t4, 32f
    or      $t4, $a3, $t4
    sb      $t4, 1($a1)
32:
	srl		$s5, $a2, 24		   # #0x0f000000
    and     $t4, $a0, $s5
    beqz	$t4, 33f
    or      $t4, $a3, $t4
    sb      $t4, 2($a1)
33:
    srl		$s5, $a2, 28		   # #0xf0000000
    and     $t4, $a0, $s5
    beqz	$t4, 34f
    or      $t4, $a3, $t4
    sb      $t4, 3($a1)
34:
    and     $t4, $a0, $a2		   # #0x0000000f
    beqz	$t4, 35f
    or      $t4, $a3, $t4
    sb      $t4, 4($a1)
35:
	srl		$s5, $a2, 4			   # #0x000000f0
    and     $t4, $a0, $s5
    beqz	$t4, 36f
    or      $t4, $a3, $t4
    sb      $t4, 5($a1)
36:
	srl		$s5, $a2, 8			   # #0x00000f00
    and     $t4, $a0, $s5
    beqz	$t4, 37f
    or      $t4, $a3, $t4
    sb      $t4, 6($a1)
37:
	srl		$s5, $a2, 12		   # #0x0000f000
    and     $t4, $a0, $s5
    beqz	$t4, 38f
    or      $t4, $a3, $t4
    sb      $t4, 7($a1)
38:
.endm

# Tile ($a1=pdest, $a3=pal, $t9=prevcode, $s1=Pico.vram) $a2,$t4,$t7: scratch, $a0=0xf
.macro Tile hflip vflip
	li		$s6, (1<<24)
    sll     $t7, $t9, 13           # $t9=code<<8; addr=(code&0x7ff)<<4;
    srl		$s5, $t7, 16
    add     $t7, $s1, $s5
    ori     $t9, $t9, 3            # emptytile=singlecolor=1, $t9 must be <code_16> 00000xxx
.if \vflip
    # we read tilecodes in reverse order if we have vflip
    addi    $t7, $t7, 8*4
.endif
    # loop through 8 lines
    li		$s5, (7<<24)
    or      $t9, $t9, $s5
    b       1f # loop_enter
    nop

0:  # singlecol_loop
	#li		$s5, (1<<24)
    sub     $t9, $t9, $s6
    add     $a1, $a1, $t2         # set pointer to next line
    bltz    $t9, 8f               # loop_exit with $a0 restore
    nop
1:
.if \vflip
	add 	$t7, $t7, $t0
    lw      $a2, ($t7)            # pack=*(unsigned int *)(Pico.vram+addr); // Get 8 pixels
.else
    lw	    $a2, ($t7)
    add 	$t7, $t7, $t1
.endif
    beqz    $a2, 2f                    # empty line
    nop
	#li		$s5, 1
	#not	$s5, $s5
	li		$s5, -2               # optimized
	and		$t9, $t9, $s5         # fix height
    rotr	$s5, $a2, 4
    bne     $s5, $a2, 3f                    # not singlecolor
    nop
    TileLineSinglecol
    b       0b
    nop

2:
	#li		$s5, 2
	#not	$s5, $s5
	li		$s5, -3				# optimized
	and		$t9, $t9, $s5
2:  # empty_loop
	#li		$s5, (1<<24)
    sub     $t9, $t9, $s6
    add     $a1, $a1, $t2          # set pointer to next line
    bltz	$t9, 8f # loop_exit with $a0 restore
    nop
.if \vflip
	add		$t7, $t7, $t0
    lw      $a2, ($t7)  			# next pack
.else
    lw      $a2, ($t7)
    add		$t7, $t7, $t1
.endif
    li      $a0, 0xf               # singlecol_loop might have messed $a0
    beqz    $a2, 2b
    nop

	#li		$s5, 3
	#not	$s5, $s5
	#li		$s5, -4					# optimized
	and		$t9, $t9, $t0           # if we are here, it means we have empty and not empty line
    b       5f
    nop

3:  # not empty, not singlecol
    li      $a0, 0xf
	#li		$s5, 3
	#not	$s5, $s5
	#li		$s5, -4					#optimized
	and		$t9, $t9, $t0
    b       6f
    nop

4:  # not empty, not singlecol loop
	#li		$s5, (1<<24)
    sub     $t9, $t9, $s6
    add     $a1, $a1, $t2          # set pointer to next line
    bltz    $t9, 9f # loop_exit
    nop
.if \vflip
	add		$t7, $t7, $t0
    lw      $a2, ($t7)  			# next pack
.else
    lw      $a2, ($t7)
    add		$t7, $t7, $t1
.endif
    beqz    $a2, 4b                 # empty line
    nop
5:
    rotr	$s5, $a2, 4
    beq     $s5, $a2, 7f			# singlecolor line
    nop
6:
.if \hflip
    TileLineFlip
.else
    TileLineNorm
.endif
    b       4b
    nop
7:
    TileLineSinglecol 1
    b       4b
    nop

8:
    li      $a0, 0xf
9:  # loop_exit
	#li		$s5, (1<<24)
    add     $t9, $t9, $s6          # fix $t9
    sub     $a1, $a1, 512*8        # restore pdest pointer
.endm

# TileLineSinglecolAl ($a1=pdest, $t4,$t7=color)
.macro TileLineSinglecolAl0
    sh		$t4, ($a1)
    sh		$t5, 2($a1)
    sh		$t6, 4($a1)
    sh		$t7, 6($a1)
    addi    $a1, $a1, (8+512)
.endm

.macro TileLineSinglecolAl1
    sb      $a0, ($a1)
    sh      $a0, 1($a1)
    sw      $a0, 3($a1)
    sb      $a0, 7($a1)
    addi    $a1, $a1, (8+512)
.endm

.macro TileLineSinglecolAl2
    sh      $a0, ($a1)
    sw      $a0, 2($a1)
    sh      $a0, 6($a1)
    addi    $a1, $a1, (8+512)
.endm

.macro TileLineSinglecolAl3
    sb      $a0, ($a1)
    sw      $a0, 1($a1)
    sh      $a0, 5($a1)
    sb      $a0, 7($a1)
    addi    $a1, $a1, (8+512)
.endm

# TileSinglecol ($a1=pdest, $a2=pixels8, $a3=pal) $t4,$t7: scratch, $a0=0xf
# kaligned==1, if dest is always aligned
.macro TileSinglecol kaligned=0
    and     $t4, $a2, 0xf       # we assume we have good $a2 from previous time
    or      $t4, $t4, $a3
    sll		$s5, $t4, 8
    or      $t4, $t4, $s5
    sll		$s5, $t4, 16
    or      $t4, $t4, $s5
    move    $t7, $t4

.if !\kaligned
	and		$s5, $a1, 2
    bnez    $s5, 42f			# not aligned?
    nop
    and		$s5, $a1, 1
    bnez    $s5, 41f
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
	and		$s5, $a1, 1
    bnez    $s5, 43f
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
    sub     $a1, $a1, 512*8        # restore pdest pointer
.endm

# DrawLayerTiles(*hcache, *scrpos, (cells<<24)|(nametab<<9)|(vscroll&0x3ff)<<11|(shift(width)<<8)|planeend, (ymask<<24)|(planestart<<16)|(htab||hscroll)

#static void DrawLayerFull(int plane, int *hcache, int planestart, int planeend)

.global DrawLayerFull_asm

DrawLayerFull_asm:
	addiu   $sp, $sp, -24
	sw      $s6, 20($sp)
	sw      $s5, 16($sp)
	sw      $s4, 12($sp)  #lr
	sw      $s3, 8($sp)   #r12
	sw      $s2, 4($sp)   #r11
	sw      $s1, ($sp)    #r10

	li		$t0, -4
	li		$t1, 4
	li		$t2, 512
	li		$t3, 1

    move    $t6, $a1        # hcache

    lui     $s2, %hi(Pico+0x22228)
    li		$s5, %lo(Pico+0x22228)
    add     $s2, $s2, $s5         # $s2=Pico.video
    lui     $s1, %hi(Pico+0x10000)
    li		$s5, %lo(Pico+0x10000)
    add     $s1, $s1, $s5         # $s1=Pico.vram
    lbu      $t5, 13($s2)          # pvid->reg(13)
    lbu      $t7, 11($s2)

    sub     $s4, $a3, $a2
    li		$s5, 0x00ff0000
    and     $s4, $s4, $s5         # $s4=cells

    sll     $t5, $t5, 10          # htab=pvid->reg(13)<<9; (halfwords)
    sll		$s5, $a0, 1
    add     $t5, $t5, $s5         # htab+=plane
    li		$s5, 0x00ff0000
    not		$s5, $s5
    and     $t5, $t5, $s5         # just in case

    and		$s5, $t7, 3           # full screen scroll? (if 0)
    lbu      $t7, 16($s2)          # ??hh??ww
    bnez	$s5, .rtrloop_skip_1
    nop
    add		$s5, $s1, $t5
    lhu      $t5, ($s5)
    li		$s5, 0x0000fc00
    not		$s5, $s5
    and     $t5, $t5, $s5         # $t5=hscroll (0-0x3ff)
    b       .rtrloop_skip_2
    nop

.rtrloop_skip_1:
    srl    $t5, $t5, 1
    or     $t5, $t5, 0x8000       # this marks that we have htab pointer, not hscroll here

.rtrloop_skip_2:
    and     $t8, $t7, 3

	sll		$s5, $t7, (1+24)
    or      $t5, $t5, $s5
    li		$s5, 0x1f000000
    or      $t5, $t5, $s5
    sub		$s5, $t8, $t3
    bltz	$s5, .rtrloop_skip_3
    nop
    beqz	$s5, .rtrloop_skip_4
    nop
    b       .rtrloop_skip_5
    nop

.rtrloop_skip_3:
	li		$s5, 0x80000000
    b       .rtrloop_skip_6
    nop

.rtrloop_skip_4:
	li		$s5, 0xc0000000
    b       .rtrloop_skip_6
    nop

.rtrloop_skip_5:
	li		$s5, 0xe0000000

.rtrloop_skip_6:
	not		$s5, $s5
	and		$t5, $t5, $s5

    sll     $t9, $a2, 24
    srl		$s5, $t9, 8
    or      $t5, $t5, $s5          # $t5=(ymask<<24)|(trow<<16)|(htab||hscroll)

    add     $t4, $t8, 5
    sub		$s5, $t4, 7
    bltz	$s5, .rtrloop_skip_7
    nop
    sub     $t4, $t4, $t3            # $t4=shift(width) (5,6,6,7)

.rtrloop_skip_7:
    or      $s4, $s4, $t4
    sll		$s5, $a3, 24
    or      $s4, $s4, $s5          # $s4=(planeend<<24)|(cells<<16)|shift(width)

    # calculate xmask:
    sll     $t8, $t8, 24+5
    li		$s5, 0x1f000000
    or      $t8, $t8, $s5

    # Find name table:
    bnez	$a0, .rtrloop_skip_8
    nop
    lbu  	$t4, 2($s2)
    srl     $t4, $t4, 3
    b		.rtrloop_skip_9
    nop

.rtrloop_skip_8:
    lbu  	$t4, 4($s2)

.rtrloop_skip_9:
    and     $t4, $t4, 7
    sll		$s5, $t4, 13
    or      $s4, $s4, $s5        # $s4|=nametab_bits{3}<<13

    lw      $s2, PicoDraw2FB     # $s2=PicoDraw2FB
    sub     $t4, $t9, (START_ROW<<24)
    sra     $t4, $t4, 24
    li      $t7, 512*8
    mul		$s5, $t4, $t7
    add		$s2, $s5, $s2		   # scrpos+=8*512*(planestart-START_ROW);

    # Get vertical scroll value:
    li		$s5, 0x012000
    add     $t7, $s1, $s5
    add     $t7, $t7,  0x000180    # $t7=Pico.vsram (Pico+0x22180)
    lw      $t7, ($t7)
    bnez	$a0, .rtrloop_skip_10
    nop
	sll		$t7, $t7, 22
    b		.rtrloop_skip_11
    nop

.rtrloop_skip_10:
    sll	    $t7, $t7, 6

.rtrloop_skip_11:
    srl     $t7, $t7, 22          # $t7=vscroll (10 bits)

	sll		$s5, $t7, 3
    or      $s4, $s4, $s5
    rotr    $s4, $s4, 24          # packed: cccccccc nnnvvvvv vvvvvsss pppppppp: cells, nametab, vscroll, shift(width), planeend

    and     $t7, $t7, 7
    beqz	$t7, .rtrloop_skip_12
    nop
    add		$s4, $s4, $t3            # we have vertically clipped tiles due to vscroll, so we need 1 more row

.rtrloop_skip_12:
	li		$s5, 8
    sub     $t7, $s5, $t7
    sw      $t7, ($t6)             # push y-offset to tilecache
    add		$t6, $t6, $t1
    #move    $t4, $t2
    mul		$s5, $t2, $t7
    add     $s2, $s5, $s2          # scrpos+=(8-(vscroll&7))*512;

    li      $t9, 0xff000000        # $t9=(prevcode<<8)|flags: 1~tile empty, 2~tile singlecolor

.rtrloop_outer:
    sll     $t4, $s4, 11
    srl     $t4, $t4, 25           # $t4=vscroll>>3 (7 bits)
    srl		$s5, $t5, 16
    add     $t4, $t4, $s5          # +trow
    srl		$s5, $t5, 24
    and     $t4, $t4, $s5          # &=ymask
    srl     $t7, $s4, 8
    and     $t7, $t7, 7            # shift(width)
    srl     $a0, $s4, 9
    and     $a0, $a0, 0x7000       # nametab
    sll		$s5, $t4, $t7
    add     $s3, $a0, $s5          # nametab_row = nametab + (((trow+(vscroll>>3))&ymask)<<shift(width));

    srl     $t4, $s4, 24
    sll		$s5, $t4, 23
    or      $s3, $s3, $s5
    sll     $s3, $s3, 1             # (nametab_row|(cells<<24)) (halfword compliant)

    # htab?
    and		$s5, $t5, 0x8000
    bnez	$s5, .rtrloop_skip_13
    nop
    sll		$t7, $t5, 22 			# hscroll (0-3FFh)
    srl	    $t7, $t7, 22
    b       .rtr_hscroll_done
    nop

.rtrloop_skip_13:
    # get hscroll from htab
    sll     $t7, $t5, 17
    li		$s5, 0x00ff0000
    and     $t4, $t5, $s5
    sll		$s5, $t4, 5
    add     $t7, $t7, $s5         # +=trow<<4
    beqz	$t4, .rtrloop_skip_14
    nop
    and     $t4, $s4, 0x3800
    sll		$s5, $t4, 7
    sub     $t7, $t7, $s5         # if(trow) htaddr-(vscroll&7)<<1;
.rtrloop_skip_14:
    srl     $t7, $t7, 16          # halfwords
    add		$s5, $s1, $t7
    lhu      $t7, ($s5)

.rtr_hscroll_done:
	li		$s5, 0xff000000
    and     $t8, $t8, $s5
    li		$s5, 0
    sub     $t4, $s5, $t7         # $t4=tilex(-ts->hscroll)>>3
    sra     $t4, $t4, 3
    and     $t4, $t4, 0xff
    or      $t8, $t8, $t4         # $t8=(xmask<<24)|tilex

    sub     $t7, $t7, $t3
    and     $t7, $t7, 7
    add     $t7, $t7, $t3           # $t7=dx((ts->hscroll-1)&7)+1

	li		$s5, 8
    bne		$t7, $s5, .rtrloop_skip_15
    nop
    li		$s5, 0x01000000
    sub     $s3, $s3, $s5          # we will loop cells+1 times, so loop less when there is no hscroll

.rtrloop_skip_15:
    add     $a1, $s2, $t7          # $a1=pdest
    li	    $a0, 0xf
    b       .rtrloop_enter
    nop

    # $t4 & $t7 are scratch in this loop
.rtrloop: # 40-41 times
    add     $a1, $a1, 8
    li		$s5, 0x01000000
    sub     $s3, $s3, $s5
    add     $t8, $t8, $t3
    bltz	$s3, .rtrloop_exit
    nop

.rtrloop_enter:
	srl		$s5, $t8, 24
    and     $t7, $t8, $s5
    sll		$s5, $t7, 1
    add     $t7, $s1, $s5
    li		$s5, 0xff000000
    not		$s5, $s5
    and     $t4, $s3, $s5          # Pico.vram(nametab_row+(tilex&xmask));
    add		$s5, $t7, $t4
    lhu      $t7, ($s5)             # $t7=code (int, but from unsigned, no sign extend)
    #and		$t7, $t7, 0xffff       # trick: only lower bits

    and     $s5, $t7, 0x8000
    bnez    $s5, .rtr_hiprio
    nop

	srl		$s5, $t9, 8
    bne     $t7, $s5, .rtr_notsamecode
    nop
    # we know stuff about this tile already
    and     $s5, $t9, $t3
    bnez    $s5, .rtrloop         # empty tile
    nop
    and     $s5, $t9, 2
    bnez    $s5, .rtr_singlecolor # singlecolor tile
    nop
    b       .rtr_samecode
    nop

.rtr_notsamecode:
	li		$s5, 0x600000
    and     $t4, $t9, $s5
    sll     $t9, $t7, 8           # remember new code

    # update cram
    and     $t7, $t7, 0x6000
    sra     $a3, $t7,  9      # $a3=pal=((code&0x6000)>>9);

.rtr_samecode:

	#move	$s1, $s1
	li		$s5, 0x100000
    and     $s5, $t9, $s5       # vflip?
    bnez    $s5, .rtr_vflip
    nop

	li		$s5, 0x080000
    and     $s5, $t9, $s5       # hflip?
    bnez    $s5, .rtr_hflip
    nop

    # Tile ($a1=pdest, $a3=pal, $t9=prevcode, $s1=Pico.vram) $a2,$t4,$t7: scratch, $a0=0xf
    Tile 0, 0
    b       .rtrloop
    nop

.rtr_hflip:
    Tile 1, 0
    b       .rtrloop
    nop

.rtr_vflip:
	li		$s5, 0x080000
    and     $s5, $t9, $s5       # hflip?
    bnez    $s5, .rtr_vflip_hflip
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
    sub     $t4, $a1, $s2
    sll		$s5, $t4, 16
    or      $t7, $t7, $s5
    li		$s5, 0x00ff0000
    and     $t4, $t5, $s5
    sll		$s5, $t4, 11
    or      $t7, $t7, $s5 # (trow<<27)
    sw      $t7, ($t6)    # cache hi priority tile
    add		$t6, $t6, $t1
    b       .rtrloop
    nop

.rtrloop_exit:
	li		$s5, 0x00010000
    add     $t5, $t5, $s5
    sll     $t4, $t5, 8
    sll		$s5, $s4, 24
    sub		$s5, $t4, $s5
    bgez    $s5, .rtrloop_outer_exit
    nop
    add     $s2, $s2, 512*8
    b       .rtrloop_outer
    nop

.rtrloop_outer_exit:

    # terminate cache list
    li	    $a0, 0
    sw      $a0, ($t6)    # save cache pointer

	lw      $s1, ($sp)
	lw      $s2, 4($sp)
	lw      $s3, 8($sp)
	lw      $s4, 12($sp)
	lw      $s5, 16($sp)
	lw      $s6, 20($sp)
	addiu   $sp, $sp, 24
    jr      $ra
    nop

.global DrawTilesFromCacheF_asm # int *hc

DrawTilesFromCacheF_asm:
	addiu   $sp, $sp, -24
	sw      $s6, 20($sp)
	sw      $s5, 16($sp)
	sw      $s4, 12($sp)
	sw      $s3, 8($sp)
	sw      $s2, 4($sp)
	sw      $s1, ($sp)

	li		$t0, -4
	li		$t1, 4
	li		$t2, 512

    li      $t9, 0xff000000 # $t9=prevcode=-1
    li      $t6, -1          # $t6=prevy=-1

    lw      $t4, PicoDraw2FB  # $t4=PicoDraw2FB
    lw      $a1, ($a0)    # read y offset
    add		$a0, $a0, 4
    li      $t7, 512
    mul     $s5, $t7, $a1
    add		$a1, $t4, $s5
    sub     $s3, $a1, (512*8*START_ROW) # $a3=scrpos

    lui     $s1, %hi(Pico+0x10000)
    li		$s5, %lo(Pico+0x10000)
    add     $s1, $s1, $s5         # $s1=Pico.vram
    move    $t8, $a0               # hc
    li      $a0, 0xf

    # scratch: $t4, $t7
	# *hcache++ = code|(dx<<16)|(trow<<27); // cache it

.dtfcf_loop:
    lw      $t7, ($t8)    # read code
    add		$t8, $t8, 4
    srl		$a1, $t7, 16   # $a1=dx;
    beqz    $a1, .dtfcf_exit   # dx is never zero, this must be a terminator, return
    nop

    # row changed?
    srl		$s5, $t7, 27
    beq		$t6, $s5, .dtfcf_skip_1
    nop
    move    $t6, $s5
    li      $t4, 512*8
    mul     $s5, $t4, $t6
    add		$t5, $s3, $s5     # $t5=pd = scrpos + prevy*512*8

.dtfcf_skip_1:
	li		$s5, 0xf800
	not		$s5, $s5
	and		$a1, $a1, $s5
    add     $a1, $t5, $a1      # $a1=pdest (halfwords)

    sll     $t7, $t7, 16
    srl     $t7, $t7, 16

	srl		$s5, $t9, 8
    bne     $t7, $s5, .dtfcf_notsamecode
    nop
    # we know stuff about this tile already
    and		$s5, $t9, 1
    bnez    $s5, .dtfcf_loop         # empty tile
    nop
    and		$s5, $t9, 2
    bnez    $s5, .dtfcf_singlecolor  # singlecolor tile
    nop
    b       .dtfcf_samecode
    nop

.dtfcf_notsamecode:
	li		$s5, 0x600000
    and     $t4, $t9, $s5
    sll     $t9, $t7, 8      # remember new code

    # update cram val
    and     $t7, $t7, 0x6000
    sra     $a3, $t7, 9         # $a3=pal=((code&0x6000)>>9);

.dtfcf_samecode:

	li		$s5, 0x100000
    and	    $s5, $t9, $s5          # vflip?
    bnez    $s5, .dtfcf_vflip

	li		$s5, 0x080000
    and     $s5, $t9, $s5	       # hflip?
    bnez    $s5, .dtfcf_hflip

    # Tile ($a1=pdest, $a3=pal, $t9=prevcode, $s1=Pico.vram) $a2,$t4,$t7: scratch, $a0=0xf
    Tile 0, 0
    b       .dtfcf_loop

.dtfcf_hflip:
    Tile 1, 0
    b       .dtfcf_loop

.dtfcf_vflip:
	li		$s5, 0x080000
    and     $s5, $t9, $s5	       # hflip?
    bnez    $s5, .dtfcf_vflip_hflip

    Tile 0, 1
    b       .dtfcf_loop

.dtfcf_vflip_hflip:
    Tile 1, 1
    b       .dtfcf_loop

.dtfcf_singlecolor:
    TileSinglecol
    b       .dtfcf_loop

.dtfcf_exit:
	lw      $s1, ($sp)
	lw      $s2, 4($sp)
	lw      $s3, 8($sp)
	lw      $s4, 12($sp)
	lw      $s5, 16($sp)
	lw      $s6, 20($sp)
	addiu   $sp, $sp, 24

	jr		$ra
	nop

# ###############

# (tile_start<<16)|row_start
.global DrawWindowFull_asm # int tstart, int tend, int prio

DrawWindowFull_asm:
	addiu   $sp, $sp, -24
	sw      $s6, 20($sp)
	sw      $s5, 16($sp)
	sw      $s4, 12($sp)
	sw      $s3, 8($sp)
	sw      $s2, 4($sp)
	sw      $s1, ($sp)

	li		$t0, -4
	li		$t1, 4
	li		$t2, 512
	li		$t3, 1

    lui     $s2, %hi(Pico+0x22228)
    li		$s5, %lo(Pico+0x22228)
    add     $s2, $s2, $s5         # $s2=Pico.video
    lbu 		$s3, 3($s2)        # pvid->reg(3)
    sll     $s3, $s3, 10

    lw      $t4, 12($s2)
    li      $t5, 1                # nametab_step
    and     $s5, $t4, 1           # 40 cell mode?
    bnez	$s5, .dwfskip_1
    nop
    and     $s3, $s3, 0xf800
    sll     $t5, $t5, 6        # nametab_step
    b		.dwfskip_2
    nop

.dwfskip_1:
	and     $s3, $s3, 0xf000     # 0x3c<<10
	sll     $t5, $t5, 7

.dwfskip_2:
    and     $t4, $a0, 0xff
    mul		$s5, $t5, $t4
    add     $s3, $s5, $s3      # nametab += nametab_step*start;

    srl     $t4, $a0, 16       # $t4=start_cell_h
    sll		$s5, $t4, 1
    add     $t7, $s3, $s5

    # fetch the first code now
    lui     $s1, %hi(Pico+0x10000)
    li		$s5, %lo(Pico+0x10000)
    add     $s1, $s1, $s5         # $s1=Pico.vram
    add		$s5, $s1, $t7
    lhu      $t7, ($s5)
    srl		$s5, $t7, 15
    bne		$a2, $s5, .dwfexit
    nop

	srl		$s5, $a1, 16
    sub     $t8, $s5, $t4         # cells (h)
    sll		$s5, $t4, 8
    or      $t8, $t8, $s5
    sll     $t4, $a1, 24
    sll		$s5, $a0, 24
    sub     $t4, $t4, $s5
    srl		$s5, $t4, 8
    or      $t8, $t8, $s5          # $t8=cells_h|(start_cell_h<<8)|(cells_v<<16)
    li		$s5, 0x010000
    sub     $t8, $t8, $s5     # adjust for algo

    li      $t9, 0xff000000       # $t9=prevcode=-1

    lw      $s2, PicoDraw2FB     # $s2=scrpos
    and     $t4, $a0, 0xff
    #ldr     $s2, ($s2)
    sub     $t4, $t4, START_ROW
    add     $s2, $s2, 512*8
    add     $s2, $s2, 8

    li      $t7, 512*8
    mul		$s5, $t7, $t4
    add     $s2, $s5, $s2      # scrpos+=8*512*(start-START_ROW);
    li      $a0, 0xf

.dwfloop_outer:
    and     $t6, $t8, 0xff00       # $t6=tilex
    srl		$s5, $t6, 5
    add     $a1, $s2, $s5          # $a1=pdest
    srl		$s5, $t6, 7
    add     $t6, $s3, $s5
    add     $t6, $s1, $t6          # $t6=Pico.vram+nametab+tilex
    sll		$s5, $t8, 24
    or      $t8, $t8, $s5
    li		$s5, 0x01000000
    sub     $t8, $t8, $s5   # cell loop counter
    b       .dwfloop_enter
    nop

    # $t4 & $t7 are scratch in this loop
.dwfloop:
    add     $a1, $a1, 8
    li		$s5, 0x01000000
    sub     $t8, $t8, $s5
    bltz    $t8, .dwfloop_exit
    nop

.dwfloop_enter:
    lhu      $t7, ($t6)      # $t7=code
    add		$t6, $t6, 2

	srl		$s5, $t9, 8
    bne     $t7, $s5, .dwf_notsamecode
    nop
    # we know stuff about this tile already
    and     $s5, $t9, 1
    bnez    $s5, .dwfloop         # empty tile
    nop
    and     $s5, $t9, 2
    bnez    $s5, .dwf_singlecolor # singlecolor tile
    nop
    b       .dwf_samecode
    nop

.dwf_notsamecode:
	li		$s5, 0x600000
    and     $t4, $t9, $s5
    sll     $t9, $t7, 8      # remember new code

    # update cram val
    and     $t7, $t7, 0x6000
    sra     $a3, $t7, 9      # $a3=pal=((code&0x6000)>>9);

.dwf_samecode:

	li		$s5, 0x100000
    and     $s5, $t9, $s5       # vflip?
    bnez    $s5, .dwf_vflip
    nop

    li		$s5, 0x080000
    and     $s5, $t9, $s5       # hflip?
    bnez    $s5, .dwf_hflip
    nop

    # Tile ($a1=pdest, $a3=pal, $t9=prevcode, $s1=Pico.vram) $a2,$t4,$t7: scratch, $a0=0xf
    Tile 0, 0
    b       .dwfloop
    nop

.dwf_hflip:
    Tile 1, 0
    b       .dwfloop
    nop

.dwf_vflip:
    li		$s5, 0x080000
    and     $s5, $t9, $s5       # hflip?
    bnez    $s5, .dwf_vflip_hflip
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
	li		$s5, 0xff000000
	not		$s5, $s5
    and     $t8, $t8, $s5  # fix $t8
    li		$s5, 0x010000
    sub     $t8, $t8, $s5
    bltz	$t8, .dwfexit
    nop
    add     $s2, $s2, 512*8
    add     $s3, $s3, $t5         # nametab+=nametab_step
    b       .dwfloop_outer
    nop

.dwfexit:
	lw      $s1, ($sp)
	lw      $s2, 4($sp)
	lw      $s3, 8($sp)
	lw      $s4, 12($sp)
	lw      $s5, 16($sp)
	lw      $s6, 20($sp)
	addiu   $sp, $sp, 24
    jr      $ra
    nop

# ---------------- sprites ---------------

.macro SpriteLoop hflip vflip
.if \vflip
	srl		$a1, $t5, 24			# height
    li      $a0, 512*8
    mul     $s5, $a1, $a0
    add		$s2, $s2, $s5			# scrpos+=height*512*8;
    sll		$s5, $a1, 3
    add     $s3, $s3, $s5           # sy+=height*8
.endif
    li      $a0,  0xf
.if \hflip
    and     $a1, $t5, 0xff
    sll		$s5, $a1, 3
    add     $t8, $t8, $s5           # sx+=width*8
58:
	sub		$s5, $t8, (8+512+8)
    bltz    $s5, 51f
    nop
    srl		$s5, $t5, 16
    add     $t9, $t9, $s5
    sub     $t5, $t5, 1             # sub width
    sub     $t8, $t8, 8
    b       58b
    nop
.else
    bgtz    $t8, 51f             # skip tiles hidden on the left of screen
    nop
58:
	srl		$s5, $t5, 16
    add     $t9, $t9, $s5
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
	li		$s5, 0xff000000
	not		$s5, $s5
	and		$t5, $t5, $s5        # fix height
    sll		$s5, $t5, 16
    or      $t5, $t5, $s5

51: # outer_enter
    sub     $t5, $t5, 1          # width--
    sll		$a1, $t5, 24
    bltz	$a1, 54f			 # end of tile
    nop
.if \hflip
	move	$s5, $t8
    sub     $t8, $t8, 8          # sx-=8
    sub		$s5, $s5, $t8
    blez	$s5, 54f       # tile offscreen
    nop
.else
    sub		$s5, $t8, 328
    bgez	$s5, 54f              # tile offscreen
    nop
.endif
    move    $t6, $s3            # $a2=sy
    add     $a1, $s2, $t8         # pdest=scrpos+sx
    b       53f
    nop

52: # inner
    add     $t9, $t9, (1<<8)       # tile++
.if !\vflip
    add     $t6, $t6, 8          # sy+=8
    add     $a1, $a1, 512*8
.endif

53: # inner_enter
    # end of sprite?
    li		$s5, 0x01000000
    sub     $t5, $t5, $s5
    bltz    $t5, 50b                 # ->outer
    nop
.if \vflip
    sub     $t6, $t6, 8          # sy-=8
    sub     $a1, $a1, 512*8
.endif

    # offscreen?
    ble     $t6, (START_ROW*8), 52b
    nop

    sub		$s5, $t6, (END_ROW*8+8)
    bgez     $s5, 52b
    nop

    # Tile ($s1=pdest, $s3=pal, $t5=prevcode, $a3=Pico.vram) $s2,$a0,$a3: scratch, $a0=0xf
    Tile \hflip, \vflip
    nop
    b       52b
    nop

54:
	lw      $s1, ($sp)
	lw      $s2, 4($sp)
	lw      $s3, 8($sp)
	lw      $s4, 12($sp)
	lw      $s5, 16($sp)
	lw      $s6, 20($sp)
	addiu   $sp, $sp, 24
	jr		$ra
	nop

.endm



.global DrawSpriteFull_asm # unsigned int *sprite

DrawSpriteFull_asm:
	addiu   $sp, $sp, -24
	sw      $s6, 20($sp)
	sw      $s5, 16($sp)
	sw      $s4, 12($sp)
	sw      $s3, 8($sp)
	sw      $s2, 4($sp)
	sw      $s1, ($sp)

	li		$t0, -4
	li		$t1, 4
	li		$t2, 512

    lw      $a3, ($a0)        # sprite[0]
    sll		$t5, $a3, 4
    srl		$t6, $t5, 30
    add     $t6, $t6, 1       # $t6=width
    sll		$t5, $t5, 2
    srl		$t5, $t5, 30
    add     $t5, $t5, 1       # $t5=height

    sll     $s3, $a3, 23
    srl     $s3, $s3, 23

    lw      $s4, 4($a0)       # $s3=code
    sub     $s3, $s3, 0x78    # $s3=sy
    sll     $t8, $s4, 7
    srl     $t8, $t8, 23
    sub     $t8, $t8, 0x78    # $t8=sx

    sll     $t9, $s4, 21
    srl     $t9, $t9, 13      # $t9=tile<<8

    and     $a3, $s4, 0x6000
    srl     $a3, $a3, 9       # $a3=pal=((code>>9)&0x30);

	lw		$s2, (PicoDraw2FB)     # $s2=scrpos
    lui     $s1, %hi(Pico+0x10000)
    li		$s5, %lo(Pico+0x10000)
    add     $s1, $s1, $s5         # $s1=Pico.vram
    sub     $a1, $s3, (START_ROW*8)
    li      $a0, 512
    mul     $s5, $a1, $a0
    add		$s2, $s2, $s5		# scrpos+=(sy-START_ROW*8)*512;

	sll     $s5, $t5, 16
    or      $t5, $t5, $s5
    sll     $s5, $t5, 8
    or      $t5, $t6, $s5        # $t5=width|(height<<8)|(height<<24)

    and		$s5, $s4, 0x1000
    bnez    $s5, .dsf_vflip   # vflip?
    nop

    and		$s5, $s4, 0x0800
    bnez    $s5, .dsf_hflip   # hflip?
    nop

    SpriteLoop 0, 0
    nop

.dsf_hflip:
    SpriteLoop 1, 0
    nop

.dsf_vflip:
    and		$s5, $s4, 0x0800
    bnez    $s5, .dsf_vflip_hflip   # hflip?
    nop

    SpriteLoop 0, 1
    nop

.dsf_vflip_hflip:
    SpriteLoop 1, 1
    nop

# vim:filetype=mipsasm

