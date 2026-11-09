	.file	"os-math.c"
	.text
.Ltext0:
	.file 0 "/home/jason/study/ck_study/animation" "experiments/os-math.c"
	.globl	os_data
	.data
	.align 4
	.type	os_data, @object
	.size	os_data, 4
os_data:
	.long	10
	.globl	os_bss
	.bss
	.align 4
	.type	os_bss, @object
	.size	os_bss, 4
os_bss:
	.zero	4
	.globl	os_message
	.section	.rodata
	.align 8
	.type	os_message, @object
	.size	os_message, 8
os_message:
	.string	"OS demo"
	.text
	.globl	os_add
	.type	os_add, @function
os_add:
.LFB0:
	.file 1 "experiments/os-math.c"
	.loc 1 8 1
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$8, %rsp
	movl	%edi, -4(%rbp)
	movl	%esi, -8(%rbp)
	.loc 1 9 17
	movl	-4(%rbp), %edx
	movl	-8(%rbp), %eax
	addl	%edx, %eax
	.loc 1 10 1
	leave
	.cfi_restore 6
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE0:
	.size	os_add, .-os_add
	.globl	os_width
	.type	os_width, @function
os_width:
.LFB1:
	.loc 1 14 1
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$16, %rsp
	.loc 1 16 5
#APP
# 16 "experiments/os-math.c" 1
	movabs $0xffffffffffffffff, %rax
	mov $5, %eax
	
# 0 "" 2
#NO_APP
	movq	%rax, -8(%rbp)
	.loc 1 19 12
	movq	-8(%rbp), %rax
	.loc 1 20 1
	leave
	.cfi_restore 6
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE1:
	.size	os_width, .-os_width
	.globl	os_equal
	.type	os_equal, @function
os_equal:
.LFB2:
	.loc 1 23 1
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$8, %rsp
	movl	%edi, -4(%rbp)
	movl	%esi, -8(%rbp)
	.loc 1 24 17
	movl	-4(%rbp), %eax
	cmpl	-8(%rbp), %eax
	sete	%al
	movzbl	%al, %eax
	.loc 1 25 1
	leave
	.cfi_restore 6
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE2:
	.size	os_equal, .-os_equal
.Letext0:
	.file 2 "/usr/include/x86_64-linux-gnu/bits/types.h"
	.file 3 "/usr/include/x86_64-linux-gnu/bits/stdint-uintn.h"
	.file 4 "experiments/os-demo.h"
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.long	0x1b1
	.value	0x5
	.byte	0x1
	.byte	0x8
	.long	.Ldebug_abbrev0
	.uleb128 0x9
	.long	.LASF710
	.byte	0x1d
	.long	.LASF0
	.long	.LASF1
	.quad	.Ltext0
	.quad	.Letext0-.Ltext0
	.long	.Ldebug_line0
	.long	.Ldebug_macro0
	.uleb128 0x1
	.byte	0x1
	.byte	0x8
	.long	.LASF693
	.uleb128 0x1
	.byte	0x2
	.byte	0x7
	.long	.LASF694
	.uleb128 0x1
	.byte	0x4
	.byte	0x7
	.long	.LASF695
	.uleb128 0x1
	.byte	0x8
	.byte	0x7
	.long	.LASF696
	.uleb128 0x1
	.byte	0x1
	.byte	0x6
	.long	.LASF697
	.uleb128 0x1
	.byte	0x2
	.byte	0x5
	.long	.LASF698
	.uleb128 0xa
	.byte	0x4
	.byte	0x5
	.string	"int"
	.uleb128 0x1
	.byte	0x8
	.byte	0x5
	.long	.LASF699
	.uleb128 0x5
	.long	.LASF701
	.byte	0x2
	.byte	0x2d
	.byte	0x1b
	.long	0x47
	.uleb128 0x1
	.byte	0x1
	.byte	0x6
	.long	.LASF700
	.uleb128 0x3
	.long	0x76
	.uleb128 0x5
	.long	.LASF702
	.byte	0x3
	.byte	0x1b
	.byte	0x14
	.long	0x6a
	.uleb128 0x4
	.long	.LASF703
	.byte	0x6
	.byte	0xc
	.long	0x5c
	.uleb128 0x4
	.long	.LASF704
	.byte	0x7
	.byte	0xc
	.long	0x5c
	.uleb128 0x6
	.long	0x7d
	.long	0xaf
	.uleb128 0xb
	.byte	0
	.uleb128 0x3
	.long	0xa4
	.uleb128 0x4
	.long	.LASF705
	.byte	0x8
	.byte	0x13
	.long	0xaf
	.uleb128 0x7
	.long	0x8e
	.byte	0x3
	.uleb128 0x9
	.byte	0x3
	.quad	os_data
	.uleb128 0x7
	.long	0x99
	.byte	0x4
	.uleb128 0x9
	.byte	0x3
	.quad	os_bss
	.uleb128 0x6
	.long	0x7d
	.long	0xef
	.uleb128 0xc
	.long	0x47
	.byte	0x7
	.byte	0
	.uleb128 0x3
	.long	0xdf
	.uleb128 0xd
	.long	0xb4
	.byte	0x1
	.byte	0x5
	.byte	0xc
	.long	0xef
	.uleb128 0x9
	.byte	0x3
	.quad	os_message
	.uleb128 0x8
	.long	.LASF708
	.byte	0x16
	.byte	0x1f
	.long	0x5c
	.quad	.LFB2
	.quad	.LFE2-.LFB2
	.uleb128 0x1
	.byte	0x9c
	.long	0x148
	.uleb128 0x2
	.long	.LASF706
	.byte	0x16
	.byte	0x2c
	.long	0x5c
	.uleb128 0x2
	.byte	0x91
	.sleb128 -20
	.uleb128 0x2
	.long	.LASF707
	.byte	0x16
	.byte	0x36
	.long	0x5c
	.uleb128 0x2
	.byte	0x91
	.sleb128 -24
	.byte	0
	.uleb128 0x8
	.long	.LASF709
	.byte	0xd
	.byte	0x24
	.long	0x82
	.quad	.LFB1
	.quad	.LFE1-.LFB1
	.uleb128 0x1
	.byte	0x9c
	.long	0x179
	.uleb128 0xe
	.long	.LASF711
	.byte	0x1
	.byte	0xf
	.byte	0xe
	.long	0x82
	.uleb128 0x2
	.byte	0x91
	.sleb128 -24
	.byte	0
	.uleb128 0xf
	.long	.LASF712
	.byte	0x1
	.byte	0x7
	.byte	0x1f
	.long	0x5c
	.quad	.LFB0
	.quad	.LFE0-.LFB0
	.uleb128 0x1
	.byte	0x9c
	.uleb128 0x2
	.long	.LASF706
	.byte	0x7
	.byte	0x2a
	.long	0x5c
	.uleb128 0x2
	.byte	0x91
	.sleb128 -20
	.uleb128 0x2
	.long	.LASF707
	.byte	0x7
	.byte	0x34
	.long	0x5c
	.uleb128 0x2
	.byte	0x91
	.sleb128 -24
	.byte	0
	.byte	0
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.uleb128 0x1
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.byte	0
	.byte	0
	.uleb128 0x2
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0x21
	.sleb128 1
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0x3
	.uleb128 0x26
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x4
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0x21
	.sleb128 4
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3c
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0x5
	.uleb128 0x16
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x6
	.uleb128 0x1
	.byte	0x1
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x7
	.uleb128 0x34
	.byte	0
	.uleb128 0x47
	.uleb128 0x13
	.uleb128 0x3a
	.uleb128 0x21
	.sleb128 1
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0x21
	.sleb128 5
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0x8
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0x21
	.sleb128 1
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x7a
	.uleb128 0x19
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x9
	.uleb128 0x11
	.byte	0x1
	.uleb128 0x25
	.uleb128 0xe
	.uleb128 0x13
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0x1f
	.uleb128 0x1b
	.uleb128 0x1f
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x10
	.uleb128 0x17
	.uleb128 0x79
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0xa
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0x8
	.byte	0
	.byte	0
	.uleb128 0xb
	.uleb128 0x21
	.byte	0
	.byte	0
	.byte	0
	.uleb128 0xc
	.uleb128 0x21
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2f
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0xd
	.uleb128 0x34
	.byte	0
	.uleb128 0x47
	.uleb128 0x13
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0xe
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0xf
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x7a
	.uleb128 0x19
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_aranges,"",@progbits
	.long	0x2c
	.value	0x2
	.long	.Ldebug_info0
	.byte	0x8
	.byte	0
	.value	0
	.value	0
	.quad	.Ltext0
	.quad	.Letext0-.Ltext0
	.quad	0
	.quad	0
	.section	.debug_macro,"",@progbits
.Ldebug_macro0:
	.value	0x5
	.byte	0x2
	.long	.Ldebug_line0
	.byte	0x7
	.long	.Ldebug_macro2
	.byte	0x3
	.uleb128 0
	.uleb128 0x1
	.file 5 "/usr/include/stdc-predef.h"
	.byte	0x3
	.uleb128 0
	.uleb128 0x5
	.byte	0x7
	.long	.Ldebug_macro3
	.byte	0x4
	.byte	0x3
	.uleb128 0x1
	.uleb128 0x4
	.byte	0x5
	.uleb128 0x2
	.long	.LASF402
	.file 6 "/usr/lib/gcc/x86_64-linux-gnu/13/include/stdint.h"
	.byte	0x3
	.uleb128 0x3
	.uleb128 0x6
	.file 7 "/usr/include/stdint.h"
	.byte	0x3
	.uleb128 0x9
	.uleb128 0x7
	.byte	0x7
	.long	.Ldebug_macro4
	.file 8 "/usr/include/x86_64-linux-gnu/bits/libc-header-start.h"
	.byte	0x3
	.uleb128 0x1a
	.uleb128 0x8
	.byte	0x6
	.uleb128 0x1f
	.long	.LASF405
	.file 9 "/usr/include/features.h"
	.byte	0x3
	.uleb128 0x21
	.uleb128 0x9
	.byte	0x7
	.long	.Ldebug_macro5
	.file 10 "/usr/include/features-time64.h"
	.byte	0x3
	.uleb128 0x18a
	.uleb128 0xa
	.file 11 "/usr/include/x86_64-linux-gnu/bits/wordsize.h"
	.byte	0x3
	.uleb128 0x14
	.uleb128 0xb
	.byte	0x7
	.long	.Ldebug_macro6
	.byte	0x4
	.file 12 "/usr/include/x86_64-linux-gnu/bits/timesize.h"
	.byte	0x3
	.uleb128 0x15
	.uleb128 0xc
	.byte	0x3
	.uleb128 0x13
	.uleb128 0xb
	.byte	0x7
	.long	.Ldebug_macro6
	.byte	0x4
	.byte	0x5
	.uleb128 0x1a
	.long	.LASF446
	.byte	0x4
	.byte	0x4
	.byte	0x7
	.long	.Ldebug_macro7
	.file 13 "/usr/include/x86_64-linux-gnu/sys/cdefs.h"
	.byte	0x3
	.uleb128 0x1f6
	.uleb128 0xd
	.byte	0x7
	.long	.Ldebug_macro8
	.byte	0x3
	.uleb128 0x240
	.uleb128 0xb
	.byte	0x7
	.long	.Ldebug_macro6
	.byte	0x4
	.file 14 "/usr/include/x86_64-linux-gnu/bits/long-double.h"
	.byte	0x3
	.uleb128 0x241
	.uleb128 0xe
	.byte	0x5
	.uleb128 0x15
	.long	.LASF521
	.byte	0x4
	.byte	0x7
	.long	.Ldebug_macro9
	.byte	0x4
	.file 15 "/usr/include/x86_64-linux-gnu/gnu/stubs.h"
	.byte	0x3
	.uleb128 0x20e
	.uleb128 0xf
	.file 16 "/usr/include/x86_64-linux-gnu/gnu/stubs-64.h"
	.byte	0x3
	.uleb128 0xa
	.uleb128 0x10
	.byte	0x7
	.long	.Ldebug_macro10
	.byte	0x4
	.byte	0x4
	.byte	0x4
	.byte	0x7
	.long	.Ldebug_macro11
	.byte	0x4
	.byte	0x3
	.uleb128 0x1b
	.uleb128 0x2
	.byte	0x5
	.uleb128 0x18
	.long	.LASF561
	.byte	0x3
	.uleb128 0x1b
	.uleb128 0xb
	.byte	0x7
	.long	.Ldebug_macro6
	.byte	0x4
	.byte	0x3
	.uleb128 0x1c
	.uleb128 0xc
	.byte	0x3
	.uleb128 0x13
	.uleb128 0xb
	.byte	0x7
	.long	.Ldebug_macro6
	.byte	0x4
	.byte	0x5
	.uleb128 0x1a
	.long	.LASF446
	.byte	0x4
	.byte	0x7
	.long	.Ldebug_macro12
	.file 17 "/usr/include/x86_64-linux-gnu/bits/typesizes.h"
	.byte	0x3
	.uleb128 0x8d
	.uleb128 0x11
	.byte	0x7
	.long	.Ldebug_macro13
	.byte	0x4
	.file 18 "/usr/include/x86_64-linux-gnu/bits/time64.h"
	.byte	0x3
	.uleb128 0x8e
	.uleb128 0x12
	.byte	0x7
	.long	.Ldebug_macro14
	.byte	0x4
	.byte	0x6
	.uleb128 0xe2
	.long	.LASF621
	.byte	0x4
	.file 19 "/usr/include/x86_64-linux-gnu/bits/wchar.h"
	.byte	0x3
	.uleb128 0x1c
	.uleb128 0x13
	.byte	0x7
	.long	.Ldebug_macro15
	.byte	0x4
	.byte	0x3
	.uleb128 0x1d
	.uleb128 0xb
	.byte	0x7
	.long	.Ldebug_macro6
	.byte	0x4
	.file 20 "/usr/include/x86_64-linux-gnu/bits/stdint-intn.h"
	.byte	0x3
	.uleb128 0x22
	.uleb128 0x14
	.byte	0x5
	.uleb128 0x14
	.long	.LASF625
	.byte	0x4
	.byte	0x3
	.uleb128 0x25
	.uleb128 0x3
	.byte	0x5
	.uleb128 0x14
	.long	.LASF626
	.byte	0x4
	.file 21 "/usr/include/x86_64-linux-gnu/bits/stdint-least.h"
	.byte	0x3
	.uleb128 0x29
	.uleb128 0x15
	.byte	0x5
	.uleb128 0x14
	.long	.LASF627
	.byte	0x4
	.byte	0x7
	.long	.Ldebug_macro16
	.byte	0x4
	.byte	0x5
	.uleb128 0xd
	.long	.LASF690
	.byte	0x4
	.byte	0x7
	.long	.Ldebug_macro17
	.byte	0x4
	.byte	0x4
	.byte	0
	.section	.debug_macro,"G",@progbits,wm4.0.ebeca98cfe8e2d20a725403d0664d717,comdat
