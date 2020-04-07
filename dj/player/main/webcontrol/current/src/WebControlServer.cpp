//
// WebControlServer.cpp - the lean, mean web serving machine
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//


#include <stdlib.h>
#include <stdio.h>
#include <cyg/infra/diag.h>
#include <stdarg.h>

#include <util/debug/debug.h>
#include <util/diag/diag.h>

#include <cyg/kernel/kapi.h>

#include <util/eventq/EventQueueAPI.h>

#include <main/ui/keys.h>
#include <core/events/SystemEvents.h>


#include <fs/fat/sdapi.h>

#include <main/ui/common/UserInterface.h>

#include <main/ui/UI.h>

#include <main/webcontrol/FunctionInterface.h>

#include <util/datastructures/SimpleList.h>

#include <cyg/kernel/thread.hxx>

extern "C"
{
#include <network.h>
}

#include <main/externcontrol/ExternControl.h>
#include <main/main/AppSettings.h>

extern void SetMainThreadPriority(int nPrio);
extern int GetMainThreadPriority();

extern void ReportSystemMemory(char* szCaption);

DEBUG_MODULE_S(WEBCONTROLSERVER, DBGLEV_DEFAULT | DBGLEV_INFO);
DEBUG_USE_MODULE(WEBCONTROLSERVER);  // debugging prefix : (46) wcs

#define WEBCONTROL_RUNNING 1
#define WEBCONTROL_NOTRUNNING 0
int g_WebControlServerStatus=WEBCONTROL_NOTRUNNING;

static cyg_flag_t g_flagStopServer;

static cyg_flag_t g_flagControlCallbackComplete;
// protect the queue
static cyg_mutex_t m_mutQueues;

static int g_ControlSocket;

static SimpleList<char*> m_lstListenerRequestQueue;
static SimpleList<char*> m_lstServerRequestQueue;
static SimpleList<int> m_lstServerSocketQueue;

int SendRefusalReply(int nSocket);
void ServerThreadEntry(unsigned int data);
int SendAsleepReply(int nSocket);
int SendPoweredDownReply(int nSocket);

#define WEBCONTROL_PORT 80

#define MAX_HTTP_COMMAND 1024
#define MAX_PATH 512

#define WEBCONTROL_LISTENER_STACKSIZE       8192*2
#define WEBCONTROL_SERVER_STACKSIZE   8192*1
#define MAX_SERVER_REQUESTS 8
#define MINIMUM_REQUEST_REFUSAL_TICKS 400   // 4s

static cyg_handle_t hListenerThread;
static cyg_handle_t hServerThread;
static cyg_thread   thdWebControlListener;
static cyg_thread   thdWebControlServer;
static cyg_sem_t  semRequestsPending;

static char achListenerStack[WEBCONTROL_LISTENER_STACKSIZE];
static char achServerStack[WEBCONTROL_SERVER_STACKSIZE];

void ListenerThreadEntry( cyg_uint32 data);
void ProcessHTTP(int nSocket,char *buf);

static cyg_flag_t g_flagServiceCallbackComplete;
int g_actionSucceeded;
int g_fnIndex;
bool g_expectingCallback;
int g_nSocket;
const char * g_path;


int StartHTTPServerSocket()
{
	struct sockaddr_in serverAddr;
	int listenfd = 0;
	int success;

	g_actionSucceeded = 0;
	g_fnIndex = -1;
	g_expectingCallback = false;

	listenfd = socket( AF_INET, SOCK_STREAM, 0 );
	if ( listenfd <= 0 )
	{
		DEBUGP(WEBCONTROLSERVER, DBGLEV_ERROR,"Out of sockets\n");
		return 0;
	}

	memset( &serverAddr, 0, sizeof(serverAddr) );
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl( INADDR_ANY );
	serverAddr.sin_port = htons( WEBCONTROL_PORT );

	success = bind( listenfd, (sockaddr*)&serverAddr,
		sizeof(serverAddr) );
	if ( success == -1 )
	{
		DEBUGP(WEBCONTROLSERVER, DBGLEV_ERROR,"Could not bind\n");
		close(listenfd);
		return 0;
	}

    // (epg,7/15/2002): backlog to zero from 1
	success = listen( listenfd, 0);
	if ( success == -1 )
	{
		DEBUGP(WEBCONTROLSERVER, DBGLEV_ERROR,"Could not listen\n");
		close(listenfd);
		return 0;
	}

	return listenfd;
}

static bool m_bAsleep = false;

