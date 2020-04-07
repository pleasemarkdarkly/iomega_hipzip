// cs8405a.c
// CS8405A SPDIF driver
// Teman Clark-Lindh
// 07/29/02
// (c) Fullplay Media
//
// special notes:
// copy protection level is set by the L and COPY bits 
// in the consumer channel status bytes

#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/infra/diag.h>

#include <devs/audio/cs8405a.h>
#include "i2c.h"

#define CS8405A_I2C_ADDRESS 0x24

// data flow control
#define DFC 0x03
#define DFC_TXOFF (1<<6)
#define DFC_AESBP (1<<5)

// Clock Source Control
#define CSC  0x04
#define CSC_RUN (1<<6)
#define CSC_CLK(n) ((n & 0x3)<<4)

// Serial Audio Input Port Data Format
#define SAI 0x05
#define SAI_SILRPOL	(1<<0)
#define SAI_SISPOL	(1<<1)
#define SAI_SIDEL	(1<<2)
#define SAI_SIJUST	(1<<3)
#define SAI_SIRES0	(1<<4)
#define SAI_SIRES1	(1<<5)
#define SAI_SISF	(1<<6)
#define SAI_SIMS	(1<<7)

// Channel Status Data Buffer Control
#define CSDBC 0x12
#define CSDBC_CAM (1<<1)
#define CSDBC_EFTCI (1<<2)
#define CSDBC_BSEL (1<<5)

// Interrupt 1 Status Register
#define INT1S 0x07
#define INT1S_EFTC (1<<1)

// Interrupt 1 Mask Register
#define INT1M 0x09
#define INT1M_EFTCM (1<<1)

// read only ID/Version address
#define CSIDVER 0x7f

// Consumer Channel byte 0
#define CCS0 0x20
#define CCS0_COPY (1<<2)
#define CCS0_PRE (1<<3) // pre-emphasis bit

// Cosumer Channel byte 1
#define CCS1 0x21
#define CCS1_CAT_CD 0x01
#define CCS1_GEN (1<<7)

// shadow registers
static cyg_uint8 _SPDIFCCSB0 = 0;
static cyg_uint8 _SPDIFCCSB1 = 0;
static cyg_uint8 _SPDIFCSC = 0;
static cyg_uint8 _SPDIFSAI = 0;
static cyg_uint8 _SPDIFDFC = 0;

static bool s_bSPDIFEnabled = false;

static void SPDIFWriteStatus();


void SPDIFEnable()
{
	cyg_uint8 ver;

	if(!s_bSPDIFEnabled)
	{
		
		// read and verify part version information
       ver = I2CRead(CS8405A_I2C_ADDRESS,CSIDVER);
       diag_printf("CS8405A ID = %x\n",ver);

	   // read the temperature
	   ver = I2CRead(0x9A,0);
	   diag_printf("Current Temp = %d C\n",ver);

		// power on part
		_SPDIFCSC |= CSC_RUN;
		I2CWrite(CS8405A_I2C_ADDRESS,CSC,_SPDIFCSC);
   
		// does nothing
		//	_SPDIFDFC |= DFC_TXOFF;
		//	I2CWrite(CS8405A_I2C_ADDRESS,DFC,_SPDIFDFC);
  

		// unmask transfer IRQ
        I2CWrite(CS8405A_I2C_ADDRESS,INT1M,INT1M_EFTCM);

		// configure consumer status register defaults
        _SPDIFCCSB0 = CCS0_COPY;
        _SPDIFCCSB1 = CCS1_CAT_CD | CCS1_GEN; // no restrictions?
        SPDIFWriteStatus();

		s_bSPDIFEnabled = true;
	}
}

void SPDIFDisable()
{

	if(s_bSPDIFEnabled)
	{
		// power off part
		_SPDIFCSC &= ~CSC_RUN;
		I2CWrite(CS8405A_I2C_ADDRESS,CSC,_SPDIFCSC);

		s_bSPDIFEnabled = false;
	}
	

}

void SPDIFSetSecurity(bool bCopy, bool bGen)
{
    if(bCopy)
        _SPDIFCCSB0 |= CCS0_COPY;
    else
        _SPDIFCCSB0 &= ~CCS0_COPY;
    
    if(bGen)
        _SPDIFCCSB1 |= CCS1_GEN;
    else
        _SPDIFCCSB1 &= ~CCS1_GEN;
    

    SPDIFWriteStatus();

}

void SPDIFSetEmphasis(bool bEnabled)
{

	if(bEnabled)
		_SPDIFCCSB0 |= CCS0_PRE;
	else
		_SPDIFCCSB0 &= ~CCS0_PRE;


	SPDIFWriteStatus();


}

static void SPDIFWriteStatus()
{

	cyg_uint8 status;

	// inhibit E->F transfer
	I2CWrite(CS8405A_I2C_ADDRESS,CSDBC,CSDBC_EFTCI);

	// transfer bytes
	I2CWrite(CS8405A_I2C_ADDRESS,CCS0,_SPDIFCCSB0);
	I2CWrite(CS8405A_I2C_ADDRESS,CCS0,_SPDIFCCSB1);

	// clear interrupt register
    I2CRead(CS8405A_I2C_ADDRESS,INT1S);

	// re-enable E->F transfer	
	I2CWrite(CS8405A_I2C_ADDRESS,CSDBC,0);

	// wait for transfer interrupt
    //	do
    //	{
    //		status = I2CRead(CS8405A_I2C_ADDRESS,INT1S);
    //	} while (!(status & INT1S_EFTC));

}


