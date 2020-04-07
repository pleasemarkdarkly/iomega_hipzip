#include <cyg/hal/drv_api.h>
#include <cyg/hal/hal_diag.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <netdev.h>
#include <eth_drv.h>
/* TODO Move mmgpio functions into kernel */
#include "../../../../../../../../player/devs/mmgpio/include/mmgpio.h"
#include "hcf.h"

#define TR() /* diag_printf("%s:%d\n", __FUNCTION__, __LINE__) */

#define INTS_DONT_WORK
#undef  INTS_DONT_WORK

#ifdef INTS_DONT_WORK
#define STACK_SIZE (CYGNUM_HAL_STACK_SIZE_MINIMUM * 4)
static char OrinocoFakeIntStack[STACK_SIZE];
static cyg_thread OrinocoFakeIntThreadData;
static cyg_handle_t OrinocoFakeIntThreadHandle;
static void OrinocoFakeInt(cyg_addrword_t);
#endif

#define ETHER_ADDR_LEN 6
#define MAX_SSID_LEN 32

#define FALSE 0
#define TRUE (!FALSE)

#define ORINOCO_INT_NUM CYGNUM_HAL_INTERRUPT_EINT1 /* TODO Set these to XINT when that is working */
#define ORINOCO_INT_PRIORITY 1 /* TODO Adjust this parameter */
#define ORINOCO_ATTRIBUTE_BASE 0x32000000
#define ORINOCO_IO_BASE 0x32000000

typedef struct
{
    IFB_STRCT IFB;
} OrinocoPrivateData_T;
static OrinocoPrivateData_T OrinocoPrivateData;
/* Make this element global instead of accessing it via private data */
static IFBP pIFB = &OrinocoPrivateData.IFB;

#define LTV_BUF_SIZE 256
static hcf_16 LTVBuffer[LTV_BUF_SIZE + 2] = { 0 };
static LTV_STRCT *pLTVB = (LTVP)&LTVBuffer[0];

#ifndef INTS_DONT_WORK
static cyg_interrupt OrinocoInterrupt;
static cyg_handle_t  OrinocoInterruptHandle;
#endif

static unsigned char MACAddress[ETHER_ADDR_LEN];

ETH_DRV_SC(DharmaSC,
	   &OrinocoPrivateData,
	   "eth0",
	   OrinocoStart,
	   OrinocoStop,
	   OrinocoControl,
	   OrinocoCanSend,
	   OrinocoSend,
	   OrinocoRecv,
	   OrinocoDeliver, // "peudoDSR" called from fast net thread
	   OrinocoInt, // poll function, encapsulates ISR and DSR
	   OrinocoIntVector);

NETDEVTAB_ENTRY(DharmaNetdev,
		"Dharma",
		DharmaOrinocoInit,
		&DharmaSC);

#ifndef INTS_DONT_WORK
static int
OrinocoISR(cyg_vector_t Vector, cyg_addrword_t Data, HAL_SavedRegisters * Regs)
{
#if defined(CYGIMP_KERNEL_INTERRUPTS_CHAIN)
#else
    hcf_action(pIFB, HCF_ACT_INT_OFF);
#ifndef INTS_DONT_WORK
    cyg_drv_interrupt_mask(ORINOCO_INT_NUM);
#endif
    return (CYG_ISR_HANDLED | CYG_ISR_CALL_DSR);
#endif
}
#endif

static void
OrinocoDeliver(struct eth_drv_sc * SC)
{
#if defined(CYGIMP_KERNEL_INTERRUPTS_CHAIN)
#else
    OrinocoInt(SC);
    hcf_action(pIFB, HCF_ACT_INT_ON);
#ifndef INTS_DONT_WORK
    cyg_drv_interrupt_acknowledge(ORINOCO_INT_NUM);
    cyg_drv_interrupt_unmask(ORINOCO_INT_NUM);
#endif
#endif
}

static int
OrinocoIntVector(struct eth_drv_sc * SC) 
{
    return (ORINOCO_INT_NUM);
}

