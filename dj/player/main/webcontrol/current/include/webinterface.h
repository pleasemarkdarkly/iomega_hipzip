//
// webinterface.h
//
// Copyright (c) 1998 - 2002 Fullplay Media Systems, Inc. All rights reserved
//
/

#ifndef WEBINTERFACE_H_
#define WEBINTERFACE_H_


#include <cyg/kernel/kapi.h>
/*
#include <stdarg.h> // va_list
#include <datastream/netstream/netstream.h>	// ethernet debugging.

#include <util/timer/Timer.h>

*/


class CWebInterface
{
public:

    //! Returns a pointer to the global web interface.
    static CWebInterface* GetInstance();

    //! Destroy the singleton global event recorder.
    static void Destroy();

	//! Process an event.  This gets called by the main event handler.
	int TransportControl(char * transport);

private:

    CWebInterface();
    ~CWebInterface();

    static CWebInterface* s_pSingleton;   // The global singleton CWebInterface

	cyg_mutex_t m_mtxWebControlAccess;
};


#endif	// WEBCONTROL_H_
