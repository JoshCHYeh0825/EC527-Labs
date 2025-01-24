	.file	"test_O_level.c"
	.text
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC0:
	.string	"\n Starting a loop "
.LC1:
	.string	"\n done "
	.text
	.globl	main
	.type	main, @function
main:
.LFB11:
	.cfi_startproc
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	movl	$.LC0, %edi
	call	puts
	movl	$200000001, %eax
.L2:
	subq	$1, %rax
	jne	.L2
	movl	$.LC1, %edi
	call	puts
	movl	$0, %eax
	addq	$8, %rsp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE11:
	.size	main, .-main
	.ident	"GCC: (GNU) 8.5.0 20210514 (Red Hat 8.5.0-22)"
	.section	.note.GNU-stack,"",@progbits
