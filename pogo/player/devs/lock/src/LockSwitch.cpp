// LockSwitch.cpp: interface to lock switch on pogo
// temancl@iobjects.com 01/02/02
// (c) Interactive Objects

#ifdef __POGO

#include <devs/lock/LockSwitch.h>
#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/hal_diag.h>
#include <cyg/hal/drv_api.h>
#include <string.h>   // memset
#include <cyg/infra/diag.h>

//
// CLockSwitch implementation
//
static CLockSwitch* s_pSingleton = 0;

CLockSwitch* CLockSwitch::GetInstance() 
{
    if( s_pSingleton == 0 ) {
        s_pSingleton = new CLockSwitch;
    }
    return s_pSingleton;
}

CLockSwitch::CLockSwitch() 
{
	// double check PBDDR
   *(volatile cyg_uint8 *)PBDDR &= ~0x04;
}

CLockSwitch::~CLockSwitch()
{
   
}

bool CLockSwitch::IsLocked()
{

	if(*(volatile cyg_uint8 *)PBDR & 0x04)
	{
		return true;
	}
	else
	{
		return false;
	}
	
}

#endif // __POGO