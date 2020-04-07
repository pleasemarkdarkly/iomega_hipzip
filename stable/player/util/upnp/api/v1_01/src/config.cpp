///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000 Intel Corporation 
// All rights reserved. 
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met: 
//
// * Redistributions of source code must retain the above copyright notice, 
// this list of conditions and the following disclaimer. 
// * Redistributions in binary form must reproduce the above copyright notice, 
// this list of conditions and the following disclaimer in the documentation 
// and/or other materials provided with the distribution. 
// * Neither name of Intel Corporation nor the names of its contributors 
// may be used to endorse or promote products derived from this software 
// without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR 
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////
//
// $Revision: 1.18 $
// $Date: 2000/10/06 01:14:18 $
//

#include <util/upnp/api/config.h>

#include <stdlib.h>
#include <stdio.h>

cyg_mutex_t GlobalDebugMutex;

#include <util/upnp/api/upnp.h>

#include <util/debug/debug.h>
#include <stdarg.h>

DEBUG_MODULE_S(UPNP, DBGLEV_DEFAULT);
DEBUG_USE_MODULE(UPNP);

DEBUG_MODULE_S(DEBUG_UPNP_SSDP, DBGLEV_DEFAULT | DBGLEV_INFO | DBGLEV_CHATTER);
//DEBUG_MODULE_S(DEBUG_UPNP_SSDP, DBGLEV_DEFAULT);
DEBUG_USE_MODULE(DEBUG_UPNP_SSDP);
DEBUG_MODULE_S(DEBUG_UPNP_SOAP, DBGLEV_DEFAULT | DBGLEV_INFO | DBGLEV_CHATTER);
//DEBUG_MODULE_S(DEBUG_UPNP_SOAP, DBGLEV_DEFAULT);
DEBUG_USE_MODULE(DEBUG_UPNP_SOAP);
DEBUG_MODULE_S(DEBUG_UPNP_GENA, DBGLEV_DEFAULT);
DEBUG_USE_MODULE(DEBUG_UPNP_GENA);
DEBUG_MODULE_S(DEBUG_UPNP_TPOOL, DBGLEV_DEFAULT);
DEBUG_USE_MODULE(DEBUG_UPNP_TPOOL);
DEBUG_MODULE_S(DEBUG_UPNP_MSERV, DBGLEV_DEFAULT);
DEBUG_USE_MODULE(DEBUG_UPNP_MSERV);
DEBUG_MODULE_S(DEBUG_UPNP_DOM, DBGLEV_DEFAULT);
DEBUG_USE_MODULE(DEBUG_UPNP_DOM);
DEBUG_MODULE_S(DEBUG_UPNP_API, DBGLEV_DEFAULT);
DEBUG_USE_MODULE(DEBUG_UPNP_API);


FILE *ErrFileHnd=NULL;
FILE *InfoFileHnd=NULL;


void UpnpDisplayBanner(FILE * fd, char **lines, int size, int starLength);

int InitLog()
{
 #ifdef _WIN32
 GlobalDebugMutex = CreateMutex(NULL,FALSE,NULL);
 if (GlobalDebugMutex == NULL) return UPNP_E_INVALID_HANDLE ;
 #endif
#ifndef USE_PTHREADS
	cyg_mutex_init(&GlobalDebugMutex);
#endif


#ifdef USE_FILESYSTEM
	if ( (ErrFileHnd  = fopen ("ErrFileName.txt" , "a")) == NULL) return -1;
	if ( (InfoFileHnd = fopen ("InfoFileName.txt", "a")) == NULL) return -1;
#endif

 DEBUGP(UPNP, DBGLEV_INFO, "UPNP_E_SUCCESS: %d\n", UPNP_E_SUCCESS);

 return UPNP_E_SUCCESS;
}

void CloseLog()
{
#ifdef USE_FILESYSTEM
  fclose(ErrFileHnd);
  fclose(InfoFileHnd);
#endif
}



#if DEBUG_LEVEL != 0

