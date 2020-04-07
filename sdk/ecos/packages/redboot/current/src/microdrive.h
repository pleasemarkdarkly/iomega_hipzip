#ifndef _MICRODRIVE_H_
#define _MICRODRIVE_H_
//==========================================================================
//
//      microdrive.h
//
//      RedBoot - Microdrive driver
//
//==========================================================================
//####COPYRIGHTBEGIN####
//                                                                          
// -------------------------------------------                              
// -------------------------------------------                              
//                                                                          
//####COPYRIGHTEND####
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    toddm
// Contributors: toddm
// Date:         2001-03-16
// Purpose:      
// Description:  
//              
// This code is part of RedBoot (tm).
//
//####DESCRIPTIONEND####
//
//==========================================================================

#define DEV_BSIZE 512

#define ENOERR 0
#define EIO    5
#define EAGAIN 11
#define ETIME  605

/*
 * Drive parameter structure for ATA/ATAPI.
 * Bit fields: WDC_* : common to ATA/ATAPI
 *             ATA_* : ATA only
 *             ATAPI_* : ATAPI only.
 */
typedef struct {
    /* drive info */
    cyg_uint16	atap_config;		/* 0: general configuration */
#define WDC_CFG_ATAPI_MASK    	0xc000
#define WDC_CFG_ATAPI    	0x8000
#define	ATA_CFG_REMOVABLE	0x0080
#define	ATA_CFG_FIXED		0x0040
#define ATAPI_CFG_TYPE_MASK	0x1f00
#define ATAPI_CFG_TYPE(x) (((x) & ATAPI_CFG_TYPE_MASK) >> 8)
#define	ATAPI_CFG_REMOV		0x0080
#define ATAPI_CFG_DRQ_MASK	0x0060
#define ATAPI_CFG_STD_DRQ	0x0000
#define ATAPI_CFG_IRQ_DRQ	0x0020
#define ATAPI_CFG_ACCEL_DRQ	0x0040
#define ATAPI_CFG_CMD_MASK	0x0003
#define ATAPI_CFG_CMD_12	0x0000
#define ATAPI_CFG_CMD_16	0x0001
/* words 1-9 are ATA only */
    cyg_uint16	atap_cylinders;		/* 1: # of non-removable cylinders */
    cyg_uint16	__reserved1;
    cyg_uint16	atap_heads;		/* 3: # of heads */
    cyg_uint16	__retired1[2];		/* 4-5: # of unform. bytes/track */
    cyg_uint16	atap_sectors;		/* 6: # of sectors */
    cyg_uint16	__retired2[3];

    cyg_uint8	atap_serial[20];	/* 10-19: serial number */
    cyg_uint16	__retired3[2];
    cyg_uint16	__obsolete1;
    cyg_uint8	atap_revision[8];	/* 23-26: firmware revision */
    cyg_uint8	atap_model[40];		/* 27-46: model number */
    cyg_uint16	atap_multi;		/* 47: maximum sectors per irq (ATA) */
    cyg_uint16	__reserved2;
    cyg_uint16	atap_capabilities1;	/* 49: capability flags */
#define WDC_CAP_IORDY	0x0800
#define WDC_CAP_IORDY_DSBL 0x0400
#define	WDC_CAP_LBA	0x0200
#define	WDC_CAP_DMA	0x0100
#define ATA_CAP_STBY	0x2000
#define ATAPI_CAP_INTERL_DMA	0x8000
#define ATAPI_CAP_CMD_QUEUE	0x4000
#define	ATAPI_CAP_OVERLP	0X2000
#define ATAPI_CAP_ATA_RST	0x1000
    cyg_uint16	atap_capabilities2;	/* 50: capability flags (ATA) */
    cyg_uint8	__junk2;
    cyg_uint8	atap_oldpiotiming;	/* 51: old PIO timing mode */
    cyg_uint8	__junk3;
    cyg_uint8	atap_olddmatiming;	/* 52: old DMA timing mode (ATA) */
    cyg_uint16	atap_extensions;	/* 53: extentions supported */
#define WDC_EXT_UDMA_MODES	0x0004
#define WDC_EXT_MODES		0x0002
#define WDC_EXT_GEOM		0x0001
/* words 54-62 are ATA only */
    cyg_uint16	atap_curcylinders;	/* 54: current logical cyliners */
    cyg_uint16	atap_curheads;		/* 55: current logical heads */
    cyg_uint16	atap_cursectors;	/* 56: current logical sectors/tracks */
    cyg_uint16	atap_curcapacity[2];	/* 57-58: current capacity */
    cyg_uint16	atap_curmulti;		/* 59: current multi-sector setting */
#define WDC_MULTI_VALID 0x0100
#define WDC_MULTI_MASK  0x00ff
    cyg_uint16	atap_capacity[2];  	/* 60-61: total capacity (LBA only) */
    cyg_uint16	__retired4;
    cyg_uint8	atap_dmamode_supp; 	/* 63: multiword DMA mode supported */
    cyg_uint8	atap_dmamode_act; 	/*     multiword DMA mode active */
    cyg_uint8	atap_piomode_supp;       /* 64: PIO mode supported */
    cyg_uint8	__junk4;
    cyg_uint16	atap_dmatiming_mimi;	/* 65: minimum DMA cycle time */
    cyg_uint16	atap_dmatiming_recom;	/* 66: recomended DMA cycle time */
    cyg_uint16	atap_piotiming;    	/* 67: mini PIO cycle time without FC */
    cyg_uint16	atap_piotiming_iordy;	/* 68: mini PIO cycle time with IORDY FC */
    cyg_uint16	__reserved3[2];
/* words 71-72 are ATAPI only */
    cyg_uint16	atap_pkt_br;		/* 71: time (ns) to bus release */
    cyg_uint16	atap_pkt_bsyclr;	/* 72: tme to clear BSY after service */
    cyg_uint16	__reserved4[2];	
    cyg_uint16	atap_queuedepth;   	/* 75: */
#define WDC_QUEUE_DEPTH_MASK 0x0F
    cyg_uint16	__reserved5[4];   	
    cyg_uint16	atap_ata_major;  	/* 80: Major version number */
#define	WDC_VER_ATA1	0x0002
#define	WDC_VER_ATA2	0x0004
#define	WDC_VER_ATA3	0x0008
#define	WDC_VER_ATA4	0x0010
#define	WDC_VER_ATA5	0x0020
    cyg_uint16   atap_ata_minor;  	/* 81: Minor version number */
    cyg_uint16	atap_cmd_set1;    	/* 82: command set supported */
#define WDC_CMD1_NOP	0x4000
#define WDC_CMD1_RB	0x2000
#define WDC_CMD1_WB	0x1000
#define WDC_CMD1_HPA	0x0400
#define WDC_CMD1_DVRST	0x0200
#define WDC_CMD1_SRV	0x0100
#define WDC_CMD1_RLSE	0x0080
#define WDC_CMD1_AHEAD	0x0040
#define WDC_CMD1_CACHE	0x0020
#define WDC_CMD1_PKT	0x0010
#define WDC_CMD1_PM	0x0008
#define WDC_CMD1_REMOV	0x0004
#define WDC_CMD1_SEC	0x0002
#define WDC_CMD1_SMART	0x0001
    cyg_uint16	atap_cmd_set2;    	/* 83: command set supported */
#define WDC_CMD2_RMSN	0x0010
#define WDC_CMD2_DM	0x0001
#define ATA_CMD2_APM	0x0008
#define ATA_CMD2_CFA	0x0004
#define ATA_CMD2_RWQ	0x0002
    cyg_uint16	atap_cmd_ext;		/* 84: command/features supp. ext. */
    cyg_uint16	atap_cmd1_en;		/* 85: cmd/features enabled */
/* bits are the same as atap_cmd_set1 */
    cyg_uint16	atap_cmd2_en;		/* 86: cmd/features enabled */
/* bits are the same as atap_cmd_set2 */
    cyg_uint16	atap_cmd_def;		/* 87: cmd/features default */
    cyg_uint8	atap_udmamode_supp; 	/* 88: Ultra-DMA mode supported */
    cyg_uint8	atap_udmamode_act; 	/*     Ultra-DMA mode active */
/* 89-92 are ATA-only */
    cyg_uint16	atap_seu_time;		/* 89: Sec. Erase Unit compl. time */
    cyg_uint16	atap_eseu_time;		/* 90: Enhanced SEU compl. time */
    cyg_uint16	atap_apm_val;		/* 91: current APM value */
    cyg_uint16	__reserved6[35];	/* 92-126: reserved */
    cyg_uint16	atap_rmsn_supp;		/* 127: remov. media status notif. */
#define WDC_RMSN_SUPP_MASK 0x0003
#define WDC_RMSN_SUPP 0x0001
    cyg_uint16	atap_sec_st;		/* 128: security status */
#define WDC_SEC_LEV_MAX	0x0100
#define WDC_SEC_ESE_SUPP 0x0020
#define WDC_SEC_EXP	0x0010
#define WDC_SEC_FROZEN	0x0008
#define WDC_SEC_LOCKED	0x0004
#define WDC_SEC_EN	0x0002
#define WDC_SEC_SUPP	0x0001
}__attribute__((packed)) ataparams_t;

