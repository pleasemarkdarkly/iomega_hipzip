//........................................................................................
//........................................................................................
//.. Last Modified By: Dan Bolstad	danb@iobjects.com									..	
//.. Modification date: 8/25/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................

/*****************************************************************************
* FileName:   errcode.c    convert critical errors to internal error code
*
* SanDisk Host Developer's Toolkit
*
* Copyright (c) 1997 - 1998 SanDisk Corporation
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*
******************************************************************************/
/*****************************************************************************
** Name:   errcode.c    convert critical errors to internal error codes
**
**      platform_convert_critical_error
******************************************************************************/

#include <fs/fat/sdapi.h>
#include <redboot.h>

#if (USE_FILE_SYSTEM)

/*****************************************************************************
* Name: platform_convert_critical_error
*
* Processing:
*       Convert critical errors to internal error codes.
*
* Entries:
*       BUS_ERC_XXXX error types
*           or
*       MMC_XXXX     error types
*
* Returns:
*       One of the following error codes:
*               - CRERR_BAD_FORMAT,
*               - CRERR_NO_CARD,
*               - CRERR_BAD_CARD,
*               - CRERR_CHANGED_CARD,
*               - CRERR_CARD_FAILURE,
*               - CRERR_ID_ERROR,
*               - CRERR_ECC_ERROR
*
******************************************************************************/
INT16 platform_convert_critical_error(INT16 error_code) /*__fn__*/
{
#if 0
        switch (error_code)
        {
                case BUS_ERC_DIAG:            /* 101 */
                case BUS_ERC_DRQ:             /* 103 */
                case BUS_ERC_TIMEOUT:         /* 104 */
                case BUS_ERC_ADDR_RANGE:      /* 106 */
                case BUS_ERC_CNTRL_INIT:      /* 107 */
                case BUS_ERC_CMD_MULT:        /* 109 */
                    return CRERR_CARD_FAILURE;/* 305 */

                case BUS_ERC_IDDRV:           /* 108 */
                    return CRERR_ID_ERROR;    /* 306 */

                case MMC_CARD_IS_NOT_RESPONDING:     /* 201- Time out error on card response */
                    return CRERR_CHANGED_CARD;       /* 304 */
                
                case MMC_CMD_CRC_ERROR:              /* 202- CRC error detected on card response */
                case MMC_DATA_CRC_ERROR:             /* 203- CRC error detected on incoming data block */
                case MMC_DATA_STATUS_CRC_ERROR:      /* 204- Card is reporting CRC error on outgoing data block. */
                case MMC_COMUNC_CRC_ERROR:           /* 207- Card is reporting CRC error */
                case MMC_CARD_ECC_FAILED:            /* 229- Internal Card ECC failed */
                case MMC_CARD_ECC_DISABLED:          /* 224- The command has been executed without */
                                                     /*     using the internal ECC. */
                    return CRERR_ECC_ERROR;          /* 307 */
                
                case MMC_CARD_IS_BUSY:               /* 205- Card is busy programming */
                case MMC_CARD_IS_NOT_READY:          /* 206- Card did not complete its initialization and is not ready. */
                case MMC_COMUNC_ILLEG_COM:           /* 208- Card is reporting illegal command */
                case MMC_ERASE_PARAM:                /* 209- Erase parameters error */
                    return CRERR_CARD_FAILURE;       /* 305 */
                
                case MMC_WP_VIOLATION:               /* 210- Attempt to write a WP sector */
                case MMC_ERROR:                      /* 211- MMC card internal error */
                case MMC_WP_ERASE_SKIP:              /* 212- Attempt to erase WP sector */
                case MMC_ADDRESS_ERROR:              /* 213- Sector messaligned error */
                case MMC_CARD_READ_FAILURE:          /* 214- Card is reporting Read command failed */
                    return CRERR_CARD_FAILURE;       /* 305 */

                case MMC_INTERFACE_ERROR:            /* 215- Error detected by the MMC HW driver */
                    return CRERR_NO_CARD;            /* 302 */

                case MMC_ILLEGAL_MODE:               /* 216- Not support in the current mode */
                case MMC_COMMAND_PARAMETER_ERROR:    /* 217- Card is reporting Address-out-of-range error */
                case MMC_ERASE_SEQ_ERROR:            /* 218- Error in the sequence of erase command */
                case MMC_ERASE_RESET:                /* 219- Erase command canceled before execution */
                case MMC_NO_CRC_STATUS:              /* 220- Time out on CRC status for Write */
                case MMC_OVERRUN:                    /* 221- Overrun */
                case MMC_UNDERRUN:                   /* 222- Underrun */
                case MMC_CIDCSD_OVERWRITE:           /* 223- a) The CID register has been already */
                                                     /*        written and can be overwriten. */
                                                     /*     b) The read only section of CSD does not */
                                                     /*        match the card content. */
                                                     /*     c) An attempt to reverse the copy (set */
                                                     /*        as original) or permanent WP bits was made. */
                case MMC_READ_FOR_DATA:              /* 225- Corresponds to buffer empty signalling */
                                                     /*     on the bus. */
                case MMC_DATA_LENGTH_ERROR:          /* 226- Data Length more then 512 bytes. */
                case MMC_TIME_OUT_RCVD:              /* 227- Time out recive data (B0 for controller) */
                case MMC_OUT_OF_RANGE:               /* 228- Address out of range error */
                    return CRERR_BAD_CARD;           /* 303 */

                default:
                    return CRERR_BAD_FORMAT;     /* 301 */
        }
#else
	switch (error_code) {
#if 0
	    case ENOMED:
		return CRERR_NO_CARD;
		
	    case EMEDCHG:
		return CRERR_CHANGED_CARD;

	    case ESLEEP:
		return CRERR_SLEEP;

#if defined(__DHARMA)
	    case ERESET:
		return CRERR_DRIVE_RESET;
#endif /* __DHARMA */
#endif
	    default:
		printf("unhandled error code %x\n", error_code);
		return CRERR_BAD_FORMAT;
	}
#endif
}
#endif