int StartWebControlServer()
{
	struct sockaddr_in serverAddr;
	int listenfd = 0;
	int success;
    static bool sbFirst = true;
	if (g_WebControlServerStatus!=WEBCONTROL_NOTRUNNING)
	{
		DEBUGP(WEBCONTROLSERVER, DBGLEV_ERROR,"Server is already running\n");
        m_bAsleep = false;
		return 0;
	}

	cyg_flag_init(&g_flagStopServer);

	cyg_flag_init(&g_flagControlCallbackComplete);

	cyg_flag_init(&g_flagServiceCallbackComplete);

	listenfd = StartHTTPServerSocket();
	if ( listenfd <= 0 )
	{
		DEBUGP(WEBCONTROLSERVER, DBGLEV_ERROR,"Fatal: Out of sockets\n");
		return 0;
	}

    cyg_semaphore_init( &semRequestsPending, 0);
    cyg_mutex_init ( &m_mutQueues );

    if (sbFirst)
    {
        cyg_thread_create( 10, ServerThreadEntry, listenfd, "web control server",
	                       (void*)achServerStack, WEBCONTROL_SERVER_STACKSIZE, &hServerThread, &thdWebControlListener);
    }
	cyg_thread_resume(hServerThread);

    if (sbFirst)
    {
	    cyg_thread_create( 4, ListenerThreadEntry, listenfd, "web control listener",
	                       (void*)achListenerStack, WEBCONTROL_LISTENER_STACKSIZE, &hListenerThread, &thdWebControlServer);
    }
	cyg_thread_resume(hListenerThread);

    sbFirst = false;

    g_WebControlServerStatus = WEBCONTROL_RUNNING;

	DEBUGP(WEBCONTROLSERVER, DBGLEV_TRACE,"Web control server started\n");

	return 1;
}

int StopWebControlServer()
{
    if (g_WebControlServerStatus == WEBCONTROL_RUNNING)
        m_bAsleep = true;
	return 1;
}

void ListenerThreadEntry( cyg_uint32 data)
{
	struct sockaddr_in clientAddr;
    int listenfd;
	int res;

	char bfQueuedRequests[MAX_SERVER_REQUESTS][MAX_HTTP_COMMAND];
    for (int i = 0; i < MAX_SERVER_REQUESTS; ++i)
    {
        m_lstListenerRequestQueue.PushBack(bfQueuedRequests[i]);
    }
    char bfRejectRequest[MAX_HTTP_COMMAND];

	int connectfd;
    socklen_t clientLen;

    int retCode;
    fd_set readSet;
    struct timeval timeout;
    int numRead;

    listenfd = data;

    cyg_tick_count_t nLastRefuseTime = NULL;

    while ( true )
    {

	        connectfd = accept( listenfd, (sockaddr*) &clientAddr, &clientLen );
        
			if (connectfd > 0)
			{
				FD_ZERO( &readSet );
				FD_SET( connectfd, &readSet );
	
				timeout.tv_sec = 20;
				timeout.tv_usec = 0;
	
				retCode = select(connectfd+1, &readSet, NULL, NULL, &timeout );
	
				if (retCode==-1)
				{
					DEBUGP(WEBCONTROLSERVER, DBGLEV_ERROR,"Error reading from socket\n");
					shutdown(connectfd,2);
					close(connectfd);
				}
				else if (retCode ==0)
				{
					DEBUGP(WEBCONTROLSERVER, DBGLEV_ERROR,"Time out on socket.  Shutting down socket\n");
					shutdown(connectfd,2);
					DEBUGP(WEBCONTROLSERVER, DBGLEV_ERROR,"Time out on socket.  Closing socket\n");
					close(connectfd);
				}
				else
				{
	                cyg_mutex_lock( &m_mutQueues );
					cyg_tick_count_t nReqTime = cyg_current_time();
					if (m_bAsleep)
					{
	                    SendAsleepReply(connectfd);
						cyg_mutex_unlock( &m_mutQueues );
					}
					// if we run out of request space, refuse requests for a little while.
					else if (m_lstListenerRequestQueue.Size() == 0 )
					{
	                    DEBUGP( WEBCONTROLSERVER, DBGLEV_TRACE, "wcs:sending refusal\n");
						nLastRefuseTime = nReqTime;
						cyg_mutex_unlock( &m_mutQueues );
						res = recv(connectfd,bfRejectRequest,MAX_HTTP_COMMAND,0);    
						SendRefusalReply(connectfd);          
					}
					else if (nLastRefuseTime && nReqTime < nLastRefuseTime + MINIMUM_REQUEST_REFUSAL_TICKS)
					{
	                    DEBUGP( WEBCONTROLSERVER, DBGLEV_TRACE, "wcs:refuse timer\n"); 
						// refuse, but don't reset the timer, or the impatient user could be repeatedly frustrated.
						cyg_mutex_unlock( &m_mutQueues );
						res = recv(connectfd,bfRejectRequest,MAX_HTTP_COMMAND,0);    
						SendRefusalReply(connectfd);          
					}
					else
					{
	                    DEBUGP( WEBCONTROLSERVER, DBGLEV_TRACE, "wcs:queue request\n"); 
						char* bfRequest = m_lstListenerRequestQueue.PopBack();
						res = recv(connectfd,bfRequest,MAX_HTTP_COMMAND,0);
	
						m_lstServerRequestQueue.PushBack(bfRequest);
						m_lstServerSocketQueue.PushBack(connectfd);
						cyg_mutex_unlock( &m_mutQueues );
						cyg_semaphore_post( &semRequestsPending );
					}
					if (res < 0)
					{
						break;
					}
				}
			}

			else
			{
				DEBUGP(WEBCONTROLSERVER, DBGLEV_ERROR,"Could not accept\n");

				// wait a bit for things to settle down

				cyg_thread_delay(200);
	
				retCode = listen( listenfd, 0);
				if (retCode < 0)
				{
					DEBUGP(WEBCONTROLSERVER, DBGLEV_ERROR,"Could not listen to get restarted\nWeb server dead.");
					break;
				}
	
			}
		
		if (cyg_flag_poll(&g_flagStopServer,1,CYG_FLAG_WAITMODE_OR))
		{
	           // wake up the server so it can exit
			cyg_semaphore_post( &semRequestsPending );
			break;
		}
		
	}

	shutdown(listenfd,2);
	close (listenfd);
}

