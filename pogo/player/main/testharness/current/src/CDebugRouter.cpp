//
// CDebugRouter.cpp
//
// Copyright (c) 1998 - 2001 Fullplay Media Systems (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifdef ENABLE_DEBUG_ROUTING

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <network.h>
#include <cyg/infra/diag.h>
#include <cyg/infra/cyg_ass.h>
#include <cyg/hal/hal_intr.h>           // exception ranges
#include <cyg/hal/hal_cache.h>
#include <cyg/hal/hal_edb7xxx.h>

#include <cyg/kernel/thread.hxx>        // thread.inl
#include <cyg/kernel/thread.inl>

#include <fs/fat/sdapi.h>
#include <util/debug/debug.h>
#include <util/diag/diag.h>
#include <io/net/Net.h>
#include <main/testharness/testharness.h>

#include <devs/audio/dai.h>

#include <_version.h>

#include <main/main/AppSettings.h>

// Note: CDebugRouter MUST be thread safe.


#define MAX_FORMAT_BUFFER 5120

DEBUG_USE_MODULE(ATA);

static char szFormatBuffer[MAX_FORMAT_BUFFER];

#define DEFAULT_OPEN_FLAGS  PO_WRONLY | PO_CREAT | PO_NOSHAREANY
#define DEFAULT_OPEN_MODE   PS_IWRITE

// The global singleton debug router.
CDebugRouter* CDebugRouter::s_pSingleton = 0;

// flag to check if we are enabling debug routing at run time
bool CDebugRouter::g_EnableDebugRouting = 0;

// flag to check if we are enabling network routing at run time.
bool CDebugRouter::g_EnableNetworkRouting = 0;

// Returns a pointer to the global debug router.

CDebugRouter*
CDebugRouter::GetInstance()
{
    if (!s_pSingleton)
        s_pSingleton = new CDebugRouter;
    return s_pSingleton;
}

// Destroy the singleton debug router.
void
CDebugRouter::Destroy()
{
    delete s_pSingleton;
    s_pSingleton = 0;
}


CDebugRouter::CDebugRouter()
{
    m_fdCurrentOutputFile = -1;
	m_EchoDebugSerial = 1;
	m_bConnected = false;
	cyg_mutex_init(&m_mtxDebugAccess);
    
    // Register with the debug interface
    dbg_set_debug_handler( (debug_handler_t)CDebugRouter::sHandleDebug );
	diag_printf("setting assert handler to debug router\n");
    dbg_set_assert_handler( (assert_handler_t)CDebugRouter::sHandleAssert );
}

void DebugFlushCB(void* arg);

void CDebugRouter::CreateLogFile()
{
	diag_printf("Always logging to file\n");
	char LogFile[128];
	UINT16 attribs;
	bool bExisted = true;
	if (!(pc_get_attributes(DEBUG_LOG_FOLDER, &attribs) == YES && (attribs & ADIRENT)))
	{
		pc_mkdir(DEBUG_LOG_FOLDER);
		bExisted = false;
		diag_printf("created log directory - %s\n", DEBUG_LOG_FOLDER);
	}

	if (bExisted)
	{
		strcpy(LogFile, DEBUG_LOG_FOLDER);
		strcat(LogFile, "/Log-");
		int Length = strlen(LogFile);
		char Num[10];
		UINT16 attribs;
		for(int i = 1;;i++)
		{
			Simpleitoa(i,Num);
			strcpy((LogFile + Length), Num);
			strcat(LogFile, ".txt");
			if (!(pc_get_attributes(LogFile, &attribs) == YES))
				break;
		}
	}
	else
	{
		strcpy(LogFile, DEBUG_LOG_FOLDER);
		strcat(LogFile, "/Log-1.txt");
	}

	diag_printf("created log file - %s\n", LogFile);

	CDebugRouter::GetInstance()->EnableDebugRouting(true);
	RouteToFile(LogFile);

	CEventRecorder::GetInstance()->EnableEventLogging(true);

	register_timer(DebugFlushCB, (void*)this, TIMER_MILLISECONDS(10000), -1, &m_hTimer);
    resume_timer(m_hTimer);
}

