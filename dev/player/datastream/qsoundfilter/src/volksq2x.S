//
//	q2x.s
//
//	History
//
//		Aug 14/2000	- msw creation (based upon qx.s)
//
//
//		Sept 5/2000 - changed the local memory so that it doesn't exist internally
//					  in the code but is passed to the routine from outside.
//
//		Sept 18/2000 - changed the loop structures to make them more efficient.  This
//					  means that the loop does a "do while" now rather than a "for" so
//					  the loop is done always one

#if 0
MAX_DELAY		EQU		32
USE_FAST_MULT	EQU		1
ADD_OTHER_RATES EQU     1
#else
#define MAX_DELAY      32
#define USE_FAST_MULT   1
#define ADD_OTHER_RATES 1	
	
#endif
	
	// local memory is mapped from the pointer in the following manner.  
	//
	//	Words 	0-31		lDelay
	//	Words 	32-63		rDelay
	//	Word  	64			lVolume
	//	Word	65			rVolume
	//	Word	66			scottFix
	//	Word	67			spread
	//	Word	68			filterDelay
	//	Word	69			pointer to start of lDelay
	//	Word	70			pointer into lDelay
	//	Word	71			pointer to start of rDelay
	//	Word	72			pointer into rDelay
	//	Word	73			pointer to filter
	//	Words	74-77		filter history buffer (left followed by right)
	//	Word	78			saturateValue
    //  Word    79          sampleRate
    //  Word    80          mode


	.extern	volksq2x44Filter
	.extern	volksq2x44LPFilter
#if (ADD_OTHER_RATES==1)
	.extern	volksq2x32Filter
	.extern	volksq2x32LPFilter
	.extern	volksq2x48Filter
	.extern	volksq2x48LPFilter
    .extern  filterTable
    .extern  delayTable
#endif

#if 0
	AREA	Q2XCode, CODE, READONLY
	ALIGN
	EXPORT	q2xProcess
	EXPORT	q2xInitialise
	EXPORT	q2xReset
	EXPORT	q2xSetVolume
	EXPORT	q2xSetSpread
	EXPORT	q2xSetMode
    IF (ADD_OTHER_RATES=1)
    EXPORT  q2xSetSampleRate
    ENDIF
#else
	.file	"volksq2x.S"
	.code	32
	.align	4
	.globl	q2xProcess
	.globl	q2xInitialise
	.globl	q2xReset
	.globl	q2xSetVolume
	.globl	q2xSetSpread
	.globl	q2xSetMode
#if (ADD_OTHER_RATES==1)
    .globl  q2xSetSampleRate
#endif

#endif

	// It is assumed that the two samples will be passed in r1 and r2.  The input format is
	// expected to be 17.15 format.  r1 and r2 will contain the output samples (also in 17.15 format
	// when done.  r0 has the address pointing to local memory.
q2xProcess:	
	stmdb	sp!, {r3-r9,lr}

	// store the samples addresses
	stmdb	sp!, {r1-r2}

	//	setup the coeff ptr - skip pass the delay buffers.  Use r5 to go through the coeff memory
	mov		r3, #MAX_DELAY
	add		r5, r0, r3, lsl #3			// r5 = r0+2*MAX_DELAY*4 - 2 buffer of bytes


	ldr		r0, [r1]
	ldr		r1, [r2]

	
//	IF (USE_FAST_MULT=0)
#if (USE_FAST_MULT==0)
		// scale input to 1.31
	
		mov		r0, r0, lsl #16
		mov		r1, r1, lsl #16
//	ENDIF
#endif


	// apply pre-volume scales.

	ldr		r6, [r5],#4
	ldr		r7, [r5],#4


//	IF (USE_FAST_MULT=1)
#if (USE_FAST_MULT==1)
		mul		r2, r6, r0
		mul		r3, r7, r1

		mov		r0, r2, asr #15			// back to 17.15 format
		mov		r1, r3, asr #15

		stmdb	sp!, {r0-r1} 			// direct version input volume scaled 17.15 format.

		ldr		r7, [r5], #4			// r7 has the -sf value
		
		mla		r2, r1, r7, r2
		mla		r3, r0, r7, r3
		
		mov		r2, r2, asr #15			// back to 17.15 format
		mov		r3, r3, asr #15
	
		// spread
			
		ldr		r7, [r5], #4			// r7 has spread value
		
		mul		r1, r2, r7				// notice the switch - r1 contains the left filter input which
		mul		r0, r3, r7				// will be accumulated into the right side so we can just add
											// the delayed straight to the filter form of r1.  A similiar
											// thing occurs with the right filter in r0.

		mov		r0, r0, asr #15			// back to 17.15 format
		mov		r1, r1, asr #15