void ServerThreadEntry( cyg_uint32 data)
{
    while (true)
    {
        cyg_semaphore_wait( &semRequestsPending );

        cyg_mutex_lock ( &m_mutQueues );
        // if there is a request, fill it.
        if (m_lstServerRequestQueue.Size())
        {
            char* achRequest = m_lstServerRequestQueue.PopFront(); 
            int nSocket = m_lstServerSocketQueue.PopFront(); 
            cyg_mutex_unlock ( &m_mutQueues );

            ProcessHTTP(nSocket,achRequest);
            cyg_mutex_lock ( &m_mutQueues );
            m_lstListenerRequestQueue.PushBack(achRequest); 
            achRequest = NULL;
        }
        else
        {
            // nothing to do.  the semaphore was inflated to allow the server to shut down, so we ought to 
            // see flagStopServer set below, and exit.
        }
        cyg_mutex_unlock ( &m_mutQueues );

        // use same shutdown mechanism as listener
		if (cyg_flag_poll(&g_flagStopServer,1,CYG_FLAG_WAITMODE_OR + CYG_FLAG_WAITMODE_CLR))
		{
			break;
		}
    }
}

void GetContentType(char * contentType, const char * filepath)
{
	char extension[4];

	strcpy(contentType,"application/octet-stream");

	int len = strlen(filepath);

	if (len > 4)
	{

		if (filepath[len-4] == '.')
		{
			strncpy(extension,(char*)&filepath[len-3],3);

			if (strcmp(extension,"jpg")==0)
			{
				strcpy(contentType,"image/jpeg");
			}
			else if (strcmp(extension,"gif")==0)
			{
				strcpy(contentType,"image/gif");
			}
			else if (strcmp(extension,"htm")==0)
			{
				strcpy(contentType,"text/html");
			}
			else if (strcmp(extension,"xml")==0)
			{
				strcpy(contentType,"text/xml");
			}
		}
	}
}




void ExternControlResponseHandler(t_ControlReturn crReturnValue, const char * szResponse)
{
	int nSocket = g_ControlSocket;

	char responseHeader[400];

	char defaultResponse[20];

	if (szResponse == NULL)
	{
		sprintf(defaultResponse,"!!%d\n\r",(int)crReturnValue);
	}
	else
	{
		sprintf(defaultResponse,"!!1\n\r");
	}

	int contentLength = (szResponse == NULL ? strlen(defaultResponse) : strlen(szResponse));

	sprintf(responseHeader,
			"HTTP/1.1 200 OK\r\n"
			"Connection: close\r\n"
			"SERVER: FullplayMediaOS\r\n"
			"Content-Length: %d\r\n"
			"Content-Type: text\r\n"
			"\r\n",
			contentLength);

	send(nSocket,responseHeader,strlen(responseHeader),0);

	if (szResponse == NULL)
	{
		send(nSocket,defaultResponse,strlen(defaultResponse),0);
	}
	else
	{
		send(nSocket,"!#",2,0);
		send(nSocket,szResponse,strlen(szResponse),0);
		send(nSocket,"\n\r",2,0);
	}

	close(nSocket);

	cyg_flag_setbits(&g_flagControlCallbackComplete,1);
}

