// mmc_fast_c0.S: assembly versions of the >= 1 byte mmc routines
// Dan Conti danc@iobjects.com 02/20/01
// (c) Interactive Objects. We sexi baby, oh yes, we sexi.

#define ASM_HEADERS
#include "mmc_phys.h"
#include "mmc_constants.h"

	.file	"mmc_fast_c0.S"
	.code	32

	// set the registers up
	.macro __mmc_setup_regs
	stmdb	sp!, {r4-r9}
	mov	r8, #HW_MMC_PORT0
	.endm

	// restore registers
	.macro __mmc_restore_regs
	ldmia	sp!, {r4-r9}
	.endm


	// precondition: r4 is value to write, r5 is clock-hi equivilant
	.macro __mmc_write_port
	strb	r4, [r8]
	strb	r5, [r8]
//	mov	r0, r0
	strb	r4, [r8]
	.endm

	// precondition: r4 is clock low port value, r5 is clock high
	// postcondition: r9 is the port value
	.macro __mmc_read_port
	strb	r5, [r8]
	ldrb	r9, [r8]
//	mov	r0, r0
	strb	r4, [r8]
	.endm

	// precondition: r4 is clock low port value, r5 is clock high
	// at 13ns per clock tick, we need to execute 4 instructions to make
	// sure we dont clock faster than 20mHz
	.macro __mmc_clock1
	strb	r4, [r8]
	mov	r0, r0
	strb	r5, [r8]
	mov	r0, r0
	mov	r0, r0
	.endm
	.macro __mmc_clock8
	__mmc_clock1
	__mmc_clock1
	__mmc_clock1
	__mmc_clock1
	__mmc_clock1
	__mmc_clock1
	__mmc_clock1
	__mmc_clock1
	strb	r4, [r8]
	.endm	


// cyg_uint8 MMCGetData_c0( void )
// Reads a single byte of data
	.globl MMCGetData_c0
MMCGetData_c0:	
	__mmc_setup_regs

	mov	r6, #8
	ldrb	r4, [r8]
	bic	r4, r4, #HW_MMC_CLK0
0:	
	orr	r5, r4, #HW_MMC_CLK0   // might be able to take this out

	__mmc_read_port
	
	mov	r0, r0, lsl #1
	tst	r9, #HW_MMC_DATA0
	orrne	r0, r0, #1
	
	subs	r6, r6, #1
	bne	0b

	and	r0, r0, #0xff
	__mmc_restore_regs
	mov	pc, lr

// end MMCGetData_c0

// void MMCSendData_c0( cyg_uint8 odata )
	.globl	MMCSendData_c0
MMCSendData_c0:
	__mmc_setup_regs
	mov	r6, #0x80
	ldrb	r4, [r8]
	bic	r4, r4, #HW_MMC_CLK0
0:
	tst	r0, r6
	orrne	r4, r4, #HW_MMC_DATA0
	biceq	r4, r4, #HW_MMC_DATA0
	
	orr	r5, r4, #HW_MMC_CLK0    // need this in
	
	__mmc_write_port

	movs	r6, r6, lsr #1
	bne	0b

	__mmc_restore_regs
	mov	pc, lr

// end MMCSendData_c0


// void MMCSendCommand_c0( cyg_uint32 Arg, cyg_uint16 Cmd, cyg_uint16 crcData )
// send a command to the card. in actuality, the Cmd part is 1 byte (in the lower half)
// the order is Cmd, Arg (high byte first), crcData
	.globl	MMCSendCommand_c0	
MMCSendCommand_c0:
	__mmc_setup_regs

	// the card needs to be clocked twice before a command
	ldrb	r4, [r8]
	bic	r4, r4, #HW_MMC_CLK0
	orr	r5, r4, #HW_MMC_CLK0

	__mmc_write_port
	mov	r0, r0
	mov	r0, r0
	__mmc_write_port

	// first send cmd (r4 is already set up)
	// unroll loop
	tst	r1, #0x80
	orrne	r4, r4, #HW_MMC_CMD0
	biceq	r4, r4, #HW_MMC_CMD0
	orr	r5, r4, #HW_MMC_CLK0
	__mmc_write_port
	tst	r1, #0x40
	orrne	r4, r4, #HW_MMC_CMD0
	biceq	r4, r4, #HW_MMC_CMD0
	orr	r5, r4, #HW_MMC_CLK0
	__mmc_write_port
	tst	r1, #0x20
	orrne	r4, r4, #HW_MMC_CMD0
	biceq	r4, r4, #HW_MMC_CMD0
	orr	r5, r4, #HW_MMC_CLK0
	__mmc_write_port
	tst	r1, #0x10
	orrne	r4, r4, #HW_MMC_CMD0
	biceq	r4, r4, #HW_MMC_CMD0
	orr	r5, r4, #HW_MMC_CLK0
	__mmc_write_port
	tst	r1, #0x08
	orrne	r4, r4, #HW_MMC_CMD0
	biceq	r4, r4, #HW_MMC_CMD0
	orr	r5, r4, #HW_MMC_CLK0
	__mmc_write_port
	tst	r1, #0x04
	orrne	r4, r4, #HW_MMC_CMD0
	biceq	r4, r4, #HW_MMC_CMD0
	orr	r5, r4, #HW_MMC_CLK0
	__mmc_write_port
	tst	r1, #0x02
	orrne	r4, r4, #HW_MMC_CMD0
	biceq	r4, r4, #HW_MMC_CMD0
	orr	r5, r4, #HW_MMC_CLK0
	__mmc_write_port
	tst	r1, #0x01
	orrne	r4, r4, #HW_MMC_CMD0
	biceq	r4, r4, #HW_MMC_CMD0
	orr	r5, r4, #HW_MMC_CLK0
	__mmc_write_port
	
	// next send arg
	mov	r6, #0x80000000
