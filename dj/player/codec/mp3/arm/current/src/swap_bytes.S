	.file	"swap_bytes.S"

	.code	32
	.text
	
// void swap_bytes(char * buf, int n)
//  n must be a multiple of 4

	.global	swap_bytes_slow
swap_bytes_slow:
0:      
	// tmp storage for swapped word
	ldr	r3, =0x00000000

	// do the swap into r3
	ldrb	r2, [r0], #1
	orr	r3, r3, r2, lsl #24
	ldrb	r2, [r0], #1
	orr	r3, r3, r2, lsl #16
	ldrb	r2, [r0], #1
	orr	r3, r3, r2, lsl #8
	ldrb	r2, [r0], #1
	orr	r3, r3, r2

	// write the swapped word back
	str	r3, [r0, #-4]
	subs	r1, r1, #4

	// loop until n is 0
	bne	0b

	mov     pc, lr

// dc- optimized routine. jesus the above code is gross
//     25% speed increase

        // purpose:      remap 0x12345678 to 0x78563412
        .global swap_bytes
swap_bytes:
0:
        mov     r3, #0
        ldr     r2, [r0]

        // these two lines swap the high and low bytes
        orr     r3, r3, r2, lsl #24
        orr     r3, r3, r2, lsr #24

        // clear off the upper and lower byte in r2
        mov     r2, r2, lsl #8
        mov     r2, r2, lsr #16
        // now r2 looks like this
        //   0x00003456
        // rotate right by 8 bits
        mov     r2, r2, ror #8
        // now r2 looks like this
        //   0x56000034
        // orr the appropriate bits back into r3
        orr     r3, r3, r2, lsl #8
        orr     r3, r3, r2, lsr #8

        str     r3, [r0], #4
        
        subs    r1, r1, #4
        bne     0b

        mov     pc, lr