#define TUPPER(c) ( (c >= 'a' && c <= 'z') ? c-'a'+'A' : c )
#define INUMERIC(c) (c>='0'&&c<='9')
#define HTODECIMAL(c) ( INUMERIC(c) ? c-'0' : TUPPER(c)-'A'+10 )

void pathUnescape(const char* in, char* out)
{
    int outlen = 0;
    int inlen = strlen(in);
    for (int i = 0; i < inlen+1; ++i)
    {
        switch (in[i])
        {
            case '%':
            {
                int n = 0;
                // first number
                ++i;
                n <<= 4;
                n += HTODECIMAL(in[i]);
                // second number
                ++i;
                n <<= 4;
                n += HTODECIMAL(in[i]);

                out[outlen++] = (char) n;
                break;
            }
            default:
                out[outlen++] = in[i];
                break;
        }
    }
}

int ProcessXMLControl(int nSocket,const char * path)
{
	g_ControlSocket = nSocket;

	DEBUGP(WEBCONTROLSERVER, DBGLEV_TRACE,"path %s\n",path);

	char * uePath = (char *) malloc(strlen(path));
	pathUnescape(path,uePath);

	ProcessControlRequest(path+1,CONTROL_HTTPGET,ExternControlResponseHandler);

	cyg_flag_wait(&g_flagControlCallbackComplete,1,CYG_FLAG_WAITMODE_OR + CYG_FLAG_WAITMODE_CLR);

	free(uePath);

	return 1;
}


/*
int g_actionSucceeded;
int g_fnIndex;
bool g_expectingCallback;
*/

void WebControlCallback(int cookie)
{
	// this is running on the main thread when it is called

	DBASSERT(WEBCONTROLSERVER,g_expectingCallback,"WebControlCallback call unexpected.");
	DBASSERT(WEBCONTROLSERVER,((g_fnIndex >= 0) && (g_fnIndex < NUM_WEB_FUNCTIONS)),"Function index out of range.");

	// tweak thread priority to avoid audio drop outs
	int nPrio = GetMainThreadPriority();
    SetMainThreadPriority(UI_THREAD_BUSY_PRIORITY);

	char * szResponse = NULL;

	g_actionSucceeded = g_FunctionHandlers[g_fnIndex].pfnHTTPFunction(g_nSocket,g_path);

	SetMainThreadPriority(nPrio);

	cyg_flag_setbits(&g_flagServiceCallbackComplete,1);
}

int ProcessHTTPFunction(int nSocket,const char * path)
{
	// first we must see what function we are doing
	char fname[20];
	memset(fname,0,20);
	int action_succeeded = 0;


	char * endfname = strstr(path,"&");

	DEBUGP(WEBCONTROLSERVER, DBGLEV_TRACE,"endfname %s\n",endfname);

	if (endfname)
	{
		if ((endfname - path) < 20)
		{
			strncpy(fname,path+1,endfname-path-1);
			
			DEBUGP(WEBCONTROLSERVER, DBGLEV_TRACE,"fname case 1 %s\n",fname);
		}
	}
	else
	{
		if (strlen(path) < 20)
		{
			strncpy(fname,path+1,strlen(path)-1);
			fname[endfname-path-1] = '\0';
			DEBUGP(WEBCONTROLSERVER, DBGLEV_TRACE,"fname case 2 %s\n",fname);
		}
		else
		{
			return 0;
		}
	}

	DEBUGP(WEBCONTROLSERVER, DBGLEV_TRACE,"Function name is %s\n",fname);

	int fnIndex = -1;
	int i;

	for (i=0;i<NUM_WEB_FUNCTIONS;i++)
	{
		if (strcmp(fname,g_FunctionHandlers[i].szFunctionName)==0)
		{
			fnIndex = i;
			break;
		}
	}

	if (fnIndex != -1)
	{
		// swap to the main ui thread - request a callback and wait for the callback to complete

		g_fnIndex = fnIndex;
		g_nSocket = nSocket;
		g_path    = path;
		g_expectingCallback = true;

		if (CEventQueue::GetInstance()->FullCount() < 10)
		{
			// send an event to request service in the main thread context
			CEventQueue::GetInstance()->PutEvent((unsigned int)EVENT_WEBCONTROL_SERVICE_REQUEST,0);

			cyg_flag_wait(&g_flagServiceCallbackComplete,1,CYG_FLAG_WAITMODE_OR + CYG_FLAG_WAITMODE_CLR);

			action_succeeded = g_actionSucceeded;
		}
		else
		{
			action_succeeded = 0;
		}

	}

	return action_succeeded;
}



