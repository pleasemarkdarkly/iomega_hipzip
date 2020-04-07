#include "TitleStream.h"
#include "DebugPlayer.h"
#include <string.h>

#define DEBUG(s...) diag_printf(##s)
//#define DEBUG(s...) /**/
#define DEBUG_FUNCFAIL(s...) diag_printf(##s)
//#define DEBUG_FUNCFAIL(s...) /**/
#define _DEBUG(s...) diag_printf(##s)
//#define _DEBUG(s...) /**/

CTitleStream::CTitleStream()
    : _ServerSocket(-1),
      _Running(false)
{
    // Get local pointer to event queue
    _EventQueue = CEventQueue::GetEventQueue();

    // Initialize semaphore used to signal end of server thread
    cyg_semaphore_init(&_TitleStreamThreadExitSem, 0);
}

CTitleStream::~CTitleStream()
{
    // Stop server thread
    if (_Running) {
	_Running = false;
	cyg_semaphore_wait(&_TitleStreamThreadExitSem);

	// Delete server thread
	cyg_bool_t Status = cyg_thread_delete(_TitleStreamThreadHandle);
	if (!Status) {
	    DEBUG_FUNCFAIL("-%s %d\n", __FUNCTION__, __LINE__);
	}

    }

	// Make sure socket is closed
	if (_ServerSocket != -1) {
	    close(_ServerSocket);
	}

    // Delete thread exit semaphore
    cyg_semaphore_destroy(&_TitleStreamThreadExitSem);
}

int
CTitleStream::Initialize(int & Port)
{
    // Create socket to accept UDP transmissions on
    _ServerSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (_ServerSocket < 0) {
	DEBUG_FUNCFAIL("-%s %d\n", __FUNCTION__, __LINE__);
	return -1;
    }

    // Bind socket to a port
    struct sockaddr_in ServerAddress;
    memset(&ServerAddress, 0, sizeof(struct sockaddr_in));
    ServerAddress.sin_family = AF_INET;
    ServerAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    int PossiblePort;
    for (PossiblePort = 10000; PossiblePort < 32767; ++PossiblePort) {
	ServerAddress.sin_port = htons(PossiblePort);
	if (bind(_ServerSocket, (struct sockaddr *)&ServerAddress, sizeof(struct sockaddr_in)) < 0) {
	    continue;
	}
	else {
	    Port = PossiblePort;
	    break;
	}
    }
    if (PossiblePort == 32767) {
	close(_ServerSocket);
	_ServerSocket = -1;
	DEBUG_FUNCFAIL("-%s %d\n", __FUNCTION__, __LINE__);
	return -1;
    }

    return 0;    
}

int
CTitleStream::Run(in_addr & InAddr, int Port)
{
    struct sockaddr_in SockAddr;
    memset(&SockAddr, 0, sizeof(struct sockaddr_in));
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_addr = InAddr;
    SockAddr.sin_port = htons(Port);

    if (connect(_ServerSocket, (struct sockaddr *)&SockAddr, sizeof(struct sockaddr_in)) < 0) {
	close(_ServerSocket);
	_ServerSocket = -1;
	DEBUG_FUNCFAIL("-%s %d\n", __FUNCTION__, __LINE__);
	return -1;
    }

    // Create thread to listen for title information
    cyg_thread_create(TITLE_STREAM_THREAD_PRIORITY,
		      CTitleStream::_StartTitleStreamThread,
		      (cyg_addrword_t)this,
		      "TitleStreamThread",
		      (void *)_TitleStreamThreadStack,
		      TITLE_STREAM_THREAD_STACK_SIZE,
		      &_TitleStreamThreadHandle,
		      &_TitleStreamThread);
    cyg_thread_resume(_TitleStreamThreadHandle);
    
    return 0;
}

void
CTitleStream::_StartTitleStreamThread(cyg_addrword_t Data)
{
    reinterpret_cast<CTitleStream *>(Data)->_TitleStreamThreadEntry();
}

void
CTitleStream::_TitleStreamThreadEntry(void)
{
    char Buffer[256];

    _Running = true;
    for (; _Running;) {
	struct timeval Timeout;
	Timeout.tv_sec = 0;
	Timeout.tv_usec = 0;

	fd_set FdSet;
	FD_ZERO(&FdSet);
	FD_SET(_ServerSocket, &FdSet);

	int SelectStatus = select(_ServerSocket + 1, NULL, &FdSet, NULL, &Timeout);
	if (SelectStatus == 0) {
#ifndef __ECOS
	    usleep(100000);
#else
	    /* TODO This loop just polls now.  Look at switching back to using a blocking socket,
	     * or a better means to delay. */
#endif
	    continue;
	}

	int BytesReceived = recv(_ServerSocket, Buffer, sizeof(Buffer) - 1, 0);
	if (BytesReceived > 0) {
	    
	    /* Null terminate string received from server */
	    Buffer[BytesReceived] = 0;

	    /* Parse x-audiocast headers */
	    char * StreamTitle;
	    char * Title = 0;
	    StreamTitle = strstr(Buffer, "x-audiocast-streamtitle");
	    if (StreamTitle != NULL) {

		/* Set pointer to beginning of title */
		Title = strchr(StreamTitle, ':');
		if (Title) {
		    Title += 2;

		    /* Null terminate title */
		    char * EndOfTitle = Title;
		    while(*EndOfTitle != '\r' && *(EndOfTitle + 1) != '\n') {
			++EndOfTitle;
		    }
		    *EndOfTitle = 0;
		}
	    }
#if 0
	    if (strstr(Buffer, "x-audiocast-streamurl") != NULL) {
	    }
	    if (strstr(Buffer, "x-audiocast-streamlength") != NULL) {
	    }
	    if (strstr(Buffer, "x-audiocast-udpseqnr") != NULL) {
	    }
#endif
	    /* Put song info event into queue */
	    if (Title) {
		_EventQueue->PutEvent(new CStreamInfoEvent(Title));
	    }
	}
    }
    
    close(_ServerSocket);
    _ServerSocket = -1;
    cyg_semaphore_post(&_TitleStreamThreadExitSem);
}

void
CTitleStream::Stats(void) 
{
    diag_printf("sizeof(CTitleStream): %d\n", sizeof(CTitleStream));
    cyg_test_dump_thread_stack_stats("CTitleStream::_TitleStreamThreadEntry", _TitleStreamThreadHandle);
}