//	ELSE
#else
		smull	r2, r3, r6, r0
		smull	r6, r4, r7, r1			// leave in 2.30|32 format in r3|r2 and r4|r6
	
	
		adds	r7, r2, r2				// up to 1.31|32 format in r0
		adc		r0, r3, r3
	
		adds	r7, r6, r6				// up to 1.31|32 format in r1
		adc		r1, r4, r4
	
		stmdb	sp!, {r0-r1} 			// direct version input volume scaled.
	
		// scott-fix
	
		ldr		r7, [r5], #4			// r7 has the -sf value
		
		smlal	r2, r3, r1, r7
		smlal	r6, r4, r0, r7
		
		adds	r2, r2, r2				// up to 1.31|32 format
		adc		r3, r3, r3
	
		adds	r6, r6, r6
		adc		r4, r4, r4
	
		// spread
			
		ldr		r7, [r5], #4			// r7 has spread value
		
		smull	r2, r1, r3, r7				// notice the switch - r1 contains the left filter input which
		smull	r6, r0, r4, r7				// will be accumulated into the right side so we can just add
											// the delayed straight to the filter form of r1.  A similiar
											// thing occurs with the right filter in r0.
	
	
		mov		r1, r1, asr #15			// convert to 17.15
		mov		r0, r0, asr #15
//	ENDIF
#endif

	// apply delay
	ldr		r6, [r5],#4				// r6 has the desired delay

	// pull of the start of the lDelay
	ldr		r8, [r5],#4				// r8 has the start address of lDelay

	ldr		r7, [r5]
	str		r1, [r7], -r6, lsl #2	// convert delay into byte count	
	cmp		r7, r8					// if address is below base address
	addlt	r7, r7, #128				// move r7 up to next 32x4 byte block
	ldr		r1, [r7]				// r1 has delayed left
	ldr		r7, [r5]				// reload original pointer
	sub		r7, r7, #4				// this is the same as advancing one word - means we don't have to compare against the end of the delay buffer
	cmp		r7, r8
	addlt	r7, r7, #128				// move to correct block
	str		r7, [r5],#4				// store new ptr

	// pull of the start of the rDelay
	ldr		r8, [r5],#4				// r8 has the start address of rDelay

	ldr		r7, [r5]
	str		r0, [r7], -r6, lsl #2	// convert delay into byte count		
	cmp		r7, r8
	addlt	r7, r7, #128					// move r7 up to next 32x4 byte block
	ldr		r0, [r7]				// r0 has delayed right
	ldr		r7, [r5]				// reload original pointer
	sub		r7, r7, #4					// rewind one word
	cmp		r7, r8
	addlt	r7, r7, #128					// move to correct block
	str		r7, [r5],#4				// store new ptr

	// single stage pole zero filter

	ldr		r6, [r5], #4			// r6 points to the filter coeffs

