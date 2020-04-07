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
// $Revision: 1.6 $
// $Date: 2000/10/05 16:40:26 $

#include <util/upnp/api/config.h>

#if EXCLUDE_SSDP == 0
#include <util/upnp/protocols/ssdplib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <util/upnp/genlib/http_client.h>

#include <util/upnp/genlib/mystring.h>


static char Token_List[][25] = {"CACHE-CONTROL","ST","LOCATION","HOST","USN","NT","NTS","EXT","SERVER","MAN","MX","DATE"};
static ParserFun FunList[NUM_TOKEN];


char * StrTok(char ** Src, char * Del)
{
   char * TmpPtr, *RetPtr;

   if(*Src != NULL)
   {
      RetPtr = *Src;
      TmpPtr = strstr(*Src,Del);
      if(TmpPtr != NULL)
      {
         *TmpPtr = '\0';
         *Src = TmpPtr+ strlen(Del);
      }
      else *Src = NULL;

      return RetPtr;
   }

   return NULL;
}


////////////////////////////////////////////////////////////////////////////////////////////////
// Function    : CheckHdr(char * Cmd,Event * Evt)
// Description : This function checks  the first statement in the  HTTP packet.
// Parameters  : Cmd : HTTP string.
//               Evt : Event structure defind in interface.h
// Return value:
///////////////////////////////////////////////////////////////////////////////////////////////
int CheckHdr(char * Cmd,Event * Evt)
{
  char Tmp[COMMAND_LEN],Seps[] = " ",*Token,*TmpPtr;

  TmpPtr =Tmp;
  strcpy(TmpPtr,Cmd);

  Token = StrTok((char **)&TmpPtr, Seps );  //Should be "HTTP" here


  if(strstr(Token,"M-SEARCH") != NULL)  //This command is for service
  {
      Token = StrTok((char **)&TmpPtr, Seps ); //Should be "*" here
      if(strstr(Token,"*") == NULL) 
      {
         Evt->ErrCode =  E_HTTP_SYNTEX ;
         return -1;
      }
      else
      {
          Token = StrTok((char **)&TmpPtr, Seps ); //Should be "HTTP" here
          if( strstr(Token,"HTTP") == NULL) 
          {
             Evt->ErrCode =  E_HTTP_SYNTEX ;
             return -1;
          }  else Evt->Cmd = SSDP_SEARCH;

      }

  } 
  else if (strstr(Token,"NOTIFY") != NULL) //This is for Client
  {

      Token = StrTok((char **)&TmpPtr, Seps ); //Should be "*" here
      if(strstr(Token,"*") == NULL)
      {
           Evt->ErrCode = E_HTTP_SYNTEX;
           return -1;

       }

      else
      {
         Token = StrTok((char **)&TmpPtr, Seps ); //Should be "OK" here
         if( strstr(Token,"HTTP") == NULL)
         {

             Evt->ErrCode = E_HTTP_SYNTEX;
             return -1;
         }

      }

      Evt->Cmd = SSDP_NOTIFY;

  }
  else if(strstr(Token,"HTTP") != NULL) //This is for Client
  {

      Token = StrTok((char **)&TmpPtr, Seps ); //Should be "*" here
      if(strstr(Token,"200") == NULL)
      {
            Evt->ErrCode = E_HTTP_SYNTEX;
            return -1;
      }
      else
      {
         Token = StrTok((char **)&TmpPtr, Seps ); //Should be "OK" here
         if( strstr(Token, "OK") != NULL)
         {
             Evt->Cmd = SSDP_OK;
             return 1;
         }
         else
         {
             Evt->ErrCode = E_HTTP_SYNTEX;
             return -1;
         }
      }
  }
  else 
  {
      Evt->ErrCode = E_HTTP_SYNTEX;
      return -1;
  }
  
  return 1;

}




////////////////////////////////////////////////////////////////////////////////////////////////
// Function    : Cache_Control(char * Cmd,Event * Evt)
// Description : This function retrieves the Maximum-Age for the resources.
// Parameters  : Cmd : Cache_Control HTTP string.
//               Evt : Event structure defind in ssdplib.h, partially filled by all the parsing function.
// Return value: 1 if True -1 if false.
///////////////////////////////////////////////////////////////////////////////////////////////

int Cache_Control(char * cmd,Event * Evt)
{
   char *Token;

   Token = StrTok((char **)&cmd,"=");
   if(Token != NULL)
   {

      Token = StrTok((char **)&cmd,"=" );
      if (Token != NULL) Evt->MaxAge = atoi(Token);
      else  return -1 ;

   }
  else
  {
      Evt->ErrCode = E_HTTP_SYNTEX;
      return -1;
  }

   return 1;
}