int ata_init(void);

int ata_get_params(ataparams_t * prms);
int ata_read(unsigned int lba, unsigned short len, unsigned char * data);
int ata_write(unsigned int lba, unsigned short len, const unsigned char * data);

#if 0

//
// Register definitions
//

/*
 * Disk Controller register definitions.
 */

/* offsets of registers in the 'regular' register region */
#define	ata_data	cmd_r0	/* data register (R/W - 16 bits) */
#define	ata_error	cmd_r1	/* error register (R) */
#define	ata_precomp	cmd_r1	/* write precompensation (W) */
#define	ata_features	cmd_r1	/* features (W), same as ata_precomp */
#define	ata_seccnt	cmd_r2	/* sector count (R/W) */
#define	ata_ireason	cmd_r2	/* interrupt reason (R/W) (for atapi) */
#define	ata_sector	cmd_r3	/* first sector number (R/W) */
#define	ata_cyl_lo	cmd_r4	/* cylinder address, low byte (R/W) */
#define	ata_cyl_hi	cmd_r5	/* cylinder address, high byte (R/W) */
#define	ata_sdh		cmd_r6	/* sector size/drive/head (R/W) */
#define	ata_command	cmd_r7	/* command register (W)	*/
#define	ata_status	cmd_r7	/* immediate status (R)	*/

