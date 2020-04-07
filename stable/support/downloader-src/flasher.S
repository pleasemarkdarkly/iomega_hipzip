/*****************************************************************************
*
* FLASHER.S - Code which is loaded into the EP72XX and EP73XX via the boot ROM
*             to program the FLASH.
*
* Copyright (c) 2001 Cirrus Logic, Inc.
*
******************************************************************************/

//*****************************************************************************
//
// The read-only code area.
//
//*****************************************************************************

	.file "flasher.S"
	.code 32
        .text
	
	//
	// This is the entry point of the program.
	//
	.globl ENTRY
ENTRY:  

	//
	// The processor is in ARM mode when we are called, but we want to run in
	// Thumb mode.  So, switch to thumb mode.  Note that this code sequence will
	// not work if any code or data is inserted between the "orr" and the
	// "thumb_entry" label.
	//

        orr     r0, pc, #0x01
	bx      r0
        
	//
	// Switch the assembler into Thumb mode.
	//

	.thumb

	//
	// Now we're in thumb mode.
	//
thumb_entry:

	//
	// Set up the stack pointer.
	//
        ldr     r0, =0x10
        lsl     r0, r0, #24
        ldr     r1, =0x96
        lsl     r1, r1, #8
        orr     r0, r1
//	ldr     r0, =0x10009600
	mov     r13, r0

	//
	// Check the FLASH to see if it is something that we recognize and can
	// program.
	//
	bl  check_flash

	//
	// See if we recognize the FLASH.
	//
	mov r1, #0x01
	cmp r0, r1
	beq loop

	//
	// Send a 'X' to the host to indicate that we will not be able to program
	// the FLASH.
	//
	mov r0, #0x58 // 'X'
	bl  SendChar

	//
	// Loop forever.
	//
	b   .

	//
	// Loop forever reading commands from the host and performing them.
	//
loop:   
	//
	// Write the command prompt to the host.
	//
	mov r0, #0x3f // '?'
	bl  SendChar

	//
	// Read a command from the host.
	//
	bl  ReceiveChar

	//
	// Determine how to handle this command.  Is this a 'B'?
	//
	cmp r0, #0x42 // 'B'
	bne loop1

	//
	// Change the baud rate of the serial port.
	//
	bl  SetBaud
	b   loop

	   //
	   // Is this a 'F'?
	   //
loop1:  
	   cmp r0, #0x46 // 'F'
	   bne loop

	   //
	   // Program data into the on-board FLASH.
	   //
	   bl  ProgramFlash
	   b   loop

	//
	// Tell the assembler to place the literal pool here.
	//
	//LTORG
.ltorg
	//*****************************************************************************
	//
	// Checks the FLASH to determine if it is a device that we know how to program.
	//
	//*****************************************************************************
check_flash:
	//
	// Load a pointer to the FLASH.
	//
	mov r0, #0x70
	lsl r0, #24

	//
	// Load the value of the system status register.
	//
//	ldr r1, =0x80000140
        ldr     r1, =0x80
        lsl     r1, #24
        ldr     r2, =0x10
        lsl     r2, #4
        add     r2, #0x40
        add     r1, r2
	ldr r1, [r1]

	//
	// Get the BOOTBIT field of the system status register.
	//
	mov r2, #0x18
	lsl r2, #24
	and r1, r2

	//
	// See if the BOOTBIT field is zero, indicating that the FLASH connected to
	// nCS0 is 32-bits wide.
	//
	mov r2, #0x00
	cmp r1, r2
	bne check_flash_16

	// hang if there isn't 16-bit flash
	//
	// The FLASH is 32-bits wide.
	//
check_flash_32: 
	mov r7, #0x00

	//
	// Write the read identifier command to the FLASH.
	//
//	ldr r1, =0x00900090
        ldr     r2, =0x90
        lsl     r1, r2, #16
        add     r1, r2
	str r1, [r0]

	//
	// Read back the manufacturer ID from each 16-bit wide FLASH.
	//
	ldr r1, [r0]

	//
	// See if the IDs are Intel.
	//
        ldr     r3, =0x89
        lsl     r2, r3, #16
        orr     r2, r3