.Ldebug_macro2:
	.value	0x5
	.byte	0
	.byte	0x5
	.uleb128 0
	.long	.LASF2
	.byte	0x5
	.uleb128 0
	.long	.LASF3
	.byte	0x5
	.uleb128 0
	.long	.LASF4
	.byte	0x5
	.uleb128 0
	.long	.LASF5
	.byte	0x5
	.uleb128 0
	.long	.LASF6
	.byte	0x5
	.uleb128 0
	.long	.LASF7
	.byte	0x5
	.uleb128 0
	.long	.LASF8
	.byte	0x5
	.uleb128 0
	.long	.LASF9
	.byte	0x5
	.uleb128 0
	.long	.LASF10
	.byte	0x5
	.uleb128 0
	.long	.LASF11
	.byte	0x5
	.uleb128 0
	.long	.LASF12
	.byte	0x5
	.uleb128 0
	.long	.LASF13
	.byte	0x5
	.uleb128 0
	.long	.LASF14
	.byte	0x5
	.uleb128 0
	.long	.LASF15
	.byte	0x5
	.uleb128 0
	.long	.LASF16
	.byte	0x5
	.uleb128 0
	.long	.LASF17
	.byte	0x5
	.uleb128 0
	.long	.LASF18
	.byte	0x5
	.uleb128 0
	.long	.LASF19
	.byte	0x5
	.uleb128 0
	.long	.LASF20
	.byte	0x5
	.uleb128 0
	.long	.LASF21
	.byte	0x5
	.uleb128 0
	.long	.LASF22
	.byte	0x5
	.uleb128 0
	.long	.LASF23
	.byte	0x5
	.uleb128 0
	.long	.LASF24
	.byte	0x5
	.uleb128 0
	.long	.LASF25
	.byte	0x5
	.uleb128 0
	.long	.LASF26
	.byte	0x5
	.uleb128 0
	.long	.LASF27
	.byte	0x5
	.uleb128 0
	.long	.LASF28
	.byte	0x5
	.uleb128 0
	.long	.LASF29
	.byte	0x5
	.uleb128 0
	.long	.LASF30
	.byte	0x5
	.uleb128 0
	.long	.LASF31
	.byte	0x5
	.uleb128 0
	.long	.LASF32
	.byte	0x5
	.uleb128 0
	.long	.LASF33
	.byte	0x5
	.uleb128 0
	.long	.LASF34
	.byte	0x5
	.uleb128 0
	.long	.LASF35
	.byte	0x5
	.uleb128 0
	.long	.LASF36
	.byte	0x5
	.uleb128 0
	.long	.LASF37
	.byte	0x5
	.uleb128 0
	.long	.LASF38
	.byte	0x5
	.uleb128 0
	.long	.LASF39
	.byte	0x5
	.uleb128 0
	.long	.LASF40
	.byte	0x5
	.uleb128 0
	.long	.LASF41
	.byte	0x5
	.uleb128 0
	.long	.LASF42
	.byte	0x5
	.uleb128 0
	.long	.LASF43
	.byte	0x5
	.uleb128 0
	.long	.LASF44
	.byte	0x5
	.uleb128 0
	.long	.LASF45
	.byte	0x5
	.uleb128 0
	.long	.LASF46
	.byte	0x5
	.uleb128 0
	.long	.LASF47
	.byte	0x5
	.uleb128 0
	.long	.LASF48
	.byte	0x5
	.uleb128 0
	.long	.LASF49
	.byte	0x5
	.uleb128 0
	.long	.LASF50
	.byte	0x5
	.uleb128 0
	.long	.LASF51
	.byte	0x5
	.uleb128 0
	.long	.LASF52
	.byte	0x5
	.uleb128 0
	.long	.LASF53
	.byte	0x5
	.uleb128 0
	.long	.LASF54
	.byte	0x5
	.uleb128 0
	.long	.LASF55
	.byte	0x5
	.uleb128 0
	.long	.LASF56
	.byte	0x5
	.uleb128 0
	.long	.LASF57
	.byte	0x5
	.uleb128 0
	.long	.LASF58
	.byte	0x5
	.uleb128 0
	.long	.LASF59
	.byte	0x5
	.uleb128 0
	.long	.LASF60
	.byte	0x5
	.uleb128 0
	.long	.LASF61
	.byte	0x5
	.uleb128 0
	.long	.LASF62
	.byte	0x5
	.uleb128 0
	.long	.LASF63
	.byte	0x5
	.uleb128 0
	.long	.LASF64
	.byte	0x5
	.uleb128 0
	.long	.LASF65
	.byte	0x5
	.uleb128 0
	.long	.LASF66
	.byte	0x5
	.uleb128 0
	.long	.LASF67
	.byte	0x5
	.uleb128 0
	.long	.LASF68
	.byte	0x5
	.uleb128 0
	.long	.LASF69
	.byte	0x5
	.uleb128 0
	.long	.LASF70
	.byte	0x5
	.uleb128 0
	.long	.LASF71
	.byte	0x5
	.uleb128 0
	.long	.LASF72
	.byte	0x5
	.uleb128 0
	.long	.LASF73
	.byte	0x5
	.uleb128 0
	.long	.LASF74
	.byte	0x5
	.uleb128 0
	.long	.LASF75
	.byte	0x5
	.uleb128 0
	.long	.LASF76
	.byte	0x5
	.uleb128 0
	.long	.LASF77
	.byte	0x5
	.uleb128 0
	.long	.LASF78
	.byte	0x5
	.uleb128 0
	.long	.LASF79
	.byte	0x5
	.uleb128 0
	.long	.LASF80
	.byte	0x5
	.uleb128 0
	.long	.LASF81
	.byte	0x5
	.uleb128 0
	.long	.LASF82
	.byte	0x5
	.uleb128 0
	.long	.LASF83
	.byte	0x5
	.uleb128 0
	.long	.LASF84
	.byte	0x5
	.uleb128 0
	.long	.LASF85
	.byte	0x5
	.uleb128 0
	.long	.LASF86
	.byte	0x5
	.uleb128 0
	.long	.LASF87
	.byte	0x5
	.uleb128 0
	.long	.LASF88
	.byte	0x5
	.uleb128 0
	.long	.LASF89
	.byte	0x5
	.uleb128 0
	.long	.LASF90
	.byte	0x5
	.uleb128 0
	.long	.LASF91
	.byte	0x5
	.uleb128 0
	.long	.LASF92
	.byte	0x5
	.uleb128 0
	.long	.LASF93
	.byte	0x5
	.uleb128 0
	.long	.LASF94
	.byte	0x5
	.uleb128 0
	.long	.LASF95
	.byte	0x5
	.uleb128 0
	.long	.LASF96
	.byte	0x5
	.uleb128 0
	.long	.LASF97
	.byte	0x5
	.uleb128 0
	.long	.LASF98
	.byte	0x5
	.uleb128 0
	.long	.LASF99
	.byte	0x5
	.uleb128 0
	.long	.LASF100
	.byte	0x5
	.uleb128 0
	.long	.LASF101
	.byte	0x5
	.uleb128 0
	.long	.LASF102
	.byte	0x5
	.uleb128 0
	.long	.LASF103
	.byte	0x5
	.uleb128 0
	.long	.LASF104
	.byte	0x5
	.uleb128 0
	.long	.LASF105
	.byte	0x5
	.uleb128 0
	.long	.LASF106
	.byte	0x5
	.uleb128 0
	.long	.LASF107
	.byte	0x5
	.uleb128 0
	.long	.LASF108
	.byte	0x5
	.uleb128 0
	.long	.LASF109
	.byte	0x5
	.uleb128 0
	.long	.LASF110
	.byte	0x5
	.uleb128 0
	.long	.LASF111
	.byte	0x5
	.uleb128 0
	.long	.LASF112
	.byte	0x5
	.uleb128 0
	.long	.LASF113
	.byte	0x5
	.uleb128 0
	.long	.LASF114
	.byte	0x5
	.uleb128 0
	.long	.LASF115
	.byte	0x5
	.uleb128 0
	.long	.LASF116
	.byte	0x5
	.uleb128 0
	.long	.LASF117
	.byte	0x5
	.uleb128 0
	.long	.LASF118
	.byte	0x5
	.uleb128 0
	.long	.LASF119
	.byte	0x5
	.uleb128 0
	.long	.LASF120
	.byte	0x5
	.uleb128 0
	.long	.LASF121
	.byte	0x5
	.uleb128 0
	.long	.LASF122
	.byte	0x5
	.uleb128 0
	.long	.LASF123
	.byte	0x5
	.uleb128 0
	.long	.LASF124
	.byte	0x5
	.uleb128 0
	.long	.LASF125
	.byte	0x5
	.uleb128 0
	.long	.LASF126
	.byte	0x5
	.uleb128 0
	.long	.LASF127
	.byte	0x5
	.uleb128 0
	.long	.LASF128
	.byte	0x5
	.uleb128 0
	.long	.LASF129
	.byte	0x5
	.uleb128 0
	.long	.LASF130
	.byte	0x5
	.uleb128 0
	.long	.LASF131
	.byte	0x5
	.uleb128 0
	.long	.LASF132
	.byte	0x5
	.uleb128 0
	.long	.LASF133
	.byte	0x5
	.uleb128 0
	.long	.LASF134
	.byte	0x5
	.uleb128 0
	.long	.LASF135
	.byte	0x5
	.uleb128 0
	.long	.LASF136
	.byte	0x5
	.uleb128 0
	.long	.LASF137
	.byte	0x5
	.uleb128 0
	.long	.LASF138
	.byte	0x5
	.uleb128 0
	.long	.LASF139
	.byte	0x5
	.uleb128 0
	.long	.LASF140
	.byte	0x5
	.uleb128 0
	.long	.LASF141
	.byte	0x5
	.uleb128 0
	.long	.LASF142
	.byte	0x5
	.uleb128 0
	.long	.LASF143
	.byte	0x5
	.uleb128 0
	.long	.LASF144
	.byte	0x5
	.uleb128 0
	.long	.LASF145
	.byte	0x5
	.uleb128 0
	.long	.LASF146
	.byte	0x5
	.uleb128 0
	.long	.LASF147
	.byte	0x5
	.uleb128 0
	.long	.LASF148
	.byte	0x5
	.uleb128 0
	.long	.LASF149
	.byte	0x5
	.uleb128 0
	.long	.LASF150
	.byte	0x5
	.uleb128 0
	.long	.LASF151
	.byte	0x5
	.uleb128 0
	.long	.LASF152
	.byte	0x5
	.uleb128 0
	.long	.LASF153
	.byte	0x5
	.uleb128 0
	.long	.LASF154
	.byte	0x5
	.uleb128 0
	.long	.LASF155
	.byte	0x5
	.uleb128 0
	.long	.LASF156
	.byte	0x5
	.uleb128 0
	.long	.LASF157
	.byte	0x5
	.uleb128 0
	.long	.LASF158
	.byte	0x5
	.uleb128 0
	.long	.LASF159
	.byte	0x5
	.uleb128 0
	.long	.LASF160
	.byte	0x5
	.uleb128 0
	.long	.LASF161
	.byte	0x5
	.uleb128 0
	.long	.LASF162
	.byte	0x5
	.uleb128 0
	.long	.LASF163
	.byte	0x5
	.uleb128 0
	.long	.LASF164
	.byte	0x5
	.uleb128 0
	.long	.LASF165
	.byte	0x5
	.uleb128 0
	.long	.LASF166
	.byte	0x5
	.uleb128 0
	.long	.LASF167
	.byte	0x5
	.uleb128 0
	.long	.LASF168
	.byte	0x5
	.uleb128 0
	.long	.LASF169
	.byte	0x5
	.uleb128 0
	.long	.LASF170
	.byte	0x5
	.uleb128 0
	.long	.LASF171
	.byte	0x5
	.uleb128 0
	.long	.LASF172
	.byte	0x5
	.uleb128 0
	.long	.LASF173
	.byte	0x5
	.uleb128 0
	.long	.LASF174
	.byte	0x5
	.uleb128 0
	.long	.LASF175
	.byte	0x5
	.uleb128 0
	.long	.LASF176
	.byte	0x5
	.uleb128 0
	.long	.LASF177
	.byte	0x5
	.uleb128 0
	.long	.LASF178
	.byte	0x5
	.uleb128 0
	.long	.LASF179
	.byte	0x5
	.uleb128 0
	.long	.LASF180
	.byte	0x5
	.uleb128 0
	.long	.LASF181
	.byte	0x5
	.uleb128 0
	.long	.LASF182
	.byte	0x5
	.uleb128 0
	.long	.LASF183
	.byte	0x5
	.uleb128 0
	.long	.LASF184
	.byte	0x5
	.uleb128 0
	.long	.LASF185
	.byte	0x5
	.uleb128 0
	.long	.LASF186
	.byte	0x5
	.uleb128 0
	.long	.LASF187
	.byte	0x5
	.uleb128 0
	.long	.LASF188
	.byte	0x5
	.uleb128 0
	.long	.LASF189
	.byte	0x5
	.uleb128 0
	.long	.LASF190
	.byte	0x5
	.uleb128 0
	.long	.LASF191
	.byte	0x5
	.uleb128 0
	.long	.LASF192
	.byte	0x5
	.uleb128 0
	.long	.LASF193
	.byte	0x5
	.uleb128 0
	.long	.LASF194
	.byte	0x5
	.uleb128 0
	.long	.LASF195
	.byte	0x5
	.uleb128 0
	.long	.LASF196
	.byte	0x5
	.uleb128 0
	.long	.LASF197
	.byte	0x5
	.uleb128 0
	.long	.LASF198
	.byte	0x5
	.uleb128 0
	.long	.LASF199
	.byte	0x5
	.uleb128 0
	.long	.LASF200
	.byte	0x5
	.uleb128 0
	.long	.LASF201
	.byte	0x5
	.uleb128 0
	.long	.LASF202
	.byte	0x5
	.uleb128 0
	.long	.LASF203
	.byte	0x5
	.uleb128 0
	.long	.LASF204
	.byte	0x5
	.uleb128 0
	.long	.LASF205
	.byte	0x5
	.uleb128 0
	.long	.LASF206
	.byte	0x5
	.uleb128 0
	.long	.LASF207
	.byte	0x5
	.uleb128 0
	.long	.LASF208
	.byte	0x5
	.uleb128 0
	.long	.LASF209
	.byte	0x5
	.uleb128 0
	.long	.LASF210
	.byte	0x5
	.uleb128 0
	.long	.LASF211
	.byte	0x5
	.uleb128 0
	.long	.LASF212
	.byte	0x5
	.uleb128 0
	.long	.LASF213
	.byte	0x5
	.uleb128 0
	.long	.LASF214
	.byte	0x5
	.uleb128 0
	.long	.LASF215
	.byte	0x5
	.uleb128 0
	.long	.LASF216
	.byte	0x5
	.uleb128 0
	.long	.LASF217
	.byte	0x5
	.uleb128 0
	.long	.LASF218
	.byte	0x5
	.uleb128 0
	.long	.LASF219
	.byte	0x5
	.uleb128 0
	.long	.LASF220
	.byte	0x5
	.uleb128 0
	.long	.LASF221
	.byte	0x5
	.uleb128 0
	.long	.LASF222
	.byte	0x5
	.uleb128 0
	.long	.LASF223
	.byte	0x5
	.uleb128 0
	.long	.LASF224
	.byte	0x5
	.uleb128 0
	.long	.LASF225
	.byte	0x5
	.uleb128 0
	.long	.LASF226
	.byte	0x5
	.uleb128 0
	.long	.LASF227
	.byte	0x5
	.uleb128 0
	.long	.LASF228
	.byte	0x5
	.uleb128 0
	.long	.LASF229
	.byte	0x5
	.uleb128 0
	.long	.LASF230
	.byte	0x5
	.uleb128 0
	.long	.LASF231
	.byte	0x5
	.uleb128 0
	.long	.LASF232
	.byte	0x5
	.uleb128 0
	.long	.LASF233
	.byte	0x5
	.uleb128 0
	.long	.LASF234
	.byte	0x5
	.uleb128 0
	.long	.LASF235
	.byte	0x5
	.uleb128 0
	.long	.LASF236
	.byte	0x5
	.uleb128 0
	.long	.LASF237
	.byte	0x5
	.uleb128 0
	.long	.LASF238
	.byte	0x5
	.uleb128 0
	.long	.LASF239
	.byte	0x5
	.uleb128 0
	.long	.LASF240
	.byte	0x5
	.uleb128 0
	.long	.LASF241
	.byte	0x5
	.uleb128 0
	.long	.LASF242
	.byte	0x5
	.uleb128 0
	.long	.LASF243
	.byte	0x5
	.uleb128 0
	.long	.LASF244
	.byte	0x5
	.uleb128 0
	.long	.LASF245
	.byte	0x5
	.uleb128 0
	.long	.LASF246
	.byte	0x5
	.uleb128 0
	.long	.LASF247
	.byte	0x5
	.uleb128 0
	.long	.LASF248
	.byte	0x5
	.uleb128 0
	.long	.LASF249
	.byte	0x5
	.uleb128 0
	.long	.LASF250
	.byte	0x5
	.uleb128 0
	.long	.LASF251
	.byte	0x5
	.uleb128 0
	.long	.LASF252
	.byte	0x5
	.uleb128 0
	.long	.LASF253
	.byte	0x5
	.uleb128 0
	.long	.LASF254
	.byte	0x5
	.uleb128 0
	.long	.LASF255
	.byte	0x5
	.uleb128 0
	.long	.LASF256
	.byte	0x5
	.uleb128 0
	.long	.LASF257
	.byte	0x5
	.uleb128 0
	.long	.LASF258
	.byte	0x5
	.uleb128 0
	.long	.LASF259
	.byte	0x5
	.uleb128 0
	.long	.LASF260
	.byte	0x5
	.uleb128 0
	.long	.LASF261
	.byte	0x5
	.uleb128 0
	.long	.LASF262
	.byte	0x5
	.uleb128 0
	.long	.LASF263
	.byte	0x5
	.uleb128 0
	.long	.LASF264
	.byte	0x5
	.uleb128 0
	.long	.LASF265
	.byte	0x5
	.uleb128 0
	.long	.LASF266
	.byte	0x5
	.uleb128 0
	.long	.LASF267
	.byte	0x5
	.uleb128 0
	.long	.LASF268
	.byte	0x5
	.uleb128 0
	.long	.LASF269
	.byte	0x5
	.uleb128 0
	.long	.LASF270
	.byte	0x5
	.uleb128 0
	.long	.LASF271
	.byte	0x5
	.uleb128 0
	.long	.LASF272
	.byte	0x5
	.uleb128 0
	.long	.LASF273
	.byte	0x5
	.uleb128 0
	.long	.LASF274
	.byte	0x5
	.uleb128 0
	.long	.LASF275
	.byte	0x5
	.uleb128 0
	.long	.LASF276
	.byte	0x5
	.uleb128 0
	.long	.LASF277
	.byte	0x5
	.uleb128 0
	.long	.LASF278
	.byte	0x5
	.uleb128 0
	.long	.LASF279
	.byte	0x5
	.uleb128 0
	.long	.LASF280
	.byte	0x5
	.uleb128 0
	.long	.LASF281
	.byte	0x5
	.uleb128 0
	.long	.LASF282
	.byte	0x5
	.uleb128 0
	.long	.LASF283
	.byte	0x5
	.uleb128 0
	.long	.LASF284
	.byte	0x5
	.uleb128 0
	.long	.LASF285
	.byte	0x5
	.uleb128 0
	.long	.LASF286
	.byte	0x5
	.uleb128 0
	.long	.LASF287
	.byte	0x5
	.uleb128 0
	.long	.LASF288
	.byte	0x5
	.uleb128 0
	.long	.LASF289
	.byte	0x5
	.uleb128 0
	.long	.LASF290
	.byte	0x5
	.uleb128 0
	.long	.LASF291
	.byte	0x5
	.uleb128 0
	.long	.LASF292
	.byte	0x5
	.uleb128 0
	.long	.LASF293
	.byte	0x5
	.uleb128 0
	.long	.LASF294
	.byte	0x5
	.uleb128 0
	.long	.LASF295
	.byte	0x5
	.uleb128 0
	.long	.LASF296
	.byte	0x5
	.uleb128 0
	.long	.LASF297
	.byte	0x5
	.uleb128 0
	.long	.LASF298
	.byte	0x5
	.uleb128 0
	.long	.LASF299
	.byte	0x5
	.uleb128 0
	.long	.LASF300
	.byte	0x5
	.uleb128 0
	.long	.LASF301
	.byte	0x5
	.uleb128 0
	.long	.LASF302
	.byte	0x5
	.uleb128 0
	.long	.LASF303
	.byte	0x5
	.uleb128 0
	.long	.LASF304
	.byte	0x5
	.uleb128 0
	.long	.LASF305
	.byte	0x5
	.uleb128 0
	.long	.LASF306
	.byte	0x5
	.uleb128 0
	.long	.LASF307
	.byte	0x5
	.uleb128 0
	.long	.LASF308
	.byte	0x5
	.uleb128 0
	.long	.LASF309
	.byte	0x5
	.uleb128 0
	.long	.LASF310
	.byte	0x5
	.uleb128 0
	.long	.LASF311
	.byte	0x5
	.uleb128 0
	.long	.LASF312
	.byte	0x5
	.uleb128 0
	.long	.LASF313
	.byte	0x5
	.uleb128 0
	.long	.LASF314
	.byte	0x5
	.uleb128 0
	.long	.LASF315
	.byte	0x5
	.uleb128 0
	.long	.LASF316
	.byte	0x5
	.uleb128 0
	.long	.LASF317
	.byte	0x5
	.uleb128 0
	.long	.LASF318
	.byte	0x5
	.uleb128 0
	.long	.LASF319
	.byte	0x5
	.uleb128 0
	.long	.LASF320
	.byte	0x5
	.uleb128 0
	.long	.LASF321
	.byte	0x5
	.uleb128 0
	.long	.LASF322
	.byte	0x5
	.uleb128 0
	.long	.LASF323
	.byte	0x5
	.uleb128 0
	.long	.LASF324
	.byte	0x5
	.uleb128 0
	.long	.LASF325
	.byte	0x5
	.uleb128 0
	.long	.LASF326
	.byte	0x5
	.uleb128 0
	.long	.LASF327
	.byte	0x5
	.uleb128 0
	.long	.LASF328
	.byte	0x5
	.uleb128 0
	.long	.LASF329
	.byte	0x5
	.uleb128 0
	.long	.LASF330
	.byte	0x5
	.uleb128 0
	.long	.LASF331
	.byte	0x5
	.uleb128 0
	.long	.LASF332
	.byte	0x5
	.uleb128 0
	.long	.LASF333
	.byte	0x5
	.uleb128 0
	.long	.LASF334
	.byte	0x5
	.uleb128 0
	.long	.LASF335
	.byte	0x5
	.uleb128 0
	.long	.LASF336
	.byte	0x5
	.uleb128 0
	.long	.LASF337
	.byte	0x5
	.uleb128 0
	.long	.LASF338
	.byte	0x5
	.uleb128 0
	.long	.LASF339
	.byte	0x5
	.uleb128 0
	.long	.LASF340
	.byte	0x5
	.uleb128 0
	.long	.LASF341
	.byte	0x5
	.uleb128 0
	.long	.LASF342
	.byte	0x5
	.uleb128 0
	.long	.LASF343
	.byte	0x5
	.uleb128 0
	.long	.LASF344
	.byte	0x5
	.uleb128 0
	.long	.LASF345
	.byte	0x5
	.uleb128 0
	.long	.LASF346
	.byte	0x5
	.uleb128 0
	.long	.LASF347
	.byte	0x5
	.uleb128 0
	.long	.LASF348
	.byte	0x5
	.uleb128 0
	.long	.LASF349
	.byte	0x5
	.uleb128 0
	.long	.LASF350
	.byte	0x5
	.uleb128 0
	.long	.LASF351
	.byte	0x5
	.uleb128 0
	.long	.LASF352
	.byte	0x5
	.uleb128 0
	.long	.LASF353
	.byte	0x5
	.uleb128 0
	.long	.LASF354
	.byte	0x5
	.uleb128 0
	.long	.LASF355
	.byte	0x5
	.uleb128 0
	.long	.LASF356
	.byte	0x5
	.uleb128 0
	.long	.LASF357
	.byte	0x5
	.uleb128 0
	.long	.LASF358
	.byte	0x5
	.uleb128 0
	.long	.LASF359
	.byte	0x5
	.uleb128 0
	.long	.LASF360
	.byte	0x5
	.uleb128 0
	.long	.LASF361
	.byte	0x5
	.uleb128 0
	.long	.LASF362
	.byte	0x5
	.uleb128 0
	.long	.LASF363
	.byte	0x5
	.uleb128 0
	.long	.LASF364
	.byte	0x5
	.uleb128 0
	.long	.LASF365
	.byte	0x5
	.uleb128 0
	.long	.LASF366
	.byte	0x5
	.uleb128 0
	.long	.LASF367
	.byte	0x5
	.uleb128 0
	.long	.LASF368
	.byte	0x5
	.uleb128 0
	.long	.LASF369
	.byte	0x5
	.uleb128 0
	.long	.LASF370
	.byte	0x5
	.uleb128 0
	.long	.LASF371
	.byte	0x5
	.uleb128 0
	.long	.LASF372
	.byte	0x5
	.uleb128 0
	.long	.LASF373
	.byte	0x5
	.uleb128 0
	.long	.LASF374
	.byte	0x5
	.uleb128 0
	.long	.LASF375
	.byte	0x5
	.uleb128 0
	.long	.LASF376
	.byte	0x5
	.uleb128 0
	.long	.LASF377
	.byte	0x5
	.uleb128 0
	.long	.LASF378
	.byte	0x5
	.uleb128 0
	.long	.LASF379
	.byte	0x5
	.uleb128 0
	.long	.LASF380
	.byte	0x5
	.uleb128 0
	.long	.LASF381
	.byte	0x5
	.uleb128 0
	.long	.LASF382
	.byte	0x5
	.uleb128 0
	.long	.LASF383
	.byte	0x5
	.uleb128 0
	.long	.LASF384
	.byte	0x5
	.uleb128 0
	.long	.LASF385
	.byte	0x5
	.uleb128 0
	.long	.LASF386
	.byte	0x5
	.uleb128 0
	.long	.LASF387
	.byte	0x5
	.uleb128 0
	.long	.LASF388
	.byte	0x5
	.uleb128 0
	.long	.LASF389
	.byte	0x5
	.uleb128 0
	.long	.LASF390
	.byte	0x5
	.uleb128 0
	.long	.LASF391
	.byte	0x5
	.uleb128 0
	.long	.LASF392
	.byte	0x5
	.uleb128 0
	.long	.LASF393
	.byte	0x5
	.uleb128 0
	.long	.LASF394
	.byte	0x5
	.uleb128 0
	.long	.LASF395
	.byte	0
	.section	.debug_macro,"G",@progbits,wm4.stdcpredef.h.19.88fdbfd5cf6f83ed579effc3e425f09b,comdat