void UpnpPrintf(Dbg_Level DLevel, Dbg_Module Module,char *DbgFileName, int DbgLineNo,char * FmtStr, ... )
{
	va_list ArgList;
	va_start(ArgList, FmtStr);

#ifdef USE_PTHREADS
	pthread_mutex_lock(&GlobalDebugMutex);
#else
	cyg_mutex_lock(&GlobalDebugMutex);
#endif

    char chSdkDbgLev = DBGLEV_OFF;
    if (DLevel == UPNP_CRITICAL)
        chSdkDbgLev = DBGLEV_ERROR;
    else if (DLevel == UPNP_PACKET)
        chSdkDbgLev = DBGLEV_CHATTER;
    else if (DLevel == UPNP_INFO)
        chSdkDbgLev = DBGLEV_INFO;
    else if (DLevel == UPNP_ALL)
        chSdkDbgLev = DBGLEV_TRACE;

static char s_szBuffer[4096];
	vsnprintf(s_szBuffer, 4095, FmtStr, ArgList);

    switch(Module)
    {
        case SSDP:
            DEBUGP(DEBUG_UPNP_SSDP, chSdkDbgLev, "%s:%d: %s\n", DbgFileName, DbgLineNo, s_szBuffer);
            break;
        case SOAP:
            DEBUGP(DEBUG_UPNP_SOAP, chSdkDbgLev, "%s:%d: %s\n", DbgFileName, DbgLineNo, s_szBuffer);
            break;
        case GENA:
            DEBUGP(DEBUG_UPNP_GENA, chSdkDbgLev, "%s:%d: %s\n", DbgFileName, DbgLineNo, s_szBuffer);
            break;
        case TPOOL:
            DEBUGP(DEBUG_UPNP_TPOOL, chSdkDbgLev, "%s:%d: %s\n", DbgFileName, DbgLineNo, s_szBuffer);
            break;
        case MSERV:
            DEBUGP(DEBUG_UPNP_MSERV, chSdkDbgLev, "%s:%d: %s\n", DbgFileName, DbgLineNo, s_szBuffer);
            break;
        case DOM:
            DEBUGP(DEBUG_UPNP_DOM, chSdkDbgLev, "%s:%d: %s\n", DbgFileName, DbgLineNo, s_szBuffer);
            break;
        case API:
            DEBUGP(DEBUG_UPNP_API, chSdkDbgLev, "%s:%d: %s\n", DbgFileName, DbgLineNo, s_szBuffer);
            break;
        default:
            break;
    }

	va_end(ArgList);
#ifdef USE_PTHREADS
	pthread_mutex_unlock(&GlobalDebugMutex);
#else
	cyg_mutex_unlock(&GlobalDebugMutex);
#endif
   
}


#endif


DBGONLY(
FILE * GetDebugFile(Dbg_Level DLevel, Dbg_Module Module)
{
    return 0;
}
)

DBGONLY(

void UpnpDisplayFileAndLine(FILE *fd,char * DbgFileName, int DbgLineNo, char dbglev = 0)
{
   int starlength=66;
   char *lines[2];
   char FileAndLine[500]; 
   lines[0]="DEBUG";
   if (DbgFileName)
     {
      sprintf(FileAndLine,"FILE: %s, LINE: %d",DbgFileName,DbgLineNo); 
      lines[1]=FileAndLine;
     }
    UpnpDisplayBanner(fd,lines,2,starlength,dbglev);
}
)

DBGONLY(

void UpnpDisplayBanner(FILE * fd, char **lines, int size, int starLength, char dbglev = 0)
{
  char *stars= (char *) malloc(starLength+1);
 
 
  char *line =NULL;
  int leftMarginLength =  starLength/2 + 1; 
  int rightMarginLength = starLength/2 + 1;
  char *leftMargin = (char *) malloc (leftMarginLength);
  char *rightMargin = (char *) malloc (rightMarginLength);
  
  int i=0;
  int LineSize=0;
  char *currentLine=(char *) malloc (starLength+1);

  memset(stars,'*',starLength);
  stars[starLength]=0;
  memset( leftMargin, 0, leftMarginLength);
  memset( rightMargin, 0, rightMarginLength);

  DEBUGP(UPNP, dbglev, "\n%s\n",stars);
  for (i=0;i<size;i++)
    {
      LineSize=strlen(lines[i]);
      line=lines[i];
      while(LineSize>(starLength-2))
	{ 
	  memcpy(currentLine,line,(starLength-2));
	  currentLine[(starLength-2)]=0;
	  DEBUGP(UPNP, dbglev, "*%s*\n",currentLine);
	  LineSize-=(starLength-2);
	  line+=(starLength-2);
	}
      if (LineSize % 2 == 0)
      {
         leftMarginLength = rightMarginLength = ((starLength-2)-LineSize)/2;
      }
      else
      {
         leftMarginLength = ((starLength-2)-LineSize)/2;
         rightMarginLength= ((starLength-2)-LineSize)/2+1;
      }		

      memset(leftMargin, ' ', leftMarginLength);
      memset(rightMargin, ' ', rightMarginLength);
      leftMargin[leftMarginLength] = 0;
      rightMargin[rightMarginLength] =0;

        DEBUGP(UPNP, dbglev, "*%s%s%s*\n",leftMargin, line, rightMargin);
    }
  DEBUGP(UPNP, dbglev, "%s\n\n",stars);
  free (leftMargin);
  free (rightMargin);
  free (stars);
  free( currentLine);
})