/* offsets of registers in the auxiliary register region */
#define	ata_aux_altsts	ctl_r0	/* alternate fixed disk status (R) */
#define	ata_aux_ctlr	ctl_r0	/* fixed disk controller control (W) */
#define  ATACTL_4BIT	 0x08	/* use four head bits (wd1003) */
#define  ATACTL_RST	 0x04	/* reset the controller */
#define  ATACTL_IDS	 0x02	/* disable controller interrupts */

/*
 * Status bits.
 */
#define	ATAS_BSY	0x80	/* busy */
#define	ATAS_DRDY	0x40	/* drive ready */
#define	ATAS_DWF	0x20	/* drive write fault */
#define	ATAS_DSC	0x10	/* drive seek complete */
#define	ATAS_DRQ	0x08	/* data request */
#define	ATAS_CORR	0x04	/* corrected data */
#define	ATAS_IDX	0x02	/* index */
#define	ATAS_ERR	0x01	/* error */
#define ATAS_BITS	"\020\010bsy\007drdy\006dwf\005dsc\004drq\003corr\002idx\001err"

/*
 * Error bits.
 */
#define	ATAE_BBK	0x80	/* bad block detected */
#define	ATAE_CRC	0x80	/* CRC error (Ultra-DMA only) */
#define	ATAE_UNC	0x40	/* uncorrectable data error */
#define	ATAE_MC		0x20	/* media changed */
#define	ATAE_IDNF	0x10	/* id not found */
#define	ATAE_MCR	0x08	/* media change requested */
#define	ATAE_ABRT	0x04	/* aborted command */
#define	ATAE_TK0NF	0x02	/* track 0 not found */
#define	ATAE_AMNF	0x01	/* address mark not found */