.Ldebug_macro3:
	.value	0x5
	.byte	0
	.byte	0x5
	.uleb128 0x13
	.long	.LASF396
	.byte	0x5
	.uleb128 0x26
	.long	.LASF397
	.byte	0x5
	.uleb128 0x27
	.long	.LASF398
	.byte	0x5
	.uleb128 0x30
	.long	.LASF399
	.byte	0x5
	.uleb128 0x31
	.long	.LASF400
	.byte	0x5
	.uleb128 0x3e
	.long	.LASF401
	.byte	0
	.section	.debug_macro,"G",@progbits,wm4.stdint.h.23.022efde71688fcb285fe42cc87d41ee3,comdat
.Ldebug_macro4:
	.value	0x5
	.byte	0
	.byte	0x5
	.uleb128 0x17
	.long	.LASF403
	.byte	0x5
	.uleb128 0x19
	.long	.LASF404
	.byte	0
	.section	.debug_macro,"G",@progbits,wm4.features.h.19.06e7ac6100b1fe678c5861fb81337c83,comdat
.Ldebug_macro5:
	.value	0x5
	.byte	0
	.byte	0x5
	.uleb128 0x13
	.long	.LASF406
	.byte	0x6
	.uleb128 0x7f
	.long	.LASF407
	.byte	0x6
	.uleb128 0x80
	.long	.LASF408
	.byte	0x6
	.uleb128 0x81
	.long	.LASF409
	.byte	0x6
	.uleb128 0x82
	.long	.LASF410
	.byte	0x6
	.uleb128 0x83
	.long	.LASF411
	.byte	0x6
	.uleb128 0x84
	.long	.LASF412
	.byte	0x6
	.uleb128 0x85
	.long	.LASF413
	.byte	0x6
	.uleb128 0x86
	.long	.LASF414
	.byte	0x6
	.uleb128 0x87
	.long	.LASF415
	.byte	0x6
	.uleb128 0x88
	.long	.LASF416
	.byte	0x6
	.uleb128 0x89
	.long	.LASF417
	.byte	0x6
	.uleb128 0x8a
	.long	.LASF418
	.byte	0x6
	.uleb128 0x8b
	.long	.LASF419
	.byte	0x6
	.uleb128 0x8c
	.long	.LASF420
	.byte	0x6
	.uleb128 0x8d
	.long	.LASF421
	.byte	0x6
	.uleb128 0x8e
	.long	.LASF422
	.byte	0x6
	.uleb128 0x8f
	.long	.LASF423
	.byte	0x6
	.uleb128 0x90
	.long	.LASF424
	.byte	0x6
	.uleb128 0x91
	.long	.LASF425
	.byte	0x6
	.uleb128 0x92
	.long	.LASF426
	.byte	0x6
	.uleb128 0x93
	.long	.LASF427
	.byte	0x6
	.uleb128 0x94
	.long	.LASF428
	.byte	0x6
	.uleb128 0x95
	.long	.LASF429
	.byte	0x6
	.uleb128 0x96
	.long	.LASF430
	.byte	0x6
	.uleb128 0x97
	.long	.LASF431
	.byte	0x6
	.uleb128 0x98
	.long	.LASF432
	.byte	0x6
	.uleb128 0x99
	.long	.LASF433
	.byte	0x6
	.uleb128 0x9a
	.long	.LASF434
	.byte	0x5
	.uleb128 0x9f
	.long	.LASF435
	.byte	0x5
	.uleb128 0xaa
	.long	.LASF436
	.byte	0x5
	.uleb128 0xb8
	.long	.LASF437
	.byte	0x5
	.uleb128 0xbc
	.long	.LASF438
	.byte	0x5
	.uleb128 0xf6
	.long	.LASF439
	.byte	0x5
	.uleb128 0xfc
	.long	.LASF440
	.byte	0x5
	.uleb128 0x103
	.long	.LASF441
	.byte	0x5
	.uleb128 0x10a
	.long	.LASF442
	.byte	0
	.section	.debug_macro,"G",@progbits,wm4.wordsize.h.4.baf119258a1e53d8dba67ceac44ab6bc,comdat
.Ldebug_macro6:
	.value	0x5
	.byte	0
	.byte	0x5
	.uleb128 0x4
	.long	.LASF443
	.byte	0x5
	.uleb128 0xc
	.long	.LASF444
	.byte	0x5
	.uleb128 0xe
	.long	.LASF445
	.byte	0
	.section	.debug_macro,"G",@progbits,wm4.features.h.433.380585d4a93ff3a7e87b9e8dfed51e51,comdat
.Ldebug_macro7:
	.value	0x5
	.byte	0
	.byte	0x5
	.uleb128 0x1b1
	.long	.LASF447
	.byte	0x5
	.uleb128 0x1b9
	.long	.LASF448
	.byte	0x5
	.uleb128 0x1d0
	.long	.LASF449
	.byte	0x5
	.uleb128 0x1db
	.long	.LASF450
	.byte	0x6
	.uleb128 0x1e8
	.long	.LASF451
	.byte	0x5
	.uleb128 0x1e9
	.long	.LASF452
	.byte	0x5
	.uleb128 0x1ed
	.long	.LASF453
	.byte	0x5
	.uleb128 0x1ee
	.long	.LASF454
	.byte	0x5
	.uleb128 0x1f0
	.long	.LASF455
	.byte	0
	.section	.debug_macro,"G",@progbits,wm4.cdefs.h.20.f694b6e0e07f4f810454f41089e46f1d,comdat