////////////////////////////////////////////////////////////////////////////////////////////////
// Function    : Location_Header(char * Cmd,Event * Evt)
// Description : This function retrieves the location information(URL)  for the resources.
// Parameters  : Cmd : Location header HTTP string.
//               Evt : Event structure defind in ssdplib.h, partially filled by all the parsing function.
// Return value: 1 if True -1 if false.
///////////////////////////////////////////////////////////////////////////////////////////////

int Location_Header(char * cmd,Event * Evt)
{
  
  if( cmd != NULL && strlen(cmd) >7)
  {
     strcpy(Evt->Location,cmd);

  }
  else
  {
      Evt->ErrCode = E_HTTP_SYNTEX;
      return -1;
  }


  return 1;
  
}



////////////////////////////////////////////////////////////////////////////////////////////////
// Function    : Os(char * Cmd,Event * Evt)
// Description : This function retrieves the OS info.
// Parameters  : Cmd : Request Id HTTP string.
//               Evt : Event structure defind in ssdplib.h, partially filled by all the parsing function.
// Return value: 1 if True -1 if false.
///////////////////////////////////////////////////////////////////////////////////////////////
int Os(char * cmd,Event * Evt)
{
  if(cmd != NULL && strlen(cmd) > 0)
  {
     strcpy(Evt->Os,cmd);
  }
  else
  {
      Evt->ErrCode = E_HTTP_SYNTEX;
      return -1;
  }

  return 1;

}




////////////////////////////////////////////////////////////////////////////////////////////////
// Function    : Unique_Service_Name(char * Cmd,Event * Evt)
// Description : This function retrieves the name of resource.
// Parameters  : Cmd : Service Name string.
//               Evt : Event structure defind in ssdplib.h, partially filled by all the parsing function.
// Return value: 1 if True -1 if false.
///////////////////////////////////////////////////////////////////////////////////////////////
int Unique_Service_Name(char * cmd, Event * Evt)
{
  char  *TempPtr, TempBuf[COMMAND_LEN], *Ptr,*ptr1,*ptr2,*ptr3, CommandFound = 0;

   if((TempPtr=strstr(cmd,"uuid:schemas")) != NULL)
   {

      ptr1= strstr(cmd,":device");
      if(ptr1!= NULL)
      ptr2= strstr(ptr1+1,":");
      else return -1;

      if(ptr2!= NULL)
      ptr3= strstr(ptr2+1,":");
      else return -1;

      if(ptr3!= NULL)
      sprintf(Evt->UDN,"uuid:%s",ptr3+1);
      else return -1;

      ptr1 = strstr(cmd,":");
      if(ptr1!= NULL)
      {
        strncpy(TempBuf,ptr1,ptr3-ptr1);

        TempBuf[ptr3-ptr1] = '\0';
        sprintf(Evt->DeviceType,"urn%s",TempBuf);
      }
      else return -1;
      return 1;

   }

   if((TempPtr=strstr(cmd,"uuid")) != NULL)
   {
      //printf("cmd = %s\n",cmd);
      if( ( Ptr = strstr(cmd,"::") ) != NULL)
      {
         strncpy(Evt->UDN,TempPtr, Ptr-TempPtr);
         Evt->UDN[Ptr-TempPtr] ='\0';
      }
      else strcpy(Evt->UDN,TempPtr);
      CommandFound  = 1;
   }

   if(strstr(cmd,"urn:") != NULL && strstr(cmd,":service:") != NULL )
   {

      if((TempPtr=strstr(cmd,"urn")) != NULL )
      {
         strcpy(Evt->ServiceType,TempPtr);
         CommandFound  = 1;
      }
   }


   if(strstr(cmd,"urn:") != NULL && strstr(cmd,":device:") != NULL)
   {
      if((TempPtr=strstr(cmd,"urn")) != NULL )
      {
         strcpy(Evt->DeviceType,TempPtr);
         CommandFound  = 1;
      }
   }

   if(  CommandFound  == 0)
   {

      return -1;
   }


  return 1;

}





////////////////////////////////////////////////////////////////////////////////////////////////
// Function    : Request_Tpye(char * Cmd,Event * Evt)
// Description : This function retrieves the request Type info
// Parameters  : Cmd : Service Name string.
//               Evt : Event structure defind in ssdplib.h, partially filled by all the parsing function.
// Return value: 1 if True -1 if false.
///////////////////////////////////////////////////////////////////////////////////////////////

int Request_Tpye(char * cmd, Event * Evt)
{

   Unique_Service_Name(cmd,Evt);
   Evt->ErrCode = NO_ERROR_FOUND;
   if(strstr(cmd,":all") != NULL) Evt->RequestType = SSDP_ALL;
   else if(strstr(cmd,":rootdevice") != NULL) Evt->RequestType = SSDP_ROOTDEVICE;
   else if(strstr(cmd,"uuid:") != NULL) Evt->RequestType = SSDP_DEVICE;
   else if(strstr(cmd,"urn:") != NULL && strstr(cmd,":device:") != NULL) Evt->RequestType = SSDP_DEVICE;
   else if(strstr(cmd,"urn:") != NULL && strstr(cmd,":service:") != NULL ) Evt->RequestType = SSDP_SERVICE;
   else
   {
      Evt->ErrCode = E_HTTP_SYNTEX;
      return -1;
   }

   return 1;
}