0:
	tst	r0, r6
	orrne	r4, r4, #HW_MMC_CMD0
	biceq	r4, r4, #HW_MMC_CMD0
	
	orr	r5, r4, #HW_MMC_CLK0

	__mmc_write_port

	movs	r6, r6, lsr #1
	bne	0b
	// last send crc
	mov	r6, #0x80
0:
	tst	r2, r6
	orrne	r4, r4, #HW_MMC_CMD0
	biceq	r4, r4, #HW_MMC_CMD0

	orr	r5, r4, #HW_MMC_CLK0

	__mmc_write_port

	movs	r6, r6, lsr #1
	bne	0b

	__mmc_restore_regs
	mov	pc, lr

// end MMCSendCommand_c0

// cyg_uint32 MMCReceive_c0( cyg_uint8* dBuf, cyg_uint16 dataLength )
// read incoming data
	// read a byte and put it in r7
	.macro __mmc_read_byte
	__mmc_read_port
	tst	r9, #HW_MMC_DATA0
	orrne	r7, r7, #0x80000000
	__mmc_read_port
	tst	r9, #HW_MMC_DATA0
	orrne	r7, r7, #0x40000000
	__mmc_read_port
	tst	r9, #HW_MMC_DATA0
	orrne	r7, r7, #0x20000000
	__mmc_read_port
	tst	r9, #HW_MMC_DATA0
	orrne	r7, r7, #0x10000000
	__mmc_read_port
	tst	r9, #HW_MMC_DATA0
	orrne	r7, r7, #0x08000000
	__mmc_read_port
	tst	r9, #HW_MMC_DATA0
	orrne	r7, r7, #0x04000000
	__mmc_read_port
	tst	r9, #HW_MMC_DATA0
	orrne	r7, r7, #0x02000000
	__mmc_read_port
	tst	r9, #HW_MMC_DATA0
	orrne	r7, r7, #0x01000000
	.endm
	.macro __mmc_read_word
	mov	r7, #0
	__mmc_read_byte
	mov	r7, r7, lsr #8
	__mmc_read_byte
	mov	r7, r7, lsr #8
	__mmc_read_byte
	mov	r7, r7, lsr #8
	__mmc_read_byte
	str	r7, [r0], #4
	.endm
	
	.globl MMCReceive_c0
MMCReceive_c0:
	__mmc_setup_regs

	// first, we clock it until the the data line goes low, or we timeout
//	mov	r6, #HW_MMC_READ_TIMEOUT
	mov	r6, #HW_MMC_AC_TIMEOUT
	ldrb	r4, [r8]
	bic	r4, r4, #HW_MMC_CLK0
	orr	r5, r4, #HW_MMC_CLK0

	cmp	r2, #0
	beq	1f

	// timeout loop
0:
	subs	r6, r6, #1
	moveq	r0, #MMC_ERR_NOT_RESPONDING
	beq	15f       // timeout
	
	__mmc_read_port
	mov	r0, r0   // clock the card a bit slower here
	mov	r0, r0
	mov	r0, r0
	mov	r0, r0
	mov	r0, r0

	tst	r9, #HW_MMC_DATA0
	bne	0b
	// end timeout loop
	
1:		

	// unroll the inner loop
	// read 4 bytes in a row
	add	r1, r0, r1
0:
	// read a word at a time
	// doing any more than a word (i.e. 4 words) yields no real speedups
	__mmc_read_word
	cmp	r0, r1
	blt	0b

	// MMC card had no error
	mov	r0, #MMC_ERR_NONE
