//
// testharness.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef TESTHARNESS_H_
#define TESTHARNESS_H_

#include <cyg/kernel/kapi.h>
#include <stdarg.h> // va_list
#include <datastream/netstream/netstream.h>	// ethernet debugging.

#include <util/timer/Timer.h>

//! The Event Recorder logs events.
class CEventRecorder
{
public:

    //! Returns a pointer to the global event recorder.
    static CEventRecorder* GetInstance();

    //! Destroy the singleton global event recorder.
    static void Destroy();

	static bool	g_LoggingEnabled;

	//! Process an event.  This gets called by the main event handler.
	void ProcessEvent(int key, void* data );

	void EnableEventLogging(bool bEnable);

    void EnableEventLogFile(bool bEnable);

private:

    CEventRecorder();
    ~CEventRecorder();

    static CEventRecorder* s_pSingleton;   // The global singleton IML manager.

    int m_fdCurrentOutputFile;

	unsigned int m_LastEventTime;


};


class CDebugRouter
{
public:
	    //! Returns a pointer to the global debug router.
    static CDebugRouter* GetInstance();

	static bool g_EnableDebugRouting;

	static bool g_EnableNetworkRouting;

    //! Destroy the singleton global debug router.
    static void Destroy();

    static void sHandleDebug( const char* mod, int sev, int settings, const char* fmt, ... );
	void HandleDebug(const char* mod, int sev, int settings, const char *fmt, va_list ap);

    static void sHandleAssert( const char*, const char*, int, const char*, const char*, const char*, ... );
    void HandleAssert(const char*, const char*, int, const char*, const char*, const char*, va_list);
    
    
	void Flush();

	void RouteToFile(const char * szFilepath);
	
	void RouteToNetwork(const char * szServer, unsigned int port);

	void EnableSerialEcho(bool bEnable);

	void EnableDebugRouting(bool bEnable);

	void EnableNetworkRouting(bool bEnable);

	void CreateLogFile();

	void DebugFlush();

private:

	CDebugRouter();
    ~CDebugRouter();

    static CDebugRouter* s_pSingleton;   // The global singleton IML manager.

	void net_diag(const char* sz);

	int m_fdCurrentOutputFile;
	CNetStream m_nsOutputStream;
	cyg_mutex_t m_mtxDebugAccess;
	bool m_EchoDebugSerial;
	bool m_bConnected;

	timer_handle_t  m_hTimer;

};


class CTestStimulator
{
public:
	    //! Returns a pointer to the global debug router.
    static CTestStimulator* GetInstance();

    //! Destroy the singleton global debug router.
    static void Destroy();

	void StartMonkeyTest(int iHoldDelay, int iMaxEventDelay, int iMaxHoldCount, int iMinEventDelay, int iMinHoldCount,unsigned int uEventMask, unsigned int uEventMaskHi);

	void StopTest();

	void ExecuteScript(const char *szFilepath);

    void ExecuteMultipleScripts(int iFileCount, bool bRandom, bool bRepeat, int iRetryCount);

	void RunStartupScript();

    void ExecuteNext();

private:

	CTestStimulator();
    ~CTestStimulator();

    void RemoveRetryFile();

    int         m_iFileCount;
    bool        m_bRandom;
    bool        m_bRepeat;
    bool        m_bRetry;
    bool        m_bReproAttempted;
    int         m_iRetryCount;
    int         m_iCurrentFile;

    static CTestStimulator* s_pSingleton;   // The global singleton event stimulator.
};

class CDeviceState
{

public:
	typedef enum EDeviceState
    {
        IDLE = 0,   
        RUNNING_SCRIPT, 
        DOING_MONKEY,  
		BUSY
    };

	// return textual description of device state
    char * GetDeviceState();
        
    //! Returns a pointer to the global device state.
    static CDeviceState* GetInstance();

    //! Destroy the singleton global device state manager.
    static void Destroy();

	void NotifyNotIdle();

// should only be called from within the test infrastructure
	void SetDeviceState(EDeviceState eDeviceState);

	~CDeviceState();

private:

    CDeviceState();

	unsigned int m_LastBusyTime;

	EDeviceState m_eDeviceState;

    static CDeviceState* s_pSingleton;   



};

enum NetDiagCommand
{
	//Client -> Server
	cInitialize,
	cDiag,

	//Server -> Client
	sReady = 1000,
	sDisconnect,
	sTestScript,
	sFail,
	sBadCommand
};

struct NetDiagHeader
{
	NetDiagHeader() {}
	NetDiagHeader(unsigned long dwLength,NetDiagCommand netCommand) : m_dwLength(dwLength), m_netCommand(netCommand) {}
	~NetDiagHeader() {}
	
	unsigned long	m_dwLength;
	NetDiagCommand	m_netCommand;
};

struct NetDiagInit
{
	long lVerbosity;
	long lTestScriptLen;
};

void Simpleitoa(int Num, char* pString);


#endif	// TESTHARNESS_H_