/*
 * Commands for Disk Controller.
 */
#define ATAC_NOP	0x00	/* NOP - Always fail with "aborted command" */
#define	ATAC_RECAL	0x10	/* disk restore code -- resets cntlr */

#define	ATAC_READ	0x20	/* disk read code */
#define	ATAC_WRITE	0x30	/* disk write code */
#define	 ATAC__LONG	 0x02	 /* modifier -- access ecc bytes */
#define	 ATAC__NORETRY	 0x01	 /* modifier -- no retrys */

#define	ATAC_FORMAT	0x50	/* disk format code */
#define	ATAC_DIAGNOSE	0x90	/* controller diagnostic */
#define	ATAC_IDP	0x91	/* initialize drive parameters */

#define	ATAC_READMULTI	0xc4	/* read multiple */
#define	ATAC_WRITEMULTI	0xc5	/* write multiple */
#define	ATAC_SETMULTI	0xc6	/* set multiple mode */

#define	ATAC_READDMA	0xc8	/* read with DMA */
#define	ATAC_WRITEDMA	0xca	/* write with DMA */

#define	ATAC_ACKMC	0xdb	/* acknowledge media change */
#define	ATAC_LOCK	0xde	/* lock drawer */
#define	ATAC_UNLOCK	0xdf	/* unlock drawer */

#define	ATAC_FLUSHCACHE	0xe7	/* Flush cache */
#define	ATAC_IDENTIFY	0xec	/* read parameters from controller */
#define	SET_FEATURES	0xef	/* set features */

#define ATAC_IDLE	0xe3	/* set idle timer & enter idle mode */
#define ATAC_IDLE_IMMED	0xe1	/* enter idle mode */
#define ATAC_SLEEP	0xe6	/* enter sleep mode */
#define ATAC_STANDBY	0xe2	/* set standby timer & enter standby mode */
#define ATAC_STANDBY_IMMED 0xe0	/* enter standby mode */
#define ATAC_CHECK_PWR	0xe5	/* check power mode */

/* Subcommands for SET_FEATURES (features register ) */
#define ATASF_EN_WR_CACHE	0x02
#define ATASF_SET_MODE    	0x03
#define ATASF_REASSIGN_EN	0x04
#define ATASF_RETRY_DS		0x33
#define ATASF_SET_CACHE_SGMT	0x54
#define ATASF_READAHEAD_DS	0x55
#define ATASF_POD_DS		0x66
#define ATASF_ECC_DS		0x77
#define ATASF_WRITE_CACHE_DS	0x82
#define ATASF_REASSIGN_DS	0x84
#define ATASF_ECC_EN		0x88
#define ATASF_RETRY_EN		0x99
#define ATASF_SET_CURRENT	0x9A
#define ATASF_READAHEAD_EN	0xAA
#define ATASF_PREFETCH_SET	0xAB
#define ATASF_POD_EN             0xCC

/* parameters uploaded to device/heads register */
#define	ATASD_IBM	0xa0	/* forced to 512 byte sector, ecc */
#define	ATASD_CHS	0x00	/* cylinder/head/sector addressing */
#define	ATASD_LBA	0x40	/* logical block addressing */

//
// Microdrive specific defines
//
#define MD_CMD_TIMEOUT 7500	/* IBM recommand command timeout value, 7.5s */
#define DEV_BSIZE 512

#define MEM_MOD_MEMCFG_MASK 0x0000ff00

// Values to play with 
#define MEMCFG_BUS_WIDTH(n)   (n<<0)
#define MEMCFG_BUS_WIDTH_32   (0<<0)
#define MEMCFG_BUS_WIDTH_16   (1<<0)
#define MEMCFG_BUS_WIDTH_8    (2<<0)
#define MEMCFG_WAIT_STATES(n) (n<<2)
#define MEMCFG_SQAEN          (1<<6)
#define MEMCFG_CLKENB         (1<<7)

#define WAIT_STATES 8