15:
	__mmc_restore_regs
	mov	pc, lr

// end MMCReceive_c0

// cyg_uint32 MMCTransmit_c0( cyg_uint8* dBuf, cyg_uint16 dataLength )
// send data out
// crc16 is defined in mmc_util.c
	.macro __mmc_write_byte
	tst	r7, #0x80
	orrne	r4, r4, #HW_MMC_DATA0
	biceq	r4, r4, #HW_MMC_DATA0
	orr	r5, r4, #HW_MMC_CLK0
	__mmc_write_port
	tst	r7, #0x40
	orrne	r4, r4, #HW_MMC_DATA0
	biceq	r4, r4, #HW_MMC_DATA0
	orr	r5, r4, #HW_MMC_CLK0
	__mmc_write_port
	tst	r7, #0x20
	orrne	r4, r4, #HW_MMC_DATA0
	biceq	r4, r4, #HW_MMC_DATA0
	orr	r5, r4, #HW_MMC_CLK0
	__mmc_write_port
	tst	r7, #0x10
	orrne	r4, r4, #HW_MMC_DATA0
	biceq	r4, r4, #HW_MMC_DATA0
	orr	r5, r4, #HW_MMC_CLK0
	__mmc_write_port
	tst	r7, #0x08
	orrne	r4, r4, #HW_MMC_DATA0
	biceq	r4, r4, #HW_MMC_DATA0
	orr	r5, r4, #HW_MMC_CLK0
	__mmc_write_port
	tst	r7, #0x04
	orrne	r4, r4, #HW_MMC_DATA0
	biceq	r4, r4, #HW_MMC_DATA0
	orr	r5, r4, #HW_MMC_CLK0
	__mmc_write_port
	tst	r7, #0x02
	orrne	r4, r4, #HW_MMC_DATA0
	biceq	r4, r4, #HW_MMC_DATA0
	orr	r5, r4, #HW_MMC_CLK0
	__mmc_write_port
	tst	r7, #0x01
	orrne	r4, r4, #HW_MMC_DATA0
	biceq	r4, r4, #HW_MMC_DATA0
	orr	r5, r4, #HW_MMC_CLK0
	__mmc_write_port
	.endm
	
	.extern CalculateDataCRC16
	.globl MMCTransmit_c0
MMCTransmit_c0:
	
	__mmc_setup_regs

	// set up the ddr
	ldr	r4, =HW_MMC_DDR0
	ldrb	r5, [r4]
#if HW_MMC_DDR0 == PDDDR
	bic	r5, r5, #HW_MMC_DATA0
#else
	orr	r5, r5, #HW_MMC_DATA0
#endif	
	strb	r5, [r4]
	
	// we need a crc on this one, so save r0, r1, lr off and make a call
	stmdb	sp!, {r0, r1, lr}
	bl	CalculateDataCRC16
	mov	r9, r0
	ldmia	sp!, {r0, r1, lr}

	// ok, at this point r0 is dBuf, r1 is dataLength, r9 is the crc
	// clock the card twice w/ data high (Nwr)
	ldrb	r4, [r8]
	orr	r4, r4, #HW_MMC_DATA0
	orr	r4, r4, #HW_MMC_CMD0
	
	bic	r4, r4, #HW_MMC_CLK0
	orr	r5, r4, #HW_MMC_CLK0

	__mmc_write_port
	mov	r0, r0
	mov	r0, r0
	__mmc_write_port

	// send the start bit
	bic	r4, r4, #HW_MMC_DATA0
	orr	r5, r4, #HW_MMC_CLK0

	__mmc_write_port

	// set up r1 as the end of the buffer
	add	r1, r0, r1
	// ship off the data
0:
	ldr	r7, [r0], #4
	__mmc_write_byte
	mov	r7, r7, lsr #8
	__mmc_write_byte
	mov	r7, r7, lsr #8
	__mmc_write_byte
	mov	r7, r7, lsr #8
	__mmc_write_byte

	teq	r0, r1
	bne	0b

	
	// now send the crc (2 bytes in r9)
	mov	r6, #0x8000
0:
	tst	r9, r6
	orrne	r4, r4, #HW_MMC_DATA0
	biceq	r4, r4, #HW_MMC_DATA0

	orr	r5, r4, #HW_MMC_CLK0

	__mmc_write_port

	movs	r6, r6, lsr #1
	bne	0b
	
	// pull dat high and clock once
	ldrb	r4, [r8]
	orr	r4, r4, #HW_MMC_DATA0
	bic	r4, r4, #HW_MMC_CLK0
	orr	r5, r4, #HW_MMC_CLK0
	
	__mmc_write_port

	// this is where it gets fun
	// switch the ddr back over so we can read back the crc response
	ldr	r4, =HW_MMC_DDR0
	ldrb	r5, [r4]