.Ldebug_macro8:
	.value	0x5
	.byte	0
	.byte	0x5
	.uleb128 0x14
	.long	.LASF456
	.byte	0x2
	.uleb128 0x23
	.string	"__P"
	.byte	0x6
	.uleb128 0x24
	.long	.LASF457
	.byte	0x5
	.uleb128 0x2d
	.long	.LASF458
	.byte	0x5
	.uleb128 0x32
	.long	.LASF459
	.byte	0x5
	.uleb128 0x39
	.long	.LASF460
	.byte	0x5
	.uleb128 0x41
	.long	.LASF461
	.byte	0x5
	.uleb128 0x42
	.long	.LASF462
	.byte	0x5
	.uleb128 0x4f
	.long	.LASF463
	.byte	0x5
	.uleb128 0x50
	.long	.LASF464
	.byte	0x5
	.uleb128 0x51
	.long	.LASF465
	.byte	0x5
	.uleb128 0x52
	.long	.LASF466
	.byte	0x5
	.uleb128 0x66
	.long	.LASF467
	.byte	0x5
	.uleb128 0x7d
	.long	.LASF468
	.byte	0x5
	.uleb128 0x7e
	.long	.LASF469
	.byte	0x5
	.uleb128 0x83
	.long	.LASF470
	.byte	0x5
	.uleb128 0x84
	.long	.LASF471
	.byte	0x5
	.uleb128 0x87
	.long	.LASF472
	.byte	0x5
	.uleb128 0x8f
	.long	.LASF473
	.byte	0x5
	.uleb128 0x90
	.long	.LASF474
	.byte	0x5
	.uleb128 0x95
	.long	.LASF475
	.byte	0x5
	.uleb128 0x96
	.long	.LASF476
	.byte	0x5
	.uleb128 0x9e
	.long	.LASF477
	.byte	0x5
	.uleb128 0x9f
	.long	.LASF478
	.byte	0x5
	.uleb128 0xd4
	.long	.LASF479
	.byte	0x5
	.uleb128 0xd5
	.long	.LASF480
	.byte	0x5
	.uleb128 0xe1
	.long	.LASF481
	.byte	0x5
	.uleb128 0xe2
	.long	.LASF482
	.byte	0x5
	.uleb128 0x100
	.long	.LASF483
	.byte	0x5
	.uleb128 0x107
	.long	.LASF484
	.byte	0x5
	.uleb128 0x109
	.long	.LASF485
	.byte	0x5
	.uleb128 0x10c
	.long	.LASF486
	.byte	0x5
	.uleb128 0x10d
	.long	.LASF487
	.byte	0x5
	.uleb128 0x110
	.long	.LASF488
	.byte	0x5
	.uleb128 0x114
	.long	.LASF489
	.byte	0x5
	.uleb128 0x12a
	.long	.LASF490
	.byte	0x5
	.uleb128 0x132
	.long	.LASF491
	.byte	0x5
	.uleb128 0x13b
	.long	.LASF492
	.byte	0x5
	.uleb128 0x145
	.long	.LASF493
	.byte	0x5
	.uleb128 0x14c
	.long	.LASF494
	.byte	0x5
	.uleb128 0x152
	.long	.LASF495
	.byte	0x5
	.uleb128 0x15b
	.long	.LASF496
	.byte	0x5
	.uleb128 0x15c
	.long	.LASF497
	.byte	0x5
	.uleb128 0x164
	.long	.LASF498
	.byte	0x5
	.uleb128 0x16e
	.long	.LASF499
	.byte	0x5
	.uleb128 0x17b
	.long	.LASF500
	.byte	0x5
	.uleb128 0x185
	.long	.LASF501
	.byte	0x5
	.uleb128 0x191
	.long	.LASF502
	.byte	0x5
	.uleb128 0x197
	.long	.LASF503
	.byte	0x5
	.uleb128 0x19e
	.long	.LASF504
	.byte	0x5
	.uleb128 0x1a7
	.long	.LASF505
	.byte	0x5
	.uleb128 0x1b0
	.long	.LASF506
	.byte	0x6
	.uleb128 0x1b8
	.long	.LASF507
	.byte	0x5
	.uleb128 0x1b9
	.long	.LASF508
	.byte	0x5
	.uleb128 0x1c2
	.long	.LASF509
	.byte	0x5
	.uleb128 0x1d4
	.long	.LASF510
	.byte	0x5
	.uleb128 0x1d5
	.long	.LASF511
	.byte	0x5
	.uleb128 0x1de
	.long	.LASF512
	.byte	0x5
	.uleb128 0x1e4
	.long	.LASF513
	.byte	0x5
	.uleb128 0x1e5
	.long	.LASF514
	.byte	0x5
	.uleb128 0x200
	.long	.LASF515
	.byte	0x5
	.uleb128 0x20f
	.long	.LASF516
	.byte	0x5
	.uleb128 0x210
	.long	.LASF517
	.byte	0x5
	.uleb128 0x225
	.long	.LASF518
	.byte	0x6
	.uleb128 0x22b
	.long	.LASF519
	.byte	0x5
	.uleb128 0x22f
	.long	.LASF520
	.byte	0
	.section	.debug_macro,"G",@progbits,wm4.cdefs.h.634.2705079e46e1467ce61c80ea50627a2a,comdat
.Ldebug_macro9:
	.value	0x5
	.byte	0
	.byte	0x5
	.uleb128 0x27a
	.long	.LASF522
	.byte	0x5
	.uleb128 0x27b
	.long	.LASF523
	.byte	0x5
	.uleb128 0x27c
	.long	.LASF524
	.byte	0x5
	.uleb128 0x27d
	.long	.LASF525
	.byte	0x5
	.uleb128 0x27e
	.long	.LASF526
	.byte	0x5
	.uleb128 0x27f
	.long	.LASF527
	.byte	0x5
	.uleb128 0x281
	.long	.LASF528
	.byte	0x5
	.uleb128 0x282
	.long	.LASF529
	.byte	0x5
	.uleb128 0x28d
	.long	.LASF530
	.byte	0x5
	.uleb128 0x28e
	.long	.LASF531
	.byte	0x5
	.uleb128 0x2a0
	.long	.LASF532
	.byte	0x5
	.uleb128 0x2ab
	.long	.LASF533
	.byte	0x5
	.uleb128 0x2b3
	.long	.LASF534
	.byte	0x5
	.uleb128 0x2b6
	.long	.LASF535
	.byte	0x5
	.uleb128 0x2c3
	.long	.LASF536
	.byte	0x5
	.uleb128 0x2c5
	.long	.LASF537
	.byte	0x5
	.uleb128 0x2ce
	.long	.LASF538
	.byte	0
	.section	.debug_macro,"G",@progbits,wm4.stubs64.h.10.7865f4f7062bab1c535c1f73f43aa9b9,comdat
.Ldebug_macro10:
	.value	0x5
	.byte	0
	.byte	0x5
	.uleb128 0xa
	.long	.LASF539
	.byte	0x5
	.uleb128 0xb
	.long	.LASF540
	.byte	0x5
	.uleb128 0xc
	.long	.LASF541
	.byte	0x5
	.uleb128 0xd
	.long	.LASF542
	.byte	0x5
	.uleb128 0xe
	.long	.LASF543
	.byte	0x5
	.uleb128 0xf
	.long	.LASF544
	.byte	0x5
	.uleb128 0x10
	.long	.LASF545
	.byte	0x5
	.uleb128 0x11
	.long	.LASF546
	.byte	0
	.section	.debug_macro,"G",@progbits,wm4.libcheaderstart.h.37.e7d4b6f4649b40d3e0dce357ae78234f,comdat
.Ldebug_macro11:
	.value	0x5
	.byte	0
	.byte	0x6
	.uleb128 0x25
	.long	.LASF547
	.byte	0x5
	.uleb128 0x2a
	.long	.LASF548
	.byte	0x6
	.uleb128 0x43
	.long	.LASF549
	.byte	0x5
	.uleb128 0x47
	.long	.LASF550
	.byte	0x6
	.uleb128 0x49
	.long	.LASF551
	.byte	0x5
	.uleb128 0x4d
	.long	.LASF552
	.byte	0x6
	.uleb128 0x4f
	.long	.LASF553
	.byte	0x5
	.uleb128 0x53
	.long	.LASF554
	.byte	0x6
	.uleb128 0x5a
	.long	.LASF555
	.byte	0x5
	.uleb128 0x5e
	.long	.LASF556
	.byte	0x6
	.uleb128 0x60
	.long	.LASF557
	.byte	0x5
	.uleb128 0x64
	.long	.LASF558
	.byte	0x6
	.uleb128 0x69
	.long	.LASF559
	.byte	0x5
	.uleb128 0x6d
	.long	.LASF560
	.byte	0
	.section	.debug_macro,"G",@progbits,wm4.types.h.109.56eb9ae966b255288cc544f18746a7ff,comdat
.Ldebug_macro12:
	.value	0x5
	.byte	0
	.byte	0x5
	.uleb128 0x6d
	.long	.LASF562
	.byte	0x5
	.uleb128 0x6e
	.long	.LASF563
	.byte	0x5
	.uleb128 0x6f
	.long	.LASF564
	.byte	0x5
	.uleb128 0x70
	.long	.LASF565
	.byte	0x5
	.uleb128 0x71
	.long	.LASF566
	.byte	0x5
	.uleb128 0x72
	.long	.LASF567
	.byte	0x5
	.uleb128 0x80
	.long	.LASF568
	.byte	0x5
	.uleb128 0x81
	.long	.LASF569
	.byte	0x5
	.uleb128 0x82
	.long	.LASF570
	.byte	0x5
	.uleb128 0x83
	.long	.LASF571
	.byte	0x5
	.uleb128 0x84
	.long	.LASF572
	.byte	0x5
	.uleb128 0x85
	.long	.LASF573
	.byte	0x5
	.uleb128 0x86
	.long	.LASF574
	.byte	0x5
	.uleb128 0x87
	.long	.LASF575
	.byte	0x5
	.uleb128 0x89
	.long	.LASF576
	.byte	0
	.section	.debug_macro,"G",@progbits,wm4.typesizes.h.24.ccf5919b8e01b553263cf8f4ab1d5fde,comdat
.Ldebug_macro13:
	.value	0x5
	.byte	0
	.byte	0x5
	.uleb128 0x18
	.long	.LASF577
	.byte	0x5
	.uleb128 0x22
	.long	.LASF578
	.byte	0x5
	.uleb128 0x23
	.long	.LASF579
	.byte	0x5
	.uleb128 0x26
	.long	.LASF580
	.byte	0x5
	.uleb128 0x27
	.long	.LASF581
	.byte	0x5
	.uleb128 0x28
	.long	.LASF582
	.byte	0x5
	.uleb128 0x29
	.long	.LASF583
	.byte	0x5
	.uleb128 0x2a
	.long	.LASF584
	.byte	0x5
	.uleb128 0x2b
	.long	.LASF585
	.byte	0x5
	.uleb128 0x2d
	.long	.LASF586
	.byte	0x5
	.uleb128 0x2e
	.long	.LASF587
	.byte	0x5
	.uleb128 0x33
	.long	.LASF588
	.byte	0x5
	.uleb128 0x34
	.long	.LASF589
	.byte	0x5
	.uleb128 0x35
	.long	.LASF590
	.byte	0x5
	.uleb128 0x36
	.long	.LASF591
	.byte	0x5
	.uleb128 0x37
	.long	.LASF592
	.byte	0x5
	.uleb128 0x38
	.long	.LASF593
	.byte	0x5
	.uleb128 0x39
	.long	.LASF594
	.byte	0x5
	.uleb128 0x3a
	.long	.LASF595
	.byte	0x5
	.uleb128 0x3b
	.long	.LASF596
	.byte	0x5
	.uleb128 0x3c
	.long	.LASF597
	.byte	0x5
	.uleb128 0x3d
	.long	.LASF598
	.byte	0x5
	.uleb128 0x3e
	.long	.LASF599
	.byte	0x5
	.uleb128 0x3f
	.long	.LASF600
	.byte	0x5
	.uleb128 0x40
	.long	.LASF601
	.byte	0x5
	.uleb128 0x41
	.long	.LASF602
	.byte	0x5
	.uleb128 0x42
	.long	.LASF603
	.byte	0x5
	.uleb128 0x43
	.long	.LASF604
	.byte	0x5
	.uleb128 0x44
	.long	.LASF605
	.byte	0x5
	.uleb128 0x45
	.long	.LASF606
	.byte	0x5
	.uleb128 0x46
	.long	.LASF607
	.byte	0x5
	.uleb128 0x47
	.long	.LASF608
	.byte	0x5
	.uleb128 0x48
	.long	.LASF609
	.byte	0x5
	.uleb128 0x49
	.long	.LASF610
	.byte	0x5
	.uleb128 0x4a
	.long	.LASF611
	.byte	0x5
	.uleb128 0x4b
	.long	.LASF612
	.byte	0x5
	.uleb128 0x51
	.long	.LASF613
	.byte	0x5
	.uleb128 0x54
	.long	.LASF614
	.byte	0x5
	.uleb128 0x57
	.long	.LASF615
	.byte	0x5
	.uleb128 0x5a
	.long	.LASF616
	.byte	0x5
	.uleb128 0x5d
	.long	.LASF617
	.byte	0x5
	.uleb128 0x67
	.long	.LASF618
	.byte	0
	.section	.debug_macro,"G",@progbits,wm4.time64.h.24.a8166ae916ec910dab0d8987098d42ee,comdat
.Ldebug_macro14:
	.value	0x5
	.byte	0
	.byte	0x5
	.uleb128 0x18
	.long	.LASF619
	.byte	0x5
	.uleb128 0x1e
	.long	.LASF620
	.byte	0
	.section	.debug_macro,"G",@progbits,wm4.wchar.h.20.510818a05484290d697a517509bf4b2d,comdat
.Ldebug_macro15:
	.value	0x5
	.byte	0
	.byte	0x5
	.uleb128 0x14
	.long	.LASF622
	.byte	0x5
	.uleb128 0x22
	.long	.LASF623
	.byte	0x5
	.uleb128 0x2a
	.long	.LASF624
	.byte	0
	.section	.debug_macro,"G",@progbits,wm4.stdint.h.77.2c483fb59bdff411b77616f53b8efe8b,comdat