#define MEM_MOD_8BIT_MEMCFG ((MEMCFG_CLKENB|MEMCFG_WAIT_STATES((8-WAIT_STATES))|MEMCFG_BUS_WIDTH_8)<<8)
#define MEM_MOD_16BIT_MEMCFG ((MEMCFG_CLKENB|MEMCFG_WAIT_STATES((8-WAIT_STATES))|MEMCFG_BUS_WIDTH_16)<<8)

//
// Controller and drive data structures
//

typedef struct {
    cyg_uint8 drive; /* drive number */
    //cyg_int8 ata_vers; /* ATA version supported */
    cyg_uint16 drive_flags; /* bitmask for drives present/absent and cap */
#define DRIVE_ATA	0x0001
#define DRIVE_ATAPI	0x0002
#define DRIVE_OLD	0x0004 
#define DRIVE (DRIVE_ATA|DRIVE_ATAPI|DRIVE_OLD)
#define DRIVE_CAP32	0x0008
#define DRIVE_DMA	0x0010 
#define DRIVE_UDMA	0x0020
#define DRIVE_MODE	0x0040 /* the drive reported its mode */
#define DRIVE_RESET	0x0080 /* reset the drive state at next xfer */
#define DRIVE_DMAERR	0x0100 /* Udma transfer had crc error, don't try DMA */
    /*
     * Current setting of drive's PIO, DMA and UDMA modes.
     * Is initialised by the disks drivers at attach time, and may be
     * changed later by the controller's code if needed
     */
    //cyg_uint8 PIO_mode; /* Current setting of drive's PIO mode */
    //cyg_uint8 DMA_mode; /* Current setting of drive's DMA mode */
    //cyg_uint8 UDMA_mode; /* Current setting of drive's UDMA mode */
    /* Supported modes for this drive */
    //cyg_uint8 PIO_cap; /* supported drive's PIO mode */
    //cyg_uint8 DMA_cap; /* supported drive's DMA mode */
    //cyg_uint8 UDMA_cap; /* supported drive's UDMA mode */
    /*
     * Drive state.
     * This is reset to 0 after a channel reset.
     */
    cyg_uint8 state;
#define RESET          0
#define RECAL          1
#define RECAL_WAIT     2
#define PIOMODE        3
#define PIOMODE_WAIT   4
#define DMAMODE        5
#define DMAMODE_WAIT   6
#define GEOMETRY       7
#define GEOMETRY_WAIT  8
#define MULTIMODE      9
#define MULTIMODE_WAIT 10
#define READY          11

    /* numbers of xfers and DMA errs. Used by ata_dmaerr() */
    //cyg_uint8 n_dmaerrs;
    //cyg_uint32 n_xfers;
    /* Downgrade after NERRS_MAX errors in at most NXFER xfers */
#define NERRS_MAX 4
#define NXFER 4000

    //struct device *drv_softc; /* ATA drives softc, if any */
    void* chnl_softc; /* channel softc */
} ata_drive_datas_t;

typedef struct { /* Per channel data */
	/* Our timeout callout */
    //struct callout ch_callout;
	/* Our location */
	int channel;
	/* Our controller's softc */
    //struct wdc_softc *wdc;
	/* Our registers */
    //bus_space_tag_t       cmd_iot;
	void *    cmd_ioh;
    //bus_space_tag_t       ctl_iot;
        void *    ctl_ioh;
	/* data32{iot,ioh} are only used for 32 bit xfers */
    //bus_space_tag_t         data32iot;
    //bus_space_handle_t      data32ioh;

    /* Register offsets */
    int cmd_r0;
    int cmd_r1;
    int cmd_r2;
    int cmd_r3;
    int cmd_r4;
    int cmd_r5;
    int cmd_r6;
    int cmd_r7;
    int ctl_r0;
	/* Our state */
	int ch_flags;
#define ATACF_ACTIVE   0x01	/* channel is active */
#define ATACF_IRQ_WAIT 0x10	/* controller is waiting for irq */
#define ATACF_DMA_WAIT 0x20	/* controller is waiting for DMA */
	cyg_uint8 ch_status;         /* copy of status register */
	cyg_uint8 ch_error;          /* copy of error register */
	/* per-drive infos */
    ata_drive_datas_t ch_drive[1];// only support 1 device on a channel

    //struct device *atapibus;

	/*
	 * channel queues. May be the same for all channels, if hw channels
	 * are not independants
	 */
    //struct channel_queue *ch_queue;
} channel_softc_t;