#if HW_MMC_DDR0 == PDDDR
	orr	r5, r5, #HW_MMC_DATA0
#else
	bic	r5, r5, #HW_MMC_DATA0
#endif
	strb	r5, [r4]

	// clock the card and look at the goodies
	// the card will release the DAT line when it is happy
	ldrb	r4, [r8]
	bic	r4, r4, #HW_MMC_CLK0
	orr	r5, r4, #HW_MMC_CLK0
0:	

	__mmc_read_port
	mov	r0, r0    // clock the card a bit slower here
	mov	r0, r0

	// the data line should be low
	tst	r9, #HW_MMC_DATA0
	bne	0b   // jump back if the line is still low

	// now we read the crc response (1 nibble)
	mov	r6, #0x8
	mov	r7, #0
0:
	
	__mmc_read_port
	tst	r9, #HW_MMC_DATA0
	orrne	r7, r7, r6

	movs	r6, r6, lsr #1
	bne	0b
	
	teq	r7, #0x5
	movne	r0, #MMC_ERR_CRC_INVALID
	bne	15f     // crc error

	// TODO this kills us. wait until DAT rises and the card is done programming
	// TODO put a timeout here
0:
	__mmc_read_port
	mov	r0, r0  // clock the card slower here
	mov	r0, r0
	tst	r9, #HW_MMC_DATA0
	beq	0b

	mov	r0, #MMC_ERR_NONE  // no error

15:
	__mmc_restore_regs
	mov	pc, lr
	
// end MMCTransmit_c0


// cyg_uint32 mmc_get_response_c0( cyg_uint8* resp_bytes, cyg_uint16 respBitLength )
// get a response over the CMD line
	.globl	MMCGetResponse_c0
MMCGetResponse_c0:
	__mmc_setup_regs

	// set the DDR so CMD is an input
	ldr	r4, =HW_MMC_DDR0
	ldrb	r5, [r4]
#if HW_MMC_DDR0 == PDDDR
	orr	r5, r5, #HW_MMC_CMD0
#else
	bic	r5, r5, #HW_MMC_CMD0
#endif
	strb	r5, [r4]

	// first part, clock the card until the CMD line goes low
//	mov	r6, #HW_MMC_TIMEOUT    // timeout
	mov	r6, #HW_MMC_CR_TIMEOUT   // timeout
	ldrb	r4, [r8]
	bic	r4, r4, #HW_MMC_CLK0
	orr	r5, r4, #HW_MMC_CLK0
0:
	subs	r6, r6, #1
	moveq	r0, #MMC_ERR_NOT_RESPONDING
	beq	15f

	__mmc_read_port
	mov	r0, r0
	mov	r0, r0

	tst	r9, #HW_MMC_CMD0
	bne	0b

	// so now the party is on
	// note that the first bit read in the previous code counts
	// unroll inner loop
0:
	mov	r7, #0
	tst	r9, #HW_MMC_CMD0
	orrne	r7, r7, #0x80
	__mmc_read_port
	tst	r9, #HW_MMC_CMD0
	orrne	r7, r7, #0x40
	__mmc_read_port
	tst	r9, #HW_MMC_CMD0
	orrne	r7, r7, #0x20
	__mmc_read_port
	tst	r9, #HW_MMC_CMD0
	orrne	r7, r7, #0x10
	__mmc_read_port
	tst	r9, #HW_MMC_CMD0
	orrne	r7, r7, #0x08
	__mmc_read_port
	tst	r9, #HW_MMC_CMD0
	orrne	r7, r7, #0x04
	__mmc_read_port
	tst	r9, #HW_MMC_CMD0
	orrne	r7, r7, #0x02
	__mmc_read_port
	tst	r9, #HW_MMC_CMD0
	orrne	r7, r7, #0x01
	__mmc_read_port

	strb	r7, [r0], #1
	subs	r1, r1, #8
	bne	0b

	mov	r0, #MMC_ERR_NONE
	
15:	
	// set the DDR back
	ldr	r4, =HW_MMC_DDR0
	ldrb	r5, [r4]
#if HW_MMC_DDR0 == PDDDR	
	bic	r5, r5, #HW_MMC_CMD0
#else
	orr	r5, r5, #HW_MMC_CMD0
#endif
	strb	r5, [r4]

	// now that the DDR is restored, clock the card 8 times
	ldrb	r4, [r8]
	bic	r4, r4, #HW_MMC_CLK0
	orr	r5, r4, #HW_MMC_CLK0

	__mmc_clock8

	__mmc_restore_regs
	mov	pc, lr
	