static LTVP
FillLTV(LTV_STRCT * LTV, hcf_16 Length, hcf_16 Type, void * Value)
{
    /* Round length to even number */
    if (Length & 0x01) {
	++Length;
    }
    LTV->len = Length / 2 + 1; /* Add 1 for typ field */
    LTV->typ = Type;
    if (Value != NULL) {
#if 1
	memcpy((hcf_16 *)&LTV->val[0], Value, Length);
#else
	/* This didn't fix anything */
	{
	    char * ByteValue = (char *)Value;
	    unsigned short WordSwapped;
	    int i;
	    for (i = 0; i < (Length / 2); ++i) {
		WordSwapped = *ByteValue++ << 8;
		WordSwapped |= *ByteValue;
		LTV->val[i] = WordSwapped;
	    }
	}
#endif
    }
    return (LTVP)LTV;
}

typedef struct {
    unsigned short Len;
    unsigned char Key[14];
} KeyInfo_T;

static int
PutParameters(IFBP pIFB)
{
    //unsigned short CreateIBSS = FALSE; /* This will try to talk to an access point */
    unsigned short CreateIBSS = TRUE;    /* This will connect to an ad-hoc network, the activity LED should
					    blink, the card will be beaconing */
    //static unsigned char DesiredSSID[MAX_SSID_LEN + 2] = { 0 }; /* NULL means to connect to any network */
    static unsigned char DesiredSSID[MAX_SSID_LEN + 2];
    static const char SSID[] = "Todd";
    //unsigned char DesiredSSID[MAX_SSID_LEN + 2] = "Wireless";
    unsigned short OwnChannel = 0; /* This should this be whatever the desired network is */
    int Status;

    unsigned short PortType = 1; /* Don't set this to 3 */

#if 0 /* Don't use encryption at this time */
    //unsigned short EncryptionEnabled = TRUE;
    unsigned short EncryptionEnabled = FALSE;
    unsigned short TxKeyID = 1;
    KeyInfo_T Key[4];
    unsigned char KeyString[] = "Todd4";
    int i;
    
    memset(Key, 0, sizeof(Key));
    Key[TxKeyID].Len = 5;
    for (i = 0; i < Key[TxKeyID].Len; ++i) {
	Key[TxKeyID].Key[i] = KeyString[i];
    }
#endif
    
    TR();
    
    /* The following is for diagnostic purposes only */
    if (hcf_get_info(pIFB, FillLTV(pLTVB, ETHER_ADDR_LEN, CFG_CNF_OWN_MAC_ADDR, NULL)) == HCF_SUCCESS) {
	int i;
	diag_printf("MAC Address: ");
	for (i = 0; i < ETHER_ADDR_LEN; i += 2) {
	    MACAddress[i] = pLTVB->val[i / 2] & 0xff;
	    MACAddress[i + 1] = pLTVB->val[i / 2] >> 8;
	    diag_printf("%02x %02x ", MACAddress[i], MACAddress[i + 1]);
	}
	diag_printf("\n");
    }

    hcf_put_info(pIFB, FillLTV(pLTVB, sizeof(CreateIBSS), CFG_CREATE_IBSS, &CreateIBSS));

    /* The DesiredSSID is a byte string, which is Pascal style in the hcf. */
    DesiredSSID[0] = strlen(SSID);
    memcpy(&DesiredSSID[2], SSID, strlen(SSID));
    hcf_put_info(pIFB, FillLTV(pLTVB, strlen(&DesiredSSID[2]) + 2, CNF_DESIRED_SSID, &DesiredSSID[0]));
    
    if (CreateIBSS) {
	if (OwnChannel == 0) {
	    OwnChannel = 10;
	}
	hcf_put_info(pIFB, FillLTV(pLTVB, sizeof(OwnChannel), CFG_CNF_OWN_CHANNEL, &OwnChannel));
    }

    hcf_put_info(pIFB, FillLTV(pLTVB, sizeof(PortType), CFG_CNF_PORT_TYPE, &PortType));

#if 0 /* Leave out the encryption nonsense */
    hcf_put_info(pIFB, FillLTV(pLTVB, sizeof(EncryptionEnabled), CFG_CNF_ENCRYPTION, &EncryptionEnabled));
    if(EncryptionEnabled) {
	hcf_put_info(pIFB, FillLTV(pLTVB, sizeof(TxKeyID), CFG_CNF_TX_KEY_ID, &TxKeyID));
	hcf_put_info(pIFB, FillLTV(pLTVB, sizeof(Key), CFG_CNF_DEFAULT_KEYS, Key));
    }
#endif
    
    Status = HCF_SUCCESS;
#if 0
    /* The only necessary parameter info is 'Create IBSS', 'SSID', and 'Own Channel' */
    Status = hcf_put_info(pIFB, FillLTV(pLTVB, sizeof(CreateIBSS), CFG_CREATE_IBSS, &CreateIBSS));
    if (Status == HCF_SUCCESS) {
	
	Status = hcf_put_info(pIFB, FillLTV(pLTVB, strlen(&DesiredSSID[2]) + 2, CNF_DESIRED_SSID, &DesiredSSID[0]));
	if (Status == HCF_SUCCESS) {

	    if (CreateIBSS) {
		if (OwnChannel == 0) {
		    OwnChannel = 10;
		}
		Status = hcf_put_info(pIFB, FillLTV(pLTVB, sizeof(OwnChannel), CFG_CNF_OWN_CHANNEL, &OwnChannel));
		if (Status == HCF_SUCCESS) {
		}
		else {
		    /* Own Channel failed */
		    diag_printf("hcf_put_info( OwnChannel ) failed: %d\n", Status);
		}
	    }
	}
	else {
	    /* Desired SSID failed */
	    diag_printf("hcf_put_info( SSID ) failed: %d\n", Status);
	}
    }
    else {
	/* Create IBSS failed */
	diag_printf("hcf_put_info( Create IBSS ) failed: %d\n", Status);
    }
#endif
    return Status;
}