////////////////////////////////////////////////////////////////////////////////////////////////
// Function    : Host(char * Cmd,Event * Evt)
// Description : This function retrieves the Host information for the resources.
// Parameters  : Cmd : Host  string.
//               Evt : Event structure defind in ssdplib.h, partially filled by all the parsing function.
// Return value: 1 if True -1 if false.
///////////////////////////////////////////////////////////////////////////////////////////////

int Host(char * cmd, Event * Evt)
{

  if(cmd != NULL && strlen(cmd) > 0)
  {
     strcpy(Evt->HostAddr,cmd);
  }
  else
  {
      Evt->ErrCode = E_HTTP_SYNTEX;
      return -1;
  }

  return 1;
}



////////////////////////////////////////////////////////////////////////////////////////////////
// Function    : Notification_Sub_Type(char * Cmd,Event * Evt)
// Description : This function retrieves the notification type from  the resources.
// Parameters  : Cmd : LocationNotification header HTTP string.
//               Evt : Event structure defind in ssdplib.h, partially filled by all the parsing function.
// Return value: 1 if True -1 if false.
///////////////////////////////////////////////////////////////////////////////////////////////

int Notification_Sub_Type(char * cmd, Event * Evt)
{
  char *Token;
  Token = StrTok((char **)&cmd,":");
  Token = StrTok((char **)&cmd,":" );
  if((strcasecmp ("alive",Token)) == 0)
  {
    Evt->Cmd = SSDP_ALIVE;
  }
  else if((strcasecmp ("byebye",Token)) == 0)
  {
      Evt->Cmd = SSDP_BYEBYE;
  }
  else
  {
      Evt->ErrCode = E_HTTP_SYNTEX;
      return -1;
  }


 return 1;
}


////////////////////////////////////////////////////////////////////////////////////////////////
// Function    : Max_Delay(char * Cmd,Event * Evt)
// Description : This function retrieves the delay information for the resources. Each resources are
//               supposed to wait for random(Evt->MX ) sec  before replying for the any query.
// Parameters  : Cmd : Delay command string.
//               Evt : Event structure defind in ssdplib.h, partially filled by all the parsing function.
// Return value: 1 if True -1 if false.
///////////////////////////////////////////////////////////////////////////////////////////////


 int Max_Delay(char * cmd, Event * Evt)
{

  char *Token;

   if(cmd != NULL)
   {
      Token = StrTok((char **)&cmd,":");
      Evt->Mx = atoi(Token);
      return 1;
   }
   else
   {
      Evt->ErrCode =  E_HTTP_SYNTEX ;
      return -1;
   }


}



////////////////////////////////////////////////////////////////////////////////////////////////
// Function    : Fun_EXT(char * Cmd,Event * Evt)
// Description : Not used
// Parameters  : Cmd : Extension  HTTP string.
//               Evt : Event structure defind in ssdplib.h, partially filled by all the parsing function.
// Return value: 1 if True -1 if false.
///////////////////////////////////////////////////////////////////////////////////////////////

int Fun_Ext(char * cmd,Event * Evt)
{
  if(cmd != NULL && strlen(cmd) > 0)
  {
     strcpy(Evt->Ext,cmd);
  }
  return 1;
}



////////////////////////////////////////////////////////////////////////////////////////////////
// Function    : int Fun_MAN(char * cmd, Event * Evt)
// Description : This function retrieves the man information for the resources.
// Parameters  : Cmd : Delay command string.
//               Evt : Event structure defind in ssdplib.h, partially filled by all the parsing function.
// Return value: 1 if True -1 if false.
///////////////////////////////////////////////////////////////////////////////////////////////

int Fun_MAN(char * cmd, Event * Evt)
{

  return 1;

}


////////////////////////////////////////////////////////////////////////////////////////////////
// Function    : int Fun_Date char * cmd, Event * Evt)
// Description : This function retrieves the man information for the resources.
// Parameters  : Cmd : Delay command string.
//               Evt : Event structure defind in ssdplib.h, partially filled by all the parsing function.
// Return value: 1 if True -1 if false.
///////////////////////////////////////////////////////////////////////////////////////////////

