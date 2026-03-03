	.data
	.globl x
x:
	.zero 4
	.data
	.globl init
init:
	.word 1
	.text
	.globl main
main:
	addi sp, sp, -16
	sw ra, 0(sp)
.L_main_entry:
	lw t0, 0(sp)
	sw t0, 4(sp)
	lw a0, 4(sp)
	call putint
	li a0, 32
	call putch
	li a0, 10
	call putint
	li a0, 32
	call putch
	li a0, 11
	call putint
	li a0, 32
	call putch
	lw t0, 0(sp)
	sw t0, 8(sp)
	lw a0, 8(sp)
	call putint
	li a0, 10
	call putch
	li a0, 0
	lw ra, 0(sp)
	addi sp, sp, 16
	ret
