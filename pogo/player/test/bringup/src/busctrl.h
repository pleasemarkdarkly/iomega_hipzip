// busctrl.h: bus control routines
// danc@iobjects.com 07/03/01
// (c) Interactive Objects

#ifndef __BUSCTRL_H__
#define __BUSCTRL_H__

int  BusInit( BusSpace_T* bus );
void BusReset( BusSpace_T* bus );
void BusPowerDown( BusSpace_T* bus );
void BusPowerUp( BusSpace_T* bus );
cyg_uint32 BusInterruptISR(cyg_vector_t Vector, cyg_addrword_t Data);
void BusInterruptDSR(cyg_vector_t Vector, cyg_ucount32 Count, cyg_addrword_t Data);

unsigned char BusRead8( BusSpace_T* bus, unsigned int Offset );
void BusWrite8( BusSpace_T* bus, unsigned int Offset, unsigned char Value );

void BusWrite16( BusSpace_T* bus, unsigned int Offset, unsigned short Value );
void BusWrite16Multiple( BusSpace_T* bus, unsigned int Offset, unsigned short* Buf, int Length );
void BusRead16Multiple( BusSpace_T* bus, unsigned int Offset, unsigned short* Buf, int Length );

void BusWrite16M( BusSpace_T* bus, unsigned int Offset, unsigned short Value );
void BusWrite16MultipleM( BusSpace_T* bus, unsigned int Offset, unsigned short* Buf, int Length );
void BusRead16MultipleM( BusSpace_T* bus, unsigned int Offset, unsigned short* Buf, int Length );

#endif // __BUSCTRL_H__