int Fun_Date (char * cmd, Event * Evt)
{

  if(cmd != NULL && strlen(cmd) > 0)
  {
     strcpy(Evt->Date,cmd);
  }
  else
  {
      Evt->ErrCode = E_HTTP_SYNTEX;
      return -1;
  }

  return 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Function    : void InitParser()
// Description : This function initializes the list of callback parser function.
// Parameters  :
//               None
// Return value: None
///////////////////////////////////////////////////////////////////////////////////////////////

void InitParser()
{

      FunList[0]=Cache_Control;
      FunList[1]=Request_Tpye; //ST
      FunList[2]=Location_Header;
      FunList[3]=Host;
      FunList[4]=Unique_Service_Name; //USN
      FunList[5]=Request_Tpye;  // Same as ST , Notification Tpye
      FunList[6]=Notification_Sub_Type;
      FunList[7]=Fun_Ext;
      FunList[8]=Os;
      FunList[9]=Fun_MAN;
      FunList[10]=Max_Delay;
      FunList[11]=Fun_Date;


}



////////////////////////////////////////////////////////////////////////////////////////////////
// Function    : void InitEvent(Event * Evt)
// Description : This function initializes the event structure.
// Parameters  :
//               Evt : Event structure defind in ssdplib.h, partially filled by all the parsing function.
// Return value: None
///////////////////////////////////////////////////////////////////////////////////////////////

void InitEvent(Event * Evt)
{


      Evt->ErrCode=0;
      Evt->MaxAge=0;
      Evt->Mx=0;
//      Evt->Cmd=SERROR;
      Evt->Cmd=SSDP_ERROR;	// - ecm
      Evt->RequestType=SSDP_SERROR;
      strcpy(Evt->UDN,"");
      strcpy(Evt->DeviceType,"");
      strcpy(Evt->ServiceType,"");
      strcpy(Evt->Location,"");
      strcpy(Evt->HostAddr,"");
      strcpy(Evt->Os,"");
      strcpy(Evt->Ext,"");
      strcpy(Evt->Date,"");

}




////////////////////////////////////////////////////////////////////////////////////////////////
// Function    : int AnalyzeCommand(char * szCommand, Event * Evt)
// Description : This is the main function called by ssdp for parsing. It check for the type of
//               token available in the HTTP header and calls the specific callback function for
//               further parsing.
// Parameters  : szCommand : HTTP header string.
//               Evt : Event structure defind in ssdplib.h, partially filled by all the parsing function.
// Return value: 1 if True -1 if false.
///////////////////////////////////////////////////////////////////////////////////////////////

int AnalyzeCommand(char * szCommand, Event * Evt)
{
   int  Idx,I,NumCommand=0, RetVal=1;
   char* TmpBuff,*TmpPtr,Seps[] = "\r\n" ,*Token,*Key,*Cmd;
   char Command_List[NUM_TOKEN][COMMAND_LEN];

   if (szCommand == NULL || strlen( szCommand) <= 0)   return -1;
   if(Evt == NULL) return -1;
   DBGONLY(UpnpPrintf(UPNP_PACKET,SSDP,__FILE__,__LINE__,"Received new packet for parsing.\n");)
 
   TmpBuff= (char *) malloc(strlen(szCommand)+2);
   TmpPtr = TmpBuff;
   InitEvent(Evt);

   strcpy(TmpPtr,szCommand);
   Token = StrTok((char **)&TmpPtr, Seps );
   while( Token != NULL )
   {
      strcpy(Command_List[NumCommand++],Token);
      Token = StrTok((char **)&TmpPtr, Seps );
   }

   strcpy(Seps,":");

   if (CheckHdr(Command_List[0],Evt))
   {
      for (I=1;I <NumCommand;I++)
      {
         TmpPtr = TmpBuff;
         strcpy(TmpBuff,Command_List[I]);
         Cmd = strchr(Command_List[I], ':');
         Token = StrTok((char **)&TmpPtr,Seps);

         for(Idx=0;Idx < NUM_TOKEN;Idx++)
         {
            Key = Token_List[Idx];
            if((strcasecmp (Key,Token)) == 0) //strcasecmp
            {

               if( (RetVal= FunList[Idx](Cmd+1,Evt)) < 0)
               {
                 DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Found error !!!! while parsing for Token  = \n %s \n",Token);)
                 free(TmpBuff);
                 return -1;
               }
               break;
            }
         }
      }
   }else RetVal= -1;

   DBGONLY(UpnpPrintf(UPNP_PACKET,SSDP,__FILE__,__LINE__,"Command Type=  %d\nRequestType = %d\nErrCode = %d\nMaxAge = %d\nMx = %d\nDeviceType = %s\nUDN = %s\nServiceType = %s\nLocation = %s\nHostAddr = %s\n",Evt->Cmd,Evt->RequestType,Evt->ErrCode,Evt->MaxAge,Evt->Mx,Evt->DeviceType,Evt->UDN,Evt->ServiceType,Evt->Location,Evt->HostAddr);)
   free(TmpBuff);
   return RetVal;
}

#endif
