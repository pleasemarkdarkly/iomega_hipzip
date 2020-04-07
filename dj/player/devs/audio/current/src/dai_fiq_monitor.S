/* In FIQ mode, registers r8 through r14 are banked.  I use them as follows:
   r8 - pointer to dai internal registers
   r9 - address of playback buffer structure
   r10 - position in playback buffer of next sample to play	
   r11 - temporary, ptr to data to read
   r12 - temporary
   r13 - temporary */

#define ASM_HEADERS
#include "dai_hw.h"

#define	DAI_BASE_ADDRESS	0x80002000
#define DAI_LEFT_CHANNEL	0x0080
#define DAI_RIGHT_CHANNEL	0x0040
#define DAI_STATUS		0x0100

	.file	"dai_fiq.S"

	.extern	DAIPlaybackBuffer
#if defined(ENABLE_DAI_RECORD)
	.extern DAIRecordBuffer
#endif
	.extern DAIStarvationCounter
    .extern DAIPeak

	
	.code	32
	.text

        // "subroutines" 

  
	.global	DAIFIQMONITOR
DAIFIQMONITOR:


	/* Load r8 with pointer to dai internal registers */
	ldr	r8, =DAI_BASE_ADDRESS

    /* don't write audio output, but loopback adc data instead */


#if defined(ENABLE_DAI_RECORD)
	/* Load registers with current sample buffer structure */
	ldr	r9, =DAIRecordBuffer
	ldr	r9, [r9]

	/* If sample buffer isn't available then */
	ldr	r10, [r9, #DAI_BUFFER_POSITION]
	ldr	r11, [r9, #DAI_BUFFER_SIZE]
	cmp	r10, r11
	blt	3f

	/*  Increment overflow counter */
	ldr	r11, =DAIOverflowCounter
	ldr	r12, [r11]
	add	r12, r12, #1
	str	r12, [r11]

#endif
   

    ldr r13, =DAIPeak
    ldrsh r12, [r13]
    
/* if there is no valid output buffer, loopback anyway - tcl */

#if 1
/* reverse stereo in software */
	ldrsh	r11, [r8, #DAI_LEFT_CHANNEL]
    strh	r11, [r8, #DAI_RIGHT_CHANNEL]
    cmp r11,r12
    movgt r12,r11    

	ldrsh	r11, [r8, #DAI_RIGHT_CHANNEL]
    strh	r11, [r8, #DAI_LEFT_CHANNEL]
    cmp r11,r12
    movgt r12,r11


	ldrsh	r11, [r8, #DAI_LEFT_CHANNEL]
    strh	r11, [r8, #DAI_RIGHT_CHANNEL]
    cmp r11,r12
    movgt r12,r11

	ldrsh	r11, [r8, #DAI_RIGHT_CHANNEL]
    strh	r11, [r8, #DAI_LEFT_CHANNEL]
    cmp r11,r12
    movgt r12,r11


	ldrsh	r11, [r8, #DAI_LEFT_CHANNEL]
    strh	r11, [r8, #DAI_RIGHT_CHANNEL]
    cmp r11,r12
    movgt r12,r11


	ldrsh	r11, [r8, #DAI_RIGHT_CHANNEL]
    strh	r11, [r8, #DAI_LEFT_CHANNEL]
    cmp r11,r12
    movgt r12,r11


	ldrsh	r11, [r8, #DAI_LEFT_CHANNEL]
    strh	r11, [r8, #DAI_RIGHT_CHANNEL]
    cmp r11,r12
    movgt r12,r11


	ldrsh	r11, [r8, #DAI_RIGHT_CHANNEL]
    strh	r11, [r8, #DAI_LEFT_CHANNEL]
    cmp r11,r12
    movgt r12,r11
#else
	ldrsh	r11, [r8, #DAI_LEFT_CHANNEL]
    strh	r11, [r8, #DAI_LEFT_CHANNEL]
    cmp r11,r12
    movgt r12,r11    

	ldrsh	r11, [r8, #DAI_RIGHT_CHANNEL]
    strh	r11, [r8, #DAI_RIGHT_CHANNEL]
    cmp r11,r12
    movgt r12,r11


	ldrsh	r11, [r8, #DAI_LEFT_CHANNEL]
    strh	r11, [r8, #DAI_LEFT_CHANNEL]
    cmp r11,r12
    movgt r12,r11

	ldrsh	r11, [r8, #DAI_RIGHT_CHANNEL]
    strh	r11, [r8, #DAI_RIGHT_CHANNEL]
    cmp r11,r12
    movgt r12,r11


	ldrsh	r11, [r8, #DAI_LEFT_CHANNEL]
    strh	r11, [r8, #DAI_LEFT_CHANNEL]
    cmp r11,r12
    movgt r12,r11


	ldrsh	r11, [r8, #DAI_RIGHT_CHANNEL]
    strh	r11, [r8, #DAI_RIGHT_CHANNEL]
    cmp r11,r12
    movgt r12,r11


	ldrsh	r11, [r8, #DAI_LEFT_CHANNEL]
    strh	r11, [r8, #DAI_LEFT_CHANNEL]
    cmp r11,r12
    movgt r12,r11


	ldrsh	r11, [r8, #DAI_RIGHT_CHANNEL]
    strh	r11, [r8, #DAI_RIGHT_CHANNEL]
    cmp r11,r12
    movgt r12,r11
#endif


    strh r12,[r13]

    
#if defined(ENABLE_DAI_RECORD)	
	b	4f

	/* Else */
3:
	/* Read 4 and write stereo samples from recording register */
	ldr	r11, [r9, #DAI_BUFFER_DATA]
	add	r11, r11, r10, lsl #1

#if 1

	ldrh	r13, [r8, #DAI_RIGHT_CHANNEL]
    strh    r13, [r8, #DAI_LEFT_CHANNEL]	
	strh	r13, [r11], #2

	ldrh	r13, [r8, #DAI_LEFT_CHANNEL]
    strh    r13, [r8, #DAI_RIGHT_CHANNEL]
	strh	r13, [r11], #2

	ldrh	r13, [r8, #DAI_RIGHT_CHANNEL]
    strh    r13, [r8, #DAI_LEFT_CHANNEL]	
	strh	r13, [r11], #2

	ldrh	r13, [r8, #DAI_LEFT_CHANNEL]
    strh    r13, [r8, #DAI_RIGHT_CHANNEL]
	strh	r13, [r11], #2

	ldrh	r13, [r8, #DAI_RIGHT_CHANNEL]
    strh    r13, [r8, #DAI_LEFT_CHANNEL]	
	strh	r13, [r11], #2

	ldrh	r13, [r8, #DAI_LEFT_CHANNEL]
    strh    r13, [r8, #DAI_RIGHT_CHANNEL]
	strh	r13, [r11], #2

	ldrh	r13, [r8, #DAI_RIGHT_CHANNEL]
    strh    r13, [r8, #DAI_LEFT_CHANNEL]	
	strh	r13, [r11], #2

	ldrh	r13, [r8, #DAI_LEFT_CHANNEL]
    strh    r13, [r8, #DAI_RIGHT_CHANNEL]
	strh	r13, [r11], #2

#else
	ldrh	r13, [r8, #DAI_LEFT_CHANNEL]
    strh    r13, [r8, #DAI_LEFT_CHANNEL]
	strh	r13, [r11], #2
	ldrh	r13, [r8, #DAI_RIGHT_CHANNEL]
    strh    r13, [r8, #DAI_RIGHT_CHANNEL]	
	strh	r13, [r11], #2
	ldrh	r13, [r8, #DAI_LEFT_CHANNEL]
    strh    r13, [r8, #DAI_LEFT_CHANNEL]
	strh	r13, [r11], #2
	ldrh	r13, [r8, #DAI_RIGHT_CHANNEL]
    strh    r13, [r8, #DAI_RIGHT_CHANNEL]
	strh	r13, [r11], #2
	ldrh	r13, [r8, #DAI_LEFT_CHANNEL]
    strh    r13, [r8, #DAI_LEFT_CHANNEL]
	strh	r13, [r11], #2
	ldrh	r13, [r8, #DAI_RIGHT_CHANNEL]	
    strh    r13, [r8, #DAI_RIGHT_CHANNEL]
	strh	r13, [r11], #2
	ldrh	r13, [r8, #DAI_LEFT_CHANNEL]
    strh    r13, [r8, #DAI_LEFT_CHANNEL]
	strh	r13, [r11], #2
	ldrh	r13, [r8, #DAI_RIGHT_CHANNEL]	
    strh    r13, [r8, #DAI_RIGHT_CHANNEL]
	strh	r13, [r11], #2
#endif

	/* Update position in sample buffer */
	add	r10, r10, #8
	str	r10, [r9, #DAI_BUFFER_POSITION]
	
	/* If at end of sample buffer then */
	ldr	r11, [r9, #DAI_BUFFER_SIZE]
	cmp	r10, r11
	blt	4f

	/* Switch to next sample buffer */
	ldr	r11, [r9, #DAI_BUFFER_NEXT]
	ldr	r9, =DAIRecordBuffer
	str	r11, [r9]
#endif
        
4:		
	/* Clear overrun and underrun bits in status register */
	ldr	r11, =0xffffffff
	str	r11, [r8, #DAI_STATUS]

	subs	pc, lr, #4