CDebugRouter::~CDebugRouter()
{
	if (m_fdCurrentOutputFile>=0)
	{
		po_close(m_fdCurrentOutputFile);
	}

	if (m_bConnected)
	{
		m_nsOutputStream.Close();
		m_bConnected = false;
	}
    dbg_set_debug_handler( (debug_handler_t) 0 );
    dbg_set_assert_handler( (assert_handler_t) 0 );
}

// Static routine which pushes debugging to the singleton
void
CDebugRouter::sHandleDebug( const char* mod, int sev, int settings, const char *fmt, ... )
{
    va_list ap;
    va_start( ap, fmt );

    CDebugRouter::GetInstance()->HandleDebug( mod, sev, settings, fmt, ap );
    
    va_end( ap );
}

void 
CDebugRouter::HandleDebug( const char* mod, int sev, int settings, const char *fmt, va_list ap )
{
    // dc- roll over the log file if it is 16mb or larger. has to be done outside of the general mutex,
    //     so add some skanky soft locking
    static volatile int passive_lock = 0;
    cyg_mutex_lock( &m_mtxDebugAccess );
    
    if( !passive_lock ) {
        short wank;
        unsigned int file_len = po_lseek( m_fdCurrentOutputFile, 0, PSEEK_CUR, &wank );
        if( !wank && file_len >= (16*1024*1024) ) {
            // since we are about to unlock the debug mutex, set the passive lock so that other threads
            // are forced to read it again when they manage to lock the mutex
            passive_lock = 1;
            cyg_mutex_unlock( &m_mtxDebugAccess );
            this->Flush();
            this->CreateLogFile();
            cyg_mutex_lock( &m_mtxDebugAccess );
            passive_lock = 0;
        }
    }

    // dc- print out a formatted map to indicate 1) module name and 2) enabled debug levels
    // horrible trickery - since we want to know about bits 0x10, 0x20, and 0x40,
    // we can do this to get 0,1,2
    char ErrorCodes[strlen(mod) + 7];
    if( sev >= DBGLEV_WARNING ) {
        sev >>= 5;
        char code[4] = 
            { (settings&DBGLEV_WARNING ? '.' : ' '),
              (settings&DBGLEV_ERROR   ? '.' : ' '),
              (settings&DBGLEV_FATAL   ? '.' : ' '),
              (sev==0?'W':(sev==1?'E':'F'))
            };
        sev < 3 && (code[sev] = code[3]);
        sprintf(ErrorCodes,"[%s/%c%c%c]",mod,code[2],code[1],code[0]);
    } else {
        sev = -1;
    }

	if (m_EchoDebugSerial)
	{
        if( sev != -1) {
            diag_printf("%s",ErrorCodes);
        }
		int buflen = diag_vprintf(fmt,ap);
        if ((buflen > MAX_FORMAT_BUFFER) && (m_fdCurrentOutputFile >= 0 || m_bConnected))
            diag_printf("**** Debug buffer overflow: %d bytes ***\n", buflen);
	}

	if (m_fdCurrentOutputFile >= 0 || m_bConnected)
	{
        int StartPos = 0;
        if( sev != -1) {
            strcpy(szFormatBuffer, ErrorCodes);
            StartPos = strlen(szFormatBuffer);
        }

        // TODO: Create a diag_vsnprintf function to protect from buffer overflow.
		diag_vsprintf((szFormatBuffer + StartPos),fmt,ap);

		if (m_fdCurrentOutputFile >= 0)
		{
			po_write(m_fdCurrentOutputFile,(UCHAR*)szFormatBuffer,strlen(szFormatBuffer));
		}

		if (m_bConnected)
		{
			net_diag(szFormatBuffer);
		}
	}


	cyg_mutex_unlock(&m_mtxDebugAccess);
}

void
CDebugRouter::sHandleAssert( const char* file, const char* func, int line, const char* module, const char* cond, const char* fmt, ... ) 
{
    va_list ap;
    va_start( ap, fmt );

    CDebugRouter::GetInstance()->HandleAssert( file, func, line, module, cond, fmt, ap );
    
    va_end( ap );
}

