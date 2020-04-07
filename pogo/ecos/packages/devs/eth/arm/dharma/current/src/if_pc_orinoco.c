#include <pkgconf/devs_eth_arm_dharma.h>
#include <cyg/hal/drv_api.h>
#include <cyg/hal/hal_diag.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/io/pcmcia.h>
#include <netdev.h>
#include <eth_drv.h>
#include <cyg/hal/mmgpio.h>
#include "hcf.h"

#define INFRASTRUCTURE_MODE
#undef INFRASTRUCTURE_MODE

/* These should be hcf.h */
#define PORT_STAT_DISABLED 1
#define PORT_STAT_SEARCHING 2
#define PORT_STAT_IBSS 3
#define PORT_STAT_ESS 4
#define PORT_STAT_ERANGE 5 /* Out of range (in ESS) */
#define PORT_STAT_WDS 6

typedef struct
{
    hcf_16 ChannelID;
    hcf_16 AverageNoiseLevel;
    hcf_16 SignalLevel;
    hcf_16 MACAddress[3];
    hcf_16 BeaconInterval;
#define CAP_ESS (1<<0)
#define CAP_IBSS (1<<1)
#define CAP_CF_POLLABLE (1<<2)
#define CAP_CF_POLL_REQ (1<<3)
#define CAP_PRIVACY (1<<4)
    hcf_16 Capability;
    hcf_16 SSIDLen;
    hcf_16 SSIDValue[16];
} ScanResult_T;

#define TR() /* diag_printf("%s:%d\n", __FUNCTION__, __LINE__) */

/* CIS definitions */
#define ORINOCO_MANUF 0x0156
#define LAN_NID 0x04

#define ETHER_ADDR_LEN 6
#define MAX_SSID_LEN 32

#define FALSE 0
#define TRUE (!FALSE)

#ifdef CYGPKG_NET
#define STACK_SIZE (32 * 1024) //CYGNUM_HAL_STACK_SIZE_TYPICAL
static char OrinocoCardHandlerStack[STACK_SIZE];
static cyg_thread OrinocoCardHandlerThreadData;
static cyg_handle_t OrinocoCardHandlerThreadHandle;
#endif  // CYGPKG_NET

/* Encryption keys */
typedef struct {
    unsigned short Len;
    unsigned char Key[14];
} KeyInfo_T;

/* Driver private data */
typedef struct
{
    IFB_STRCT IFB;
    struct cf_slot * Slot;
    struct cyg_netdevtab_entry * Table;
} OrinocoPrivateData_T;
static OrinocoPrivateData_T OrinocoPrivateData;
/* Make this element global instead of accessing it via private data */
static IFBP pIFB = &OrinocoPrivateData.IFB;

#define LTV_BUF_SIZE 256
static hcf_16 LTVBuffer[LTV_BUF_SIZE + 2] = { 0 };
static LTV_STRCT *pLTVB = (LTVP)&LTVBuffer[0];

static hcf_16 MailBox[512];

/* Has the interface been enabled? */
static bool Running = false;

static unsigned char MACAddress[ETHER_ADDR_LEN];