typedef struct {
    cyg_uint8 drive;
    cyg_uint8 r_command;  /* Parameters to upload to registers */
    cyg_uint8 r_head;
    cyg_uint16 r_cyl;
    cyg_uint8 r_sector;
    cyg_uint8 r_count;
    cyg_uint8 r_features;
    cyg_uint8 r_st_bmask; /* status register mask to wait for before command */
    cyg_uint8 r_st_pmask; /* status register mask to wait for after command */
    cyg_uint8 r_error;    /* error register after command done */
    volatile cyg_uint16 flags;
#define AT_READ     0x0001 /* There is data to read */
#define AT_WRITE    0x0002 /* There is data to write (excl. with AT_READ) */
#define AT_WAIT     0x0008 /* wait in controller code for command completion */
#define AT_POLL     0x0010 /* poll for command completion (no interrupts) */
#define AT_DONE     0x0020 /* command is done */
#define AT_ERROR    0x0040 /* command is done with error */
#define AT_TIMEOU   0x0080 /* command timed out */
#define AT_DF       0x0100 /* Drive fault */
#define AT_READREG  0x0200 /* Read registers on completion */
    int timeout;	 /* timeout (in ms) */
    void *data;          /* Data buffer address */
    int bcount;           /* number of bytes to transfer */
    //void (*callback) __P((void*)); /* command to call once command completed */
    //void *callback_arg;  /* argument passed to *callback() */
} ata_command_t;

/*
 * Drive parameter structure for ATA/ATAPI.
 * Bit fields: WDC_* : common to ATA/ATAPI
 *             ATA_* : ATA only
 *             ATAPI_* : ATAPI only.
 */
