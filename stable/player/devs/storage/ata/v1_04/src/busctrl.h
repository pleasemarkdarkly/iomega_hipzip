// busctrl.h: bus control routines
// danc@iobjects.com 07/03/01
// (c) Interactive Objects

#ifndef __BUSCTRL_H__
#define __BUSCTRL_H__

#ifdef __OPTIMIZE__
#define EXT_INL extern inline
#else
#define EXT_INL
#endif
void ata_hard_reset(BusSpace_T* bus );
int  BusInit( BusSpace_T* bus );
void BusReset( BusSpace_T* bus );
void BusPowerDown( BusSpace_T* bus );
void BusPowerUp( BusSpace_T* bus );
cyg_uint32 BusInterruptISR(cyg_vector_t Vector, cyg_addrword_t Data);
void BusInterruptDSR(cyg_vector_t Vector, cyg_ucount32 Count, cyg_addrword_t Data);

EXT_INL unsigned char BusRead8( BusSpace_T* Bus, unsigned int Offset )
#ifdef __OPTIMIZE__
{
    return *((volatile char *)(Bus->Base + Offset));
}
#else
;
#endif

EXT_INL void BusWrite8( BusSpace_T* Bus, unsigned int Offset, unsigned char Value )
#ifdef __OPTIMIZE__
{
    *((volatile char *)(Bus->Base + Offset)) = Value;
}
#else
;
#endif
#undef EXT_INL

void BusWrite16( BusSpace_T* bus, unsigned int Offset, unsigned short Value );
void BusWrite16Multiple( BusSpace_T* bus, unsigned int Offset, unsigned short* Buf, int Length );
void BusRead16Multiple( BusSpace_T* bus, unsigned int Offset, unsigned short* Buf, int Length );

void BusWrite16M( BusSpace_T* bus, unsigned int Offset, unsigned short Value );
void BusWrite16MultipleM( BusSpace_T* bus, unsigned int Offset, unsigned short* Buf, int Length );
void BusRead16MultipleM( BusSpace_T* bus, unsigned int Offset, unsigned short* Buf, int Length );

#endif // __BUSCTRL_H__