qLeft:	
	ldr		r7, [r6, #0]
	smull	r2, r3, r1, r7			// in*a0
	smlal	r2, r3, r1, r7			// in*a0*2 - so that the filter can be louder than the directs

	ldr		r7, [r6, #4]
	ldr		r8, [r5], #4
	smlal	r2, r3, r8, r7			// in*a0-a1*z-1/2
	smlal	r2, r3, r8, r7			// in*a0-a1*z-1	

	ldr		r7, [r6, #8]
	ldr		r9, [r5]
	smlal	r2, r3, r9, r7			// in*a0-a1*z-1-a2*z-2

	stmdb	sp!, {r2-r3}			// save variable to stash in history

	ldr		r7, [r6, #12]
	smlal	r2, r3, r8, r7			// in*a0-a1*z-1-a2*z-2+b1*z-1/2
	smlal	r2, r3, r8, r7			// in*a0-a1*z-1-a2*z-2+b1*z-1

	ldr		r7, [r6, #16]
	smlal	r2, r3, r9, r7			// in*a0-a1*z-1-a2*z-2+b1*z-1+b2*z-2

	str		r8, [r5], #-4

	ldmia	sp!, {r8-r9}
	adds	r8, r8, r8
	adc		r9, r9, r9
		
	str		r9, [r5], #8		

	adds	r2, r2, r2
	adc		r1, r3, r3

		
qRight:	
	ldr		r7, [r6, #0]
	smull	r2, r3, r0, r7			// in*a0
	smlal	r2, r3, r0, r7			// in*a0*2 - so that the filter can be louder than the directs

	ldr		r7, [r6, #4]
	ldr		r8, [r5], #4
	smlal	r2, r3, r8, r7			// in*a0-a1*z-1/2
	smlal	r2, r3, r8, r7			// in*a0-a1*z-1	

	ldr		r7, [r6, #8]
	ldr		r9, [r5]
	smlal	r2, r3, r9, r7			// in*a0-a1*z-1-a2*z-2

	stmdb	sp!, {r2-r3}			// save variable to stash in history

	ldr		r7, [r6, #12]
	smlal	r2, r3, r8, r7			// in*a0-a1*z-1-a2*z-2+b1*z-1/2
	smlal	r2, r3, r8, r7			// in*a0-a1*z-1-a2*z-2+b1*z-1

	ldr		r7, [r6, #16]
	smlal	r2, r3, r9, r7			// in*a0-a1*z-1-a2*z-2+b1*z-1+b2*z-2/2

	str		r8, [r5], #-4

	ldmia	sp!, {r8-r9}
	adds	r8, r8, r8
	adc		r9, r9, r9
	
	str		r9, [r5], #8		

	adds	r2, r2, r2
	adc		r0, r3, r3


	// accumlate with directs - pull off from the stack the direct values

	
	ldmia	sp!, {r2-r3}

//	IF (USE_FAST_MULT=0)
#if (USE_FAST_MULT==0)
		// r2 and r3 are in 1.31 format.  The filtered output is in 17.15 format
		// lets convert the delayed directs to 17.15 add the two elements and saturate
		// them.
		mov		r2, r2, asr #16
		mov		r3, r3, asr #16
//	ENDIF
#endif
	
	adds	r0, r0, r2		// add right filter to delayed left
	ldr		r2, [r5]		// load up the saturated value
	cmp		r0, r2			// and saturate
	movgt	r0, r2
	cmn		r0, r2
	mvnlt	r0, r2
	adds	r1, r1, r3		// add left filter to delayed right
	cmp		r1, r2			// and saturate
	movgt	r1, r2
	cmn		r1, r2
	mvnlt	r1, r2

	ldmia	sp!, {r2-r3}
	str		r0, [r2]
	str		r1, [r3]

	ldmia	sp!, {r3-r9,pc}


	//// q2xInitialise entry point takes a pointer to local memory contained in r0.  Returns no results.
	//// This is responsible for setting up pointers to buffers and default values for things like the
	//// scottfix.

q2xInitialise:	
	stmdb	sp!, {r1-r6,lr}

	mov		r1, r0				// r1 is going to walk through memory

//	IF (USE_FAST_MULT=1)
#if (USE_FAST_MULT==1)
		mov		r2, #0x7f
		mov		r3, #0xff
		orr		r4, r3, r2, lsl #8
//	ELSE
#else
		mov		r4, #0x7fffffff
//	ENDIF
#endif

	mov		r3, #MAX_DELAY
	add		r1, r1, r3, lsl #3		// r1->lVolume 

	str		r4, [r1], #4		// r1->lVolume
	str		r4, [r1], #4		// r1->rVolume

//	IF (USE_FAST_MULT=1)
#if (USE_FAST_MULT==1)
		mvn		r2, r4
//	ELSE
#else
		mov		r2, #0x80000000
//	ENDIF
#endif

	str		r2, [r1], #4		// r1->scottFix

//	IF (USE_FAST_MULT=1)
#if (USE_FAST_MULT==1)
		mov		r2, #0x5a
		mov		r5, #0x9d
		orr		r6, r5, r2, lsl #8
//	ELSE
#else
		mov		r6, #0x5A9DF7AB
//	ENDIF
#endif

	str		r6, [r1], #4		// r1->spread

	sub		r2, r3, #31
	str		r2, [r1], #4		// r1->delay amount

	str		r0, [r1], #4		// r1->start of lDelay
	str		r0, [r1], #4		// r1->lDelay Ptr
	add		r2, r0, r3, lsl #2		// r2 = rDelay
	str		r2, [r1], #4 		// r1->start of lDelay
	str		r2, [r1], #4 		// r1->rDelay Ptr
	ldr		r2, =volksq2x44LPFilter
	str		r2, [r1], #20		// r1->qCoeffPtr

	str		r4, [r1], #4		// r1->saturateValue
//    IF (ADD_OTHER_RATES=1)
#if (ADD_OTHER_RATES==1)
        mov     r2, #1
        str     r2, [r1], #4        // store the sampleRate base value 

        mov     r2, #0              // default mode
        str     r2, [r1], #4        // r1->mode
//    ENDIF
#endif

	ldmia	sp!,{r1-r6,pc}


	//// q2xReset entry point takes a pointer to local memory contained in r0.  This is responsible
	//// for clearing all local buffer contents.  Returns no results
	
q2xReset:	
	stmdb	sp!, {r1-r3,lr}
	mov		r1, r0				// r1 is going to point to rDelay
	mov		r2, #0
	// clear qFilter data
	mov		r3, #MAX_DELAY
	add		r1, r1, r3, lsl #2
clrDelay:	
		str		r2, [r0],#4
		str		r2, [r1],#4
		subs	r3, r3, #1
		bne		clrDelay

	// now need to skip 10 words from r1 to the q filter data
	mov		r3, #10
	add		r1, r1, r3, lsl #2		// convert to byte offset
	mov		r3, #4
clrQData:	
		str		r2, [r1],#4
		subs	r3, r3, #1
		bne		clrQData
doneClrQData:	
	ldmia		sp!,{r1-r3,pc}

	
	// r0 will have the memory ptr, r1 the left volume and r2 the right volume.

q2xSetVolume:	
	stmdb	sp!, {r3,lr}

	mov		r3, #MAX_DELAY
	add		r0, r0, r3, lsl #3		// r0->lVolume

	str		r1, [r0], #4
	str		r2, [r0]	
	ldmia		sp!,{r3,pc}

	// r0 will have the memory ptr, r1 the spread value.
q2xSetSpread:	
	stmdb	sp!, {r2-r3,lr}

	mov		r3, #MAX_DELAY
	add		r0, r0, r3, lsl #3		// r0->lVolume

	mov		r3, #3
	add		r0, r0, r3, lsl #2		// r0->spread

	str		r1, [r0]

	ldmia		sp!,{r2-r3,pc}


//    IF (ADD_OTHER_RATES=0)
#if (ADD_OTHER_RATES==0)
	// r0 will have the memory ptr, r1 the mode.
q2xSetMode:	
	stmdb	sp!, {r2-r3,lr}

	mov		r3, #MAX_DELAY
	add		r0, r0, r3, lsl #3		// r0->lVolume

	mov		r3, #9
	add		r0, r0, r3, lsl #2		// r0->qDataPtr

    cmp		r1, #0
    ldreq	r2, =volksq2x44LPFilter
    ldrne	r2, =volksq2x44Filter
 
	str		r2, [r0], #4		// r0->history buffer
	// clear history buffer
	mov		r2, #0
	// clear qFilter data
	mov		r3, #4
reClrQData:	
		str		r2, [r0], #4
		subs	r3, r3, #1
		bne		reClrQData
doneReClrQData:	
	ldmia		sp!,{r2-r3,pc}
//    ELSE
#else

	// r0 will have the memory ptr, r1 the mode.
q2xSetMode:	
	stmdb	sp!, {r2-r3,lr}

	mov		r3, #MAX_DELAY
	add		r2, r0, r3, lsl #3		// r2->lVolume

	mov		r3, #16
	add		r2, r2, r3, lsl #2		// r2->mode
    str     r1, [r2]                  // save new mode

    bl      changeSampleRateInfo

    ldmia	sp!, {r2-r3,pc}

    // r0 will have the memory ptr, r1 the rate 0=32,1=44,2=48
q2xSetSampleRate:	
	stmdb	sp!, {r2-r3,lr}

	mov		r3, #MAX_DELAY
	add		r2, r0, r3, lsl #3		// r2->lVolume

	mov		r3, #15
	add		r2, r2, r3, lsl #2		// r2->sampleRate
    str     r1, [r2]                // save the sample Rate value

    bl      changeSampleRateInfo

    ldmia	sp!, {r2-r3,pc}

    // r0 will have the memory ptr.

changeSampleRateInfo:	
    stmdb    sp!, {r1-r6,lr}
    
	mov		r3, #MAX_DELAY
	add		r0, r0, r3, lsl #3		// r0->lVolume

	mov		r3, #4
	add		r0, r0, r3, lsl #2		// r0->filterDelay
    mov     r4, r0                  // save the address of the filterDelay
    
    mov     r3, #5 
	add		r0, r0, r3, lsl #2		// r0->qDataPtr
    mov     r5, r0                  // save the qfilterptr
    
    mov     r3, #1
    add     r0, r0, r3, lsl #2      // r0->filter history buffer

    // clear qfilter history buffer
	mov		r2, #0
	mov		r3, #4
reClrQData:	
		str		r2, [r0], #4
		subs	r3, r3, #1
		bne		reClrQData
doneReClrQData:	
	
    mov     r3, #1
    add     r0, r0, r3, lsl #2      // r0->sampleRate Index
    ldr     r3, [r0], #4            // r0->mode 

    ldr     r2, =delayTable
    add     r2, r2, r3, lsl #2      // convert sample rate index into word offset
    ldr     r6, [r2]
    mov     r1, #MAX_DELAY
    sub     r6, r1, r6
    str     r6, [r4]                // save the new delay value

    ldr     r2, =filterTable
    add     r2, r2, r3, lsl #3      // if index is 1 then it will skip 8 bytes (2 words)

    ldr     r3, [r0]
    add     r2, r2, r3, lsl #2      // offset by mode*4 bytes
    ldr     r3, [r2]
    str     r3, [r5]    

    ldmia   sp!, {r1-r6, pc}

//    ENDIF
#endif


//	END