int ProcessHTTPFileRequest(int nSocket, const char * path)
{
	char filepath[100];

	unsigned char scratch[1024];

	strcpy(filepath,"A:/webroot");

	if (path[0] != '/')
	{
		strcat(filepath,"/");
	}

	if (strcmp(path,"/")==0)
	{
		strcat(filepath,"/index.htm");
	}
	else
	{
		strcat(filepath,path);
	}

	int fd = po_open(filepath,PO_RDONLY,PS_IREAD);

	if (fd < 0)
	{
		DEBUGP(WEBCONTROLSERVER, DBGLEV_ERROR,"File not found: %s\n",filepath);
		// no such file
		return 0;
	}

	// now get the file size
	STAT fs;
	pc_fstat(fd,&fs);
	ULONG filesize = fs.st_size;
	// now get the content type

	char contentType[30];

	GetContentType(contentType,filepath);

	DEBUGP(WEBCONTROLSERVER, DBGLEV_TRACE,"Content type is %s\n",contentType);

	char responseHeader[400];

	sprintf(responseHeader,
			"HTTP/1.1 200 OK\r\n"
			"Connection: close\r\n"
			"SERVER: FullplayMediaOS\r\n"
			"Content-Length: %d\r\n"
			"Content-Type: %s\r\n"
			"\r\n",
			filesize, contentType);

	send(nSocket,responseHeader,strlen(responseHeader),0);

	int bytesRead;

	while (bytesRead = po_read(fd,scratch,1024))
	{
		send(nSocket,scratch,bytesRead,0);
	}
	po_close(fd);

	shutdown(nSocket,SD_BOTH);
	close(nSocket);

	return 1;
}



int ProcessHTTPGet(int nSocket,const char * path)
{
	if ((path[0]=='/') && (path[1]=='?'))
	{
		return ProcessHTTPFunction(nSocket,path);
	}
	else if ((path[0]=='/') && (path[1]=='!'))
	{
		return ProcessXMLControl(nSocket,path);
	}
	else
	{
		return ProcessHTTPFileRequest(nSocket,path);
	}
}



int ProcessHTTPPost(int nSocket,const char * path, const char * buf)
{
	// not supporting any post oeprations yet
	return 0;
}



void ProcessHTTP(int nSocket,char *buf)
{

	// look at the HTTP request



	char method[4];
	char path[MAX_PATH];

	memset(method,0,4);
	memset(path,0,MAX_PATH);

	char errorResponse[] = "HTTP/1.1 404 File Not Found\r\n\r\n";

	int result;

	result = 0;

	DEBUGP(WEBCONTROLSERVER, DBGLEV_TRACE,"about to scanf\n");

	if (sscanf(buf,"%s %s HTTP",method,path)==2)
	{
		DEBUGP(WEBCONTROLSERVER, DBGLEV_TRACE,"HTTP method:%s path:%s\n",method,path);

		if (CDJPlayerState::GetInstance()->GetPowerState() != CDJPlayerState::POWER_ON) 
		{
			SendPoweredDownReply(nSocket);
		}
		else
		{
			if (strcmp(method,"GET")==0)
			{
				result = ProcessHTTPGet(nSocket, path);
			}
			else if (strcmp(method,"POST")==0)
			{
				result = ProcessHTTPPost(nSocket,path,buf);
			}
	
			if (result != 1)
			{
				send(nSocket,errorResponse,strlen(errorResponse),0);
				shutdown(nSocket,SD_BOTH);
				close(nSocket);
			}
		}
	}
	else
	{
		DEBUGP(WEBCONTROLSERVER, DBGLEV_ERROR,"scanf failed, closing nSocket\n");
		shutdown(nSocket,SD_BOTH);
		close(nSocket);
	}

}
        
 