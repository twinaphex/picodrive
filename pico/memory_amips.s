#*
#* memory handlers with banking support
#* Robson Santana, 2016. email: robson.poli@gmail.com
#*

.eqv SRR_MAPPED,     1       #(1 <<  0)
.eqv SRR_READONLY,   2       #(1 <<  1)
.eqv SRF_EEPROM,     2       #(1 <<  1)
.eqv POPT_EN_32X,    1048576 #(1 << 20)

.set noreorder
.set noat

.text
.align 4

.global PicoRead8_sram
.global PicoRead8_io
.global PicoRead16_sram
.global PicoRead16_io
.global PicoWrite8_io
.global PicoWrite16_io

PicoRead8_sram: # u32 a, u32 d
    lui     $a2, %hi(SRam)
    li		$t0, %lo(SRam)
    add     $a2, $a2, $t0
    lui     $a3, %hi(Pico+0x22200)
    li		$t0, %lo(Pico+0x22200)
    add     $a3, $a3, $t0
    lw	    $a1, 8($a2)     # SRam.end
    sub		$t0, $a0, $a1
    bgtz    $t0, m_read8_nosram
    nop
    lw	    $a1, 4($a2)     # SRam.start
    sub		$t0, $a0, $a1
    bltz    $t0, m_read8_nosram
    nop
    lbu	    $a1, 0x11($a3)  # Pico.m.sram_reg
    li		$t0, SRR_MAPPED
    and		$t0, $a1, $t0
    beqz    $t0, m_read8_nosram
    nop
    lw	    $a1, 0x0c($a2)
    li		$t0, SRF_EEPROM
    and		$t0, $a1, $t0
    bnez    $t0, m_read8_eeprom
    nop
    lw	    $a1, 4($a2)		# SRam.start
    lw      $a2, ($a2)      # SRam.data
    sub     $a0, $a0, $a1
    add     $a0, $a0, $a2
    lbu	    $v0, ($a0)
    jr      $ra
    nop

m_read8_nosram:
    lw      $a1, 4($a3)     # romsize
    sub		$t0, $a0, $a1
    bgtz    $t0, m_read8_nosram_gt
    nop
    # XXX: banking unfriendly
    lw      $a1, ($a3)
    xor     $a0, $a0, 1
    add		$t0, $a1, $a0
    lbu     $v0, ($t0)
    jr      $ra
    nop

m_read8_nosram_gt:
    li      $v0, 0
    jr      $ra             # bad location
    nop

m_read8_eeprom:
	addiu   $sp, $sp, -8     # allocate 2 words on the stack
	sw      $a0, 4($sp)      # save $a0 in the upper one
	sw      $ra, ($sp)
	jal     EEPROM_read
	nop
	lw      $ra, ($sp)
	lw      $a1, 4($sp)      # reload $r0 so we can return to caller
	addiu   $sp, $sp, 8      # restore $sp, freeing the allocated space
	and		$t0, $a1, 1
	bnez    $t0, m_read8_eeprom_ne
	nop
	srl		$v0, $v0, 8
m_read8_eeprom_ne:
	move	$v0, $a0
	jr $ra                    # return
    nop

# ######################################################################

PicoRead8_io: # u32 a, u32 d
	li		$t0, 0x001f
	not		$t0, $t0
	and		$a2, $a0, $t0     # most commonly we get i/o port read,
	li		$t0, 0xa10000
	sub		$t0, $a2, $t0
    beqz    $t0, io_ports_read_     # so check for it first
    nop

m_read8_not_io:
    and     $a2, $a0, 0xfc00
    li		$t0, 0x1000
    sub		$t0, $a2, $t0
    bnez	$t0, m_read8_not_brq
    nop

    lui     $a3, %hi(Pico+0x22200)
    li		$t0, %lo(Pico+0x22200)
    add     $a3, $a3, $t0
    move    $a1, $a0
    lbu     $v0, 8($a3)       # Pico.m.rotate
    addi    $t0, $v0, 1
    sb    	$t0, 8($a3)
	sll     $t0, $v0, 6
	xor     $v0, $v0, $t0

	and		$t0, $a1, 1
	bnez	$t0, m_read8_not_io_ra      # odd addr -> open bus
	nop
	li		$t0, 1
	not		$t0, $t0
	and		$v0, $v0, $t0     # bit0 defined in this area
	li		$t0, 0x1100
	sub		$t0, $a1, $t0
    beqz	$t0, m_read8_not_io_ra      # not busreq
    nop

    lbu     $a1, 8+0x01($a3)  # Pico.m.z80Run
    lbu     $a2, 8+0x0f($a3)  # Pico.m.z80_reset
    or      $v0, $v0, $a1
    or      $v0, $v0, $a2
	jr $ra                    # return
    nop

m_read8_not_io_ra:
	jr $ra                    # return
    nop

