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

/* The docs say to read 6 samples on receive in contrast to writing
   4 on transmit.  Turn this off for now, since it's always been 4, and
   I want to test DAIINT. */
//#define DAI_RECEIVE_6_SAMPLES
#undef DAI_RECEIVE_6_SAMPLES	
	
	.file	"dai_fiq.S"

	.extern	DAIPlaybackBuffer
#if defined(ENABLE_DAI_RECORD) 
	.extern DAIRecordBuffer
#endif
	.extern DAIStarvationCounter
	
	.code	32
	.text

        // "subroutines"

        // starvation
        // inputs - r8 is ptr to dai base
        
_play_starvation:
 	// Increment starvation counter
	ldr	r11, =DAIStarvationCounter
	ldr	r12, [r11]
	add	r12, r12, #1
	str	r12, [r11]
	
	// Load 4 stereo samples of silence into playback registers
	mov	r11, #0
	str	r11, [r8, #DAI_LEFT_CHANNEL]
	str	r11, [r8, #DAI_RIGHT_CHANNEL]
	str	r11, [r8, #DAI_LEFT_CHANNEL]
	str	r11, [r8, #DAI_RIGHT_CHANNEL]
	str	r11, [r8, #DAI_LEFT_CHANNEL]
	str	r11, [r8, #DAI_RIGHT_CHANNEL]
	str	r11, [r8, #DAI_LEFT_CHANNEL]
	str	r11, [r8, #DAI_RIGHT_CHANNEL]
        
        b       _record_step

	.global	DAIFIQ
DAIFIQ:


	/* Load r8 with pointer to dai internal registers */
	ldr	r8, =DAI_BASE_ADDRESS
		
	/* Load registers with current sample buffer structure */
	ldr	r9, =DAIPlaybackBuffer
	ldr	r9, [r9]
		
	/* If sample buffer is empty then */
        /* dc- use r12 as buffer size and save a mem access */
        ldr     r10, [r9, #DAI_BUFFER_POSITION]
        ldr     r12, [r9, #DAI_BUFFER_SIZE]
        cmp     r10, r12

        // jump to playback starvation code
        bge     _play_starvation
	
	/* Else */
1:
        ldr     r11, [r9, #DAI_BUFFER_DATA]
        add     r11, r11, r10, lsl #1

        // read up interleaved data
        // doing a full word ldr + shift is faster than multiple halfword ldrs,
        // even on a cache hit
        ldr     r13, [r11], #4
        strh    r13, [r8, #DAI_LEFT_CHANNEL]
        mov     r13, r13, lsr #16
        strh    r13, [r8, #DAI_RIGHT_CHANNEL]
        
        ldr     r13, [r11], #4
        strh    r13, [r8, #DAI_LEFT_CHANNEL]
        mov     r13, r13, lsr #16
        strh    r13, [r8, #DAI_RIGHT_CHANNEL]
        
        ldr     r13, [r11], #4
        strh    r13, [r8, #DAI_LEFT_CHANNEL]
        mov     r13, r13, lsr #16
        strh    r13, [r8, #DAI_RIGHT_CHANNEL]
        
        ldr     r13, [r11], #4
        strh    r13, [r8, #DAI_LEFT_CHANNEL]
        mov     r13, r13, lsr #16
        strh    r13, [r8, #DAI_RIGHT_CHANNEL]

                
        /* Update position in sample buffer */
	add	r10, r10, #8
	str	r10, [r9, #DAI_BUFFER_POSITION]
	
	/* If at end of sample buffer then */
        /* dc- save a memory access by using r12 for buffer size */
        cmp     r10, r12
	blt	_record_step

	/* Switch to next sample buffer */
	ldr	r11, [r9, #DAI_BUFFER_NEXT]
	ldr	r9, =DAIPlaybackBuffer
	str	r11, [r9]

        
_record_step:
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
        // dc- since recording isn't always enabled, extracting
        //  record_starvation isn't practical, since it will slow
        //  down normal playback (branch)
	/* Read and throw away 4 stereo samples from recording register */
	ldrh	r11, [r8, #DAI_LEFT_CHANNEL]
	ldrh	r11, [r8, #DAI_RIGHT_CHANNEL]
	ldrh	r11, [r8, #DAI_LEFT_CHANNEL]
	ldrh	r11, [r8, #DAI_RIGHT_CHANNEL]
	ldrh	r11, [r8, #DAI_LEFT_CHANNEL]
	ldrh	r11, [r8, #DAI_RIGHT_CHANNEL]
	ldrh	r11, [r8, #DAI_LEFT_CHANNEL]
	ldrh	r11, [r8, #DAI_RIGHT_CHANNEL]

#if defined(DAI_RECEIVE_6_SAMPLES)	
	ldrh	r11, [r8, #DAI_LEFT_CHANNEL]
	ldrh	r11, [r8, #DAI_RIGHT_CHANNEL]
	ldrh	r11, [r8, #DAI_LEFT_CHANNEL]
	ldrh	r11, [r8, #DAI_RIGHT_CHANNEL]
#endif	
        
#if defined(ENABLE_DAI_RECORD)	
	b	4f

	/* Else */
3:
	/* Read 4 stereo samples from recording register */
	ldr	r11, [r9, #DAI_BUFFER_DATA]
	add	r11, r11, r10, lsl #1


#if 1
    /* reverse input stereo in Sahara build */
	
        ldr	r13, [r8, #DAI_RIGHT_CHANNEL]
	strh	r13, [r11], #2
	ldr	r13, [r8, #DAI_LEFT_CHANNEL]	
	strh	r13, [r11], #2

	ldr	r13, [r8, #DAI_RIGHT_CHANNEL]
	strh	r13, [r11], #2
	ldr	r13, [r8, #DAI_LEFT_CHANNEL]	
	strh	r13, [r11], #2

	ldr	r13, [r8, #DAI_RIGHT_CHANNEL]
	strh	r13, [r11], #2
	ldr	r13, [r8, #DAI_LEFT_CHANNEL]	
	strh	r13, [r11], #2

	ldr	r13, [r8, #DAI_RIGHT_CHANNEL]
	strh	r13, [r11], #2
	ldr	r13, [r8, #DAI_LEFT_CHANNEL]	
	strh	r13, [r11], #2

#if defined(DAI_RECEIVE_6_SAMPLES)
	ldr	r13, [r8, #DAI_RIGHT_CHANNEL]
	strh	r13, [r11], #2
	ldr	r13, [r8, #DAI_LEFT_CHANNEL]	
	strh	r13, [r11], #2
	
	ldr	r13, [r8, #DAI_RIGHT_CHANNEL]
	strh	r13, [r11], #2
	ldr	r13, [r8, #DAI_LEFT_CHANNEL]	
	strh	r13, [r11], #2
#endif	
#else
	ldr	r13, [r8, #DAI_LEFT_CHANNEL]
	strh	r13, [r11], #2
	ldr	r13, [r8, #DAI_RIGHT_CHANNEL]	
	strh	r13, [r11], #2

	ldr	r13, [r8, #DAI_LEFT_CHANNEL]
	strh	r13, [r11], #2
	ldr	r13, [r8, #DAI_RIGHT_CHANNEL]	
	strh	r13, [r11], #2

	ldr	r13, [r8, #DAI_LEFT_CHANNEL]
	strh	r13, [r11], #2
	ldr	r13, [r8, #DAI_RIGHT_CHANNEL]	
	strh	r13, [r11], #2

	ldr	r13, [r8, #DAI_LEFT_CHANNEL]
	strh	r13, [r11], #2
	ldr	r13, [r8, #DAI_RIGHT_CHANNEL]	
	strh	r13, [r11], #2

#if defined(DAI_RECEIVE_6_SAMPLES)
	ldr	r13, [r8, #DAI_RIGHT_CHANNEL]
	strh	r13, [r11], #2
	ldr	r13, [r8, #DAI_LEFT_CHANNEL]	
	strh	r13, [r11], #2
	
	ldr	r13, [r8, #DAI_RIGHT_CHANNEL]
	strh	r13, [r11], #2
	ldr	r13, [r8, #DAI_LEFT_CHANNEL]	
	strh	r13, [r11], #2
#endif	
#endif
	/* Update position in sample buffer */
#if defined(DAI_RECEIVE_6_SAMPLES)	
	add	r10, r10, #12
#else	
	add	r10, r10, #8
#endif	
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

	/* Check to see if DAIINT is cleared. */
	ldr	r11, [r8, #0x0240]
	and	r11, r11, #0x1
	cmp	r11, #0

	beq	5f	/* DAIINT is cleared, we are done */

	/* DAIINT is not cleared, so we need to run the FIQ again. */
	/* The FIQ may immediately get called again, but that doesn't fit
	   with the fact that spurious IRQs result from the DAIINT bit. */
	b	DAIFIQ
5:			
	subs	pc, lr, #4