/* GPIO for power supply */
#define SDA (1<<4)
#define SCL (1<<5)
#define SLA0 (1<<2)

typedef enum 
{
    VL_0,
    VL_33,
    VL_5,
    VL_12
} VoltageLevel_T;

static void
ClockPowerSupplyBit(int Bit)
{
    /* Data is clocked in on positive leading edge of clock,
       so set data line... */
    if (Bit) {
	*(volatile cyg_uint8 *)PDDR |= SDA;
    }
    else {
	*(volatile cyg_uint8 *)PDDR &= ~SDA;
    }

    /* ...and clock it in */
    *(volatile cyg_uint8 *)PDDR |= SCL;
    HAL_DELAY_US(5);
    *(volatile cyg_uint8 *)PDDR &= ~SCL;
}
    
static bool
ConfigurePowerSupply(VoltageLevel_T Voltage) 
{
    int ConfigurationData;
    int BitMask;

    /* Configure the GPIO first
      PD4 - SDA
      PD5 - SCL
      PB2 - SLA0 */
    *(volatile cyg_uint8 *)PDDDR &= ~(SDA | SCL);
    *(volatile cyg_uint8 *)PBDDR |= SLA0;
    
    /* Configuration will be for 5V, this gives the following
       bits, MSB (D8) first: 1 0110 1010 */
    if (Voltage != VL_5) {
	diag_printf("XXX Not supported XXX\n");
	return false;
    }
    ConfigurationData = 0x16a;

    /* The interface cannot be clocked faster than 2.5MHz, which
       is a 0.4uS pulse width, so we will be fine */

    /* The datasheet shows a clock before the first data bit is
       clocked in */
    *(volatile cyg_uint8 *)PDDR |= SCL;
    HAL_DELAY_US(5);
    *(volatile cyg_uint8 *)PDDR &= ~SCL;

    /* Now clock the data in */
    for (BitMask = 0x100; BitMask; BitMask >>= 1) {
	ClockPowerSupplyBit(ConfigurationData & BitMask);
    }

    /* Now latch the data in */
    *(volatile cyg_uint8 *)PBDR |= SLA0;
    HAL_DELAY_US(5);
    *(volatile cyg_uint8 *)PBDR &= ~SLA0;

    return true;
}