m_read8_not_brq:
    #lui     $t2, %hi(PicoOpt)
    #li		$t0, %lo(PicoOpt)
    #add     $t2, $t2, $t0
    #lw      $t2, ($t2)
    lw      $t2, (PicoOpt)
    li		$t0, POPT_EN_32X
    and		$t0, $t2, $t0
    bnez	$t0, PicoRead8_32x_
    nop
    li      $v0, 0
	jr $ra                    # return
    nop

PicoRead8_32x_:
    j	    PicoRead8_32x
    nop

io_ports_read_:
	j       io_ports_read
	nop

# ######################################################################

PicoRead16_sram: # u32 a, u32 d
    lui     $a2, %hi(SRam)
    li		$t0, %lo(SRam)
    add     $a2, $a2, $t0
    lui     $a3, %hi(Pico+0x22200)
    li		$t0, %lo(Pico+0x22200)
    add     $a3, $a3, $t0
    lw	    $a1, 8($a2)     # SRam.end
    sub		$t0, $a0, $a1
    bgtz    $t0, m_read16_nosram
    nop
    lw	    $a1, 4($a2)     # SRam.start
    sub		$t0, $a0, $a1
    bltz    $t0, m_read16_nosram
    nop
    lbu	    $a1, 0x11($a3)  # Pico.m.sram_reg
    li		$t0, SRR_MAPPED
    and		$t0, $a1, $t0
    beqz    $t0, m_read16_nosram
    nop
    lw	    $a1, 0x0c($a2)
    li		$t0, SRF_EEPROM
    and		$t0, $a1, $t0
    bnez    $t0, EEPROM_read_
    nop
    lw	    $a1, 4($a2)		# SRam.start
    lw      $a2, ($a2)      # SRam.data
    sub     $a0, $a0, $a1
    add     $a0, $a0, $a2
    lbu     $a1, ($a0)
    addi	$a0, $a0, 1
    lbu	    $t0, ($a0)
    sll		$t0, $a1, 8
    or      $v0, $a0, $t0
    jr      $ra
    nop

EEPROM_read_:
	j		EEPROM_read
	nop


m_read16_nosram:
    lw      $a1, 4($a3)     # romsize
    sub		$t0, $a0, $a1
    bgtz    $t0, m_read16_nosram_gt
    nop
    # XXX: banking unfriendly
    lw      $a1, ($a3)
    add		$t0, $a1, $a0
    lhu     $v0, ($t0)
    #andi	$v0, $v0, 0xffff
    jr      $ra
    nop

m_read16_nosram_gt:
    li      $v0, 0
    jr      $ra             # bad location
    nop


PicoRead16_io: # u32 a, u32 d
	li		$t0, 0x001f
	not		$t0, $t0
	and		$a2, $a0, $t0    # most commonly we get i/o port read,
	li		$t0, 0xa10000
	sub		$t0, $a2, $t0
    bnez    $t0, m_read16_not_io     # so check for it first
    nop

	addiu   $sp, $sp, -4     # allocate 2 words on the stack
	sw      $ra, ($sp)
    jal     io_ports_read    # same as read8
    nop
	sll     $t0, $v0, 8
	or      $v0, $v0, $t0    # only has bytes mirrored
	lw      $ra, ($sp)
	addiu   $sp, $sp, 4      # restore $sp, freeing the allocated space
	jr 		$ra
	nop

m_read16_not_io:
    andi    $a2, $a0, 0xfc00
    li		$t0, 0x1000
    sub		$t0, $a2, $t0
    bnez    $t0, m_read16_not_brq
    nop

    lui     $a3, %hi(Pico+0x22200)
    li		$t0, %lo(Pico+0x22200)
    add     $a3, $a3, $t0
    andi    $a2, $a0, 0xff00
    lw      $v0, 8($a3)       # Pico.m.rotate
    addi    $v0, $v0, 1
    sb    	$v0, 8($a3)
	sll     $t0, $v0, 5
	xor     $v0, $v0, $t0
	sll     $t0, $v0, 8
	xor     $v0, $v0, $t0
	li		$t0, 0x100        # bit8 defined in this area
	not		$t0, $t0
	and		$v0, $v0, $t0
	li		$t0, 0x1100
	sub		$t0, $a2, $t0
    bnez    $t0, m_read16_not_io_ra     # not busreq
    nop

    lbu     $a1, 8+0x01($a3)  # Pico.m.z80Run
    lbu     $a2, 8+0x0f($a3)  # Pico.m.z80_reset
    sll		$t0, $a1, 8
    or      $v0, $v0, $t0
    sll		$t0, $a2, 8
    or      $v0, $v0, $t0
	jr 		$ra                    # return
    nop

m_read16_not_io_ra:
	jr 		$ra                    # return
    nop

