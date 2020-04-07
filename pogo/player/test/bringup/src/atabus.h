#ifndef ATABUS_H
#define ATABUS_H

#include <cyg/io/io.h>
#include <devs/storage/ata/atadrv.h>
#include "ataproto.h"


#define MEMCFG_BUS_WIDTH(n)   ((n)<<0)
/* These are valid only for 32 bit configuration */
#define MEMCFG_BUS_WIDTH_32   (0<<0)
#define MEMCFG_BUS_WIDTH_16   (1<<0)
#define MEMCFG_BUS_WIDTH_8    (2<<0)
#if 0 /* These are for 16 bit configuration */
#define MEMCFG_BUS_WIDTH_16   (0<<0)
#define MEMCFG_BUS_WIDTH_32   (1<<0)
#define MEMCFG_BUS_WIDTH_8    (3<<0)
#endif
#define MEMCFG_WAIT_STATES(n) ((n)<<2)
#define MEMCFG_SQAEN(n)       ((n)<<6)   // where n is 0 or 1
#define MEMCFG_CLKENB(n)      ((n)<<7)   // where n is 0 or 1

struct BusSpace_S
{
    /* Function to initialize bus controller */
    int  (*Init)( BusSpace_T* bus );
    void (*Reset)( BusSpace_T* bus );
    void (*HardReset)( BusSpace_T* bus );
    void (*PowerDown)( BusSpace_T* bus );
    void (*PowerUp)( BusSpace_T* bus );
    
    /* Functions to access bus */
    void (*Write16)( BusSpace_T* bus, unsigned int Offset, unsigned short Value);
    void (*Write16Multiple)( BusSpace_T* bus, unsigned int Offset, unsigned short * Buf, int Length);
    void (*Read16Multiple)( BusSpace_T* bus, unsigned int Offset, unsigned short * Buf, int Length);
    
    /* Base address of bus */
    unsigned int Base;
    
    /* Offsets to individual registers */
    unsigned int DataReg;
    unsigned int ErrorReg;
#define FeaturesReg ErrorReg
    unsigned int SectorCountReg;
#define IRQReasonReg SectorCountReg
    unsigned int SectorNumberReg;
    unsigned int CylinderLowReg;
    unsigned int CylinderHighReg;
    unsigned int DeviceHeadReg;
    unsigned int CommandReg;
#define StatusReg CommandReg
    unsigned int AlternateStatusReg;
#define ControlReg AlternateStatusReg
    
#ifndef DADIO_BOOT /* No iterrupts or threads in Redboot */
    /* Interrupt line for bus */
    int IRQ;
    cyg_handle_t IRQHandle;
    cyg_interrupt IRQObject;
    cyg_uint32 (*ISR)(cyg_vector_t Vector, cyg_addrword_t Data);
    void (*DSR)(cyg_vector_t Vector, cyg_ucount32 Count, cyg_addrword_t Data);
    cyg_sem_t IRQSem;

    /* User defined callback for media change events */
    void (*MediaChangeCB)(int Bus, int Drive, Cyg_ErrNo MediaStatus);
    
    /* Bus lock */
    cyg_mutex_t Mutex;
    
#endif /* DADIO BOOT */
    /* Memcfg settings */
    volatile cyg_uint32* MemcfgRegister;
    cyg_uint32 MemcfgMask;
    cyg_uint32 MemcfgDefaultSetting;
    cyg_uint32 Memcfg16bitSetting;
    
    /* Stored registers */
    unsigned char Status;
    unsigned char Error;
    
    /* Has this bus been initialized? */
    bool Initialized;
    
    /* Devices on bus */
    ATADeviceInfo_T Device[2];
};

unsigned char Read8(unsigned int Base, unsigned int Offset);
void Read16Multiple(unsigned int Base, unsigned int Offset, unsigned short * Buf, int Length);
void Write8(unsigned int Base, unsigned int Offset, unsigned char Value);
void Write16(unsigned int Base, unsigned int Offset, unsigned short Value);
void Write16Multiple(unsigned int Base, unsigned int Offset, unsigned short * Buf, int Length);

/* TODO Make these declarations a config option */
extern BusSpace_T IDEBus;
extern BusSpace_T MMBus;
extern BusSpace_T HipZipBus;

#endif /* ATABUS_H */