typedef struct {
    /* drive info */
    cyg_uint16	atap_config;		/* 0: general configuration */
#define WDC_CFG_ATAPI_MASK    	0xc000
#define WDC_CFG_ATAPI    	0x8000
#define	ATA_CFG_REMOVABLE	0x0080
#define	ATA_CFG_FIXED		0x0040
#define ATAPI_CFG_TYPE_MASK	0x1f00
#define ATAPI_CFG_TYPE(x) (((x) & ATAPI_CFG_TYPE_MASK) >> 8)
#define	ATAPI_CFG_REMOV		0x0080
#define ATAPI_CFG_DRQ_MASK	0x0060
#define ATAPI_CFG_STD_DRQ	0x0000
#define ATAPI_CFG_IRQ_DRQ	0x0020
#define ATAPI_CFG_ACCEL_DRQ	0x0040
#define ATAPI_CFG_CMD_MASK	0x0003
#define ATAPI_CFG_CMD_12	0x0000
#define ATAPI_CFG_CMD_16	0x0001
/* words 1-9 are ATA only */
    cyg_uint16	atap_cylinders;		/* 1: # of non-removable cylinders */
    cyg_uint16	__reserved1;
    cyg_uint16	atap_heads;		/* 3: # of heads */
    cyg_uint16	__retired1[2];		/* 4-5: # of unform. bytes/track */
    cyg_uint16	atap_sectors;		/* 6: # of sectors */
    cyg_uint16	__retired2[3];

    cyg_uint8	atap_serial[20];	/* 10-19: serial number */
    cyg_uint16	__retired3[2];
    cyg_uint16	__obsolete1;
    cyg_uint8	atap_revision[8];	/* 23-26: firmware revision */
    cyg_uint8	atap_model[40];		/* 27-46: model number */
    cyg_uint16	atap_multi;		/* 47: maximum sectors per irq (ATA) */
    cyg_uint16	__reserved2;
    cyg_uint16	atap_capabilities1;	/* 49: capability flags */
#define WDC_CAP_IORDY	0x0800
#define WDC_CAP_IORDY_DSBL 0x0400
#define	WDC_CAP_LBA	0x0200
#define	WDC_CAP_DMA	0x0100
#define ATA_CAP_STBY	0x2000
#define ATAPI_CAP_INTERL_DMA	0x8000
#define ATAPI_CAP_CMD_QUEUE	0x4000
#define	ATAPI_CAP_OVERLP	0X2000
#define ATAPI_CAP_ATA_RST	0x1000
    cyg_uint16	atap_capabilities2;	/* 50: capability flags (ATA) */
    cyg_uint8	__junk2;
    cyg_uint8	atap_oldpiotiming;	/* 51: old PIO timing mode */
    cyg_uint8	__junk3;
    cyg_uint8	atap_olddmatiming;	/* 52: old DMA timing mode (ATA) */
    cyg_uint16	atap_extensions;	/* 53: extentions supported */
#define WDC_EXT_UDMA_MODES	0x0004
#define WDC_EXT_MODES		0x0002
#define WDC_EXT_GEOM		0x0001
/* words 54-62 are ATA only */
    cyg_uint16	atap_curcylinders;	/* 54: current logical cyliners */
    cyg_uint16	atap_curheads;		/* 55: current logical heads */
    cyg_uint16	atap_cursectors;	/* 56: current logical sectors/tracks */
    cyg_uint16	atap_curcapacity[2];	/* 57-58: current capacity */
    cyg_uint16	atap_curmulti;		/* 59: current multi-sector setting */
#define WDC_MULTI_VALID 0x0100
#define WDC_MULTI_MASK  0x00ff
    cyg_uint16	atap_capacity[2];  	/* 60-61: total capacity (LBA only) */
    cyg_uint16	__retired4;
    cyg_uint8	atap_dmamode_supp; 	/* 63: multiword DMA mode supported */
    cyg_uint8	atap_dmamode_act; 	/*     multiword DMA mode active */
    cyg_uint8	atap_piomode_supp;       /* 64: PIO mode supported */
    cyg_uint8	__junk4;
    cyg_uint16	atap_dmatiming_mimi;	/* 65: minimum DMA cycle time */
    cyg_uint16	atap_dmatiming_recom;	/* 66: recomended DMA cycle time */
    cyg_uint16	atap_piotiming;    	/* 67: mini PIO cycle time without FC */
    cyg_uint16	atap_piotiming_iordy;	/* 68: mini PIO cycle time with IORDY FC */
    cyg_uint16	__reserved3[2];
/* words 71-72 are ATAPI only */
    cyg_uint16	atap_pkt_br;		/* 71: time (ns) to bus release */
    cyg_uint16	atap_pkt_bsyclr;	/* 72: tme to clear BSY after service */
    cyg_uint16	__reserved4[2];	
    cyg_uint16	atap_queuedepth;   	/* 75: */
#define WDC_QUEUE_DEPTH_MASK 0x0F
    cyg_uint16	__reserved5[4];   	
    cyg_uint16	atap_ata_major;  	/* 80: Major version number */
#define	WDC_VER_ATA1	0x0002
#define	WDC_VER_ATA2	0x0004
#define	WDC_VER_ATA3	0x0008
#define	WDC_VER_ATA4	0x0010
#define	WDC_VER_ATA5	0x0020
    cyg_uint16   atap_ata_minor;  	/* 81: Minor version number */
    cyg_uint16	atap_cmd_set1;    	/* 82: command set supported */
#define WDC_CMD1_NOP	0x4000
#define WDC_CMD1_RB	0x2000
#define WDC_CMD1_WB	0x1000
#define WDC_CMD1_HPA	0x0400
#define WDC_CMD1_DVRST	0x0200
#define WDC_CMD1_SRV	0x0100
#define WDC_CMD1_RLSE	0x0080
#define WDC_CMD1_AHEAD	0x0040
#define WDC_CMD1_CACHE	0x0020
#define WDC_CMD1_PKT	0x0010
#define WDC_CMD1_PM	0x0008
#define WDC_CMD1_REMOV	0x0004
#define WDC_CMD1_SEC	0x0002
#define WDC_CMD1_SMART	0x0001
    cyg_uint16	atap_cmd_set2;    	/* 83: command set supported */
#define WDC_CMD2_RMSN	0x0010
#define WDC_CMD2_DM	0x0001
#define ATA_CMD2_APM	0x0008
#define ATA_CMD2_CFA	0x0004
#define ATA_CMD2_RWQ	0x0002
    cyg_uint16	atap_cmd_ext;		/* 84: command/features supp. ext. */
    cyg_uint16	atap_cmd1_en;		/* 85: cmd/features enabled */
/* bits are the same as atap_cmd_set1 */
    cyg_uint16	atap_cmd2_en;		/* 86: cmd/features enabled */
/* bits are the same as atap_cmd_set2 */
    cyg_uint16	atap_cmd_def;		/* 87: cmd/features default */
    cyg_uint8	atap_udmamode_supp; 	/* 88: Ultra-DMA mode supported */
    cyg_uint8	atap_udmamode_act; 	/*     Ultra-DMA mode active */
/* 89-92 are ATA-only */
    cyg_uint16	atap_seu_time;		/* 89: Sec. Erase Unit compl. time */
    cyg_uint16	atap_eseu_time;		/* 90: Enhanced SEU compl. time */
    cyg_uint16	atap_apm_val;		/* 91: current APM value */
    cyg_uint16	__reserved6[35];	/* 92-126: reserved */
    cyg_uint16	atap_rmsn_supp;		/* 127: remov. media status notif. */
#define WDC_RMSN_SUPP_MASK 0x0003
#define WDC_RMSN_SUPP 0x0001
    cyg_uint16	atap_sec_st;		/* 128: security status */
#define WDC_SEC_LEV_MAX	0x0100
#define WDC_SEC_ESE_SUPP 0x0020
#define WDC_SEC_EXP	0x0010
#define WDC_SEC_FROZEN	0x0008
#define WDC_SEC_LOCKED	0x0004
#define WDC_SEC_EN	0x0002
#define WDC_SEC_SUPP	0x0001
} ataparams_t;