void
CDebugRouter::HandleAssert( const char* file, const char* func, int line, const char* module, const char* cond, const char* fmt, va_list ap )
{
    const char* thread_name = Cyg_Thread::self()->get_name();

    // Lock this after the print_mem_usage() since the above uses the debug interface
    cyg_mutex_lock( &m_mtxDebugAccess );
    
    if( m_EchoDebugSerial )
    {
        diag_printf("   File: %s\n   Function: %s (line %d)\n   Module: %s\n   Condition: %s\n   Thread Name: %s\n",
            file, func, line, module, cond, thread_name );
        diag_vprintf(fmt, ap);
    }

    if( m_fdCurrentOutputFile >= 0 ) {
        diag_sprintf( szFormatBuffer, "****ASSERTION FAILED****\n   File: %s\n   Function: %s (line %d)\n   Module: %s\n   Condition: %s\n   Thread Name: %s\n",
            file, func, line, module, cond, thread_name );
        po_write( m_fdCurrentOutputFile, (UCHAR*) szFormatBuffer, strlen(szFormatBuffer) );
        
        diag_vsprintf(szFormatBuffer, fmt, ap );
        po_write( m_fdCurrentOutputFile, (UCHAR*) szFormatBuffer, strlen(szFormatBuffer) );
    }
	if (m_bConnected)
	{
		diag_sprintf( szFormatBuffer, "****ASSERTION FAILED****\n   File: %s\n   Function: %s (line %d)\n   Module: %s\n   Condition: %s\n   Thread Name: %s\n",
            file, func, line, module, cond, thread_name );
        net_diag(szFormatBuffer);
        diag_vsprintf(szFormatBuffer, fmt, ap );
		net_diag(szFormatBuffer);
    }

    cyg_mutex_unlock( &m_mtxDebugAccess );

    // do this after the lock but before the flush so that output will be logged and displayed.
    //    print_thread_states();
#ifdef ENABLE_LEAK_TRACING
    dump_leak_trace();
#endif
    
    // An assertion should stop the system; this may not be the most appropriate way to do it though.
    this->Flush();

	diag_printf("Rebooting\n");
	
	// make sure dac is off before resetting.
	DAIDisable();
	
	int oldints;
	HAL_DISABLE_INTERRUPTS(oldints);
	HAL_UCACHE_DISABLE();
	HAL_UCACHE_INVALIDATE_ALL();

	
#ifdef ENABLE_FIORI_TEST
    // for fiori, halt on assertions
    while(1);
#else
    // Trigger POR
    *(volatile unsigned char*)PDDDR &= ~(0x01);
    *(volatile unsigned char*)PDDR  |= 0x01;

     // Soft reset for rev-02 boards
    void (*f)() = (void(*)())hal_vsr_table[0];
    f();
#endif
}

void 
CDebugRouter::Flush()
{
	cyg_mutex_lock(&m_mtxDebugAccess);

	if (m_fdCurrentOutputFile >= 0)
	{
		char CloseMessage[124];
		strcpy(CloseMessage, "\nClosing Log File\n");
		po_write(m_fdCurrentOutputFile,(UCHAR*)CloseMessage,strlen(CloseMessage));
		po_close(m_fdCurrentOutputFile);
	}
	CDebugRouter::g_EnableDebugRouting = false;

	if (m_bConnected)
	{
		m_nsOutputStream.Close();
	}
	CDebugRouter::g_EnableNetworkRouting = false;
	m_bConnected = false;

	cyg_mutex_unlock(&m_mtxDebugAccess);
}

void
CDebugRouter::RouteToFile(const char * szFilepath)
{
	cyg_mutex_lock(&m_mtxDebugAccess);

	if (m_fdCurrentOutputFile >= 0)
	{
		po_close(m_fdCurrentOutputFile);
	}

	DEBUG_DISABLE(ATA, DBGLEV_DEFAULT | DBGLEV_TRACE | DBGLEV_INFO | DBGLEV_CHATTER);

	m_fdCurrentOutputFile = po_open(szFilepath,DEFAULT_OPEN_FLAGS,DEFAULT_OPEN_MODE);

	if (m_fdCurrentOutputFile < 0)
	{
		diag_printf("\nFailed to open file %s for debug output.\n",szFilepath);
	}

	cyg_mutex_unlock(&m_mtxDebugAccess);
}