.Ldebug_macro16:
	.value	0x5
	.byte	0
	.byte	0x5
	.uleb128 0x4d
	.long	.LASF628
	.byte	0x5
	.uleb128 0x5f
	.long	.LASF124
	.byte	0x5
	.uleb128 0x60
	.long	.LASF133
	.byte	0x5
	.uleb128 0x69
	.long	.LASF629
	.byte	0x5
	.uleb128 0x6a
	.long	.LASF630
	.byte	0x5
	.uleb128 0x6b
	.long	.LASF631
	.byte	0x5
	.uleb128 0x6c
	.long	.LASF632
	.byte	0x5
	.uleb128 0x6e
	.long	.LASF633
	.byte	0x5
	.uleb128 0x6f
	.long	.LASF634
	.byte	0x5
	.uleb128 0x70
	.long	.LASF635
	.byte	0x5
	.uleb128 0x71
	.long	.LASF636
	.byte	0x5
	.uleb128 0x74
	.long	.LASF637
	.byte	0x5
	.uleb128 0x75
	.long	.LASF638
	.byte	0x5
	.uleb128 0x76
	.long	.LASF639
	.byte	0x5
	.uleb128 0x77
	.long	.LASF640
	.byte	0x5
	.uleb128 0x7b
	.long	.LASF641
	.byte	0x5
	.uleb128 0x7c
	.long	.LASF642
	.byte	0x5
	.uleb128 0x7d
	.long	.LASF643
	.byte	0x5
	.uleb128 0x7e
	.long	.LASF644
	.byte	0x5
	.uleb128 0x80
	.long	.LASF645
	.byte	0x5
	.uleb128 0x81
	.long	.LASF646
	.byte	0x5
	.uleb128 0x82
	.long	.LASF647
	.byte	0x5
	.uleb128 0x83
	.long	.LASF648
	.byte	0x5
	.uleb128 0x86
	.long	.LASF649
	.byte	0x5
	.uleb128 0x87
	.long	.LASF650
	.byte	0x5
	.uleb128 0x88
	.long	.LASF651
	.byte	0x5
	.uleb128 0x89
	.long	.LASF652
	.byte	0x5
	.uleb128 0x8d
	.long	.LASF653
	.byte	0x5
	.uleb128 0x8f
	.long	.LASF654
	.byte	0x5
	.uleb128 0x90
	.long	.LASF655
	.byte	0x5
	.uleb128 0x95
	.long	.LASF656
	.byte	0x5
	.uleb128 0x97
	.long	.LASF657
	.byte	0x5
	.uleb128 0x99
	.long	.LASF658
	.byte	0x5
	.uleb128 0x9a
	.long	.LASF659
	.byte	0x5
	.uleb128 0x9f
	.long	.LASF660
	.byte	0x5
	.uleb128 0xa2
	.long	.LASF661
	.byte	0x5
	.uleb128 0xa4
	.long	.LASF662
	.byte	0x5
	.uleb128 0xa5
	.long	.LASF663
	.byte	0x5
	.uleb128 0xaa
	.long	.LASF664
	.byte	0x5
	.uleb128 0xaf
	.long	.LASF665
	.byte	0x5
	.uleb128 0xb0
	.long	.LASF666
	.byte	0x5
	.uleb128 0xb1
	.long	.LASF667
	.byte	0x5
	.uleb128 0xba
	.long	.LASF668
	.byte	0x5
	.uleb128 0xbc
	.long	.LASF669
	.byte	0x5
	.uleb128 0xbf
	.long	.LASF670
	.byte	0x5
	.uleb128 0xc6
	.long	.LASF671
	.byte	0x5
	.uleb128 0xc7
	.long	.LASF672
	.byte	0x5
	.uleb128 0xd3
	.long	.LASF673
	.byte	0x5
	.uleb128 0xd4
	.long	.LASF674
	.byte	0x5
	.uleb128 0xd8
	.long	.LASF675
	.byte	0x5
	.uleb128 0xe4
	.long	.LASF676
	.byte	0x5
	.uleb128 0xe5
	.long	.LASF677
	.byte	0x5
	.uleb128 0xe9
	.long	.LASF678
	.byte	0x5
	.uleb128 0xea
	.long	.LASF679
	.byte	0x5
	.uleb128 0xed
	.long	.LASF680
	.byte	0x5
	.uleb128 0xee
	.long	.LASF681
	.byte	0x5
	.uleb128 0xef
	.long	.LASF682
	.byte	0x5
	.uleb128 0xf1
	.long	.LASF683
	.byte	0x5
	.uleb128 0xf7
	.long	.LASF684
	.byte	0x5
	.uleb128 0xf8
	.long	.LASF685
	.byte	0x5
	.uleb128 0xf9
	.long	.LASF686
	.byte	0x5
	.uleb128 0xfb
	.long	.LASF687
	.byte	0x5
	.uleb128 0x102
	.long	.LASF688
	.byte	0x5
	.uleb128 0x103
	.long	.LASF689
	.byte	0
	.section	.debug_macro,"G",@progbits,wm4.osdemo.h.4.a92382e10ee31744a93652e1195fbd0b,comdat
.Ldebug_macro17:
	.value	0x5
	.byte	0
	.byte	0x5
	.uleb128 0x4
	.long	.LASF691
	.byte	0x5
	.uleb128 0x5
	.long	.LASF692
	.byte	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF212:
	.string	"__FLT16_NORM_MAX__ 6.55040000000000000000000000000000000e+4F16"
.LASF103:
	.string	"__SIG_ATOMIC_MAX__ 0x7fffffff"
.LASF250:
	.string	"__FLT64_HAS_QUIET_NAN__ 1"
.LASF634:
	.string	"INT16_MAX (32767)"
.LASF195:
	.string	"__LDBL_MAX__ 1.18973149535723176502126385303097021e+4932L"
.LASF207:
	.string	"__FLT16_MIN_10_EXP__ (-4)"
.LASF233:
	.string	"__FLT32_HAS_INFINITY__ 1"
.LASF240:
	.string	"__FLT64_MAX_EXP__ 1024"
.LASF158:
	.string	"__FLT_MIN_10_EXP__ (-37)"
.LASF403:
	.string	"_STDINT_H 1"
.LASF294:
	.string	"__FLT64X_EPSILON__ 1.08420217248550443400745280086994171e-19F64x"
.LASF308:
	.string	"__BFLT16_NORM_MAX__ 3.38953138925153547590470800371487867e+38BF16"
.LASF303:
	.string	"__BFLT16_MIN_10_EXP__ (-37)"
.LASF355:
	.string	"__GCC_ATOMIC_TEST_AND_SET_TRUEVAL 1"
.LASF328:
	.string	"__DEC64_EPSILON__ 1E-15DD"
.LASF182:
	.string	"__DBL_DENORM_MIN__ ((double)4.94065645841246544176568792868221372e-324L)"
.LASF367:
	.string	"__amd64 1"
.LASF146:
	.string	"__INTPTR_MAX__ 0x7fffffffffffffffL"
.LASF481:
	.string	"__flexarr []"
.LASF220:
	.string	"__FLT32_MANT_DIG__ 24"
.LASF553:
	.string	"__GLIBC_USE_IEC_60559_EXT"
.LASF345:
	.string	"__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8 1"
.LASF509:
	.string	"__attribute_artificial__ __attribute__ ((__artificial__))"
.LASF440:
	.string	"__USE_ISOC11 1"
.LASF673:
	.string	"SIG_ATOMIC_MIN (-2147483647-1)"
.LASF667:
	.string	"UINTPTR_MAX (18446744073709551615UL)"
.LASF14:
	.string	"__ATOMIC_RELEASE 3"
.LASF537:
	.string	"__attr_dealloc_free __attr_dealloc (__builtin_free, 1)"
.LASF208:
	.string	"__FLT16_MAX_EXP__ 16"
.LASF274:
	.string	"__FLT32X_DECIMAL_DIG__ 17"
.LASF377:
	.string	"__k8__ 1"
.LASF173:
	.string	"__DBL_MIN_EXP__ (-1021)"
.LASF140:
	.string	"__INT_FAST64_MAX__ 0x7fffffffffffffffL"
.LASF348:
	.string	"__GCC_ATOMIC_CHAR16_T_LOCK_FREE 2"
.LASF632:
	.string	"INT64_MIN (-__INT64_C(9223372036854775807)-1)"
.LASF456:
	.string	"_SYS_CDEFS_H 1"
.LASF275:
	.string	"__FLT32X_MAX__ 1.79769313486231570814527423731704357e+308F32x"
.LASF516:
	.string	"__glibc_unlikely(cond) __builtin_expect ((cond), 0)"
.LASF365:
	.string	"__SIZEOF_WINT_T__ 4"
.LASF436:
	.string	"__GNUC_PREREQ(maj,min) ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))"
.LASF260:
	.string	"__FLT128_NORM_MAX__ 1.18973149535723176508575932662800702e+4932F128"
.LASF514:
	.string	"__va_arg_pack_len() __builtin_va_arg_pack_len ()"
.LASF272:
	.string	"__FLT32X_MAX_EXP__ 1024"
.LASF494:
	.string	"__attribute_const__ __attribute__ ((__const__))"
.LASF432:
	.string	"__GLIBC_USE_DEPRECATED_GETS"
.LASF228:
	.string	"__FLT32_NORM_MAX__ 3.40282346638528859811704183484516925e+38F32"
.LASF423:
	.string	"__USE_LARGEFILE64"
.LASF551:
	.string	"__GLIBC_USE_IEC_60559_BFP_EXT_C2X"
.LASF49:
	.string	"__CHAR32_TYPE__ unsigned int"
.LASF695:
	.string	"unsigned int"
.LASF373:
	.string	"__ATOMIC_HLE_ACQUIRE 65536"
.LASF640:
	.string	"UINT64_MAX (__UINT64_C(18446744073709551615))"
.LASF245:
	.string	"__FLT64_MIN__ 2.22507385850720138309023271733240406e-308F64"
.LASF93:
	.string	"__LONG_LONG_WIDTH__ 64"
.LASF470:
	.string	"__CONCAT(x,y) x ## y"
.LASF166:
	.string	"__FLT_DENORM_MIN__ 1.40129846432481707092372958328991613e-45F"
.LASF302:
	.string	"__BFLT16_MIN_EXP__ (-125)"
.LASF31:
	.string	"__SIZEOF_SIZE_T__ 8"
.LASF626:
	.string	"_BITS_STDINT_UINTN_H 1"
.LASF479:
	.string	"__warnattr(msg) __attribute__((__warning__ (msg)))"
.LASF35:
	.string	"__ORDER_BIG_ENDIAN__ 4321"
.LASF357:
	.string	"__GCC_CONSTRUCTIVE_SIZE 64"
.LASF333:
	.string	"__DEC128_MIN__ 1E-6143DL"
.LASF608:
	.string	"__TIMER_T_TYPE void *"
.LASF133:
	.string	"__UINT64_C(c) c ## UL"
.LASF451:
	.string	"__GNU_LIBRARY__"
.LASF171:
	.string	"__DBL_MANT_DIG__ 53"
.LASF169:
	.string	"__FLT_HAS_QUIET_NAN__ 1"
.LASF428:
	.string	"__USE_GNU"
.LASF540:
	.string	"__stub_chflags "
.LASF80:
	.string	"__INT_MAX__ 0x7fffffff"
.LASF170:
	.string	"__FLT_IS_IEC_60559__ 1"
.LASF466:
	.string	"__NTHNL(fct) __attribute__ ((__nothrow__)) fct"
.LASF205:
	.string	"__FLT16_DIG__ 3"
.LASF541:
	.string	"__stub_fchflags "
.LASF645:
	.string	"INT_LEAST8_MAX (127)"
.LASF52:
	.string	"__INT16_TYPE__ short int"
.LASF219:
	.string	"__FLT16_IS_IEC_60559__ 1"
.LASF321:
	.string	"__DEC32_EPSILON__ 1E-6DF"
.LASF311:
	.string	"__BFLT16_DENORM_MIN__ 9.18354961579912115600575419704879436e-41BF16"
.LASF477:
	.string	"__glibc_objsize0(__o) __bos0 (__o)"
.LASF188:
	.string	"__LDBL_DIG__ 18"
.LASF288:
	.string	"__FLT64X_MAX_EXP__ 16384"
.LASF12:
	.string	"__ATOMIC_SEQ_CST 5"
.LASF507:
	.string	"__always_inline"
.LASF442:
	.string	"__USE_ISOC95 1"
.LASF620:
	.string	"__TIME64_T_TYPE __TIME_T_TYPE"
.LASF27:
	.string	"__SIZEOF_SHORT__ 2"
.LASF558:
	.string	"__GLIBC_USE_IEC_60559_FUNCS_EXT_C2X 0"
.LASF58:
	.string	"__UINT64_TYPE__ long unsigned int"
.LASF138:
	.string	"__INT_FAST32_MAX__ 0x7fffffffffffffffL"
.LASF59:
	.string	"__INT_LEAST8_TYPE__ signed char"
.LASF643:
	.string	"INT_LEAST32_MIN (-2147483647-1)"
.LASF629:
	.string	"INT8_MIN (-128)"
.LASF65:
	.string	"__UINT_LEAST32_TYPE__ unsigned int"
.LASF545:
	.string	"__stub_sigreturn "
.LASF472:
	.string	"__ptr_t void *"
.LASF564:
	.string	"__S32_TYPE int"
.LASF73:
	.string	"__UINT_FAST32_TYPE__ long unsigned int"
.LASF37:
	.string	"__BYTE_ORDER__ __ORDER_LITTLE_ENDIAN__"
.LASF197:
	.string	"__LDBL_MIN__ 3.36210314311209350626267781732175260e-4932L"
.LASF223:
	.string	"__FLT32_MIN_10_EXP__ (-37)"
.LASF183:
	.string	"__DBL_HAS_DENORM__ 1"
.LASF62:
	.string	"__INT_LEAST64_TYPE__ long int"
.LASF84:
	.string	"__WCHAR_MIN__ (-__WCHAR_MAX__ - 1)"
.LASF650:
	.string	"UINT_LEAST16_MAX (65535)"
.LASF5:
	.string	"__STDC_UTF_32__ 1"
.LASF402:
	.string	"OS_DEMO_H "
.LASF495:
	.string	"__attribute_maybe_unused__ __attribute__ ((__unused__))"
.LASF117:
	.string	"__INT_LEAST16_MAX__ 0x7fff"
.LASF527:
	.string	"__LDBL_REDIR_DECL(name) "
.LASF425:
	.string	"__USE_MISC"
.LASF78:
	.string	"__SCHAR_MAX__ 0x7f"
.LASF40:
	.string	"__GNUC_EXECUTION_CHARSET_NAME \"UTF-8\""
.LASF511:
	.string	"__extern_always_inline extern __always_inline __attribute__ ((__gnu_inline__))"
.LASF237:
	.string	"__FLT64_DIG__ 15"
.LASF67:
	.string	"__INT_FAST8_TYPE__ signed char"
.LASF301:
	.string	"__BFLT16_DIG__ 2"
.LASF203:
	.string	"__LDBL_IS_IEC_60559__ 1"
.LASF559:
	.string	"__GLIBC_USE_IEC_60559_TYPES_EXT"
.LASF672:
	.string	"PTRDIFF_MAX (9223372036854775807L)"
.LASF281:
	.string	"__FLT32X_HAS_INFINITY__ 1"
.LASF417:
	.string	"__USE_UNIX98"
.LASF265:
	.string	"__FLT128_HAS_INFINITY__ 1"
.LASF639:
	.string	"UINT32_MAX (4294967295U)"
.LASF685:
	.string	"UINT16_C(c) c"
.LASF385:
	.string	"__MMX_WITH_SSE__ 1"
.LASF63:
	.string	"__UINT_LEAST8_TYPE__ unsigned char"
.LASF363:
	.string	"__SIZEOF_INT128__ 16"
.LASF465:
	.string	"__NTH(fct) __attribute__ ((__nothrow__ __LEAF)) fct"
.LASF647:
	.string	"INT_LEAST32_MAX (2147483647)"
.LASF531:
	.string	"__glibc_macro_warning(message) __glibc_macro_warning1 (GCC warning message)"
.LASF681:
	.string	"INT16_C(c) c"
.LASF266:
	.string	"__FLT128_HAS_QUIET_NAN__ 1"
.LASF259:
	.string	"__FLT128_MAX__ 1.18973149535723176508575932662800702e+4932F128"
.LASF100:
	.string	"__UINTMAX_MAX__ 0xffffffffffffffffUL"
.LASF410:
	.string	"__USE_ISOCXX11"
.LASF61:
	.string	"__INT_LEAST32_TYPE__ int"
.LASF119:
	.string	"__INT_LEAST16_WIDTH__ 16"
.LASF520:
	.string	"__attribute_copy__(arg) __attribute__ ((__copy__ (arg)))"
.LASF536:
	.string	"__attr_dealloc(dealloc,argno) __attribute__ ((__malloc__ (dealloc, argno)))"
.LASF690:
	.string	"_GCC_WRAP_STDINT_H "
.LASF651:
	.string	"UINT_LEAST32_MAX (4294967295U)"
.LASF458:
	.string	"__glibc_has_attribute(attr) __has_attribute (attr)"