static bool
PowerOnCard(void)
{
    bool Status;
    
    /* NOTE All delays are longer than necessary */
    
    /* Card detect goes high - wait at least 50mS */
    //cyg_thread_delay(10);
    HAL_DELAY_US(10 * 1000);
    
    /* Apply VCC - wait at least 300 mS */
    Status = ConfigurePowerSupply(VL_5);
    if (Status == true) {
	//cyg_thread_delay(60);
	HAL_DELAY_US(60 * 1000);
    
	/* Assert reset for at least 10 uS */
	*(volatile cyg_uint8 *)PBDDR |= (1<<0);
	*(volatile cyg_uint8 *)PBDR |= (1<<0);
	HAL_DELAY_US(20);
	
	/* De-assert reset - wait at least 20 mS */
	*(volatile cyg_uint8 *)PBDR &= ~(1<<0);
	//cyg_thread_delay(10);
	HAL_DELAY_US(10 * 1000);
	
	/* Card access is now allowed */
	return true;
    }
    else {
	/* Could not configure power supply */
	return false;
    }
}

static void
SoftResetCard(volatile cyg_uint16 * COR) 
{
    /* NOTE All delays are longer than necessary */
    
    /* Assert reset for at least 10 uS */
    *COR = 0x80;
    HAL_DELAY_US(20);
    
    /* De-assert reset - wait at least 20 mS */
    *COR = 0x00;
    //cyg_thread_delay(10);
    HAL_DELAY_US(10 * 1000);
    
    /* Card access is now allowed */
}

static void
PrintCIS(volatile cyg_uint16 * CIS)
{
    int Position = 0;
    cyg_uint8 Tag;
    cyg_uint8 Link;
    int i;
    
    for (;;) {
	Tag = CIS[Position++] & 0xff;
	diag_printf("TAG %02x ", Tag);
	if (Tag == 0xff) {
	    break;
	}
	Link = CIS[Position++] & 0xff;
	diag_printf("LNK %02x ", Link);
	for (i = 0; i < Link; ++i) {
	    diag_printf("%02x ", CIS[Position++] & 0xff);
	}
	diag_printf("\n");
    }
    diag_printf("\n");
}

static bool
InitializePCMCIAInterface(void) 
{
    volatile cyg_uint16 * AttributeMemory = (volatile cyg_uint16 *)ORINOCO_ATTRIBUTE_BASE;
    bool Status;

    TR();
    
    /* TODO Figure out a way to add the driver after threads are running, or at least
       execute this code later.  Problem occurs when calling upper level init with MAC
       address - the interface and the card must be initialized to obtain this info. */
    
    /* Set up GPIO before anything else so that we know what state the
       outputs are in before trying to power on the card */
    SetMMGPO(MMGPO_PCMCIA_IO_SELECT, (MMGPO_PCMCIA_8_16_SELECT | MMGPO_PCMCIA_REG));

    /* Card insertion */
    diag_printf("nEINT2: %x\n", *(volatile cyg_uint32 *)INTSR1 & INTSR1_EINT2);
    diag_printf("CARD_DETECT (MMI_4): %x\n", GetMMGPI() & MMGPI_PCMCIA_CD);
    
    /* Determine voltage of card */
    diag_printf("VS1 (MMI_0): %x VS2 (MMI_1): %x\n", GetMMGPI() & MMGPI_PCMCIA_VS1,
	  GetMMGPI() & MMGPI_PCMCIA_VS2);

    /* Turn on card */
    diag_printf("Powering on card\n");
    Status = PowerOnCard();
    if (Status == true) {

	/* Wait for card to be ready */
	diag_printf("Waiting for card to be ready\n");
	/* TODO Let this timeout */
	while (!(GetMMGPI() & MMGPI_PCMCIA_RDY))
	    ;
	
	/* Configure bus to 16bit databus, 8 wait states, EXPRDY.
	   Card is found at nCS3 | A25 */
	*(volatile cyg_uint32 *)MEMCFG1 &= ~0xff000000;
	*(volatile cyg_uint32 *)MEMCFG1 |= 0x81000000 | ((0<<2)<<24);
	*(volatile cyg_uint32 *)SYSCON1 |= SYSCON1_EXCKEN; /* This doesn't seem to do anything, but I will play it safe */
	
	/* Soft reset card and wait for ready again */
	diag_printf("Soft resetting card\n");
	SoftResetCard((volatile cyg_uint16 *)0x320003e0); /* 0x3e0 for Orinoco */
	
	/* Wait for card to be ready */
	diag_printf("Waiting for card to be ready\n");
	/* TODO Let this timeout */
	while (!(GetMMGPI() & MMGPI_PCMCIA_RDY))
	    ;
	
	/* Read CIS */
	diag_printf("Reading CIS\n");
	PrintCIS(AttributeMemory);
	
	return true;
    }
    else {
	/* Could not power on card */
	return false;
    }
}