ETH_DRV_SC(DharmaSC,
	   &OrinocoPrivateData,
	   "eth1",
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

static void
DoDelay(int Ticks)
{
#ifdef CYGPKG_KERNEL
    cyg_thread_delay(Ticks);
#else
    CYGACC_CALL_IF_DELAY_US(10000 * Ticks);
#endif
}

#if defined(CYGIMP_KERNEL_INTERRUPTS_CHAIN)
static int
OrinocoISR(cyg_vector_t Vector, cyg_addrword_t Data)
{
    int Status;

    TR();

    cyg_drv_interrupt_mask(CYGNUM_HAL_INTERRUPT_EINT1);
    hcf_action(pIFB, HCF_ACT_INT_OFF);
    Status = hcf_service_nic(pIFB);
    if (pIFB->IFB_RxLen == 0) {
	//diag_printf("ORINOCO0\n");
	hcf_action(pIFB, HCF_ACT_INT_ON);
	cyg_drv_interrupt_unmask(CYGNUM_HAL_INTERRUPT_EINT1);
	return 0;
    }
    else {
	//diag_printf("ORINOCO1\n");
	return (CYG_ISR_HANDLED | CYG_ISR_CALL_DSR);
    }
}
#endif

static void
OrinocoDSR(cyg_vector_t Vector, cyg_ucount32 Count, cyg_addrword_t Data) 
{
    TR();
    
#if defined(CYGIMP_KERNEL_INTERRUPTS_CHAIN)
    eth_drv_dsr(Vector, Count, Data);
#else
    hcf_action(pIFB, HCF_ACT_INT_OFF);
    eth_drv_dsr(Vector, Count, Data);
#endif
}

static void
OrinocoDeliver(struct eth_drv_sc * SC)
{
    OrinocoInt(SC);
    hcf_action(pIFB, HCF_ACT_INT_ON);
}

static int
OrinocoIntVector(struct eth_drv_sc * SC) 
{
    OrinocoPrivateData_T * PrivateData = (OrinocoPrivateData_T *)SC->driver_private;

    return PrivateData->Slot->int_num;
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
	memcpy((hcf_16 *)&LTV->val[0], Value, Length);
    }
    return (LTVP)LTV;
}