.LASF663:
	.string	"UINT_FAST32_MAX (18446744073709551615UL)"
.LASF648:
	.string	"INT_LEAST64_MAX (__INT64_C(9223372036854775807))"
.LASF510:
	.string	"__extern_inline extern __inline __attribute__ ((__gnu_inline__))"
.LASF251:
	.string	"__FLT64_IS_IEC_60559__ 1"
.LASF544:
	.string	"__stub_setlogin "
.LASF232:
	.string	"__FLT32_HAS_DENORM__ 1"
.LASF393:
	.string	"__unix__ 1"
.LASF342:
	.string	"__GCC_HAVE_SYNC_COMPARE_AND_SWAP_1 1"
.LASF286:
	.string	"__FLT64X_MIN_EXP__ (-16381)"
.LASF619:
	.string	"_BITS_TIME64_H 1"
.LASF579:
	.string	"__SYSCALL_ULONG_TYPE __ULONGWORD_TYPE"
.LASF20:
	.string	"__PIE__ 2"
.LASF156:
	.string	"__FLT_DIG__ 6"
.LASF137:
	.string	"__INT_FAST16_WIDTH__ 64"
.LASF19:
	.string	"__pie__ 2"
.LASF204:
	.string	"__FLT16_MANT_DIG__ 11"
.LASF666:
	.string	"INTPTR_MAX (9223372036854775807L)"
.LASF307:
	.string	"__BFLT16_MAX__ 3.38953138925153547590470800371487867e+38BF16"
.LASF148:
	.string	"__UINTPTR_MAX__ 0xffffffffffffffffUL"
.LASF400:
	.string	"__STDC_IEC_60559_COMPLEX__ 201404L"
.LASF149:
	.string	"__GCC_IEC_559 2"
.LASF254:
	.string	"__FLT128_MIN_EXP__ (-16381)"
.LASF708:
	.string	"os_equal"
.LASF234:
	.string	"__FLT32_HAS_QUIET_NAN__ 1"
.LASF450:
	.string	"__GLIBC_USE_C2X_STRTOL 0"
.LASF128:
	.string	"__UINT_LEAST16_MAX__ 0xffff"
.LASF145:
	.string	"__UINT_FAST64_MAX__ 0xffffffffffffffffUL"
.LASF196:
	.string	"__LDBL_NORM_MAX__ 1.18973149535723176502126385303097021e+4932L"
.LASF496:
	.string	"__attribute_used__ __attribute__ ((__used__))"
.LASF7:
	.string	"__GNUC__ 13"
.LASF126:
	.string	"__UINT_LEAST8_MAX__ 0xff"
.LASF548:
	.string	"__GLIBC_USE_LIB_EXT2 0"
.LASF331:
	.string	"__DEC128_MIN_EXP__ (-6142)"
.LASF435:
	.string	"__KERNEL_STRICT_NAMES "
.LASF44:
	.string	"__WCHAR_TYPE__ int"
.LASF106:
	.string	"__INT8_MAX__ 0x7f"
.LASF335:
	.string	"__DEC128_EPSILON__ 1E-33DL"
.LASF414:
	.string	"__USE_POSIX199506"
.LASF388:
	.string	"__CET__ 3"
.LASF315:
	.string	"__BFLT16_IS_IEC_60559__ 0"
.LASF256:
	.string	"__FLT128_MAX_EXP__ 16384"
.LASF616:
	.string	"__STATFS_MATCHES_STATFS64 1"
.LASF154:
	.string	"__FLT_RADIX__ 2"
.LASF279:
	.string	"__FLT32X_DENORM_MIN__ 4.94065645841246544176568792868221372e-324F32x"
.LASF306:
	.string	"__BFLT16_DECIMAL_DIG__ 4"
.LASF469:
	.string	"__PMT(args) args"
.LASF582:
	.string	"__GID_T_TYPE __U32_TYPE"
.LASF30:
	.string	"__SIZEOF_LONG_DOUBLE__ 16"
.LASF185:
	.string	"__DBL_HAS_QUIET_NAN__ 1"
.LASF299:
	.string	"__FLT64X_IS_IEC_60559__ 1"
.LASF561:
	.string	"_BITS_TYPES_H 1"
.LASF199:
	.string	"__LDBL_DENORM_MIN__ 3.64519953188247460252840593361941982e-4951L"
.LASF590:
	.string	"__PID_T_TYPE __S32_TYPE"
.LASF249:
	.string	"__FLT64_HAS_INFINITY__ 1"
.LASF142:
	.string	"__UINT_FAST8_MAX__ 0xff"
.LASF374:
	.string	"__ATOMIC_HLE_RELEASE 131072"
.LASF191:
	.string	"__LDBL_MAX_EXP__ 16384"
.LASF570:
	.string	"__SWORD_TYPE long int"
.LASF542:
	.string	"__stub_gtty "
.LASF577:
	.string	"_BITS_TYPESIZES_H 1"
.LASF602:
	.string	"__USECONDS_T_TYPE __U32_TYPE"
.LASF521:
	.string	"__LDOUBLE_REDIRECTS_TO_FLOAT128_ABI 0"
.LASF75:
	.string	"__INTPTR_TYPE__ long int"
.LASF327:
	.string	"__DEC64_MAX__ 9.999999999999999E384DD"
.LASF612:
	.string	"__CPU_MASK_TYPE __SYSCALL_ULONG_TYPE"
.LASF625:
	.string	"_BITS_STDINT_INTN_H 1"
.LASF352:
	.string	"__GCC_ATOMIC_INT_LOCK_FREE 2"
.LASF554:
	.string	"__GLIBC_USE_IEC_60559_EXT 0"
.LASF379:
	.string	"__MMX__ 1"
.LASF593:
	.string	"__BLKCNT_T_TYPE __SYSCALL_SLONG_TYPE"
.LASF493:
	.string	"__attribute_pure__ __attribute__ ((__pure__))"
.LASF711:
	.string	"value"
.LASF125:
	.string	"__INT_LEAST64_WIDTH__ 64"
.LASF532:
	.string	"__HAVE_GENERIC_SELECTION 1"
.LASF660:
	.string	"INT_FAST64_MAX (__INT64_C(9223372036854775807))"
.LASF252:
	.string	"__FLT128_MANT_DIG__ 113"
.LASF34:
	.string	"__ORDER_LITTLE_ENDIAN__ 1234"
.LASF630:
	.string	"INT16_MIN (-32767-1)"
.LASF366:
	.string	"__SIZEOF_PTRDIFF_T__ 8"
.LASF330:
	.string	"__DEC128_MANT_DIG__ 34"
.LASF441:
	.string	"__USE_ISOC99 1"
.LASF53:
	.string	"__INT32_TYPE__ int"
.LASF430:
	.string	"__KERNEL_STRICT_NAMES"
.LASF565:
	.string	"__U32_TYPE unsigned int"
.LASF378:
	.string	"__code_model_small__ 1"
.LASF409:
	.string	"__USE_ISOC95"
.LASF296:
	.string	"__FLT64X_HAS_DENORM__ 1"
.LASF318:
	.string	"__DEC32_MAX_EXP__ 97"
.LASF662:
	.string	"UINT_FAST16_MAX (18446744073709551615UL)"
.LASF408:
	.string	"__USE_ISOC99"
.LASF678:
	.string	"WINT_MIN (0u)"
.LASF431:
	.string	"__GLIBC_USE_ISOC2X"
.LASF215:
	.string	"__FLT16_DENORM_MIN__ 5.96046447753906250000000000000000000e-8F16"
.LASF399:
	.string	"__STDC_IEC_559_COMPLEX__ 1"
.LASF448:
	.string	"__GLIBC_USE_DEPRECATED_GETS 0"
.LASF287:
	.string	"__FLT64X_MIN_10_EXP__ (-4931)"
.LASF637:
	.string	"UINT8_MAX (255)"
.LASF401:
	.string	"__STDC_ISO_10646__ 201706L"
.LASF664:
	.string	"UINT_FAST64_MAX (__UINT64_C(18446744073709551615))"
.LASF473:
	.string	"__BEGIN_DECLS "
.LASF611:
	.string	"__SSIZE_T_TYPE __SWORD_TYPE"
.LASF359:
	.string	"__HAVE_SPECULATION_SAFE_VALUE 1"
.LASF257:
	.string	"__FLT128_MAX_10_EXP__ 4932"
.LASF261:
	.string	"__FLT128_MIN__ 3.36210314311209350626267781732175260e-4932F128"
.LASF603:
	.string	"__SUSECONDS_T_TYPE __SYSCALL_SLONG_TYPE"
.LASF135:
	.string	"__INT_FAST8_WIDTH__ 8"
.LASF239:
	.string	"__FLT64_MIN_10_EXP__ (-307)"
.LASF633:
	.string	"INT8_MAX (127)"
.LASF533:
	.string	"__attr_access(x) __attribute__ ((__access__ x))"
.LASF178:
	.string	"__DBL_MAX__ ((double)1.79769313486231570814527423731704357e+308L)"
.LASF8:
	.string	"__GNUC_MINOR__ 3"
.LASF615:
	.string	"__RLIM_T_MATCHES_RLIM64_T 1"
.LASF368:
	.string	"__amd64__ 1"
.LASF701:
	.string	"__uint64_t"
.LASF206:
	.string	"__FLT16_MIN_EXP__ (-13)"
.LASF226:
	.string	"__FLT32_DECIMAL_DIG__ 9"
.LASF253:
	.string	"__FLT128_DIG__ 33"
.LASF56:
	.string	"__UINT16_TYPE__ short unsigned int"
.LASF688:
	.string	"INTMAX_C(c) c ## L"
.LASF94:
	.string	"__WCHAR_WIDTH__ 32"
.LASF347:
	.string	"__GCC_ATOMIC_CHAR_LOCK_FREE 2"
.LASF642:
	.string	"INT_LEAST16_MIN (-32767-1)"
.LASF82:
	.string	"__LONG_LONG_MAX__ 0x7fffffffffffffffLL"
.LASF64:
	.string	"__UINT_LEAST16_TYPE__ short unsigned int"
.LASF160:
	.string	"__FLT_MAX_10_EXP__ 38"
.LASF278:
	.string	"__FLT32X_EPSILON__ 2.22044604925031308084726333618164062e-16F32x"
.LASF710:
	.string	"GNU C11 13.3.0 -mno-red-zone -mtune=generic -march=x86-64 -g3 -O0 -std=c11 -fPIE -fno-omit-frame-pointer -fno-optimize-sibling-calls -fno-ipa-stack-alignment -fasynchronous-unwind-tables -fstack-protector-strong -fstack-clash-protection -fcf-protection"
.LASF649:
	.string	"UINT_LEAST8_MAX (255)"
.LASF340:
	.string	"__NO_INLINE__ 1"
.LASF513:
	.string	"__va_arg_pack() __builtin_va_arg_pack ()"
.LASF674:
	.string	"SIG_ATOMIC_MAX (2147483647)"
.LASF98:
	.string	"__INTMAX_MAX__ 0x7fffffffffffffffL"
.LASF446:
	.string	"__TIMESIZE __WORDSIZE"
.LASF242:
	.string	"__FLT64_DECIMAL_DIG__ 17"
.LASF344:
	.string	"__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4 1"
.LASF285:
	.string	"__FLT64X_DIG__ 18"
.LASF222:
	.string	"__FLT32_MIN_EXP__ (-125)"
.LASF42:
	.string	"__SIZE_TYPE__ long unsigned int"
.LASF454:
	.string	"__GLIBC_MINOR__ 39"
.LASF153:
	.string	"__DEC_EVAL_METHOD__ 2"
.LASF659:
	.string	"INT_FAST32_MAX (9223372036854775807L)"
.LASF162:
	.string	"__FLT_MAX__ 3.40282346638528859811704183484516925e+38F"
.LASF174:
	.string	"__DBL_MIN_10_EXP__ (-307)"
.LASF700:
	.string	"char"
.LASF112:
	.string	"__UINT32_MAX__ 0xffffffffU"
.LASF384:
	.string	"__SSE2_MATH__ 1"
.LASF190:
	.string	"__LDBL_MIN_10_EXP__ (-4931)"
.LASF221:
	.string	"__FLT32_DIG__ 6"
.LASF77:
	.string	"__GXX_ABI_VERSION 1018"
.LASF248:
	.string	"__FLT64_HAS_DENORM__ 1"
.LASF589:
	.string	"__OFF64_T_TYPE __SQUAD_TYPE"
.LASF151:
	.string	"__FLT_EVAL_METHOD__ 0"
.LASF292:
	.string	"__FLT64X_NORM_MAX__ 1.18973149535723176502126385303097021e+4932F64x"
.LASF280:
	.string	"__FLT32X_HAS_DENORM__ 1"
.LASF370:
	.string	"__x86_64__ 1"
.LASF668:
	.string	"INTMAX_MIN (-__INT64_C(9223372036854775807)-1)"
.LASF99:
	.string	"__INTMAX_C(c) c ## L"
.LASF665:
	.string	"INTPTR_MIN (-9223372036854775807L-1)"
.LASF291:
	.string	"__FLT64X_MAX__ 1.18973149535723176502126385303097021e+4932F64x"
.LASF350:
	.string	"__GCC_ATOMIC_WCHAR_T_LOCK_FREE 2"
.LASF526:
	.string	"__LDBL_REDIR2_DECL(name) "
.LASF599:
	.string	"__ID_T_TYPE __U32_TYPE"
.LASF606:
	.string	"__KEY_T_TYPE __S32_TYPE"
.LASF123:
	.string	"__INT_LEAST64_MAX__ 0x7fffffffffffffffL"
.LASF116:
	.string	"__INT_LEAST8_WIDTH__ 8"
.LASF524:
	.string	"__LDBL_REDIR1_NTH(name,proto,alias) name proto __THROW"
.LASF110:
	.string	"__UINT8_MAX__ 0xff"
.LASF623:
	.string	"__WCHAR_MAX __WCHAR_MAX__"
.LASF508:
	.string	"__always_inline __inline __attribute__ ((__always_inline__))"
.LASF111:
	.string	"__UINT16_MAX__ 0xffff"
.LASF438:
	.string	"__GLIBC_USE(F) __GLIBC_USE_ ## F"
.LASF631:
	.string	"INT32_MIN (-2147483647-1)"
.LASF439:
	.string	"__GLIBC_USE_ISOC2X 0"
.LASF70:
	.string	"__INT_FAST64_TYPE__ long int"
.LASF213:
	.string	"__FLT16_MIN__ 6.10351562500000000000000000000000000e-5F16"
.LASF3:
	.string	"__STDC_VERSION__ 201112L"
.LASF102:
	.string	"__INTMAX_WIDTH__ 64"
.LASF320:
	.string	"__DEC32_MAX__ 9.999999E96DF"
.LASF186:
	.string	"__DBL_IS_IEC_60559__ 1"
.LASF87:
	.string	"__PTRDIFF_MAX__ 0x7fffffffffffffffL"
.LASF413:
	.string	"__USE_POSIX199309"
.LASF60:
	.string	"__INT_LEAST16_TYPE__ short int"
.LASF224:
	.string	"__FLT32_MAX_EXP__ 128"
.LASF389:
	.string	"__gnu_linux__ 1"
.LASF576:
	.string	"__STD_TYPE typedef"
.LASF585:
	.string	"__MODE_T_TYPE __U32_TYPE"
.LASF372:
	.string	"__SIZEOF_FLOAT128__ 16"
.LASF418:
	.string	"__USE_XOPEN2K"
.LASF172:
	.string	"__DBL_DIG__ 15"
.LASF114:
	.string	"__INT_LEAST8_MAX__ 0x7f"
.LASF562:
	.string	"__S16_TYPE short int"
.LASF23:
	.string	"__LP64__ 1"
.LASF445:
	.string	"__SYSCALL_WORDSIZE 64"