static bool
DharmaOrinocoInit(struct cyg_netdevtab_entry * Tab)
{
    struct eth_drv_sc * SC = (struct eth_drv_sc *)Tab->device_instance;
    OrinocoPrivateData_T * PrivateData = (OrinocoPrivateData_T *)SC->driver_private;
    IFBP pIFB = &PrivateData->IFB;
    bool InitStatus = false;
    volatile cyg_uint16 * AttributeMemory = (volatile cyg_uint16 *)ORINOCO_ATTRIBUTE_BASE;
    int Status;

    TR();
    
    /* Initialize the interface */
    InitStatus = InitializePCMCIAInterface();
    if (InitStatus == true) {

	/* Configure the card */
	diag_printf("Configuring the card\n");
	AttributeMemory[0x3e0 >> 1] = 0x41;
	diag_printf("COR %08x = %02x\n", &AttributeMemory[0x3e0 >> 1], AttributeMemory[0x3e0 >> 1]);

	/* Wait for card to be ready */
	diag_printf("Waiting for card to be ready\n");
	/* TODO Let this timeout */
	while (!(GetMMGPI() & MMGPI_PCMCIA_RDY))
	    ;
    
	/* Set the interface to I/O mode */
	diag_printf("Setting interface for I/O mode\n");
	SetMMGPO(0, (MMGPO_PCMCIA_REG | MMGPO_PCMCIA_8_16_SELECT | MMGPO_PCMCIA_IO_SELECT));

	/* Configure bus to 16bit databus, 8 wait states, EXPRDY.
	   Card is found at nCS3 | A25 */
	*(volatile cyg_uint32 *)MEMCFG1 &= ~0xff000000;
	/* 1 wait state + EXPRDY + EXCKEN I would think would work, but nope.
	   Max wait states + EXPRDY doesn't work either, but catches some of the bytes. */
	*(volatile cyg_uint32 *)MEMCFG1 |= 0x81000000 | ((0<<2)<<24);
	*(volatile cyg_uint32 *)SYSCON1 |= SYSCON1_EXCKEN; /* This doesn't seem to do anything, but I will play it safe */
	
	/* Initialize host control library */
	hcf_connect(pIFB, ORINOCO_IO_BASE);
	
	/* Report that card is inserted. */
	Status = hcf_action(pIFB, HCF_ACT_CARD_IN);
	// Status = hcf_action(pIFB, HCF_ACT_CARD_OUT);
	if (Status == HCF_SUCCESS) {
	    
	    /* Configure parameters of card.  This can be done without the card inserted. */
	    Status = PutParameters(pIFB);
	    if (Status == HCF_SUCCESS) {

#ifndef INTS_DONT_WORK
		/* Register interrupt handler */
		cyg_drv_interrupt_create(ORINOCO_INT_NUM,
					 ORINOCO_INT_PRIORITY,
					 (cyg_addrword_t)SC,
					 (cyg_ISR_t *)OrinocoISR,
					 (cyg_DSR_t *)eth_drv_dsr,
					 &OrinocoInterruptHandle,
					 &OrinocoInterrupt);
		cyg_drv_interrupt_attach(OrinocoInterruptHandle);
		cyg_drv_interrupt_acknowledge(ORINOCO_INT_NUM);
#else
		cyg_thread_create(1,                 // Priority
				  OrinocoFakeInt,   // entry
				  0,                 // entry parameter
				  "Orinoco Int",      // Name
				  &OrinocoFakeIntStack[0],         // Stack
				  STACK_SIZE,        // Size
				  &OrinocoFakeIntThreadHandle,    // Handle
				  &OrinocoFakeIntThreadData       // Thread data structure
		    );
		cyg_thread_resume(OrinocoFakeIntThreadHandle);  // Start it
#endif		
		
		/* Initialize upper level driver */
		(SC->funs->eth_drv->init)(SC, MACAddress);
		
		InitStatus = true;
	    }
	    else {
		/* PutParameters failed */
		InitStatus = false;
	    }
	}
	else {
	    
	    /* hcf_action(CARD_IN) failed */
	    diag_printf("hcf_action(pIFB, HCF_ACT_CARD_IN) failed: %d\n", Status);
	    InitStatus = false;
	}
    }

    return InitStatus;
}