static int
PutParameters(IFBP pIFB)
{
    static unsigned char DesiredSSID[MAX_SSID_LEN + 2] = "\0\0" CYGDAT_ARM_DHARMA_SET_SSID;
    unsigned short OwnChannel = CYGDAT_ARM_DHARMA_SET_CHANNEL;
    unsigned short PortType = 1; /* Don't set this to 3 */
#if defined(INFRASTRUCTURE_MODE)
    unsigned short CreateIBSS = FALSE; /* This will try to connect to an access point */
#else
    unsigned short CreateIBSS = TRUE;    /* This will connect to an ad-hoc network, the activity LED should
					    blink, the card will be beaconing */
#endif
    int Status;

#if 0 /* Don't use encryption at this time */
    unsigned short EncryptionEnabled = TRUE;
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
    DesiredSSID[0] = strlen(CYGDAT_ARM_DHARMA_SET_SSID);
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

//
// This runs as a separate thread to handle the card.  In particular, insertions
// and deletions need to be handled and they take time/coordination, thus the
// separate thread.
//
#ifdef CYGPKG_NET
static void
#else
static int
#endif
OrinocoCardHandler(cyg_addrword_t Param)
{
    struct eth_drv_sc * SC = (struct eth_drv_sc *)Param;
    OrinocoPrivateData_T * PrivateData = (OrinocoPrivateData_T *)SC->driver_private;
    struct cf_slot * Slot;
    struct cf_cftable CFTable;
    struct cf_config Config;
    unsigned char CISBuffer[256];
    int CISLength;
    int CISOffset;
    unsigned char * pCIS;
    unsigned char * Product, * Manufacturer, * Revision, * Date;
    int COR = 0;
#ifndef CYGPKG_NET
    int Tries = 0;
#endif
    bool First = true;
    int Status;
    int i;
    
    Slot = PrivateData->Slot;
    cyg_drv_dsr_lock();
    while (true) {
        cyg_drv_dsr_unlock();   // Give DSRs a chance to run (card insertion)
        cyg_drv_dsr_lock();
        if ((Slot->state == CF_SLOT_STATE_Inserted) ||
            ((Slot->state == CF_SLOT_STATE_Ready) && First)) {
            First = false;
            if (Slot->state != CF_SLOT_STATE_Ready) {
                cf_change_state(Slot, CF_SLOT_STATE_Ready);
            }
            if (Slot->state != CF_SLOT_STATE_Ready) {
                diag_printf("CF card won't go ready!\n");
#ifndef CYGPKG_NET
                return false;
#else
                continue;
#endif
            }
            CISLength = sizeof(CISBuffer);
            CISOffset = 0;
            if (cf_get_CIS(Slot, CF_CISTPL_MANFID, CISBuffer, &CISLength, &CISOffset)) {
                if (*(short *)&CISBuffer[2] != ORINOCO_MANUF) {
                    diag_printf("Not an Orinoco, sorry\n");
                    continue;
                }
            } 
            CISOffset = 0;
            if (cf_get_CIS(Slot, CF_CISTPL_VERS_1, CISBuffer, &CISLength, &CISOffset)) {
                // Find individual strings
                pCIS = &CISBuffer[4];
                Product = pCIS;
                while (*pCIS++) ;  // Skip to nul
                Manufacturer = pCIS;
                while (*pCIS++) ;  // Skip to nul
                Revision = pCIS;
                while (*pCIS++) ;  // Skip to nul
                Date = pCIS;
#ifndef CYGPKG_NET
                if (Tries != 0) printf("\n");
#endif
		diag_printf("%s: %s %s %s\n", Manufacturer, Product, Revision, Date);
            }
            CISOffset = 0;
            if (cf_get_CIS(Slot, CF_CISTPL_CONFIG, CISBuffer, &CISLength, &CISOffset)) {
                if (cf_parse_config(CISBuffer, CISLength, &Config)) {
                    COR = Config.base;
                }
            }
            if (!COR) {
                diag_printf("Couldn't find COR pointer!\n");
                continue;
            }
            /* Fetch hardware address from card */
	    CISOffset = 0;
	    while (cf_get_CIS(Slot, CF_CISTPL_FUNCE, CISBuffer, &CISLength, &CISOffset)) {
		if (CISBuffer[2] == LAN_NID) {
		    for (i = 0; i < ETHER_ADDR_LEN; ++i) {
			MACAddress[i] = CISBuffer[i + 4];
		    }
		    break;
		}
	    }
            CISOffset = 0;
            if (cf_get_CIS(Slot, CF_CISTPL_CFTABLE_ENTRY, CISBuffer, &CISLength, &CISOffset)) {
                if (cf_parse_cftable(CISBuffer, CISLength, &CFTable)) {
		    /* 0x40 - Level mode interrupts */
                    cf_set_COR(Slot, COR, (CFTable.cor | 0x40));

		    /* This is the part that breaks the abstraction.  I thought that I could
		       hide it under cf_set_COR, but that is in the device independent portion of
		       the driver. */
		    /* Put interface into I/O mode */
		    SetMMGPO(0, (MMGPO_PCMCIA_REG | MMGPO_PCMCIA_8_16_SELECT | MMGPO_PCMCIA_IO_SELECT));

		    /* Initialize host control library */
		    hcf_connect(pIFB, (hcf_io)PrivateData->Slot->io);

		    /* Register a mailbox.  Size of mailbox is in number of words */
		    pLTVB->len = 4;
		    pLTVB->typ = CFG_REG_MB;
		    *(cyg_uint32 *)(&pLTVB->val[0]) = (cyg_uint32)MailBox;
		    pLTVB->val[2] = sizeof(MailBox) / sizeof(hcf_16);
		    Status = hcf_put_info(pIFB, pLTVB);
		    diag_printf("CFG_REG_MB status %d\n", Status);

		    /* Unmasking creates problems when interrupts are unchained, since someone else will do it
		       and we need to be ready to handle it.  So if interrupts are chained, let this driver do
		       the unmasking.  Note that this should be done in the pcmcia layer, but it won't work that
		       way.  The abstraction is broken all over the place. */
#if defined(CYGIMP_KERNEL_INTERRUPTS_CHAIN)
		    {
			extern cyg_handle_t cf_irq_interrupt_handle;
			cyg_drv_interrupt_attach(cf_irq_interrupt_handle);
		    }
#endif
		    /* Unmask the interrupt now that it is safe to do so */
		    cyg_drv_interrupt_unmask(CYGNUM_HAL_INTERRUPT_EINT1);
		    
		    // Initialize upper level driver
                    (SC->funs->eth_drv->init)(SC, MACAddress);
                    // Tell system card is ready to talk
                    PrivateData->Table->status = CYG_NETDEVTAB_STATUS_AVAIL;
#ifndef CYGPKG_NET
                    cyg_drv_dsr_unlock();
                    return true;
#endif
                } else {
                    diag_printf("Can't parse CIS\n");
                    continue;
                }
            } else {
                diag_printf("Can't fetch config info\n");
                continue;
            }
        } else if (Slot->state == CF_SLOT_STATE_Removed) {
            diag_printf("PCMCIA card removed!\n");
        } else {
            cyg_drv_dsr_unlock();
            DoDelay(50);  // FIXME!
#ifndef CYGPKG_NET
            if (Tries == 0) printf("... Waiting for network card: ");
            printf(".");
            if (++Tries == 10) {
                // 5 seconds have elapsed - give up
                return false;
            }
            cf_hwr_poll(Slot);  // Check to see if card has been inserted
#endif
            cyg_drv_dsr_lock();
        }
    }
}

static bool
DharmaOrinocoInit(struct cyg_netdevtab_entry * Table)
{
    struct eth_drv_sc * SC = (struct eth_drv_sc *)Table->device_instance;
    OrinocoPrivateData_T * PrivateData = (OrinocoPrivateData_T *)SC->driver_private;

    TR();

    /* Initialize the PCMCIA interface */
    cf_init();
    PrivateData->Slot = cf_get_slot(0);
    PrivateData->Table = Table;

#ifdef CYGPKG_NET
    // Create card handling [background] thread
    cyg_thread_create(CYGPKG_NET_THREAD_PRIORITY-1,          // Priority
                      OrinocoCardHandler,                   // entry
                      (cyg_addrword_t)SC,                    // entry parameter
                      "Orinoco card support",                // Name
                      &OrinocoCardHandlerStack[0],         // Stack
                      STACK_SIZE,                            // Size
                      &OrinocoCardHandlerThreadHandle,    // Handle
                      &OrinocoCardHandlerThreadData       // Thread data structure
            );
    cyg_thread_resume(OrinocoCardHandlerThreadHandle);    // Start it

    // Initialize environment, setup interrrupt handler - actually the DSR
    cf_register_handler(PrivateData->Slot, OrinocoDSR, (cyg_addrword_t)SC);
#if defined(CYGIMP_KERNEL_INTERRUPTS_CHAIN)
    cf_register_handler_isr(PrivateData->Slot, OrinocoISR, (cyg_addrword_t)SC);
#endif
    
    return false;  // Device is not ready until inserted, powered up, etc.
#else
    // Initialize card
    return OrinocoCardHandler(SC);
#endif    
}

static void
OrinocoStop(struct eth_drv_sc * SC)
{
    OrinocoPrivateData_T * PrivateData = (OrinocoPrivateData_T *)SC->driver_private;
    IFBP pIFB = &PrivateData->IFB;
    int NumTries;
    int Status;
    
    TR();

    if (Running) {	
	Running = false;

	/* Disable the card */
	hcf_action(pIFB, HCF_ACT_INT_OFF);

	NumTries = 0;
	do {
	    Status = hcf_disable(pIFB, HCF_PORT_0);
	    if (Status != HCF_SUCCESS) {
		diag_printf("hcf_disable failed: %d.  Trying again...\n", __FUNCTION__, Status);
	    }
	} while (Status != HCF_SUCCESS && ++NumTries < 10);
	if (Status != HCF_SUCCESS) {
	    diag_printf("FIXME Could not stop Orinoco card FIXME\n");
	}
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
    int NumTries;
    int Status;

    TR();

    if (!Running) {
	
	/* The card is inserted at this point */
	Status = hcf_action(pIFB, HCF_ACT_CARD_IN);
	if (Status == HCF_SUCCESS) {

	    /* Configure parameters of card.  This can be done without the card inserted. */
	    Status = PutParameters(pIFB);
	    if (Status == HCF_SUCCESS) {
	
		/* Turn on interrupts */
		Status = hcf_action(pIFB, HCF_ACT_INT_ON);
		if (Status == HCF_SUCCESS) {
	
		    /* Start the card - try this multiple times if it fails */
		    NumTries = 0;
		    do {
			Status = hcf_enable(pIFB, HCF_PORT_0);
		    } while (Status != HCF_SUCCESS && ++NumTries < 10);
		    if (Status == HCF_SUCCESS) {
			Running = true;
		    }
		    else {
			diag_printf("FIXME Could not enable card: %d FIXME\n", __FUNCTION__, Status);
		    }
		}
		else {
		    /* This error is ignored by the DOS Driver reference code */
		    diag_printf("%s status %d\n", __FUNCTION__, Status);
		}
	    }
	    else {
		/* This error is ignored by the DOS driver reference code */
		diag_printf("%s status %d\n", __FUNCTION__, Status);
	    }
	}
	else {
	    /* This error is ignored by the DOS driver reference code */
	    diag_printf("%s status %d\n", __FUNCTION__, Status);
	}
    }
}

static int
OrinocoControl(struct eth_drv_sc * SC, unsigned long Key,
	       void * Data, int DataLength) 
{
    OrinocoPrivateData_T * PrivateData = (OrinocoPrivateData_T *)SC->driver_private;
    IFBP pIFB = &PrivateData->IFB;
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
	    ScanResult_T * ScanResult;
	    int i;

#if defined(INFRASTRUCTURE_MODE)
	    /* Don't do this when in infrastructure mode */
	    /* TODO Figure out another way to check link status */
	    EthStats->operational = 3;
#else
	    /* 1 = UNKNOWN, 2 = DOWN, 3 = UP */
	    /* Start out with UNKNOWN until we find another node */
	    EthStats->operational = 1;
	    
	    if (Running) {
	    
		/* Start a network scan to look for other nodes */
		hcf_action(pIFB, HCF_ACT_SCAN);
		
		/* TODO If anybody else is waiting for info from the MailBox, this will
		   cause problems.  Right now no one is, so it's not a problem to discard
		   any unwanted info. */
		do {
		    Status = hcf_get_info(pIFB, FillLTV(pLTVB, LTV_BUF_SIZE * 2, CFG_MB_INFO, NULL));
		} while (Status == HCF_SUCCESS && pLTVB->typ != CFG_NULL && pLTVB->typ != CFG_SCAN);
		
		/* Check that we got the info we wanted */
		if (Status == HCF_SUCCESS) {
		    
		    /* Now see if there are any other other hosts that match our channel and SSID */
		    for (i = 0; i < (pLTVB->len - 1 /* the len field */); i += sizeof(ScanResult_T) / sizeof(hcf_16)) {
			ScanResult = (ScanResult_T *)(&pLTVB->val[0]);
			diag_printf("Channel: %d MAC: %04x %04x %04x SSID %s\n", ScanResult->ChannelID,
				    ScanResult->MACAddress[0], ScanResult->MACAddress[1], ScanResult->MACAddress[2],
				    ScanResult->SSIDValue);
			if (ScanResult->ChannelID == CYGDAT_ARM_DHARMA_SET_CHANNEL &&
			    strcmp((char *)ScanResult->SSIDValue, CYGDAT_ARM_DHARMA_SET_SSID) == 0) {
			    
			    /* Link is up */
			    EthStats->operational = 3;
			    diag_printf("Matching node found\n");
			}
		    }
		}
	    }
#endif
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
    
    TR();
    
    return 1;
}

static void
OrinocoSend(struct eth_drv_sc * SC, struct eth_drv_sg * SGList, int SGLength,
	    int TotalLength, unsigned long Key) 
{
    /* The packet buffer is needed to work around a bug in the orinoco card.  Basically data
       corruption will result when an odd number of bytes is written.  So by saving all the
       writes into one buffer incrementing to the next even size, we can avoid the problem. */
    static unsigned char PacketBuffer[1514]; /* This is the maximum size for an 802.3 frame */
    OrinocoPrivateData_T * PrivateData = (OrinocoPrivateData_T *)SC->driver_private;
    IFBP pIFB = &PrivateData->IFB;
    int Length;
    int i;

    TR();
    
    hcf_action(pIFB, HCF_ACT_INT_OFF);

    /* Wait until a buffer is free on the card */
    while (pIFB->IFB_PIFRscInd == 0) {
	hcf_service_nic(pIFB);
	if (pIFB->IFB_RxLen != 0) {
	    /* The theory here is that since interrupts are masked, if this case does occur,
	       it will still get handled when interrupts are unmasked.  So maybe no need for
	       the FIXME. */
	    diag_printf("FIXME May need to fake an interrupt FIXME\n");
	}
    }
    
    /* Copy SG list into local buffer */
    Length = 0;
    for (i = 0; i < SGLength; ++i) {
	memcpy(&PacketBuffer[Length], (unsigned char *)SGList[i].buf, SGList[i].len);
	Length += SGList[i].len;
    }
    /* Evenize the length */
    if (Length & 0x1) {
	++Length;
    }
    if (Length > sizeof(PacketBuffer)) {
	diag_printf("FIXME %s too much data FIXME\n", __FUNCTION__);
    }
    hcf_put_data(pIFB, PacketBuffer, Length, HCF_PORT_0);
    hcf_send(pIFB, HCF_PORT_0);
    hcf_action(pIFB, HCF_ACT_INT_ON);
    
    /* Buffer is free */
    /* See if there is a way to tell when the transmission is done so that
       code flow can be closer to cs8900.
       Answer is Nope (Would be HREG_EV_TX, but hcf doesn't use that) */
    (SC->funs->eth_drv->tx_done)(SC, Key, 0);
}

static void
OrinocoInt(struct eth_drv_sc * SC)
{
    OrinocoPrivateData_T * PrivateData = (OrinocoPrivateData_T *)SC->driver_private;
    IFBP pIFB = &PrivateData->IFB;
    int Status;

    TR();
    
#if defined(CYGIMP_KERNEL_INTERRUPTS_CHAIN)
    for (;;) {
	
	/* hcf_service_nic has already been called from the ISR, but nothing has been
	   done about it yet. */
	if (pIFB->IFB_RxLen != 0) {
	    ReceivePacket(SC);
	}
	else {
	    break;
	}

	Status = hcf_service_nic(pIFB);
    }
#else
    for (;;) {
	
	/* Status is only saved for debugging purposes.  The documentation says as much, so
	   we can't really use it for anything. */
	Status = hcf_service_nic(pIFB);

#if 0
	/* Would like to have a semaphore here, but it doesn't appear as though we get
	   interrupts solely on this.  In other words, we need to be rx/tx'ng to get
	   here. */
	if (pIFB->MBInfoLen != 0) {
	}
#endif
	
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
    return byte;
}

hcf_16
in_port_word(unsigned int port) 
{
    hcf_16 word = *((volatile hcf_16 *)port);
    return word;
}

void
out_port_byte(unsigned int port, hcf_8 value) 
{
    *((volatile hcf_8 *)port) = value;
}

void
out_port_word(unsigned int port, hcf_16 value) 
{
    *((volatile hcf_16 *)port) = value;
}