//	ldr r2, =0x00890089
	cmp r1, r2
	bne unknown_device


	//
	// Load the device IDs from each 16-bit wide FLASH.
	//
	ldrh    r1, [r0, #4]
	ldrh    r2, [r0, #6]

	//
	// See if the IDs are identical.
	//
	cmp r1, r2
	beq id_is_ok

	//
	// This is a FLASH configuration we can not handle, either an unknown
	// manufacturer, an unknown device ID, or the FLASH devices do not match.
	// Return an error.
	//

	
unknown_device: 
	mov r0, #0x00
	mov pc, lr

	//
	// The FLASH is 16-bits wide.
	//
.ltorg
// try intel, then try SST (since sst might loop forever?)
check_sst_flash_16:

//////////////////////////////// REVERSAL TEST ///////////////
	mov r3, #0x70
	lsl r3, #24
	// software product ID entry command sequence	
	ldr r0, =0x0000AAAA // was 5555
	mov r1, #0x000000AA
	strh r1,[r3,r0]

	ldr r0, =0x00005554 // was 2AAA
	mov r1, #0x00000055
	strh r1,[r3,r0]

	ldr r0, =0x0000AAAA // was 5555
	mov r1, #0x00000090
	strh r1,[r3,r0]
	
	// wait 150ns?

	ldr r0, =0x00001000
	mov r1, #0
sst_wait_loop2:
	sub r0,#1
	cmp r0,r1
	bne sst_wait_loop2
	
	// manufacturer ID at 0x0000 = 0x00BF
	// this checks out
	mov r1,#0xBF
	mov r2,#0x0000
	ldrh r0,[r3,r2]
	cmp r0,r1
	// bl SendChar
	bne unknown_device

	
#if 0	

	mov r3, #0x70
	lsl r3, #24

	// not sure if device ID works.
	//device ID at 0x0001 = 0x2781
	ldr r1,=0x2781
	mov r2,#0x0001
	ldrh r0,[r3,r2]
//	cmp r2,r1
//	bl SendChar
	// bne unknown_device
	// assume this means we've got a 1mb 16-bit part
#endif
	
	mov r3, #0x70
	lsl r3, #24

	// exit commands
	ldr r0, =0x0000AAAA // was 5555
	mov r1, #0x000000AA
	strh r1,[r3,r0]

	ldr r0, =0x00005554 // was 2AAA
	mov r1, #0x00000055
	strh r1,[r3,r0]

	ldr r0, =0x0000AAAA // was 5555
	mov r1, #0x000000F0
	strh r1,[r3,r0]

#if 0
	// reset flash into read mode
	mov r0,#0
	mov r1,#0xF0
	strh r1,[r3,r0]

#endif
	
	/////////////////////////END REVERSAL TEST

	// store size (2^20, 0x1 << 20, 1mb) in r8
	mov r0,#1
	lsl r0,r0,#21
	mov r8,r0


	// store erase code in r9
	ldr r0,=sst_39_erase
	mov r9,r0
	
	// store program code in r10
	ldr r0,=sst_39_program
	mov r10,r0

	b check_done

	.ltorg
	
check_flash_16: 
	mov r7, #0x01

	//
	// Write the read identifier command to the FLASH.
	//
	mov r1, #0x90
	strh    r1, [r0]

	//
	// Read back the manufacturer ID from the FLASH.
	//
	ldrh    r1, [r0]

	//
	// See if the ID is Intel.
	//
	cmp r1, #0x89
	beq is_intel

	//
	// Rev. B of the EP7209 reference design board has the data bus to the FLASH
	// reversed, so try the read identifier command bit reversed.
	//
	mov r1, #0x09
	lsl r1, #8
	strh    r1, [r0]

	//
	// Re-load the manufacturer ID (bit reversed?).
	//
	ldrh    r1, [r0]

	//
	// See if the ID is Intel, bit reversed.
	//
	mov r2, #0x91
	lsl r2, #8
	cmp r1, r2
	// bne unknown_device
	// sst will jump to unknown_device if it fails
	bne check_sst_flash_16

	//
	// The data bus is reversed, so set the flag to indicate this condition.
	//
	mov r1, #0x02
	orr r7, r1

	//
	// We have an Intel FLASH, so read the device ID.
	//
is_intel:   
	ldrh    r1, [r0, #2]

	//
	// On the EP73XX evaluation board, the FLASH can be jumpered to be either 32
	// or 16 bits wide.  If it is jumpered to be 16 bits wide, the LSB of the
	// FLASH (which must be high to read the device ID) is connected to A2 of
	// the processor.  In this case, the read at offset 2 will actually read the
	// manufacturer ID.  So, read the manufacturer ID and see if it matches the
	// device ID.
	//
	ldrh    r2, [r0]
	cmp r1, r2
	bne have_id

	//
	// The manufacturer ID matches the device ID, so read the device ID from
	// from offset 4 (which will assert the LSB of the FLASH, giving the real
	// device ID).
	//
	ldrh    r1, [r0, #4]

	//
	// We now have the device ID of the FLASH.  See if it is bit reversed.
	//
have_id:
	mov r2, #0x02
	tst r7, r2
	beq id_is_ok

	//
	// The device ID is bit reversed, so we need to reverse it.  We will go
	// through a sequence of reversing sets of bits in a manner similar to a
	// binary traversal.  If we consider the 16 bits to be as follows:
	//
	// fedcba98 76543210
	//
	// Then we will reverse the bits in a sequence that will make the bits look
	// as follows (if viewed at each step of the sequence):
	//
	// 76543210 fedcba98
	// 32107654 ba98fedc
	// 10325476 98badcfe
	// 01234567 89abcdef
	//
	// At each step, a smaller set of bits is swapped, and the same swap is
	// performed across the entire 16-bit word.
	//
	// Start by swapping the two bytes.
	//
	mov r2, r1
	lsl r2, #8
	lsr r1, #8
	orr r1, r2

	//
	// Now, swap the two nibbles of each byte.
	//
	mov r2, r1
//	ldr r3, =0x0000f0f0
        ldr     r3, =0xf0
        lsl     r3, r3, #8
        add     r3, r3, #0xf0
	lsl r2, #4
	and r1, r3
	and r2, r3
	lsr r1, #4
	orr r1, r2

	//
	// Now, swap the two halves of each nibble.
	//
	mov r2, r1
//	ldr r3, =0x0000cccc
        ldr     r3, =0xcc
        lsl     r3, r3, #8
        add     r3, r3, #0xcc
	lsl r2, #2
	and r1, r3
	and r2, r3
	lsr r1, #2
	orr r1, r2

	//
	// Finally, swap each pair of bits.
	//
	mov r2, r1
//	ldr r3, =0x0000aaaa
        ldr     r3, =0xaa
        lsl     r3, r3, #8
        add     r3, r3, #0xaa
	lsl r2, #1
	and r1, r3
	and r2, r3
	lsr r1, #1
	orr r1, r2

	//
	// We now have the device ID.  See if this is a 32Mb Intel J3 FLASH.
	//
id_is_ok:   
	cmp r1, #0x16
	beq intel_j3_32Mb

	//
	// See if this is a 64Mb Intel J3 FLASH.
	//
	cmp r1, #0x17
	beq intel_j3_64Mb

	//
	// See if this is a 128Mb Intel J3 FLASH.
	//
	cmp r1, #0x18
	beq intel_j3_128Mb

	//
	// The remaining FLASH device IDs that we recognize are all 0x88.., so load
	// 0x8800 into r3.
	//
	mov r3, #0x88
	lsl r3, #8
#if 0
	//
	// See if this is a 16Mb Intel B3 FLASH.
	//
	mov r2, #0x91
	orr r2, r3
	cmp r1, r2
	beq intel_b3_16Mb

	//
	// See if this is a 8Mb Intel B3 FLASH.
	//
	mov r2, #0x93
	orr r2, r3
	cmp r1, r2
	beq intel_b3_8Mb

	//
	// See if this is a 4Mb Intel B3 FLASH.
	//
	mov r2, #0x95
	orr r2, r3
	cmp r1, r2
	beq intel_b3_4Mb

	//
	// See if this is a 32Mb Intel B3 FLASH.
	//
	mov r2, #0x97
	orr r2, r3
	cmp r1, r2
	beq intel_b3_32Mb
#endif
	//
	// See if this is a 8Mb Intel C3 FLASH.
	//
	mov r2, #0xc1
	orr r2, r3
	cmp r1, r2
	beq intel_c3_8Mb

	//
	// See if this is a 16Mb Intel C3 FLASH.
	//
	mov r2, #0xc3
	orr r2, r3
	cmp r1, r2
	beq intel_c3_16Mb

	//
	// See if this is a 32Mb Intel C3 FLASH.
	//
	mov r2, #0xc5
	orr r2, r3
	cmp r1, r2
	beq intel_c3_32Mb

	//
	// See if this is a 64Mb Intel C3 FLASH.
	//
	mov r2, #0xcd
	orr r2, r3
	cmp r1, r2
	beq intel_c3_64Mb

	//
	// We do not recognize the device ID, so this is an unknown device.
	//
	b   unknown_device
.ltorg
	//
	// This is a 4Mb Intel B3 FLASH.  Set the size to 1, which will get shifted
	// by 19 to create 512KB.
	//
	#if 0
intel_b3_4Mb:   
	mov r0, #0x01

	//
	// Shift the size by 19 bits.
	//
intel_b3:   
	lsl r0, #19

	//
	// Save the size of the FLASH.
	//
	mov r8, r0

	//
	// Save the pointer to the routine to erase an Intel B3 FLASH.
	//
//	ldr r0, =intel_b3_erase
        mov     r0, pc
        add     r0, r0, #16
        ldr     r0, [r0]
	mov r9, r0

	//
	// Save the pointer to the routine to program an Intel B3 FLASH.
	//
//	ldr r0, =intel_b3_c3_program
        mov     r0, pc
        add     r0, r0, #8
        ldr     r0, [r0]
	mov r10, r0

	//
	// Perform the size check.
	//
	b   check_size

        .long   intel_b3_erase
        .long   intel_b3_c3_program
        
	//
	// This is a 8Mb Intel B3 FLASH.  Set the size to 2, which will get shifted
	// by 19 to create 1MB.
	//
intel_b3_8Mb:   
	mov r0, #0x02

	//
	// The remainder of the handling is common.
	//
	b   intel_b3

	//
	// This is a 16Mb Intel B3 FLASH.  Set the size to 4, which will get shifted
	// by 19 to create 2MB.
	//
intel_b3_16Mb:  
	mov r0, #0x04

	//
	// The remainder of the handling is common.
	//
	b   intel_b3

	//
	// This is a 32Mb Intel B3 FLASH.  Set the size to 8, which will get shifted
	// by 19 to create 4MB.
	//
intel_b3_32Mb:  
	mov r0, #0x08

	//
	// The remainder of the handling is common.
	//
	b   intel_b3

	//
	// This is a 8Mb Intel C3 FLASH.  Set the size to 1, which will get shifted
	// by 20 to create 1MB.
	//
#endif
intel_c3_8Mb:   
	mov r0, #0x01

	//
	// Shift the size by 20 bits.
	//
intel_c3:   
	lsl r0, #20

	//
	// Save the size of the FLASH.
	//
	mov r8, r0

	//
	// Save the pointer to the routine to erase an Intel C3 FLASH.
	//
//	ldr r0, =intel_c3_erase
//	mov r9, r0
        mov     r0, pc
        add     r0, r0, #14
        ldr     r0, [r0]
        mov     r9, r0

	//
	// Save the pointer to the routine to program an Intel C3 FLASH.
	//
//	ldr r0, =intel_b3_c3_program
//	mov r10, r0
        mov     r0, pc
        add     r0, r0, #10
        ldr     r0, [r0]
        mov     r10, r0

	//
	// Perform the size check.
	//
	b   check_size

        .long   intel_c3_erase
        .long   intel_b3_c3_program
        
	//
	// This is a 16Mb Intel C3 FLASH.  Set the size to 2, which will get shifted
	// by 20 to create 2MB.
	//
intel_c3_16Mb:  
	mov r0, #0x02

	//
	// The remainder of the handling is common.
	//
	b   intel_c3

	//
	// This is a 32Mb Intel C3 FLASH.  Set the size to 4, which will get shifted
	// by 20 to create 4MB.
	//
intel_c3_32Mb:  
	mov r0, #0x04

	//
	// The remainder of the handling is common.
	//
	b   intel_c3

	//
	// This is a 64Mb Intel C3 FLASH.  Set the size to 4, which will get shifted
	// by 20 to create 8MB.
	//
intel_c3_64Mb:  
	mov r0, #0x08

	//
	// The remainder of the handling is common.
	//
	b   intel_c3

	//
	// This is a 32Mb Intel J3 FLASH.  Set the size to 1, which will get shifted
	// by 22 to create 4MB.
	//
intel_j3_32Mb:  
	mov r0, #0x01

	//b unknown_device

	// Shift the size by 22 bits.
	//
intel_j3:   
	lsl r0, #22

	//
	// Save the size of the FLASH.
	//
	mov r8, r0

	//
	// Get a pointer to the routines for erasing and program an Intel J3 FLASH.
	//
//	ldr r0, =intel_j3_erase
//	ldr r1, =intel_j3_program
        mov     r0, pc
        add     r0, #10
        ldr     r0, [r0]
        
        mov     r1, pc
        add     r1, #8
        ldr     r1, [r1]

	        b       f
        
        .long   intel_j3_erase
        .long   intel_j3_program
f:      
        
	//
	// See if we have a reversed data bus.
	//
	mov r2, #2
	tst r7, r2
	beq save_j3

	//
	// Increment the address of the erase and program routines by 4, so that we
	// will use the bit reversed versions.
	//
	add r0, #4
	add r1, #4

	//
	// Save the pointers to the routines to erase and program an Intel J3 FLASH.
	//
save_j3:
	mov r9, r0
	mov r10, r1

	//
	// Perform the size check.
	//
	b   check_size

	//
	// This is a 64Mb Intel J3 FLASH.  Set the size to 2, which will get shifted
	// by 22 to create 8MB.
	//
intel_j3_64Mb:  
	mov r0, #0x02

	//
	// The remainder of the handling is common.
	//
	b   intel_j3

	//
	// This is a 128Mb Intel J3 FLASH.  Set the size to 4, which will get
	// shifted by 22 to create 16MB.
	//
intel_j3_128Mb: 
	mov r0, #0x04

	//
	// The remainder of the handling is common.
	//
	b   intel_j3

	//
	// We now have the size of the FLASH and pointers to the routines that will
	// erase and program it.  See if we are in a 32-bit wide configuration, in
	// which case we must double the size of the FLASH (since there are two 16
	// bit wides devices creating the 32-bit wide "device").
	//
check_size: 
	mov r0, #0x01
	tst r7, r0
	bne check_done

	//
	// The FLASH is 32-bits wide, so double the size.
	//
	mov r0, r8
	lsl r0, #1
	mov r8, r0

	//
	// Increment the address of the erase and program routines by 2, so that we
	// will use the 32-bit wide versions instead of the 16-bit wide versions.
	//
	mov r0, r9
	add r0, #2
	mov r9, r0
	mov r0, r10
	add r0, #2
	mov r10, r0

	//
	// We've completed our checking successfully.  Return success.
	//
check_done: 
	mov r0, #0x01
	mov pc, lr

	// what the hell
.ltorg
	
	//
	// Tell the assembler to place the literal pool here.
	//

	//*****************************************************************************
	//
	// Waits for an internal erase or program operation to complete on a 16-bit
	// wide Intel FLASH (B3, C3, or J3).
	//
	//*****************************************************************************
        .thumb_func
intel_wait_16:  
	//
	// Load the bit mask for which we are waiting.
	//
	mov r1, #0x80

	//
	// Read from the FLASH memory until bit 7 is one.
	//
intel_wait_16_loop: 
	ldrh    r2, [r0]
	and r2, r1
	cmp r2, r1
	bne intel_wait_16_loop

	//
	// Clear the status register in the FLASH.
	//
	mov r1, #0x50
	strh    r1, [r0]

	//
	// Return to the caller.
	//
	mov pc, lr

	//*****************************************************************************
	//
	// Waits for an internal erase or program operation to complete on a 16-bit
	// wide Intel FLASH (B3, C3, or J3) with a reversed data bus.
	//
	//*****************************************************************************
        .thumb_func
intel_wait_16_rev:  
	//
	// Load the bit mask for which we are waiting.
	//
	mov r1, #0x01
	lsl r1, #8

	//
	// Read from the FLASH memory until bit 7 is one.
	//
intel_wait_16_rev_loop: 
	ldrh    r2, [r0]
	and r2, r1
	cmp r2, r1
	bne intel_wait_16_rev_loop

	//
	// Clear the status register in the FLASH.
	//
	mov r1, #0x0a
	lsl r1, #8
	strh    r1, [r0]

	//
	// Return to the caller.
	//
	mov pc, lr

	//*****************************************************************************
	//
	// Waits for an internal erase or program operation to complete on a 32-bit
	// wide Intel FLASH (B3, C3, or J3).
	//
	//*****************************************************************************
        .thumb_func
intel_wait_32:  
	//
	// Load the bit mask for which we are waiting.
	//
	ldr r1, =0x00800080

	//
	// Read from the FLASH memory until bit 7 is one.
	//
intel_wait_32_loop: 
	ldr r2, [r0]
	and r2, r1
	cmp r2, r1
	bne intel_wait_32_loop

	//
	// Clear the status register in the FLASH.
	//
	ldr r1, =0x00500050
	str r1, [r0]

	//
	// Return to the caller.
	//
	mov pc, lr

// SST 39 Flash erase commands - erases the entire 1mb chip

sst_39_erase:
	
	push    {r4-r7, lr}

	mov r3, #0x70
	lsl r3, #24

        // use r4 to hold the block address
        mov     r4, r3
        // use r5 to hold the block limit
        mov     r5, r0
        add     r5, r4
        
sst_39_erase_loop:      
        
	// 5555
	ldr     r0, =0x0000AAAA
	mov     r1, #0x000000AA
	strh    r1, [r3,r0]		

	// 2AAA
	ldr     r0, =0x00005554
	mov     r1, #0x00000055
	strh    r1, [r3,r0]		

	// 5555
	ldr     r0, =0x0000AAAA
	mov     r1, #0x00000080
	strh    r1, [r3,r0]		


	// 5555
	ldr     r0, =0x0000AAAA
	mov     r1, #0x000000AA
	strh    r1, [r3,r0]		


	// 2AAA
	ldr     r0, =0x00005554
	mov     r1, #0x00000055
	strh    r1, [r3,r0]		

	// 5555
//	ldr     r0, =0x0000AAAA
//	mov     r1, #0x00000010
//	strh    r1, [r3,r0]
        ldr     r1, =0x00000030
        strh    r1, [r4]

sst_39_erase_verify:

	// some 'random' address to read from
//	ldr     r1, =0x0000AAAA
//	ldrh    r0, [r3,r1]
        ldrh    r0, [r4]
	mov	r2, #0x80
	tst	r0, r2
	beq     sst_39_erase_verify

        ldr     r1, =0x1000
        add     r4, r1
        cmp     r4, r5
        blt     sst_39_erase_loop
        
	pop {r4-r7, pc}

// SST erase done
	
sst_39_program:
	push {r4-r7, lr}

	// data in ram, flash offset, length of data
	mov r4,r0		
	mov r5,r1		
	mov r6,r2		

	// r0 is flash base offset 0x7000 0000
	mov r0, #0x70
	lsl r0, #24

sst_39_write_loop:

	// r2 = data
	// r1 = offset
	// r0 = base
		
	cmp r6,#0
	beq sst_39_write_done

	// 5555
	ldr r1,=0xAAAA
	mov r2,#0xAA
	strh r2,[r0,r1]

	// 2AAA
	ldr r1,=0x5554
	mov r2,#0x55
	strh r2,[r0,r1]

	// 5555
	ldr r1,=0xAAAA
	mov r2,#0xA0
	strh r2,[r0,r1]

	// load new data into register
	ldrh r1,[r4]		
	strh r1,[r0,r5]		// store in flash at offset r5

	// take complement of r1, read [r0,r5] until it == r4

	ldr r3,=0x0000FFFF
	and r1,r1,r3
	
sst_39_wait:	 
	ldrh r3,[r0,r5]
	cmp r1,r3
	bne sst_39_wait 
	
	add r4,#2		// increment ram pointer
	add r5,#2		// increment offset in flash
	sub r6,#2		// decrement size
		
	b sst_39_write_loop

sst_39_write_done:	
	pop {r4-r7, pc}

	// literal pool for SST programming
.ltorg

// B3 code removed - temancl running out of space on SST
	
intel_b3_c3_program:
	b   intel_b3_c3_program_16

	//*****************************************************************************
	//
	// The routine for programming a pair of 16-bit wide Intel B3/C3 FLASHes in a
	// 32-bit configuration.
	//
	//*****************************************************************************
intel_b3_c3_program_32: 
	//
	// Save the link register to the stack.
	//
	push    {r4-r7, lr}

	//
	// Save the offset, length, and pointer to the data to be programmed into
	// the FLASH.
	//
	mov r4, r0
	mov r5, r1
	mov r6, r2

	//
	// Load a pointer to the FLASH.
	//
	mov r0, #0x70
	lsl r0, #24

	//
	// Get the command to program a block of the FLASH.
	//
	ldr r7, =0x00400040

	//
	// Program the FLASH a word at a time.
	//
intel_b3_c3_program_32_word:
	//
	// See if we have programmed all the required words.
	//
	cmp r6, #0
	beq intel_b3_c3_program_32_done

	//
	// Load the next word to be programmed.
	//
	ldr r1, [r4]
	add r4, #4

	//
	// Program the next word.
	//
	str r7, [r0, r5]
	str r1, [r0, r5]

	//
	// Wait until the word has been programmed.
	//
	bl  intel_wait_32

	//
	// Increment the address to be programmed.
	//
	add r5, #4

	//
	// Decrement the count of words to be programmed.
	//
	sub r6, #4

	//
	// Loop back.
	//
	b   intel_b3_c3_program_32_word

	//
	// We're done programming the FLASH, so put the FLASH into read array mode.
	//
intel_b3_c3_program_32_done:
	ldr r1, =0x00ff00ff
	str r1, [r0]

	//
	// Return to the caller.
	//
	pop {r4-r7, pc}

	//*****************************************************************************
	//
	// The routine for programming a 16-bit wide Intel B3/C3 FLASH.
	//
	//*****************************************************************************
intel_b3_c3_program_16: 
	//
	// Save the link register to the stack.
	//
	push    {r4-r7, lr}

	//
	// Save the offset, length, and pointer to the data to be programmed into
	// the FLASH.
	//
	mov r4, r0
	mov r5, r1
	mov r6, r2

	//
	// Load a pointer to the FLASH.
	//
	mov r0, #0x70
	lsl r0, #24

	//
	// Get the command to program a block of the FLASH.
	//
	mov r7, #0x40

	//
	// Program the FLASH a word at a time.
	//
intel_b3_c3_program_16_word:
	//
	// See if we have programmed all the required words.
	//
	cmp r6, #0
	beq intel_b3_c3_program_16_done

	//
	// Load the next word to be programmed.
	//
	ldrh    r1, [r4]
	add r4, #2

	//
	// Program the next word.
	//
	strh    r7, [r0, r5]
	strh    r1, [r0, r5]

	//
	// Wait until the word has been programmed.
	//
	bl  intel_wait_16

	//
	// Increment the address to be programmed.
	//
	add r5, #2

	//
	// Decrement the count of words to be programmed.
	//
	sub r6, #2

	//
	// Loop back.
	//
	b   intel_b3_c3_program_16_word

	//
	// We're done programming the FLASH, so put the FLASH into read array mode.
	//
intel_b3_c3_program_16_done:
	mov r1, #0xff
	str r1, [r0]

	//
	// Return to the caller.
	//
	pop {r4-r7, pc}

	//*****************************************************************************
	//
	// The routine to erase a Intel C3 FLASH.  This must be a branch to the routine
	// for a single 16-bit wide FLASH, immediately followed by the routine for a
	// 32-bit wide FLASH (i.e. two 16-bit wide FLASHes).
	//
	//*****************************************************************************
intel_c3_erase: 
	b   intel_c3_erase_16

	//*****************************************************************************
	//
	// The routine for erasing a pair of 16-bit wide Intel C3 FLASHes in a 32-bit
	// configuration.
	//
	//*****************************************************************************
intel_c3_erase_32:  
	//
	// Save the link register to the stack.
	//
	push    {r4-r7, lr}

	//
	// Save the length of the FLASH to be erased.
	//
	mov r4, r0

	//
	// Load a pointer to the FLASH.
	//
	mov r0, #0x70
	lsl r0, #24

	//
	// Get the command to unlock a block of the FLASH.
	//
	ldr r3, =0x00600060

	//
	// Get the commands to erase a block of the FLASH.
	//
	ldr r6, =0x00200020
	ldr r7, =0x00d000d0

	//
	// Start erasing the FLASH from block zero.
	//
	mov r5, #0x00
intel_c3_erase_32_block:
	//
	// See if we have erased all the required blocks.
	//
	cmp r5, r4
	bge intel_c3_erase_32_done

	//
	// Unlock this block.
	//
	str r3, [r0, r5]
	str r7, [r0, r5]

	//
	// Erase this block.
	//
	str r6, [r0, r5]
	str r7, [r0, r5]

	//
	// Wait until the erase has completed.
	//
	bl  intel_wait_32

	//
	// Increment the block address.  The first 8 blocks of the FLASH have a
	// block size of 16K and the remaining blocks have a size of 128K.  So,
	// if the offset is less than 128K, increment it by 16K, otherwise
	// increment it by 128K.
	//
	mov r1, #0x02
	lsl r1, #16
	cmp r5, r1
	blt intel_c3_erase_32_small
	mov r1, #0x20
	b   intel_c3_erase_32_increment
intel_c3_erase_32_small:
	mov r1, #0x04
intel_c3_erase_32_increment:
	lsl r1, #12
	add r5, r1

	//
	// Loop back.
	//
	b   intel_c3_erase_32_block

	//
	// We're done erasing the FLASH, so return to the caller.
	//
intel_c3_erase_32_done: 
	pop {r4-r7, pc}

	//*****************************************************************************
	//
	// The routine for erasing a single 16-bit wide Intel C3 FLASH.
	//
	//*****************************************************************************
intel_c3_erase_16:  
	//
	// Save the link register to the stack.
	//
	push    {r4-r7, lr}

	//
	// Save the length of the FLASH to be erased.
	//
	mov r4, r0

	//
	// Load a pointer to the FLASH.
	//
	mov r0, #0x70
	lsl r0, #24

	//
	// Get the command to unlock a block of the FLASH.
	//
	mov r3, #0x60

	//
	// Get the commands to erase a block of the FLASH.
	//
	mov r6, #0x20
	mov r7, #0xd0

	//
	// Start erasing the FLASH from block zero.
	//
	mov r5, #0x00
intel_c3_erase_16_block:
	//
	// See if we have erased all the required blocks.
	//
	cmp r5, r4
	bge intel_c3_erase_16_done

	//
	// Unlock this block.
	//
	strh    r3, [r0, r5]
	strh    r7, [r0, r5]

	//
	// Erase this block.
	//
	strh    r6, [r0, r5]
	strh    r7, [r0, r5]

	//
	// Wait until the erase has completed.
	//
	bl  intel_wait_16

	//
	// Increment the block address.  The first 8 blocks of the FLASH have a
	// block size of 8K and the remaining blocks have a size of 64K.  So,
	// if the offset is less than 64K, increment it by 8K, otherwise
	// increment it by 64K.
	//
	mov r1, #0x01
	lsl r1, #16
	cmp r5, r1
	blt intel_c3_erase_16_small
	mov r1, #0x10
	b   intel_c3_erase_16_increment
intel_c3_erase_16_small:
	mov r1, #0x02
intel_c3_erase_16_increment:
	lsl r1, #12
	add r5, r1

	//
	// Loop back.
	//
	b   intel_c3_erase_16_block
.ltorg
	//
	// We're done erasing the FLASH, so return to the caller.
	//
intel_c3_erase_16_done: 
	pop {r4-r7, pc}

	//*****************************************************************************
	//
	// The routine to erase a Intel J3 FLASH.  This must be a branch to the routine
	// for a single 16-bit wide FLASH, immediately followed by a branch to the
	// routine for a 32-bit wide FLASH (i.e. two 16-bit wide FLASHes), immediately
	// followed by the routien for a single 16-bit wide FLASH with a reversed data
	// bus.
	//
	//*****************************************************************************
intel_j3_erase: 
	b   intel_j3_erase_16
	b   intel_j3_erase_32

	//*****************************************************************************
	//
	// The routine for erasing a 16-bit wide Intel J3 FLASH with a reversed data
	// bus.
	//
	//*****************************************************************************
intel_j3_erase_16_rev:  
        //
	// Save the link register to the stack.
	//
	push    {r4-r7, lr}

	//
	// Save the length of the FLASH to be erased.
	//
	mov r4, r0

	//
	// Load a pointer to the FLASH.
	//
	mov r0, #0x70
	lsl r0, #24

	//
	// Get the commands to erase a block of the FLASH.
	//
	mov r6, #0x04
	lsl r6, #8
	mov r7, #0x0b
	lsl r7, #8

	//
	// Start erasing the FLASH from block zero.
	//
	mov r5, #0x00
intel_j3_erase_16_rev_block:
	//
	// See if we have erased all the required blocks.
	//
	cmp r5, r4
	bge intel_j3_erase_16_rev_done

	//
	// Erase this block.
	//
	strh    r6, [r0, r5]
	strh    r7, [r0, r5]

	//
	// Wait until the erase has completed.
	//
	bl  intel_wait_16_rev

	//
	// Increment the block address.  Each block is 128K.
	//
	mov r1, #0x02
	lsl r1, #16
	add r5, r1

	//
	// Loop back.
	//
	b   intel_j3_erase_16_rev_block

	//
	// We're done erasing the FLASH, so return to the caller.
	//
intel_j3_erase_16_rev_done: 
	pop {r4-r7, pc}

	//*****************************************************************************
	//
	// The routine for erasing a pair of 16-bit wide Intel J3 FLASHes in a 32-bit
	// configuration.
	//
	//*****************************************************************************
intel_j3_erase_32:  
	//
	// Save the link register to the stack.
	//
	push    {r4-r7, lr}

	//
	// Save the length of the FLASH to be erased.
	//
	mov r4, r0

	//
	// Load a pointer to the FLASH.
	//
	mov r0, #0x70
	lsl r0, #24

	//
	// Get the commands to erase a block of the FLASH.
	//
	ldr r6, =0x00200020
	ldr r7, =0x00d000d0

	//
	// Start erasing the FLASH from block zero.
	//
	mov r5, #0x00
intel_j3_erase_32_block:
	//
	// See if we have erased all the required blocks.
	//
	cmp r5, r4
	bge intel_j3_erase_32_done

	//
	// Erase this block.
	//
	str r6, [r0, r5]
	str r7, [r0, r5]

	//
	// Wait until the erase has completed.
	//
	bl  intel_wait_32

	//
	// Increment the block address.  Each block is 256K.
	//
	mov r1, #0x04
	lsl r1, #16
	add r5, r1

	//
	// Loop back.
	//
	b   intel_j3_erase_32_block

	//
	// We're done erasing the FLASH, so return to the caller.
	//
intel_j3_erase_32_done: 
	pop {r4-r7, pc}

	//*****************************************************************************
	//
	// The routine for erasing a 16-bit wide Intel J3 FLASH.
	//
	//*****************************************************************************
intel_j3_erase_16:  
	//
	// Save the link register to the stack.
	//
	push    {r4-r7, lr}

	//
	// Save the length of the FLASH to be erased.
	//
	mov r4, r0

	//
	// Load a pointer to the FLASH.
	//
	mov r0, #0x70
	lsl r0, #24

	//
	// Get the commands to erase a block of the FLASH.
	//
	mov r6, #0x20
	mov r7, #0xd0

	//
	// Start erasing the FLASH from block zero.
	//
	mov r5, #0x00
intel_j3_erase_16_block:
	//
	// See if we have erased all the required blocks.
	//
	cmp r5, r4
	bge intel_j3_erase_16_done

	//
	// Erase this block.
	//
	strh    r6, [r0, r5]
	strh    r7, [r0, r5]

	//
	// Wait until the erase has completed.
	//
	bl  intel_wait_16

	//
	// Increment the block address.  Each block is 128K.
	//
	mov r1, #0x02
	lsl r1, #16
	add r5, r1

	//
	// Loop back.
	//
	b   intel_j3_erase_16_block

	//
	// We're done erasing the FLASH, so return to the caller.
	//
intel_j3_erase_16_done: 
	pop {r4-r7, pc}

	//*****************************************************************************
	//
	// The routine to program a Intel J3 FLASH.  This must be a branch to the
	// routine for a single 16-bit wide FLASH, immediately followed by a branch to
	// the routine for a 32-bit wide FLASH (i.e. two 16-bit wide FLASHes),
	// immediately followed by the routine for a single 16-bit wide FLASH with a
	// reversed data bus.
	//
	//*****************************************************************************
intel_j3_program:   
	b   intel_j3_program_16
	b   intel_j3_program_32

	//*****************************************************************************
	//
	// The routine for programming a 16-bit wide Intel J3 FLASH with a reversed
	// data bus.
	//
	//*****************************************************************************
intel_j3_program_16_rev:
	//
	// Save the link register to the stack.
	//
	push    {r4-r7, lr}

	//
	// Save the offset, length, and pointer to the data to be programmed into
	// the FLASH.
	//
	mov r4, r0
	mov r5, r1
	mov r6, r2

	//
	// Load a pointer to the FLASH.
	//
	mov r0, #0x70
	lsl r0, #24

	//
	// Program the FLASH a sixteen words at a time.
	//
intel_j3_program_16_rev_16words:
	//
	// See if we have programmed all the required words.
	//
	cmp r6, #0
	beq intel_j3_program_16_rev_done

	//
	// Send the write to buffer command.
	//
	mov r1, #0x17
	lsl r1, #8
	strh    r1, [r0, r5]
	mov r1, #0xf0
	lsl r1, #8
	strh    r1, [r0, r5]

	//
	// Write the next 16 words to the FLASH.
	//
	mov r2, #0x10
	mov r3, r5
intel_j3_program_16_rev_word:   
	//
	// Load the next word to be programmed.
	//
	ldrh    r1, [r4]
	add r4, #2

	//
	// Write it to the FLASH.
	//
	strh    r1, [r0, r3]
	add r3, #2

	//
	// Decrement the count of words to write.
	//
	sub r2, #1
	bne intel_j3_program_16_rev_word

	//
	// Write the program confirm command.
	//
	mov r1, #0x0b
	lsl r1, #8
	strh    r1, [r0, r5]

	//
	// Wait until the words have been programmed.
	//
	bl  intel_wait_16_rev

	//
	// Increment the address to be programmed.
	//
	add r5, #32

	//
	// Decrement the count of words to be programmed.
	//
	sub r6, #32

	//
	// Loop back.
	//
	b   intel_j3_program_16_rev_16words

	//
	// We're done programming the FLASH, so put the FLASH into read array mode.
	//
intel_j3_program_16_rev_done:   
	mov r1, #0xff
	lsl r1, #8
	strh    r1, [r0]

	//
	// Return to the caller.
	//
	pop {r4-r7, pc}

	//*****************************************************************************
	//
	// The routine for programming a pair of 16-bit wide Intel J3 FLASHes.
	//
	//*****************************************************************************
intel_j3_program_32:
	//
	// Save the link register to the stack.
	//
	push    {r4-r7, lr}

	//
	// Save the offset, length, and pointer to the data to be programmed into
	// the FLASH.
	//
	mov r4, r0
	mov r5, r1
	mov r6, r2

	//
	// Load a pointer to the FLASH.
	//
	mov r0, #0x70
	lsl r0, #24

// unlock a block of flash, just put the offset
// in r2
//        ldr     r1, =0x00600060
//        ldr     r2, =0x7c0000
//        str     r1, [r0, r2]
//        ldr     r1, =0x00D000D0
//        str     r1, [r0, r2]
// 0:
//        b       0b
        
	//
	// Program the FLASH a sixteen words at a time.
	//
intel_j3_program_32_16words:
	//
	// See if we have programmed all the required words.
	//
	cmp r6, #0
	beq intel_j3_program_32_done

	//
	// Send the write to buffer command.
	//
	ldr r1, =0x00e800e8
	str r1, [r0, r5]
	ldr r1, =0x000f000f
	str r1, [r0, r5]

	//
	// Write the next 16 words to the FLASH.
	//
	mov r2, #0x10
	mov r3, r5
intel_j3_program_32_word:
        
	//
	// Load the next word to be programmed.
	//
	ldr r1, [r4]
	add r4, #4

	//
	// Write it to the FLASH.
	//
	str r1, [r0, r3]
	add r3, r3, #4

	//
	// Decrement the count of words to write.
	//
	sub r2, #1
	bne intel_j3_program_32_word

	//
	// Write the program confirm command.
	//
	ldr r1, =0x00d000d0
	str r1, [r0, r5]

	//
	// Wait until the words have been programmed.
	//
	bl  intel_wait_32

	//
	// Increment the address to be programmed.
	//
	add r5, #64

	//
	// Decrement the count of words to be programmed.
	//
	sub r6, #64

	//
	// Loop back.
	//
	b   intel_j3_program_32_16words

	//
	// We're done programming the FLASH, so put the FLASH into read array mode.
	//
intel_j3_program_32_done:   
	ldr r1, =0x00ff00ff
	str r1, [r0]

	//
	// Return to the caller.
	//
	pop {r4-r7, pc}

	//*****************************************************************************
	//
	// The routine for programming a 16-bit wide Intel J3 FLASH.
	//
	//*****************************************************************************
intel_j3_program_16:

        //
	// Save the link register to the stack.
	//
	push    {r4-r7, lr}

	//
	// Save the offset, length, and pointer to the data to be programmed into
	// the FLASH.
	//
	mov r4, r0
	mov r5, r1
	mov r6, r2

	//
	// Load a pointer to the FLASH.
	//
	mov r0, #0x70
	lsl r0, #24

	//
	// Program the FLASH a sixteen words at a time.
	//
intel_j3_program_16_16words:
	//
	// See if we have programmed all the required words.
	//
	cmp r6, #0
	beq intel_j3_program_16_done

	//
	// Send the write to buffer command.
	//
	mov r1, #0xe8
	strh    r1, [r0, r5]
	mov r1, #0x0f
	strh    r1, [r0, r5]

	//
	// Write the next 16 words to the FLASH.
	//
	mov r2, #0x10
	mov r3, r5
intel_j3_program_16_word:   
	//
	// Load the next word to be programmed.
	//
	ldrh    r1, [r4]
	add r4, #2

	//
	// Write it to the FLASH.
	//
	strh    r1, [r0, r3]
	add r3, #2

	//
	// Decrement the count of words to write.
	//
	sub r2, #1
	bne intel_j3_program_16_word

	//
	// Write the program confirm command.
	//
	mov r1, #0xd0
	strh    r1, [r0, r5]

	//
	// Wait until the words have been programmed.
	//
	bl  intel_wait_16

	//
	// Increment the address to be programmed.
	//
	add r5, #32

	//
	// Decrement the count of words to be programmed.
	//
	sub r6, #32

	//
	// Loop back.
	//
	b   intel_j3_program_16_16words

	//
	// We're done programming the FLASH, so put the FLASH into read array mode.
	//
intel_j3_program_16_done:   
	mov r1, #0xff
	strh    r1, [r0]

	//
	// Return to the caller.
	//
	pop {r4-r7, pc}

	//*****************************************************************************
	//
	// SendChar sends a character to UART1.
	//
	//*****************************************************************************
        .thumb_func
SendChar:   
	//
	// Load a pointer to the system status register.
	//
	ldr r1, =0x80000140

	//
	// Wait until there is space in the transmit FIFO for UART1.
	//
	mov r3, #0x80
	lsl r3, #16
sendchar0:  
	ldr r2, [r1]
	tst r2, r3
	bne sendchar0

	//
	// Write the character to UART1.
	//
	ldr r1, =0x80000480
	str r0, [r1]

	//
	// Return to the caller.
	//
	mov pc, lr


        //*****************************************************************************
        //
        // SendLong sends a long to UART1
        //
        //*****************************************************************************
        .thumb_func
SendLong:
        mov     r5, lr
        mov     r4, r0
        lsr     r0, #24
        bl      SendChar
        mov     r0, r4
        lsr     r0, #16
        bl      SendChar
        mov     r0, r4
        lsr     r0, #8
        bl      SendChar
        mov     r0, r4
        bl      SendChar

        mov     pc, r5
        
	//*****************************************************************************
	//
	// ReceiveChar receives a character from UART1.
	//
	//*****************************************************************************
        .thumb_func
ReceiveChar:
	//
	// Load a pointer to the system status register.
	//
	ldr r1, =0x80000140

	//
	// Wait until there is a character in the receive FIFO for UART1.
	//
	mov r2, #0x40
	lsl r2, #16
receivechar0:   
	ldr r0, [r1]
	tst r0, r2
	bne receivechar0

	//
	// Read the character from UART1.
	//
	ldr r1, =0x80000480
	ldr r0, [r1]
	mov r1, #0xff
	and r0, r1

	//
	// Return to the caller.
	//
	mov pc, lr

	//*****************************************************************************
	//
	// SetBaud sets the baud rate to the specified rate.
	//
	//*****************************************************************************
	SetBaud:
	//
	// Save the link register to the stack.
	//
	push    {lr}

	//
	// Read the baud rate indicator from the host.
	//
	bl  ReceiveChar

	//
	// Get the baud rate divisor based on the requested baud rate.
	//
	mov r1, #0x17
	cmp r0, #0x30
	beq setbaud0
	mov r1, #0x0b
	cmp r0, #0x31
	beq setbaud0
	mov r1, #0x05
	cmp r0, #0x32
	beq setbaud0
	mov r1, #0x03
	cmp r0, #0x33
	beq setbaud0
	mov r1, #0x01

	//
	// Set the data length to 8 bits per character and enable the FIFO.
	//
	setbaud0:   
	mov r0, #0x07
	lsl r0, #16
	orr r1, r0

	//
	// Change the configuration of UART1.
	//
	ldr r0, =0x800004c0
	str r1, [r0]

	//
	// Wait until we receive a '-' character from the host.
	//
	setbaud1:   
	bl  ReceiveChar
	cmp r0, #0x2d
	bne setbaud1

	//
	// Return to the caller.
	//
	pop {pc}

	//*****************************************************************************
	//
	// ReadLong reads a 32-bit value from the serial port.
	//
	//*****************************************************************************
        .thumb_func
ReadLong:   
	//
	// Save the link register to the stack.
	//
	push    {r4, lr}

	//
	// Read the first byte.
	//
	bl  ReceiveChar

	//
	// The first byte is the least significant 8 bits of the long value.
	//
	mov r4, r0

	//
	// Read the second byte.
	//
	bl  ReceiveChar

	//
	// The second byte is the next 8 significant bits of the long value.
	//
	lsl r0, #8
	orr r4, r0

	//
	// Read the third byte.
	//
	bl  ReceiveChar

	//
	// The third byte is the next 8 significant bits of the long value.
	//
	lsl r0, #16
	orr r4, r0

	//
	// Read the fourth byte.
	//
	bl  ReceiveChar

	//
	// The fourth byte is the most significant 8 bits of the long value.
	//
	lsl r0, #24
	orr r0, r4

	//
	// Return to the caller.
	//
	pop {r4, pc}

	//*****************************************************************************
	//
	// ReadBlock reads a 1K block of data from the host.
	//
	//*****************************************************************************
        .thumb_func
ReadBlock:  
	//
	// Save the link register to the stack.
	//
	push    {r4-r5, lr}

	//
	// Load a pointer to the data buffer.
	//
	ldr r4, =DataBuffer

	//
	// Read 1K bytes of data.
	//
	mov r5, #0x04
	lsl r5, #8
	read_byte:  
	//
	// Read the next byte of data.
	//
	bl  ReceiveChar

	//
	// Store this character in the data buffer.
	//
	strb    r0, [r4]
	add r4, #1

	//
	// Decrement the count of bytes.
	//
	sub r5, #1
	bne read_byte

	//
	// Return to the caller.
	//
	pop {r4-r5, pc}

	//*****************************************************************************
	//
	// ProgramFlash programs the data receive via the serial port into the on-board
	// FLASH.
	//
	//*****************************************************************************
ProgramFlash:   
	//
	// Save the link register to the stack.
	//
	push    {lr}

	//
	// Read the length of the data to be programmed.
	//
	bl  ReadLong
	mov r4, r0

	//
	// See if the data is small enough to fit into the FLASH.
	//
	mov r1, r8
	cmp r0, r1
	ble size_ok

	//
	// Tell the host that the data is too large.
	//
	mov r0, #0x26 // '&'
	bl  SendChar
        
        //
        // Indicate the measured size of the flash
        //
        mov r0, r8
        bl  SendLong

	//
	// We're done.
	//
	pop {pc}

	//
	// Erase the FLASH.
	//
size_ok:

	mov lr, pc
	mov pc, r9
	//b sst_39_erase

	
	//
	// Tell the host that we're ready to start receiving file data.
	//
	mov r0, #0x21 // '!'
	bl  SendChar

	
	//
	// Now, read data in 1K blocks until we've read all the file data.
	//
	mov r5, #0x00
	mov r11, r5
read_block: 
	//
	// Read 1K bytes from the serial port.
	//
	bl  ReadBlock

	//
	// Read the checksum from the host.
	//
	bl  ReadLong

	//
	// Checksum the data we read.
	//
	mov r1, #0x04
	lsl r1, #8
	ldr r2, =DataBuffer
checksum:   
	//
	// Read this byte of the data buffer.
	//
	ldrb    r3, [r2]
	add r2, #1

	//
	// Subtract this byte from the checksum from the host.
	//
	sub r0, r3

	//
	// Decrement the count of bytes.
	//
	sub r1, #1
	bne checksum

	//
	// See if the checksum matched (i.e. the counter is now zero).
	//
	cmp r0, #0x00
	beq checksum_ok

	//
	// The checksum was bad, so ask the host to send this block again.
	//
	mov r0, #0x40 // '@'
	bl  SendChar
	b   read_block

	//
	// The checksum matched, so program this block of data into the FLASH.
	// First, get some values which we'll need.
	//
checksum_ok:
	ldr r0, =DataBuffer
	mov r1, r5
	mov r2, #0x04
	lsl r2, #8
	mov lr, pc
	 mov pc, r10
	//b sst_39_program

	//
	// Verify that the data was programmed correctly.
	//
	ldr r0, =DataBuffer
	mov r1, #0x70
	lsl r1, #24
	add r1, r5
	mov r2, #0x04
	lsl r2, #8
verify_word:
	//
	// Load this word from FLASH and from the internal buffer.
	//
	ldr r6, [r0]
	add r0, #4
	ldr r7, [r1]
	add r1, #4

	//
	// See if the two words are identical.
	//
	cmp r6, r7
	beq word_ok

	//
	// Indicate that there was an error.
	//
	mov r11, r0

	//
	// Decrement the word count.
	//
	word_ok:
	sub r2, #4
	bne verify_word

	//
	// Tell the host that we're done with this block and are ready for the
	// next one.
	//
	mov r0, #0x23 // '#'
	bl  SendChar

	//
	// Increment the block address.
	//
	mov r0, #0x04
	lsl r0, #8
	add r5, r0

	//
	// Decrement the count of bytes to be programmed.
	//
	sub r4, r0

	//
	// Loop back to the next block.
	//
	bne read_block

	//
	// See if there was an error programming the FLASH.
	//
	mov r0, r11
	cmp r0, #0x00
	beq program_done

	//
	// Tell the host that there was an error programming the FLASH.
	//
	mov r0, #0x5e // '^'
	bl  SendChar

#if 1
        mov     r0, r11
        bl      SendLong
#if 0
        bl      SendChar
        lsr     r0, r0, #8
        bl      SendChar
        lsr     r0, r0, #8
        bl      SendChar
        lsr     r0, r0, #8
        bl      SendChar
#endif
#endif
        
	//
	// Tell the host that we're done programming the FLASH.
	//
program_done:   
	mov r0, #0x2a // '*'
	bl  SendChar

	//
	// Return to the caller.
	//
	pop {pc}
//.ltorg
	//*****************************************************************************
	//
	// The zero-initialized read-write data used by the application.
	//
	//*****************************************************************************
	//AREAFlasher_Data, DATA, NOINIT
	.data

	//*****************************************************************************
	//
	// A buffer to contain 1K bytes of data read from the serial port that is to be
	// programmed into the FLASH.
	//
	//*****************************************************************************
	DataBuffer: 
	.long   0x100

	//END