m_read16_not_brq:
    #lui     $a2, %hi(PicoOpt)
    #li		$t0, %lo(PicoOpt)
    #add     $a2, $a2, $t0
    #lw      $a2, ($a2)
    lw      $a2, (PicoOpt)
    #lw      $a2, ($a2)
    li		$t0, POPT_EN_32X
    and		$t0, $a2, $t0
    bnez	$t0, PicoRead16_32x_
    nop
    li      $v0, 0
	jr      $ra                    # return
    nop

PicoRead16_32x_:
    j	    PicoRead16_32x
    nop
# ######################################################################

PicoWrite8_io: # u32 a, u32 d
	li		$t0, 0xffffe1
	and		$a2, $a0, $t0       # most commonly we get i/o port write,
	li		$t0, 0xa10000
    xor     $a2, $a2, $t0   # so check for it first
    xor     $a2, $a2, 1
    beqz    $a2, io_ports_write_
    nop

m_write8_not_io:
    and     $t0, $a0, 1
    bnez    $t0, m_write8_not_z80ctl # even addrs only
    nop
    and     $a2, $a0, 0xff00
    li      $t0, 0x1100
    sub		$t0, $a2, $t0
    bnez    $t0, m_write8_not_io_1
    nop
    move    $a0, $a1
    j       ctl_write_z80busreq
    nop
m_write8_not_io_1:
    li      $t0, 0x1200
    sub		$t0, $a2, $t0
    bnez    $t0, m_write8_not_z80ctl
    nop
    move    $a0, $a1
    j       ctl_write_z80reset
    nop

m_write8_not_z80ctl:
    # unlikely
    li		$t0, 0xa10000
    xor     $a2, $a0, $t0
    xori    $a2, $a2, 0x003000
    xor     $a2, $a2, 0x0000f1
    bnez    $a2, m_write8_not_sreg
    nop

    lui     $a3, %hi(Pico+0x22200)
    li		$t0, %lo(Pico+0x22200)
    add     $a3, $a3, $t0
    lbu     $a2, (8+9)($a3)       # Pico.m.sram_reg
    and     $a1, $a1, (SRR_MAPPED|SRR_READONLY)
	li		$t0, (SRR_MAPPED|SRR_READONLY)
	not		$t0, $t0
	and		$a2, $a2, $t0
    or      $a2, $a2, $a1
    sb      $a2, (8+9)($a3)
	jr      $ra                    # return
    nop

m_write8_not_sreg:
	lw		$t2, (PicoOpt)
    #lui     $t2, %hi(PicoOpt)
    #li		$t0, %lo(PicoOpt)
    #add     $t2, $t2, $t0
    #lw      $t2, ($t2)
    li		$t0, POPT_EN_32X
    and		$t0, $t2, $t0
    bnez	$t0, PicoWrite8_32x_
    nop
	jr      $ra
    nop

PicoWrite8_32x_:
	j       PicoWrite8_32x
	nop

io_ports_write_:
	j       io_ports_write
	nop

# ######################################################################

PicoWrite16_io: # u32 a, u32 d
	li		$t0, 0xffffe1
	and		$a2, $a0, $t0     # most commonly we get i/o port read,
	li		$t0, 0xa10000
	sub		$t0, $a2, $t0
    beqz    $t0, io_ports_write_     # so check for it first
    nop

m_write16_not_io:
    and     $a2, $a0, 0xff00
    li      $t0, 0x1100
    sub		$t0, $a2, $t0
    bnez    $t0, m_write16_not_io_1
    nop
    srl     $a0, $a1, 8
    j       ctl_write_z80busreq
    nop

m_write16_not_io_1:
    li      $t0, 0x1200
    sub		$t0, $a2, $t0
    bnez    $t0, m_write16_not_z80ctl
    nop
    srl     $a0, $a1, 8
    j       ctl_write_z80reset
    nop

m_write16_not_z80ctl:
    # unlikely

	li		$t0, 0xa10000
    xor     $a2, $a0, $t0
    xor     $a2, $a2, 0x003000
    xor     $a2, $a2, 0x0000f0
    bnez    $a2, m_write8_not_sreg
    nop

    lui     $a3, %hi(Pico+0x22200)
    li		$t0, %lo(Pico+0x22200)
    add     $a3, $a3, $t0
    lbu     $a2, (8+9)($a3)       # Pico.m.sram_reg
    and     $a1, $a1, (SRR_MAPPED|SRR_READONLY)
	li		$t0, (SRR_MAPPED|SRR_READONLY)
	not		$t0, $t0
	and		$a2, $a2, $t0
    or      $a2, $a2, $a1
    sb      $a2, (8+9)($a3)
	jr      $ra                    # return
    nop

m_write16_not_sreg:
    lui     $t2, %hi(PicoOpt)
    li		$t0, %lo(PicoOpt)
    add     $t2, $t2, $t0
    lw      $t2, ($t2)
    li		$t0, POPT_EN_32X
    and		$t0, $t0, $t2
    bnez	$t0, PicoWrite16_32x_
    nop
	jr      $ra
    nop

PicoWrite16_32x_:
	j       PicoWrite16_32x
	nop



