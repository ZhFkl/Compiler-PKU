	.text
	.globl main
main:
	addi sp, sp, -16
6
9
	li t0, 1
	sw t0, 0(sp)
8
	lw t0, 0(sp)
	sw t0, 4(sp)
12
	lw t0, 4(sp)
	li t1, 1
	add t0, t0, t1
	sw t0, 8(sp)
9
	lw t0, 8(sp)
	sw t0, 0(sp)
8
	lw t0, 0(sp)
	sw t0, 12(sp)
16
	lw a0, 12(sp)
	addi sp, sp, 16
	ret