/* TODO Get rid of this.  This requires making sure INT_ON/OFF calls are balanced,
   even though Start can be called multiple times and while running */
static bool Running = false;

static void
OrinocoStop(struct eth_drv_sc * SC)
{
    OrinocoPrivateData_T * PrivateData = (OrinocoPrivateData_T *)SC->driver_private;
    IFBP pIFB = &PrivateData->IFB;
    int Status;
    
    TR();

    if (Running) {

	/* Disable the card */
	hcf_action(pIFB, HCF_ACT_INT_OFF);
	
	Status = hcf_disable(pIFB, HCF_PORT_0);
	if (Status != HCF_SUCCESS) {
	    diag_printf("FIXME %s status %d FIXME\n", __FUNCTION__, Status);
	}
	
	Running = false;
    }
}

/* Copied from if_edb7xxx.c: */
//
// This function is called to "start up" the interface.  It may be called
// multiple times, even when the hardware is already running.  It will be
// called whenever something "hardware oriented" changes and should leave
// the hardware ready to send/receive packets.
//
static void
OrinocoStart(struct eth_drv_sc * SC, unsigned char * MACAddress, int Flags)
{
    OrinocoPrivateData_T * PrivateData = (OrinocoPrivateData_T *)SC->driver_private;
    IFBP pIFB = &PrivateData->IFB;
    int Status;

    TR();

    if (!Running) {
	/* The card must be inserted at this point */
#ifndef INTS_DONT_WORK
	cyg_drv_interrupt_unmask(ORINOCO_INT_NUM);
#endif
	
	/* Turn on interrupts */
	hcf_action(pIFB, HCF_ACT_INT_ON);
	
	/* Start the card */
	Status = hcf_enable(pIFB, HCF_PORT_0);
	if (Status != HCF_SUCCESS) {
	    diag_printf("FIXME %s status %d FIXME\n", __FUNCTION__, Status);
	}

	Running = true;
    }
}

static int
OrinocoControl(struct eth_drv_sc * SC, unsigned long Key,
	       void * Data, int DataLength) 
{
    int Status;

    TR();
    
    switch (Key) {
	case ETH_DRV_SET_MAC_ADDRESS:
	{
	    Status = 0;
	    break;
	}

	case ETH_DRV_GET_IF_STATS: 
	{
	    struct ether_drv_stats * EthStats = (struct ether_drv_stats *)Data;
	    
	    /* TODO This reports link OK all the time */
	    EthStats->operational = 3;

	    Status = 0;
	    break;
	}
	
	default:
	{
	    Status = 1;
	    break;
	}
    }

    return Status;
}

static void
ReceivePacket(struct eth_drv_sc * SC) 
{
    OrinocoPrivateData_T * PrivateData = (OrinocoPrivateData_T *)SC->driver_private;
    IFBP pIFB = &PrivateData->IFB;

    //diag_printf("RxLen[0] %d\n", pIFB->IFB_RxLen);
    (SC->funs->eth_drv->recv)(SC, pIFB->IFB_RxLen);
}

static void
OrinocoRecv(struct eth_drv_sc * SC, struct eth_drv_sg * SGList, int SGLength) 
{
    OrinocoPrivateData_T * PrivateData = (OrinocoPrivateData_T *)SC->driver_private;
    IFBP pIFB = &PrivateData->IFB;
    int TotalLength = 0;
    unsigned char * Data;
    int Length;
    int i;

    TR();
    
    for (i = 0; i < SGLength; ++i) {
	Data = (unsigned char *)SGList[i].buf;
	Length = SGList[i].len;

	hcf_get_data(pIFB, TotalLength, Data, Length);
	TotalLength += Length;
    }
}