.LASF353:
	.string	"__GCC_ATOMIC_LONG_LOCK_FREE 2"
.LASF269:
	.string	"__FLT32X_DIG__ 15"
.LASF707:
	.string	"right"
.LASF268:
	.string	"__FLT32X_MANT_DIG__ 53"
.LASF525:
	.string	"__LDBL_REDIR_NTH(name,proto) name proto __THROW"
.LASF538:
	.string	"__attribute_returns_twice__ __attribute__ ((__returns_twice__))"
.LASF22:
	.string	"_LP64 1"
.LASF406:
	.string	"_FEATURES_H 1"
.LASF591:
	.string	"__RLIM_T_TYPE __SYSCALL_ULONG_TYPE"
.LASF587:
	.string	"__FSWORD_T_TYPE __SYSCALL_SLONG_TYPE"
.LASF467:
	.string	"__COLD __attribute__ ((__cold__))"
.LASF546:
	.string	"__stub_stty "
.LASF369:
	.string	"__x86_64 1"
.LASF289:
	.string	"__FLT64X_MAX_10_EXP__ 4932"
.LASF180:
	.string	"__DBL_MIN__ ((double)2.22507385850720138309023271733240406e-308L)"
.LASF346:
	.string	"__GCC_ATOMIC_BOOL_LOCK_FREE 2"
.LASF144:
	.string	"__UINT_FAST32_MAX__ 0xffffffffffffffffUL"
.LASF21:
	.string	"__FINITE_MATH_ONLY__ 0"
.LASF680:
	.string	"INT8_C(c) c"
.LASF563:
	.string	"__U16_TYPE unsigned short int"
.LASF243:
	.string	"__FLT64_MAX__ 1.79769313486231570814527423731704357e+308F64"
.LASF618:
	.string	"__FD_SETSIZE 1024"
.LASF654:
	.string	"INT_FAST16_MIN (-9223372036854775807L-1)"
.LASF421:
	.string	"__USE_XOPEN2K8XSI"
.LASF375:
	.string	"__GCC_ASM_FLAG_OUTPUTS__ 1"
.LASF132:
	.string	"__UINT_LEAST64_MAX__ 0xffffffffffffffffUL"
.LASF457:
	.string	"__PMT"
.LASF569:
	.string	"__UQUAD_TYPE unsigned long int"
.LASF394:
	.string	"__ELF__ 1"
.LASF277:
	.string	"__FLT32X_MIN__ 2.22507385850720138309023271733240406e-308F32x"
.LASF76:
	.string	"__UINTPTR_TYPE__ long unsigned int"
.LASF705:
	.string	"os_message"
.LASF118:
	.string	"__INT16_C(c) c"
.LASF360:
	.string	"__GCC_HAVE_DWARF2_CFI_ASM 1"
.LASF28:
	.string	"__SIZEOF_FLOAT__ 4"
.LASF322:
	.string	"__DEC32_SUBNORMAL_MIN__ 0.000001E-95DF"
.LASF298:
	.string	"__FLT64X_HAS_QUIET_NAN__ 1"
.LASF168:
	.string	"__FLT_HAS_INFINITY__ 1"
.LASF503:
	.string	"__nonnull(params) __attribute_nonnull__ (params)"
.LASF305:
	.string	"__BFLT16_MAX_10_EXP__ 38"
.LASF712:
	.string	"os_add"
.LASF217:
	.string	"__FLT16_HAS_INFINITY__ 1"
.LASF584:
	.string	"__INO64_T_TYPE __UQUAD_TYPE"
.LASF452:
	.string	"__GNU_LIBRARY__ 6"
.LASF2:
	.string	"__STDC__ 1"
.LASF482:
	.string	"__glibc_c99_flexarr_available 1"
.LASF381:
	.string	"__SSE2__ 1"
.LASF572:
	.string	"__SLONG32_TYPE int"
.LASF175:
	.string	"__DBL_MAX_EXP__ 1024"
.LASF25:
	.string	"__SIZEOF_LONG__ 8"
.LASF16:
	.string	"__ATOMIC_CONSUME 1"
.LASF424:
	.string	"__USE_FILE_OFFSET64"
.LASF310:
	.string	"__BFLT16_EPSILON__ 7.81250000000000000000000000000000000e-3BF16"
.LASF229:
	.string	"__FLT32_MIN__ 1.17549435082228750796873653722224568e-38F32"
.LASF45:
	.string	"__WINT_TYPE__ unsigned int"
.LASF644:
	.string	"INT_LEAST64_MIN (-__INT64_C(9223372036854775807)-1)"
.LASF107:
	.string	"__INT16_MAX__ 0x7fff"
.LASF490:
	.string	"__attribute_malloc__ __attribute__ ((__malloc__))"
.LASF89:
	.string	"__SCHAR_WIDTH__ 8"
.LASF131:
	.string	"__UINT32_C(c) c ## U"
.LASF69:
	.string	"__INT_FAST32_TYPE__ long int"
.LASF29:
	.string	"__SIZEOF_DOUBLE__ 8"
.LASF9:
	.string	"__GNUC_PATCHLEVEL__ 0"
.LASF127:
	.string	"__UINT8_C(c) c"
.LASF624:
	.string	"__WCHAR_MIN __WCHAR_MIN__"
.LASF594:
	.string	"__BLKCNT64_T_TYPE __SQUAD_TYPE"
.LASF314:
	.string	"__BFLT16_HAS_QUIET_NAN__ 1"
.LASF210:
	.string	"__FLT16_DECIMAL_DIG__ 5"
.LASF429:
	.string	"__USE_FORTIFY_LEVEL"
.LASF547:
	.string	"__GLIBC_USE_LIB_EXT2"
.LASF86:
	.string	"__WINT_MIN__ 0U"
.LASF434:
	.string	"__GLIBC_USE_C2X_STRTOL"
.LASF187:
	.string	"__LDBL_MANT_DIG__ 64"
.LASF235:
	.string	"__FLT32_IS_IEC_60559__ 1"
.LASF691:
	.string	"OS_LEFT 3"
.LASF163:
	.string	"__FLT_NORM_MAX__ 3.40282346638528859811704183484516925e+38F"
.LASF4:
	.string	"__STDC_UTF_16__ 1"
.LASF198:
	.string	"__LDBL_EPSILON__ 1.08420217248550443400745280086994171e-19L"
.LASF476:
	.string	"__bos0(ptr) __builtin_object_size (ptr, 0)"
.LASF617:
	.string	"__KERNEL_OLD_TIMEVAL_MATCHES_TIMEVAL64 1"
.LASF500:
	.string	"__attribute_format_arg__(x) __attribute__ ((__format_arg__ (x)))"
.LASF201:
	.string	"__LDBL_HAS_INFINITY__ 1"
.LASF614:
	.string	"__INO_T_MATCHES_INO64_T 1"
.LASF150:
	.string	"__GCC_IEC_559_COMPLEX 2"
.LASF104:
	.string	"__SIG_ATOMIC_MIN__ (-__SIG_ATOMIC_MAX__ - 1)"
.LASF670:
	.string	"UINTMAX_MAX (__UINT64_C(18446744073709551615))"
.LASF364:
	.string	"__SIZEOF_WCHAR_T__ 4"
.LASF498:
	.string	"__attribute_deprecated__ __attribute__ ((__deprecated__))"
.LASF255:
	.string	"__FLT128_MIN_10_EXP__ (-4931)"
.LASF38:
	.string	"__FLOAT_WORD_ORDER__ __ORDER_LITTLE_ENDIAN__"
.LASF343:
	.string	"__GCC_HAVE_SYNC_COMPARE_AND_SWAP_2 1"
.LASF638:
	.string	"UINT16_MAX (65535)"
.LASF316:
	.string	"__DEC32_MANT_DIG__ 7"
.LASF677:
	.string	"WCHAR_MAX __WCHAR_MAX"
.LASF231:
	.string	"__FLT32_DENORM_MIN__ 1.40129846432481707092372958328991613e-45F32"
.LASF324:
	.string	"__DEC64_MIN_EXP__ (-382)"
.LASF143:
	.string	"__UINT_FAST16_MAX__ 0xffffffffffffffffUL"
.LASF518:
	.string	"__attribute_nonstring__ __attribute__ ((__nonstring__))"
.LASF684:
	.string	"UINT8_C(c) c"
.LASF108:
	.string	"__INT32_MAX__ 0x7fffffff"
.LASF610:
	.string	"__FSID_T_TYPE struct { int __val[2]; }"
.LASF596:
	.string	"__FSBLKCNT64_T_TYPE __UQUAD_TYPE"
.LASF535:
	.string	"__attr_access_none(argno) __attribute__ ((__access__ (__none__, argno)))"
.LASF478:
	.string	"__glibc_objsize(__o) __bos (__o)"
.LASF13:
	.string	"__ATOMIC_ACQUIRE 2"
.LASF129:
	.string	"__UINT16_C(c) c"
.LASF109:
	.string	"__INT64_MAX__ 0x7fffffffffffffffL"
.LASF628:
	.string	"__intptr_t_defined "
.LASF475:
	.string	"__bos(ptr) __builtin_object_size (ptr, __USE_FORTIFY_LEVEL > 1)"
.LASF595:
	.string	"__FSBLKCNT_T_TYPE __SYSCALL_ULONG_TYPE"
.LASF460:
	.string	"__glibc_has_extension(ext) 0"
.LASF387:
	.string	"__SEG_GS 1"
.LASF383:
	.string	"__SSE_MATH__ 1"
.LASF586:
	.string	"__NLINK_T_TYPE __SYSCALL_ULONG_TYPE"
.LASF47:
	.string	"__UINTMAX_TYPE__ long unsigned int"
.LASF216:
	.string	"__FLT16_HAS_DENORM__ 1"
.LASF461:
	.string	"__LEAF , __leaf__"
.LASF686:
	.string	"UINT32_C(c) c ## U"
.LASF534:
	.string	"__fortified_attr_access(a,o,s) __attr_access ((a, o, s))"
.LASF512:
	.string	"__fortify_function __extern_always_inline __attribute_artificial__"
.LASF105:
	.string	"__SIG_ATOMIC_WIDTH__ 32"
.LASF683:
	.string	"INT64_C(c) c ## L"
.LASF499:
	.string	"__attribute_deprecated_msg__(msg) __attribute__ ((__deprecated__ (msg)))"
.LASF79:
	.string	"__SHRT_MAX__ 0x7fff"
.LASF483:
	.string	"__REDIRECT(name,proto,alias) name proto __asm__ (__ASMNAME (#alias))"
.LASF300:
	.string	"__BFLT16_MANT_DIG__ 8"
.LASF36:
	.string	"__ORDER_PDP_ENDIAN__ 3412"
.LASF656:
	.string	"INT_FAST64_MIN (-__INT64_C(9223372036854775807)-1)"
.LASF161:
	.string	"__FLT_DECIMAL_DIG__ 9"
.LASF571:
	.string	"__UWORD_TYPE unsigned long int"
.LASF241:
	.string	"__FLT64_MAX_10_EXP__ 308"
.LASF471:
	.string	"__STRING(x) #x"
.LASF189:
	.string	"__LDBL_MIN_EXP__ (-16381)"
.LASF95:
	.string	"__WINT_WIDTH__ 32"
.LASF552:
	.string	"__GLIBC_USE_IEC_60559_BFP_EXT_C2X 0"
.LASF293:
	.string	"__FLT64X_MIN__ 3.36210314311209350626267781732175260e-4932F64x"
.LASF497:
	.string	"__attribute_noinline__ __attribute__ ((__noinline__))"
.LASF134:
	.string	"__INT_FAST8_MAX__ 0x7f"
.LASF24:
	.string	"__SIZEOF_INT__ 4"
.LASF230:
	.string	"__FLT32_EPSILON__ 1.19209289550781250000000000000000000e-7F32"
.LASF404:
	.string	"__GLIBC_INTERNAL_STARTING_HEADER_IMPLEMENTATION "
.LASF698:
	.string	"short int"
.LASF661:
	.string	"UINT_FAST8_MAX (255)"
.LASF81:
	.string	"__LONG_MAX__ 0x7fffffffffffffffL"
.LASF325:
	.string	"__DEC64_MAX_EXP__ 385"
.LASF627:
	.string	"_BITS_STDINT_LEAST_H 1"
.LASF271:
	.string	"__FLT32X_MIN_10_EXP__ (-307)"
.LASF412:
	.string	"__USE_POSIX2"
.LASF699:
	.string	"long int"
.LASF622:
	.string	"_BITS_WCHAR_H 1"
.LASF588:
	.string	"__OFF_T_TYPE __SYSCALL_SLONG_TYPE"
.LASF101:
	.string	"__UINTMAX_C(c) c ## UL"
.LASF329:
	.string	"__DEC64_SUBNORMAL_MIN__ 0.000000000000001E-383DD"
.LASF658:
	.string	"INT_FAST16_MAX (9223372036854775807L)"
.LASF309:
	.string	"__BFLT16_MIN__ 1.17549435082228750796873653722224568e-38BF16"
.LASF297:
	.string	"__FLT64X_HAS_INFINITY__ 1"
.LASF447:
	.string	"__USE_FORTIFY_LEVEL 0"
.LASF332:
	.string	"__DEC128_MAX_EXP__ 6145"
.LASF270:
	.string	"__FLT32X_MIN_EXP__ (-1021)"
.LASF492:
	.string	"__attribute_alloc_align__(param) __attribute__ ((__alloc_align__ param))"
.LASF147:
	.string	"__INTPTR_WIDTH__ 64"
.LASF583:
	.string	"__INO_T_TYPE __SYSCALL_ULONG_TYPE"
.LASF356:
	.string	"__GCC_DESTRUCTIVE_SIZE 64"
.LASF304:
	.string	"__BFLT16_MAX_EXP__ 128"
.LASF71:
	.string	"__UINT_FAST8_TYPE__ unsigned char"
.LASF121:
	.string	"__INT32_C(c) c"
.LASF598:
	.string	"__FSFILCNT64_T_TYPE __UQUAD_TYPE"
.LASF202:
	.string	"__LDBL_HAS_QUIET_NAN__ 1"
.LASF51:
	.string	"__INT8_TYPE__ signed char"
.LASF85:
	.string	"__WINT_MAX__ 0xffffffffU"
.LASF506:
	.string	"__wur "
.LASF390:
	.string	"__linux 1"
.LASF689:
	.string	"UINTMAX_C(c) c ## UL"
.LASF555:
	.string	"__GLIBC_USE_IEC_60559_FUNCS_EXT"
.LASF519:
	.string	"__attribute_copy__"
.LASF522:
	.string	"__LDBL_REDIR1(name,proto,alias) name proto"
.LASF702:
	.string	"uint64_t"
.LASF57:
	.string	"__UINT32_TYPE__ unsigned int"
.LASF426:
	.string	"__USE_ATFILE"
.LASF420:
	.string	"__USE_XOPEN2K8"
.LASF529:
	.string	"__REDIRECT_NTH_LDBL(name,proto,alias) __REDIRECT_NTH (name, proto, alias)"
.LASF319:
	.string	"__DEC32_MIN__ 1E-95DF"
.LASF484:
	.string	"__REDIRECT_NTH(name,proto,alias) name proto __asm__ (__ASMNAME (#alias)) __THROW"
.LASF415:
	.string	"__USE_XOPEN"
.LASF262:
	.string	"__FLT128_EPSILON__ 1.92592994438723585305597794258492732e-34F128"
.LASF675:
	.string	"SIZE_MAX (18446744073709551615UL)"
.LASF607:
	.string	"__CLOCKID_T_TYPE __S32_TYPE"
.LASF463:
	.string	"__THROW __attribute__ ((__nothrow__ __LEAF))"
.LASF26:
	.string	"__SIZEOF_LONG_LONG__ 8"
.LASF214:
	.string	"__FLT16_EPSILON__ 9.76562500000000000000000000000000000e-4F16"
