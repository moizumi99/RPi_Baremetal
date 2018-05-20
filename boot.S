// To keep this in the first portion of the binary.
.section ".text.boot"
 
// Make _start global.
.globl _start
 
// Entry point for the kernel.
// r15 -> should begin execution at 0x8000.
// r0 -> 0x00000000
// r1 -> 0x00000C42
// r2 -> 0x00000100 - start of ATAGS
// preserve these registers as argument for kernel_main
_start:
	// Setup the stack.
	mov sp, #0x8000
 
	// Clear out bss.
	ldr r4, =__bss_start
	ldr r9, =__bss_end
	mov r5, #0
	mov r6, #0
	mov r7, #0
	mov r8, #0
	b       2f
 
1:
	// store multiple at r4.
	stmia r4!, {r5-r8}
 
	// If we are still below bss_end, loop.
2:
	cmp r4, r9
	blo 1b
 
	// Call kernel_main
    bl	_enable_jtag
	bl  kernel_main
 
	// halt
halt:
	wfe
	b halt


// Enable JTAG debugging ---------------------------------------
.globl _enable_jtag
_enable_jtag:	
		ldr 	r0, _gpio_address
		// set pin4 to ALT5
		mov		r3, #0b010
		lsl		r1, r3, #12
		str 	r1, [r0]
		// set pin22 to ALT4
		mov		r2, #0b011
		lsl 	r1, r2, #6
		// set pin24 to ALT4
		orr 	r1, r1, r2, lsl #12
		// set pin25 to ALT4
		orr 	r1, r1, r2, lsl #15
		// set pin27 to ALT4
		orr 	r1, r1, r2, lsl #21
		str 	r1, [r0, #8]
		bx		lr
// GPIO address
.globl _gpio_address
_gpio_address:	.word	0x20200000
.globl _gpio_address_end
_gpio_address_end:
		mov		r0,r0	// this meaningless sentence inserted to preserve the label

// interrupt enable