static int
OrinocoCanSend(struct eth_drv_sc * SC) 
{
    OrinocoPrivateData_T * PrivateData = (OrinocoPrivateData_T *)SC->driver_private;
    IFBP pIFB = &PrivateData->IFB;
    int s;
    
    TR();
    
    if (pIFB->IFB_PIFRscInd == 0) {
#if 0
	/* TODO This is a hack for either interrupts not working reliably or me
	   not understanding exactly how this value gets updated asynchronously */
	s = splnet();
	hcf_action(pIFB, HCF_ACT_INT_OFF);
        OrinocoInt(&DharmaSC);
	hcf_action(pIFB, HCF_ACT_INT_ON);
        splx(s);
#endif
	if (pIFB->IFB_PIFRscInd == 0) {
	    return 0;
	}
	else {
	    return 1;
	}
    }
    else {
	return 1;
    }
}

static void
OrinocoSend(struct eth_drv_sc * SC, struct eth_drv_sg * SGList, int SGLength,
	    int TotalLength, unsigned long Key) 
{
    OrinocoPrivateData_T * PrivateData = (OrinocoPrivateData_T *)SC->driver_private;
    IFBP pIFB = &PrivateData->IFB;
    unsigned char * Data;
    int Length;
    int i;

    TR();
    
    if (pIFB->IFB_PIFRscInd != 0) {
	hcf_action(pIFB, HCF_ACT_INT_OFF);
	for (i = 0; i < SGLength; ++i) {
	    Data = (unsigned char *)SGList[i].buf;
	    Length = SGList[i].len;
	    hcf_put_data(pIFB, Data, Length, HCF_PORT_0);
	}
	hcf_send(pIFB, HCF_PORT_0);
	hcf_action(pIFB, HCF_ACT_INT_ON);

	/* Buffer is free */
	/* See if there is a way to tell when the transmission is done so that
	   code flow can be closer to cs8900.
	   Answer is Nope (Would be HREG_EV_TX, but hcf doesn't use that) */
	(SC->funs->eth_drv->tx_done)(SC, Key, 0);
    }
    else {
	/* No resources available, cannot transmit */
	/* Free the buffer with error indication */
	(SC->funs->eth_drv->tx_done)(SC, Key, 1);
    }
}

static void
OrinocoInt(struct eth_drv_sc * SC)
{
#if defined(CYGIMP_KERNEL_INTERRUPTS_CHAIN)
#else
    OrinocoPrivateData_T * PrivateData = (OrinocoPrivateData_T *)SC->driver_private;
    IFBP pIFB = &PrivateData->IFB;

    TR();
    
    for (;;) {
	hcf_service_nic(pIFB);
	if (pIFB->IFB_RxLen != 0) {
	    ReceivePacket(SC);
	}
	else {
	    break;
	}
    }
#endif
}

hcf_8
in_port_byte(unsigned int port) 
{
    hcf_8 byte = *((volatile hcf_8 *)port);
    //diag_printf("IN_PORT_BYTE(%x) = %x\n", port, byte);
    return byte;
}

hcf_16
in_port_word(unsigned int port) 
{
    hcf_16 word = *((volatile hcf_16 *)port);
    //diag_printf("IN_PORT_WORD(%x) = %x\n", port, word);
    return word;
}

void
out_port_byte(unsigned int port, hcf_8 value) 
{
    //diag_printf("OUT_PORT_BYTE(%x, %x)\n", port, value);
    *((volatile hcf_8 *)port) = value;
}

void
out_port_word(unsigned int port, hcf_16 value) 
{
    //diag_printf("OUT_PORT_WORD(%x, %x)\n", port, value);
    *((volatile hcf_16 *)port) = value;
}

#ifdef INTS_DONT_WORK
void
OrinocoFakeInt(cyg_addrword_t param)
{
    int s;

    while (true) {
        cyg_thread_delay(5);
        s = splnet();
	hcf_action(pIFB, HCF_ACT_INT_OFF);
        OrinocoInt(&DharmaSC);
	hcf_action(pIFB, HCF_ACT_INT_ON);
        splx(s);
    }
}
#endif
