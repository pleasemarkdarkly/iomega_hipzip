//........................................................................................
//........................................................................................
//.. Last Modified By: Eric Gibbs	ericg@iobjects.com									..	
//.. Modification date: 8/16/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
/********************************************************************************
* Filename: sdmmc.h
*
* SanDisk Host Developer's Toolkit
*
* Copyright (c) 1997 - 1999 SanDisk Corporation
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*
* Description:
*       Interface definition for an application driver to the SanDisk
*       MMC card using SPI interface.
*
*       This driver provides a basic, sector based, read write commands.
*       Addressing is LBA mode only (Head, cylinder, sector format is 
*       not supported).  This module is platform independent.
*
********************************************************************************/

#ifndef _SDMMC_H_


#ifdef __cplusplus
extern "C" {
#endif



#include <fs/fat/sdapi.h>
#include <fs/fat/drive.h>


/* Data block length */
#define DEFAULT_BLK_LEN         512

#define DEFAULT_ERASE_GROUP     16
#define BYTE_LENGTH             6       /* Length in bytes of MMC responses */
#define CID_BYTE_LENGTH         17      /* Length in bytes of MMC card id. CID */
#define CMD_BYTE_LENGTH         6       /* Length in bytes of MMC commands  */
#define RESPONSE_BIT_LENGTH     48      /* Length in bits of MMC responses */
#define CID_BIT_LENGTH          136     /* Length in bit of MMC card id. CID */


/* CLOCK */
#define MMC_Ncr_MAX             64      /* Maximum number of clock cycles to wait for card response */
#define MMC_Ncc_MIN             8       /* Minimum number of clock cycles between commands */
#define MMC_Nwr_MIN             2

#define RESET_DELAY             30      /* Delay for reset */
#define READ_ACCESS_DELAY       550


#define TMPWPBIT_ON             0x10
#define TMPWPBIT_OFF            0xEF


/* -------------------- TRANSFER OR STAND_BY STATE ---------------------------- */
#define STANDBY_STATE           0       /* cmd9, cmd10, cmd14 */
					/* NO check response byte 1 and byte 2(bits 31 - 16) */
					/* in function mmcCommandAndAnalisRespons(...) */
#define TRANSFER_STATE          1
/* ---------------------------------------------------------------------------- */

/* MultiMedia Card command definitions */
#define CMD_START_BIT           0x40

/* ------------------------- CMD's names: ---------------------------- */
#define GO_IDLE_STATE           (0 + CMD_START_BIT)
#define SEND_OP_COND            (1 + CMD_START_BIT)
#define ALL_SEND_CID            (2 + CMD_START_BIT)
#define SET_RELATIVE_ADDR       (3 + CMD_START_BIT)
#define SET_DSR                 (4 + CMD_START_BIT)
#define SELECT_DESELECT_CARD    (7 + CMD_START_BIT)
#define SEND_CSD                (9 + CMD_START_BIT)
#define SEND_CID                (10 + CMD_START_BIT)
#define READ_DAT_UNTIL_STOP     (11 + CMD_START_BIT)
#define STOP_TRANSMISSION       (12 + CMD_START_BIT)
#define SEND_STATUS             (13 + CMD_START_BIT)
#define SET_BUS_WIDTH_REGISTER  (14 + CMD_START_BIT)
#define GO_INACTIVE_STATE       (15 + CMD_START_BIT)
#define SET_BLOCKLEN            (16 + CMD_START_BIT)
#define READ_BLOCK              (17 + CMD_START_BIT)
#define READ_MULTIPLE_BLOCK     (18 + CMD_START_BIT)
#define WRITE_DAT_UNTIL_STOP    (20 + CMD_START_BIT)
#define WRITE_BLOCK             (24 + CMD_START_BIT)
#define WRITE_MULTIPLE_BLOCK    (25 + CMD_START_BIT)
#define PROGRAM_CID             (26 + CMD_START_BIT)
#define PROGRAM_CSD             (27 + CMD_START_BIT)
#define SET_WRITE_PROT          (28 + CMD_START_BIT)
#define CLR_WRITE_PROT          (29 + CMD_START_BIT)
#define SEND_WRITE_PROT         (30 + CMD_START_BIT)
#define TAG_SECTOR_START        (32 + CMD_START_BIT)
#define TAG_SECTOR_END          (33 + CMD_START_BIT)
#define UNTAG_SECTOR            (34 + CMD_START_BIT)
#define TAG_ERASE_GROUP_START   (35 + CMD_START_BIT)
#define TAG_ERASE_GROUP_END     (36 + CMD_START_BIT)
#define UNTAG_ERASE_GROUP       (37 + CMD_START_BIT)
#define ERASE_SECTORS           (38 + CMD_START_BIT)
#define CRC_ON_OFF              (59 + CMD_START_BIT)



/* MMC protocol reponse type */
typedef enum
{
	R0 = 0,         /* NONE response */
	R1 = 1,         /* Basic MMC response format */
	R2,             /* R2 response. Used by ALL_SEND_CID(CMD2), 
			   SEND_CID(CMD10) and SEND_CSD(CMD9)
			*/
	R3              /* R3 response. Used by SEND_OP_COND(CMD1) */

} RESP_TYPE;

typedef enum
{
	IDLE = 0,
	READY,
	IDENT,
	STANDBY,
	TRANSFER,
	DATA,
	RCV,
	PRG,
	DIS,
	RES1,
	RES2,
	RES3,
	RES4,
	RES5,
	RES6,
	RES7
} CARD_STATE;

#if (USE_MMC || USE_MMC_EMULATION)

#define OUT_OF_RANGE_ERROR      0x80000000      /* Bit 31 */
#define ADDRESS_ERROR           0x40000000      /* Bit 30 */
#define BLK_LENGTH_ERROR        0x20000000      /* Bit 29 */
#define ERASE_SEQ_ERROR         0x10000000      /* Bit 28 */
#define ERASE_PARAM             0x08000000      /* Bit 27 */
#define WP_VIOLATION            0x04000000      /* Bit 26 */
#define UNUSED_BIT_25                           /* Bit 25 */
#define UNUSED_BIT_24                           /* Bit 24 */
#define CMD_CRC_ERROR           0x00800000      /* Bit 23 */
#define COMUNC_ILLEG_COM        0x00400000      /* Bit 22 */
#define CARD_ECC_FAILED         0x00200000      /* Bit 21 */
#define CONTROLLER_ERROR        0x00100000      /* Bit 20 */
#define EERROR                  0x00080000      /* Bit 19 */
#define UNDERRUN                0x00040000      /* Bit 18 */
#define OVERRUN                 0x00020000      /* Bit 17 */
#define CIDCSD_OVERWRITE        0x00010000      /* Bit 16 */
#define WP_ERASE_SKIP           0x00008000      /* Bit 15 */
#define CARD_ECC_DISABLED       0x00004000      /* Bit 14 */
#define ERASE_RESET             0x00002000      /* Bit 13 */

#else

/* Constant masks for MMC status registers bits */

/*#define CARD_ECC_DISABLED       0x20000
#define CIDCSD_OVERWRITE        0x10000 */
#define OVERRUN                 0x8000
#define CMD_PARAM_ERROR         0x4000
#define ADDRESS_ERROR           0x2000
#define ERASE_SEQ_ERROR         0x1000
#define CMD_CRC_ERROR           0x0800
#define COMUNC_ILLEG_COM        0x0400
#define ERASE_RESET             0x0200
#define CARD_IS_NOT_READY       0x0100
#define CMD_PARAM_ERROR2        0x0080
#define ERASE_PARAM             0x0040
#define WP_VIOLATION            0x0020
#define CARD_READ_FAILURE       0x0010
#define EERROR                  0x0008
#define ERROR2                  0x0004
#define WP_ERASE_SKIP           0x0002
#define UNDERRUN                0x0001
#endif


typedef enum
{
	CSD_Structure = 0,
	CSD_Protocol_Ver,
	CSD_Res1,
	CSD_TimeAccess1,
	CSD_TimeAccess2,
	CSD_TransferRate,
	CSD_CmdClassHiByte,
	CSD_CmdClassLoByte,
	CSD_BlockLength,
	CSD_PartialSize,
	CSD_WriteMisalign,
	CSD_ReadMisalign,
	CSD_DSR,
	CSD_ExternalVPP,
/*
	CSD_Mantissa,
	CSD_Exponent,
*/
	CSD_CSizeHiByte,
	CSD_CSizeLoByte,
	CSD_VDDReadMin,
	CSD_VDDReadMax,
	CSD_VDDWriteMin,
	CSD_VDDWriteMax,
	CSD_CSizeMult,
	CSD_EraseSecSize,
	CSD_EraseGrpSize,
	CSD_WPGrpSize,
	CSD_WPGrpEnable,
	CSD_DefaultECC,
	CSD_StreamWrSpeedFactor,
	CSD_WrDataBlockLn,
	CSD_WrBlockPartial,
	CSD_Res2,
	CSD_Res3,
	CSD_Copy,
	CSD_PermWRProtect,
	CSD_TempWRProtect,
	CSD_ECC_Code,
	CSD_CRC,
	CSD_Res4,
	CSD_Error
} CSDFields;

/* MMC Card specific Data structure */
typedef struct
{
	UCHAR   fields[38];
} MMC_CSD;


/*--------------------------------------------------------------------------*/
typedef struct
{
	ULONG   manufact_ID;    /* Card Individual number */
	ULONG   ser_numb;       /* serial number */
	TEXT    model_numb[7];  /* Number of card model */
	UCHAR   hwinfo;         /* Hardware and firmware revision */
	UCHAR   date_code;      /* date manufacture */

} MMC_IDENT;

/*--------------------------------------------------------------------------*/



/*******************************************************************************
* Name: GetCSDStructFields
*
* Description:
*
* Input:
*       UCHAR *respond_csd
*       CSDFields *respCSD
*
* Output:
*
* Return:
*
********************************************************************************/
SDVOID GetCSDStructFields( UCHAR *respond_csd, UCHAR *respCSD );



MMC_CC mmcCommandAndResponse(PDEVICE_CONTROLLER pc, UINT16 Cmd, UINT32 Arg, UINT16 noBlocks, RESP_TYPE Resp );


/*******************************************************************************
* Name: receive_data
*
* Description:
*       Handle all data transfer from the target (device) to host.
* Input:
*       PDEVICE_CONTROLLER      pc
*       UINT16  data_length     Length of data to transfer
*
* Output:
*
* Return:
*       Completion code.
*
********************************************************************************/
MMC_CC receive_data(PDEVICE_CONTROLLER pc, UINT16 data_length);


/*******************************************************************************
* Name: send_data
*
* Description:
*       Handle all data transfer from the host to the target (device).
* Input:
*       PDEVICE_CONTROLLER      pc
*       UINT16  data_length     Length of data to transfer
*
* Output:
*
* Return:
*       Completion code.
*
********************************************************************************/
MMC_CC send_data(PDEVICE_CONTROLLER pc, UINT16 data_length);


/*******************************************************************************
* Name: mmcReset
*
* Description:
*       Resets the SanDisk card and the  hardware.  It must be called after
*       power before any other command can be used.
*
* Input:
*       SPI time base. The duration (in hundred nanoSeconds of the
*       SPI clock. It will be used for calculating various card and
*       communication time-outs.
*
* Output:
*       None.
*
* Returns:
*       Completion code
*
******************************************************************************/
MMC_CC mmcReset ( PDEVICE_CONTROLLER pc, ULONG setupInfo );


MMC_CC MMCAnalysisResponse( PDEVICE_CONTROLLER pc );


/***********************************************************************************
* Name: mmcIdentify
*
* Description:
*       Available in MMC mode only. Identifiies and sets and RCA
*       for an MMC card on the bus.
*
*       This function starts card identification cycle and (if a
*       valid response is received) sets the RCA of the identified
*       card. The CID of the identified card is returned to the
*       application.                                                                     
*
* Input:
*       RCA     A session address for the identified card.
* 
* Output:
*       None.
*
* Returns:
*       Completion code
*
************************************************************************************/
MMC_CC mmcIdentify ( PDEVICE_CONTROLLER pc, UINT16 RCA );


/*******************************************************************************
* Name: mmcSetStandbyState
*
* Description:
*       In MMC mode A select card command is sent to the card.
*
* Input:
*       RCA             The address of the card to be deselected.
*
* Output:
*       None.
*
* Returns:
*       Completion code.
*
********************************************************************************/
MMC_CC mmcSetStandbyState ( PDEVICE_CONTROLLER pc, UINT16 RCA );

/*******************************************************************************
* Name: mmcSetXferState
*
* Description:
*       In MMC mode A select card command is sent to the card.
*
* Input:
*       RCA             The address of the card to be selected.
*
* Output:
*       None.
*
* Returns:
*       Completion code.
*
********************************************************************************/
MMC_CC mmcSetXferState ( PDEVICE_CONTROLLER pc, UINT16 RCA );

/*******************************************************************************
* mmcGetConfiguration  - Retrieves the card configuration parameters.
*                        (Card Specific Data)
*
* Input:
*       PDEVICE_CONTROLLER      pc.
*       UCHAR   mmcCSD
*                               
* Output:
*       UCHAR   *mmcCSD - Card configuration record.
*
* Returns:
*       Completion code
*
******************************************************************************/
MMC_CC mmcGetConfiguration ( PDEVICE_CONTROLLER pc, UINT16 RCA, UCHAR *mmcCSD );


/*******************************************************************************
* mmcGetCardIdentification  - Retrieves the card Identification parameters.
*                             (Card Identification Data)
*
* output:     *mmcIdent - Card Identification record.
*
* returns:    completion code
*
******************************************************************************/
MMC_CC mmcGetCardIdentification( PDEVICE_CONTROLLER pc, UINT16 RCA, UCHAR *mmcIdent );


/*******************************************************************************
* mmcSetTempWP  -       Set new configuration to CSD register.
*
* input:                RCA and parameters for send to structure CSD
*
* output:               struct FieldCSD
*
* returns:              completion code
*
******************************************************************************/
MMC_CC mmcSetTempWP( PDEVICE_CONTROLLER pc, UINT16 RCA, SDBOOL temp_WP );


MMC_CC mmcBlkLengthConfiguration( PDEVICE_CONTROLLER pc, UINT16 RCA );

#if (!USE_MULTI)
/*******************************************************************************
* Name: mmcRead       -       Reads one card sector
*
* Input:
*       PDEVICE_CONTROLLER      pc
*       UINT16  RCA             Card address
*       ULONG   dataAddress     Sector address
*
* Output:
*       sector data
*
* Returns:
*       Completion code
*
******************************************************************************/
MMC_CC mmcRead ( PDEVICE_CONTROLLER pc, UINT16 RCA, ULONG dataAddress, UINT16 noBlocks );

/*******************************************************************************
* mmcWrite      -       Writes one card sector
*
* Input:
*       PDEVICE_CONTROLLER      pc
*       UINT16  RCA             Card address
*       ULONG   dataAddress     Sector address
*
* Output:
*       None.
*
* Returns:      completion code
*
*****************************************************************************/
MMC_CC mmcWrite ( PDEVICE_CONTROLLER pc, UINT16 RCA, ULONG dataAddress, UINT16 noBlocks );
#endif

#if (USE_MULTI)
/*******************************************************************************
* Name: mmcReadMultiple       -       Reads multiple card sectors
*
* Input:
*       PDEVICE_CONTROLLER      pc
*       UINT16  RCA             Card address
*       ULONG   dataAddress     Sector address
*
* Output:
*       sector data
*
* Returns:
*       Completion code
*
******************************************************************************/
MMC_CC mmcReadMultiple( PDEVICE_CONTROLLER pc, UINT16 RCA, ULONG dataAddress, UINT16 noBlocks );


/*******************************************************************************
* mmcWriteMultiple      -       Writes multiple card sector
*
* Input:
*       PDEVICE_CONTROLLER      pc
*       UINT16  RCA             Card address
*       ULONG   dataAddress     Sector address
*
* Output:
*       None.
*
* Returns:      completion code
*
*****************************************************************************/
MMC_CC mmcWriteMultiple( PDEVICE_CONTROLLER pc, UINT16 RCA, ULONG dataAddress, UINT16 noBlocks );
#endif
 
/*****************************************************************************
* mmcGetWPMap   -       If state of the card has write protect
*                       this function asks the card to send the
*                       status of the write protection bits
*
* input:        dataAddress -    Sector address
*               RCA         -    card address
*
* output:       bufPtr      -    read data
*
* returns:      completion code
*
******************************************************************************/
MMC_CC mmcGetWPMap( PDEVICE_CONTROLLER pc, UINT16 RCA, ULONG dataAddress );


/*******************************************************************************
* mmcSetGroupWP - Sets the write protection bit of the addressed group 
*
* input:        dataAddress -Sector address
*
* output:       None.
*
* returns:      completion code
*
******************************************************************************/
MMC_CC mmcSetGroupWP( PDEVICE_CONTROLLER pc, UINT16 RCA, ULONG dataAddress );



/*****************************************************************************
* mmcClearGroupWP - Clear the write protection bit of the addressed group 
*
* input:      groupAddress -    Write protect address group
*
* output:     None.
*
* returns:    completion code
*
******************************************************************************/
MMC_CC mmcClearGroupWP( PDEVICE_CONTROLLER pc, UINT16 RCA, ULONG groupAddress );


/*****************************************************************************
* mmcEraseSectors       -       Erase selected Sector one card
*
* input:        Sectors address (start ,end ,untage )
*
* output:       None.
*
* returns:      completion code
*
******************************************************************************/
MMC_CC mmcEraseSectors( PDEVICE_CONTROLLER pc, UINT16 RCA, ULONG start, ULONG end, ULONG untag );


/*****************************************************************************
* mmcEraseGroup - Erase selected group one card
*
* input:        Group address (start ,end ,untage )
*
* output:       None.
*
* returns:      completion code
*
******************************************************************************/
MMC_CC mmcEraseGroup( PDEVICE_CONTROLLER pc, UINT16 RCA, ULONG start, ULONG end, ULONG untag );


/*******************************************************************************
* mmcGetStatus - Retrieves the card current status.
*
* input:
*       PDEVICE_CONTROLLER      pc
*       UINT16          RCA
*
* output:
*       None.
*
* returns:
*       Completion code
*
******************************************************************************/
MMC_CC mmcGetStatus( PDEVICE_CONTROLLER pc, UINT16 RCA);


#if ( USE_SPI || USE_SPI_EMULATION )
#if (USE_SET_FEATURES)
/*******************************************************************************
*
* mmcTurnCRCOnOff - Turns the CRC option ON or OFF.
*
* input:      1 - turns the option ON
*             0 - turns the option OFF.
*
* output:     None.
*
* returns:    completion code
*
*****************************************************************************/
MMC_CC mmcTurnCRCOnOff( PDEVICE_CONTROLLER pc, UINT16 RCA, UINT16 opt );
#endif /* (USE_SET_FEATURES) */
#endif  /* (USE_SPI || USE_SPI_EMULATION) */


/*********************************************************************************
* Name: mmedia_io
*
* Description:
*
*
* Input:
*       INT16 driveno
*       ULONG sector
*       SDVOID *buffer
*       UCOUNT scount
*       INT16 op
*
* Output:
*
* Return:
*       Completion code
*
****************************************************************************************/
MMC_CC mmedia_io( INT16 driveno, ULONG sector, UTINY *buffer, UCOUNT scount, INT16 op );



/****************************************************************************************
* Name: mmedia_drive_open
*
* Description:
*       Initialize the drive, configure the drive's data structure
*
* Input:
*       INT16 driveno
*
* Output:
*       None
*
* Return:
*       YES if successful
*       NO if failure
*
*****************************************************************************************/
SDBOOL mmedia_drive_open ( INT16 driveno );


/**************************************************************************************** 
* Name: mmedia_drive_close
*
* Description:
*       Remove drive's data structure and restore old system configuration.
*
* Input:
*       INT16 driveno
*
* Output:
*       None
*
* Return:
*       YES if successful
*       NO if failure
*
*****************************************************************************************/ 
SDBOOL mmedia_drive_close ( INT16 driveno );


/***************************************************************************************
* Name: mmedia_read_serial
*
* Description:
*       This routine does the following operations:
*       . calls identify drive to get serial no
*       . copies the string into the output string and null terminate  
*
* Inputs:
*       driveno   - drive number
*       pserialno -  pointer to a buffer to hold the model number (at least 21 bytes)
*
* Returns:
*       YES on success else NO.
*       If NO pc->error_no will contain the error.
*   
****************************************************************************************/
SDBOOL mmedia_read_serial(INT16 driveno, PDRV_GEOMETRY_DESC idDrvPtr);



/*********************************  FOR SPI INTERFACE ************************************/
#if (USE_SPI || USE_SPI_EMULATION)


/*****************************************************************************
* spiCardInit  - Initialize spi devices.
*
* input:      None.
*
* output:     None.
*
* returns:    None
*
******************************************************************************/
SDVOID spiCardInit ( INT16 ctrlno );
MMC_CC IsCardBusy(PDEVICE_CONTROLLER pc);


/*****************************************************************************
* selectChip - Activates CS signal.
*
* input:
*       UINT16          driveno         Device Number
*
* output:
*       None.
*
* Returns:
*       Completion code
*
******************************************************************************/
#define selectChip(driveno)  (spi_cs_enable(driveno))


/******************************************************************************
* deSelectChip - deactivates CS signal.
*
* Input:
*       UINT16          driveno         Device Number
*
* Output:
*       None.
*
* Returns:
*       Completion code
*
******************************************************************************/
#define deSelectChip(driveno) (spi_cs_disable(driveno))


#endif  /* (USE_SPI || USE_SPI_EMULATION) */




/*********************************  FOR MMC INTERFACE ************************************/
#if (USE_MMC || USE_MMC_EMULATION)

/*****************************************************************************
*
* mmcCardInit  - Initialize the mmc cards.
*
* input:      None.
*
* output:     None.
*
* returns:    None
*
******************************************************************************/
SDBOOL mmcBusStart(INT16 ctrlNo);   /* Initialize the cards with 80 clocks */
SDBOOL mmc_init_setup( SDVOID );
MMC_CC mmcConfigDevice( INT16 driveno );
MMC_CC IsCardBusy(PDEVICE_CONTROLLER pc);


#endif /* (USE_MMC || USE_MMC_EMULATION) */


#ifdef __cplusplus
}
#endif


#define _SDMMC_H_

#endif


