# Copyright (C) 2011, 2012, 2013 The uOFW team
# See the file COPYING for copying permission.

.set noreorder

.text
.align 4

.global invalidate_icache_region
.global _DaedalusICacheInvalidate

# sceKernelInvalidateIcacheRange gives me problems, trying this instead
# Invalidates an n byte region starting at the start address
# $4: start location
# $5: length

invalidate_icache_region:
  ins $4, $0, 0, 6                # align to 64 bytes
  addiu $2, $5, 63                # align up to 64 bytes
  srl $2, $2, 6                   # divide by 64
  beq $2, $0, done                # exit early on 0
  nop

iir_loop:
  cache 8, ($4)                   # invalidate icache line
  cache 8, ($4)                   # do it again for good luck :P
  addiu $2, $2, -1                # next loop iteration
  addiu $4, $4, 64                # go to next cache line
  bne $2, $0, iir_loop            # loop
  nop


done:
  jr $ra                          # return
  nop
