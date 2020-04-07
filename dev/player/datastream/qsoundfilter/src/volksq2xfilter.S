//
//	q2xfilter.s
//
//	History
//
//		Aug 14/2000	- msw creation (based upon qxfilter.s)
//
//
#if 0
	AREA	FilterROMData, DATA, READONLY
	ALIGN
	EXPORT	volksq2x44Filter [DATA]
	EXPORT	volksq2x44LPFilter [DATA]
	EXPORT	volksq2x32Filter [DATA]
	EXPORT	volksq2x32LPFilter [DATA]
	EXPORT	volksq2x48Filter [DATA]
	EXPORT	volksq2x48LPFilter [DATA]
    EXPORT  filterTable [DATA]
    EXPORT  delayTable [DATA]
#endif
	.file "volksq2xfilter.S"
	.code	32
	.align	4
	
	.globl	volksq2x44Filter
	.globl	volksq2x44LPFilter
	.globl	volksq2x32Filter
	.globl	volksq2x32LPFilter
	.globl	volksq2x48Filter
	.globl	volksq2x48LPFilter
	.globl  filterTable
	.globl  delayTable
	
//// Pole - amp = 0.9 freq = 3300Hz
//// Zero - amp = 0.92 freq = 4200Hz
//// scale multiplier so that iir read 0.4dB as the maximum amplitude
volksq2x44Filter:			.long		0x442F547F				// a0
					.long		0x66B324B4				// -a1/2	inverted because we can only madd
					.long		0x9851EB86				// -a2	inverted because we can only madd
					.long		0x9EB3BE44				// b1/2
					.long		0x6C56D5CF				// b2
	//// LP at 3500
volksq2x44LPFilter:			.long		0x05D2FF4E				// a0
					.long		0x54010E8C				// -a1/2	inverted because we can only madd
					.long		0xC0B1E5AB				// -a2	inverted because we can only madd
					.long		0x7fffffff				// b1/2
					.long		0x7fffffff				// b2

volksq2x32Filter:			.long		0x388F432B				// a0
					.long		0x5BD9F15F				// -a1/2	inverted because we can only madd
					.long		0x9851EB86				// -a2	inverted because we can only madd
					.long		0xB0107E1B				// b1/2
					.long		0x6C56D5CF				// b2
	//// LP at 3500
volksq2x32LPFilter:			.long		0x0A075534				// a0
					.long		0x444E0C6D				// -a1/2	inverted because we can only madd
					.long		0xCF469253				// -a2	inverted because we can only madd
					.long		0x7fffffff				// b1/2
					.long		0x7fffffff				// b2

volksq2x48Filter:			.long		0x47655AD0				// a0
					.long		0x689E3B62				// -a1/2	inverted because we can only madd
					.long		0x9851EB86				// -a2	inverted because we can only madd
					.long		0x9B97D507				// b1/2
					.long		0x6C56D5CF				// b2
	//// LP at 3500
volksq2x48LPFilter:			.long		0x05071A15				// a0
					.long		0x577339C5				// -a1/2	inverted because we can only madd
					.long		0xBCFD241F				// -a2	inverted because we can only madd
					.long		0x7fffffff				// b1/2
					.long		0x7fffffff				// b2

filterTable:	             .long     volksq2x32LPFilter      // mode 0 
                        .long     volksq2x32Filter        // mode 1
                        .long     volksq2x44LPFilter      // mode 0
                        .long     volksq2x44Filter        // mode 1
                        .long     volksq2x48LPFilter      // mode 0
                        .long     volksq2x48Filter        // mode 1

delayTable:	              .long     0x0000016               //// 22 samples @ 32
                        .long     0x000001f               //// 31 samples @ 44.1
                        .long     0x000001f               //// 31 samples @ 48


//	END


