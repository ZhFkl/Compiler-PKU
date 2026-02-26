	.text
	.globl main
main:
entry:
	lw t0, 0(sp)
	bnez t0, then_0
	j else_0
then_0:
	li a0, 1
	ret
else_0:
	j end_0
end_0:
	li a0, 0
	ret