void
CDebugRouter::RouteToNetwork(const char * szServer, unsigned int port)
{
	diag_printf("CDebugRouter::RouteToNetwork called\n");
	bool bRes;
	cyg_mutex_lock(&m_mtxDebugAccess);

	if (m_bConnected)
	{
		m_bConnected = false;
		m_nsOutputStream.Close();
	}

	bRes = m_nsOutputStream.Open(szServer, port);
	if (bRes)
	{
        char szTempMAC[13];
        char mac[6];
        GetInterfaceAddresses( "eth0", NULL, mac );
        diag_sprintf(szTempMAC, "%02x%02x%02x%02x%02x%02x\n", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5] );
        diag_printf("MAC=%s\n", szTempMAC);

		NetDiagHeader netClient(strlen(szTempMAC)+1, cInitialize);
		m_nsOutputStream.Write((const char*)&netClient, sizeof(NetDiagHeader));
		m_nsOutputStream.Write(szTempMAC,strlen(szTempMAC)+1);
		NetDiagHeader netResponse;
		bRes = m_nsOutputStream.Read((void*)&netResponse, sizeof(NetDiagHeader));
		m_bConnected = (netResponse.m_netCommand == sReady);

		if(m_bConnected)
		{
			strcpy(szFormatBuffer, "Version: ");
			strcat(szFormatBuffer, DDO_VERSION_STR);
			strcat(szFormatBuffer, "\n");
			net_diag(szFormatBuffer);
		}
	}
	else
	{
		diag_printf("\nFailed to open server %s port %d for debug output.\n",szServer, port);
		m_bConnected = false;
	}

	cyg_mutex_unlock(&m_mtxDebugAccess);
}

void
CDebugRouter::EnableSerialEcho(bool bEnable)
{
	cyg_mutex_lock(&m_mtxDebugAccess);
		
	m_EchoDebugSerial = bEnable;

	cyg_mutex_unlock(&m_mtxDebugAccess);
}

void 
CDebugRouter::EnableDebugRouting(bool bEnable)
{
	cyg_mutex_lock(&m_mtxDebugAccess);
		
	CDebugRouter::g_EnableDebugRouting = bEnable;

	cyg_mutex_unlock(&m_mtxDebugAccess);
}

void
CDebugRouter::EnableNetworkRouting(bool bEnable)
{
	cyg_mutex_lock(&m_mtxDebugAccess);
		
	CDebugRouter::g_EnableNetworkRouting = bEnable;

	cyg_mutex_unlock(&m_mtxDebugAccess);
}

void
CDebugRouter::net_diag(const char* sz)
{
	int length = strlen(sz) + 1;
	NetDiagHeader netClient(length,cDiag);
	m_nsOutputStream.Write((const char*)&netClient,sizeof(NetDiagHeader));
	m_nsOutputStream.Write(sz, length);
}

void
CDebugRouter::DebugFlush()
{
	cyg_mutex_lock(&m_mtxDebugAccess);
		
	if (m_fdCurrentOutputFile >= 0)
	{
		po_flush(m_fdCurrentOutputFile);
	}

	cyg_mutex_unlock(&m_mtxDebugAccess);
}

//this should overide the cyg_assert_fail in cyg_ass.h
void cyg_assert_fail( const char* sz_func, const char* sz_file,
    cyg_uint32 linenum, const char* sz_msg)
{
	CDebugRouter::GetInstance()->sHandleAssert(sz_file, sz_func, linenum, "ecos", "ecos assert", sz_msg);
    // DC- this line is never executed, but prevents the compiler from complaining about this function
    //     (which has the "noreturn" attribute)
    while(1);
}

//only works for non-signed ints
void Simpleitoa(int Num, char* pString)
{
	int i;
	for(i = 0;Num > 0;i++)
	{
		pString[i] = char((Num % 10) + 48);
		Num/= 10;
	}
	pString[i] = '\0';
	for(int j = 0, k = i - 1;j < i/2;j++,k--)
	{
		char temp = pString[j];
		pString[j] = pString[k];
		pString[k] = temp;
	}
}

void DebugFlushCB(void* arg)
{
    ((CDebugRouter*)arg)->DebugFlush();
}

#endif