.LASF55:
	.string	"__UINT8_TYPE__ unsigned char"
.LASF90:
	.string	"__SHRT_WIDTH__ 16"
.LASF706:
	.string	"left"
.LASF397:
	.string	"__STDC_IEC_559__ 1"
.LASF574:
	.string	"__S64_TYPE long int"
.LASF244:
	.string	"__FLT64_NORM_MAX__ 1.79769313486231570814527423731704357e+308F64"
.LASF597:
	.string	"__FSFILCNT_T_TYPE __SYSCALL_ULONG_TYPE"
.LASF139:
	.string	"__INT_FAST32_WIDTH__ 64"
.LASF427:
	.string	"__USE_DYNAMIC_STACK_SIZE"
.LASF657:
	.string	"INT_FAST8_MAX (127)"
.LASF604:
	.string	"__SUSECONDS64_T_TYPE __SQUAD_TYPE"
.LASF560:
	.string	"__GLIBC_USE_IEC_60559_TYPES_EXT 0"
.LASF115:
	.string	"__INT8_C(c) c"
.LASF703:
	.string	"os_data"
.LASF39:
	.string	"__SIZEOF_POINTER__ 8"
.LASF246:
	.string	"__FLT64_EPSILON__ 2.22044604925031308084726333618164062e-16F64"
.LASF284:
	.string	"__FLT64X_MANT_DIG__ 64"
.LASF605:
	.string	"__DADDR_T_TYPE __S32_TYPE"
.LASF83:
	.string	"__WCHAR_MAX__ 0x7fffffff"
.LASF155:
	.string	"__FLT_MANT_DIG__ 24"
.LASF443:
	.string	"__WORDSIZE 64"
.LASF91:
	.string	"__INT_WIDTH__ 32"
.LASF194:
	.string	"__LDBL_DECIMAL_DIG__ 21"
.LASF358:
	.string	"__GCC_ATOMIC_POINTER_LOCK_FREE 2"
.LASF539:
	.string	"__stub___compat_bdflush "
.LASF573:
	.string	"__ULONG32_TYPE unsigned int"
.LASF386:
	.string	"__SEG_FS 1"
.LASF468:
	.string	"__P(args) args"
.LASF601:
	.string	"__TIME_T_TYPE __SYSCALL_SLONG_TYPE"
.LASF72:
	.string	"__UINT_FAST16_TYPE__ long unsigned int"
.LASF453:
	.string	"__GLIBC__ 2"
.LASF635:
	.string	"INT32_MAX (2147483647)"
.LASF338:
	.string	"__USER_LABEL_PREFIX__ "
.LASF696:
	.string	"long unsigned int"
.LASF530:
	.string	"__glibc_macro_warning1(message) _Pragma (#message)"
.LASF349:
	.string	"__GCC_ATOMIC_CHAR32_T_LOCK_FREE 2"
.LASF517:
	.string	"__glibc_likely(cond) __builtin_expect ((cond), 1)"
.LASF566:
	.string	"__SLONGWORD_TYPE long int"
.LASF392:
	.string	"__unix 1"
.LASF32:
	.string	"__CHAR_BIT__ 8"
.LASF600:
	.string	"__CLOCK_T_TYPE __SYSCALL_SLONG_TYPE"
.LASF11:
	.string	"__ATOMIC_RELAXED 0"
.LASF6:
	.string	"__STDC_HOSTED__ 1"
.LASF97:
	.string	"__SIZE_WIDTH__ 64"
.LASF528:
	.string	"__REDIRECT_LDBL(name,proto,alias) __REDIRECT (name, proto, alias)"
.LASF398:
	.string	"__STDC_IEC_60559_BFP__ 201404L"
.LASF487:
	.string	"__ASMNAME2(prefix,cname) __STRING (prefix) cname"
.LASF218:
	.string	"__FLT16_HAS_QUIET_NAN__ 1"
.LASF92:
	.string	"__LONG_WIDTH__ 64"
.LASF113:
	.string	"__UINT64_MAX__ 0xffffffffffffffffUL"
.LASF568:
	.string	"__SQUAD_TYPE long int"
.LASF419:
	.string	"__USE_XOPEN2KXSI"
.LASF491:
	.string	"__attribute_alloc_size__(params) __attribute__ ((__alloc_size__ params))"
.LASF376:
	.string	"__k8 1"
.LASF48:
	.string	"__CHAR16_TYPE__ short unsigned int"
.LASF354:
	.string	"__GCC_ATOMIC_LLONG_LOCK_FREE 2"
.LASF159:
	.string	"__FLT_MAX_EXP__ 128"
.LASF515:
	.string	"__restrict_arr __restrict"
.LASF15:
	.string	"__ATOMIC_ACQ_REL 4"
.LASF501:
	.string	"__attribute_format_strfmon__(a,b) __attribute__ ((__format__ (__strfmon__, a, b)))"
.LASF693:
	.string	"unsigned char"
.LASF317:
	.string	"__DEC32_MIN_EXP__ (-94)"
.LASF455:
	.string	"__GLIBC_PREREQ(maj,min) ((__GLIBC__ << 16) + __GLIBC_MINOR__ >= ((maj) << 16) + (min))"
.LASF54:
	.string	"__INT64_TYPE__ long int"
.LASF164:
	.string	"__FLT_MIN__ 1.17549435082228750796873653722224568e-38F"
.LASF575:
	.string	"__U64_TYPE unsigned long int"
.LASF671:
	.string	"PTRDIFF_MIN (-9223372036854775807L-1)"
.LASF10:
	.string	"__VERSION__ \"13.3.0\""
.LASF682:
	.string	"INT32_C(c) c"
.LASF709:
	.string	"os_width"
.LASF557:
	.string	"__GLIBC_USE_IEC_60559_FUNCS_EXT_C2X"
.LASF679:
	.string	"WINT_MAX (4294967295u)"
.LASF382:
	.string	"__FXSR__ 1"
.LASF462:
	.string	"__LEAF_ATTR __attribute__ ((__leaf__))"
.LASF621:
	.string	"__STD_TYPE"
.LASF122:
	.string	"__INT_LEAST32_WIDTH__ 32"
.LASF41:
	.string	"__GNUC_WIDE_EXECUTION_CHARSET_NAME \"UTF-32LE\""
.LASF395:
	.string	"__DECIMAL_BID_FORMAT__ 1"
.LASF437:
	.string	"__glibc_clang_prereq(maj,min) 0"
.LASF578:
	.string	"__SYSCALL_SLONG_TYPE __SLONGWORD_TYPE"
.LASF391:
	.string	"__linux__ 1"
.LASF18:
	.string	"__PIC__ 2"
.LASF641:
	.string	"INT_LEAST8_MIN (-128)"
.LASF66:
	.string	"__UINT_LEAST64_TYPE__ long unsigned int"
.LASF449:
	.string	"__GLIBC_USE_DEPRECATED_SCANF 0"
.LASF549:
	.string	"__GLIBC_USE_IEC_60559_BFP_EXT"
.LASF88:
	.string	"__SIZE_MAX__ 0xffffffffffffffffUL"
.LASF581:
	.string	"__UID_T_TYPE __U32_TYPE"
.LASF504:
	.string	"__returns_nonnull __attribute__ ((__returns_nonnull__))"
.LASF567:
	.string	"__ULONGWORD_TYPE unsigned long int"
.LASF351:
	.string	"__GCC_ATOMIC_SHORT_LOCK_FREE 2"
.LASF290:
	.string	"__FLT64X_DECIMAL_DIG__ 21"
.LASF485:
	.string	"__REDIRECT_NTHNL(name,proto,alias) name proto __asm__ (__ASMNAME (#alias)) __THROWNL"
.LASF416:
	.string	"__USE_XOPEN_EXTENDED"
.LASF136:
	.string	"__INT_FAST16_MAX__ 0x7fffffffffffffffL"
.LASF653:
	.string	"INT_FAST8_MIN (-128)"
.LASF283:
	.string	"__FLT32X_IS_IEC_60559__ 1"
.LASF165:
	.string	"__FLT_EPSILON__ 1.19209289550781250000000000000000000e-7F"
.LASF312:
	.string	"__BFLT16_HAS_DENORM__ 1"
.LASF405:
	.string	"__GLIBC_INTERNAL_STARTING_HEADER_IMPLEMENTATION"
.LASF152:
	.string	"__FLT_EVAL_METHOD_TS_18661_3__ 0"
.LASF184:
	.string	"__DBL_HAS_INFINITY__ 1"
.LASF505:
	.string	"__attribute_warn_unused_result__ __attribute__ ((__warn_unused_result__))"
.LASF124:
	.string	"__INT64_C(c) c ## L"
.LASF652:
	.string	"UINT_LEAST64_MAX (__UINT64_C(18446744073709551615))"
.LASF474:
	.string	"__END_DECLS "
.LASF238:
	.string	"__FLT64_MIN_EXP__ (-1021)"
.LASF43:
	.string	"__PTRDIFF_TYPE__ long int"
.LASF68:
	.string	"__INT_FAST16_TYPE__ long int"
.LASF179:
	.string	"__DBL_NORM_MAX__ ((double)1.79769313486231570814527423731704357e+308L)"
.LASF336:
	.string	"__DEC128_SUBNORMAL_MIN__ 0.000000000000000000000000000000001E-6143DL"
.LASF692:
	.string	"OS_RIGHT 2"
.LASF396:
	.string	"_STDC_PREDEF_H 1"
.LASF669:
	.string	"INTMAX_MAX (__INT64_C(9223372036854775807))"
.LASF646:
	.string	"INT_LEAST16_MAX (32767)"
.LASF422:
	.string	"__USE_LARGEFILE"
.LASF687:
	.string	"UINT64_C(c) c ## UL"
.LASF697:
	.string	"signed char"
.LASF371:
	.string	"__SIZEOF_FLOAT80__ 16"
.LASF282:
	.string	"__FLT32X_HAS_QUIET_NAN__ 1"
.LASF176:
	.string	"__DBL_MAX_10_EXP__ 308"
.LASF523:
	.string	"__LDBL_REDIR(name,proto) name proto"
.LASF609:
	.string	"__BLKSIZE_T_TYPE __SYSCALL_SLONG_TYPE"
.LASF694:
	.string	"short unsigned int"
.LASF17:
	.string	"__pic__ 2"
.LASF339:
	.string	"__GNUC_STDC_INLINE__ 1"
.LASF380:
	.string	"__SSE__ 1"
.LASF556:
	.string	"__GLIBC_USE_IEC_60559_FUNCS_EXT 0"
.LASF33:
	.string	"__BIGGEST_ALIGNMENT__ 16"
.LASF613:
	.string	"__OFF_T_MATCHES_OFF64_T 1"
.LASF263:
	.string	"__FLT128_DENORM_MIN__ 6.47517511943802511092443895822764655e-4966F128"
.LASF550:
	.string	"__GLIBC_USE_IEC_60559_BFP_EXT 0"
.LASF543:
	.string	"__stub_revoke "
.LASF704:
	.string	"os_bss"
.LASF157:
	.string	"__FLT_MIN_EXP__ (-125)"
.LASF362:
	.string	"__SSP_STRONG__ 3"
.LASF313:
	.string	"__BFLT16_HAS_INFINITY__ 1"
.LASF480:
	.string	"__errordecl(name,msg) extern void name (void) __attribute__((__error__ (msg)))"
.LASF334:
	.string	"__DEC128_MAX__ 9.999999999999999999999999999999999E6144DL"
.LASF337:
	.string	"__REGISTER_PREFIX__ "
.LASF676:
	.string	"WCHAR_MIN __WCHAR_MIN"
.LASF488:
	.string	"__REDIRECT_FORTIFY __REDIRECT"
.LASF407:
	.string	"__USE_ISOC11"
.LASF211:
	.string	"__FLT16_MAX__ 6.55040000000000000000000000000000000e+4F16"
.LASF580:
	.string	"__DEV_T_TYPE __UQUAD_TYPE"
.LASF341:
	.string	"__STRICT_ANSI__ 1"
.LASF130:
	.string	"__UINT_LEAST32_MAX__ 0xffffffffU"
.LASF459:
	.string	"__glibc_has_builtin(name) __has_builtin (name)"
.LASF486:
	.string	"__ASMNAME(cname) __ASMNAME2 (__USER_LABEL_PREFIX__, cname)"
.LASF273:
	.string	"__FLT32X_MAX_10_EXP__ 308"
.LASF46:
	.string	"__INTMAX_TYPE__ long int"
.LASF209:
	.string	"__FLT16_MAX_10_EXP__ 4"
.LASF276:
	.string	"__FLT32X_NORM_MAX__ 1.79769313486231570814527423731704357e+308F32x"
.LASF247:
	.string	"__FLT64_DENORM_MIN__ 4.94065645841246544176568792868221372e-324F64"
.LASF444:
	.string	"__WORDSIZE_TIME64_COMPAT32 1"
.LASF225:
	.string	"__FLT32_MAX_10_EXP__ 38"
.LASF655:
	.string	"INT_FAST32_MIN (-9223372036854775807L-1)"
.LASF258:
	.string	"__FLT128_DECIMAL_DIG__ 36"
.LASF96:
	.string	"__PTRDIFF_WIDTH__ 64"
.LASF120:
	.string	"__INT_LEAST32_MAX__ 0x7fffffff"
.LASF74:
	.string	"__UINT_FAST64_TYPE__ long unsigned int"
.LASF636:
	.string	"INT64_MAX (__INT64_C(9223372036854775807))"
.LASF464:
	.string	"__THROWNL __attribute__ ((__nothrow__))"
.LASF192:
	.string	"__LDBL_MAX_10_EXP__ 4932"
.LASF326:
	.string	"__DEC64_MIN__ 1E-383DD"
.LASF227:
	.string	"__FLT32_MAX__ 3.40282346638528859811704183484516925e+38F32"
.LASF181:
	.string	"__DBL_EPSILON__ ((double)2.22044604925031308084726333618164062e-16L)"
.LASF361:
	.string	"__PRAGMA_REDEFINE_EXTNAME 1"
.LASF50:
	.string	"__SIG_ATOMIC_TYPE__ int"
.LASF295:
	.string	"__FLT64X_DENORM_MIN__ 3.64519953188247460252840593361941982e-4951F64x"
.LASF411:
	.string	"__USE_POSIX"
.LASF264:
	.string	"__FLT128_HAS_DENORM__ 1"
.LASF433:
	.string	"__GLIBC_USE_DEPRECATED_SCANF"
.LASF267:
	.string	"__FLT128_IS_IEC_60559__ 1"
.LASF177:
	.string	"__DBL_DECIMAL_DIG__ 17"
.LASF200:
	.string	"__LDBL_HAS_DENORM__ 1"
.LASF592:
	.string	"__RLIM64_T_TYPE __UQUAD_TYPE"
.LASF489:
	.string	"__REDIRECT_FORTIFY_NTH __REDIRECT_NTH"
.LASF502:
	.string	"__attribute_nonnull__(params) __attribute__ ((__nonnull__ params))"
.LASF236:
	.string	"__FLT64_MANT_DIG__ 53"
.LASF323:
	.string	"__DEC64_MANT_DIG__ 16"
.LASF167:
	.string	"__FLT_HAS_DENORM__ 1"
.LASF141:
	.string	"__INT_FAST64_WIDTH__ 64"
.LASF193:
	.string	"__DECIMAL_DIG__ 21"
	.section	.debug_line_str,"MS",@progbits,1
.LASF1:
	.string	"/home/jason/study/ck_study/animation"
.LASF0:
	.string	"experiments/os-math.c"
	.ident	"GCC: (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	1f - 0f
	.long	4f - 1f
	.long	5
0:
	.string	"GNU"
1:
	.align 8
	.long	0xc0000002
	.long	3f - 2f
2:
	.long	0x3
3:
	.align 8
4:
