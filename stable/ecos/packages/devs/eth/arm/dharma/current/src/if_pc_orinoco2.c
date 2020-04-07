#include <cyg/hal/drv_api.h>
#include <cyg/hal/hal_diag.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/io/pcmcia.h>
#include <netdev.h>
#include <eth_drv.h>
/* TODO Move mmgpio functions into kernel */
#include "../../../../../../../../player/devs/mmgpio/include/mmgpio.h"
#include "hcf.h"

#define TR() /* diag_printf("%s:%d\n", __FUNCTION__, __LINE__) */

/* CIS definitions */
#define ORINOCO_MANUF 0x0156
#define LAN_NID 0x04

#define ETHER_ADDR_LEN 6
#define MAX_SSID_LEN 32

#define FALSE 0
#define TRUE (!FALSE)

#define ORINOCO_INT_NUM CYGNUM_HAL_INTERRUPT_EINT1 /* TODO Set these to XINT when that is working */
#define ORINOCO_INT_PRIORITY 1 /* TODO Adjust this parameter */
#define ORINOCO_ATTRIBUTE_BASE 0x32000000
#define ORINOCO_IO_BASE 0x32000000

#ifdef CYGPKG_NET
#define STACK_SIZE (32 * 1024) //CYGNUM_HAL_STACK_SIZE_TYPICAL
static char OrinocoCardHandlerStack[STACK_SIZE];
static cyg_thread OrinocoCardHandlerThreadData;
static cyg_handle_t OrinocoCardHandlerThreadHandle;
#endif  // CYGPKG_NET

static void
do_delay(int ticks)
{
#ifdef CYGPKG_KERNEL
    cyg_thread_delay(ticks);
#else
    CYGACC_CALL_IF_DELAY_US(10000*ticks);
#endif
}

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

static void
OrinocoDSR(cyg_vector_t Vector, cyg_ucount32 Count, cyg_addrword_t Data) 
{
#if defined(CYGIMP_KERNEL_INTERRUPTS_CHAIN)
#else
    hcf_action(pIFB, HCF_ACT_INT_OFF);
    eth_drv_dsr(Vector, Count, Data);
#endif
}

static void
OrinocoDeliver(struct eth_drv_sc * SC)
{
#if defined(CYGIMP_KERNEL_INTERRUPTS_CHAIN)
#else
    OrinocoInt(SC);
    hcf_action(pIFB, HCF_ACT_INT_ON);
#endif
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
		/* TODO Look up Orinoco manufacturer code */
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
                diag_printf("%s: %s %s %s\n", Manufacturer, Product, Revision, Date);
#endif
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

		    /* TODO This is the part that breaks the abstraction */
		    /* Put interface into I/O mode */
		    SetMMGPO(0, (MMGPO_PCMCIA_REG | MMGPO_PCMCIA_8_16_SELECT | MMGPO_PCMCIA_IO_SELECT));

		    /* Unmask the interrupt now that it is safe to do so
		       TODO This part breaks the abstraction - Bob is looking into fixing this */
		    cyg_drv_interrupt_unmask(CYGNUM_HAL_INTERRUPT_EINT1);
		    
		    /* Configure bus to 16bit databus, 8 wait states, EXPRDY.
		       Card is found at nCS3 | A25 */
		    *(volatile cyg_uint32 *)MEMCFG1 &= ~0xff000000;
		    /* 1 wait state + EXPRDY + EXCKEN I would think would work, but nope.
		       Max wait states + EXPRDY doesn't work either, but catches some of the bytes. */
		    *(volatile cyg_uint32 *)MEMCFG1 |= 0x81000000 | ((0<<2)<<24);
		     /* This doesn't seem to do anything, but I will play it safe */
		    *(volatile cyg_uint32 *)SYSCON1 |= SYSCON1_EXCKEN;

		    /* Initialize host control library */
		    hcf_connect(pIFB, (hcf_io)PrivateData->Slot->io);

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
            do_delay(50);  // FIXME!
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
    cf_register_handler(PrivateData->Slot, OrinocoDSR, SC);

    return false;  // Device is not ready until inserted, powered up, etc.
#else
    // Initialize card
    return OrinocoCardHandler(SC);
#endif    
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
	
	/* The card must be inserted at this point - TODO Is this still true? */
	Status = hcf_action(pIFB, HCF_ACT_CARD_IN);
	if (Status == HCF_SUCCESS) {

	    /* Configure parameters of card.  This can be done without the card inserted. */
	    Status = PutParameters(pIFB);
	    if (Status == HCF_SUCCESS) {
	
		/* Turn on interrupts */
		hcf_action(pIFB, HCF_ACT_INT_ON);
	
		/* Start the card */
		Status = hcf_enable(pIFB, HCF_PORT_0);
		if (Status == HCF_SUCCESS) {
		    Running = true;
		}
		else {
		    diag_printf("FIXME %s status %d FIXME\n", __FUNCTION__, Status);
		}
	    }
	    else {
		diag_printf("FIXME %s status %d FIXME\n", __FUNCTION__, Status);
	    }
	}
	else {
	    diag_printf("FIXME %s status %d FIXME\n", __FUNCTION__, Status);
	}
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
    
    if (pIFB->IFB_PIFRscInd == 0) {
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