//
// Function prototypes
//

ata_drive_datas_t * ata_init(void);

int ata_get_params(ata_drive_datas_t * drvp, cyg_uint8 flags, ataparams_t * prms);
int ata_read(ata_drive_datas_t * drvp, unsigned int lba, unsigned short len, unsigned char * data);
int ata_write(ata_drive_datas_t * drvp, unsigned int lba, unsigned short len, const unsigned char * data);
#define CMD_OK    0
#define CMD_ERR   1
#define CMD_AGAIN 2

int ata_hard_reset(channel_softc_t * chp);
int ata_exec_command(ata_drive_datas_t * drvp, ata_command_t * ata_c);
#define ATA_COMPLETE 0x01
#define ATA_QUEUED   0x02

int ataprobe(channel_softc_t * chp);
void atacommand(channel_softc_t * chp, int drive, cyg_uint8 command, cyg_uint16 cylin,
		cyg_uint8 head, cyg_uint8 sector, cyg_uint8 count, cyg_uint8 features);
int atawait(channel_softc_t * chp, int mask, int bits, int timeout);
int __atawait_reset(channel_softc_t * chp, int drv_mask);
void __atacommand_start(channel_softc_t * chp, ata_command_t * ata_c);
int __atacommand_intr(channel_softc_t * chp, ata_command_t * ata_c, int irq);
void __atacommand_done(channel_softc_t * chp, ata_command_t * ata_c);

cyg_uint8 bus_space_read_1(volatile cyg_uint8 * ioh, int off);
void bus_space_write_1(volatile cyg_uint8 * ioh, int off, cyg_uint8 val);
void bus_space_read_multi_2(volatile cyg_uint16 * ioh, int off, cyg_uint16 * data, int count);
void bus_space_write_multi_2(volatile cyg_uint16 * ioh, int off, cyg_uint16 * data, int count);

#endif

#endif // _MICRODRIVE_H_
