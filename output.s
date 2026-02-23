	.text
	.globl main
main:
	addi sp, sp, -16
	li t0, 0
	li t1, 2147483647
	sub t0, t0, t1
	sw t0, 0(sp)
	lw a0, 0(sp)
	addi sp, sp, 16
	ret
