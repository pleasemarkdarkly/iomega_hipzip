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
// $Revision: 1.55 $
// $Date: 2000/10/06 01:14:48 $
//     

//Utility functions for url parsing
//and connecting to an http server as a client

#include <util/upnp/genlib/http_client.h>
#include <util/upnp/genlib/mystring.h>

#include <arpa/inet.h>
#include <netinet/in.h>

//*************************************************************************
//* Name: copy_sockaddr_in
//*
//* Description:  Copies one socket address into another.
//*
//* In:           struct sockaddr_in *in, struct sockadd_in *out 
//*
//* Out:          The fields of out are set the same as in.
//*
//* Return Codes: None
//* Error Codes:  None               
//*************************************************************************
void copy_sockaddr_in(const struct sockaddr_in *in, struct sockaddr_in *out)
{
    //  bzero(out->sin_zero,8);
    memset(out->sin_zero,0,8);
    out->sin_family=in->sin_family;
    out->sin_port=in->sin_port;
    out->sin_addr.s_addr=in->sin_addr.s_addr;
}


//*************************************************************************
//* Name: copy_token
//*
//* Description:  Tokens are generally pointers into other strings
//*               this copies the offset and size from a token (in)
//*               relative to one string (in_base) into a token (out)
//*               relative to another string (out_base)
//*
//* In:           char * in_base, token * in, char * out_base 
//*
//* Out:          The fields of out are set relative to out_base.
//*
//* Return Codes: None 
//* Error Codes:  None               
//*************************************************************************
void copy_token(const token *in, const char * in_base, token * out, char * out_base)
{
    out->size=in->size;
    out->buff = out_base + (in->buff-in_base);
}

//*************************************************************************
//* Name: copy_URL_list
//*
//* Description:  Copies one URL_list into another.
//*               This includes dynamically allocating the out->URLs 
//*               field (the full string),
//*               and the structures used to hold the parsedURLs.
//*               This memory MUST be freed by the caller through: 
//*               free_URL_list(&out)
//*
//* In:           URL_list *in, URL_list *out
//*
//* Out:          out->URLs contains a newly allocated copy of in->URls
//*               out->parsedURLs contains a newly allocated copy of 
//*               out->parsedURls
//*
//* Return Codes: HTTP_SUCCESS 
//* Error Codes:  UPNP_E_OUTOF_MEMORY       
//*************************************************************************
int copy_URL_list( URL_list *in, URL_list *out)
{
    int len = strlen(in->URLs)+1;
    int i=0;

    out->URLs=NULL;
    out->parsedURLs=NULL;
    out->size=0;

    out->URLs= (char *) malloc (len);
    out->parsedURLs= (uri_type *) malloc (sizeof(uri_type) * in->size);

    if ( (out->URLs==NULL) || (out->parsedURLs==NULL))
        return UPNP_E_OUTOF_MEMORY;
  
    memcpy(out->URLs,in->URLs,len);
  
    for (i=0;i<in->size;i++)
    {
        //copy the parsed uri
        out->parsedURLs[i].type=in->parsedURLs[i].type;
        copy_token(&in->parsedURLs[i].scheme,in->URLs,
            &out->parsedURLs[i].scheme, out->URLs);
      
        out->parsedURLs[i].path_type=in->parsedURLs[i].path_type;
        copy_token(&in->parsedURLs[i].pathquery,in->URLs,
            &out->parsedURLs[i].pathquery,out->URLs);
        copy_token(&in->parsedURLs[i].fragment,in->URLs,
            &out->parsedURLs[i].fragment,out->URLs);
        copy_token(&in->parsedURLs[i].hostport.text,
            in->URLs,&out->parsedURLs[i].hostport.text,
            out->URLs);
      
        copy_sockaddr_in(&in->parsedURLs[i].hostport.IPv4address,
            &out->parsedURLs[i].hostport.IPv4address);
    }
    out->size=in->size;
    return HTTP_SUCCESS;

}

//*************************************************************************
//* Name: free_URL_list
//*
//* Description:  Frees the memory associated with a URL_list.
//*               Frees the dynamically allocated members of of list.
//*               Does NOT free the pointer to the list itself 
//*               ( i.e. does NOT free(list))
//* In:           URL_list *list
//*
//* Out:          list->URLs is freed 
//*               list->parsedURLs is freed
//*
//* Return Codes: None
//* Error Codes:  None       
//*************************************************************************  
void free_URL_list(URL_list * list)
{
    if (list->URLs)
        free(list->URLs);
    if (list->parsedURLs)
        free(list->parsedURLs);
    list->size=0;
}


//*************************************************************************
//* Name: print_uri
//*
//* Description:  Function useful in debugging for printing a parsed uri.
//*               Compiled out with DBGONLY macro. 
//* In:           uri_type *in
//*
//* Out:          prints the tokens making up a uri to the screen.
//*
//* Return Codes: None
//* Error Codes:  None       
//*************************************************************************
DBGONLY(
    void print_uri( uri_type *in, Dbg_Level level, Dbg_Module Module,
        char *DbgFileName,int DbgLineNo)
{
    FILE *fd = GetDebugFile(level,Module);
    if (!fd)
        return;
    if (DbgFileName)
    {
        //get lock
        cyg_mutex_lock(&GlobalDebugMutex);
        UpnpDisplayFileAndLine(fd,DbgFileName,DbgLineNo);
    }

    fprintf(fd,"  Scheme: ");
    print_token(&in->scheme,level,Module,NULL,0);
    fprintf(fd,"  Host/Port: ");
    print_token(&in->hostport.text,level,Module,NULL,0);
    fprintf(fd,"  PathQuery: ");
    print_token(&in->pathquery,level,Module,NULL,0);
    fprintf(fd,"  Fragment: ");
    print_token(&in->fragment,level,Module,NULL,0);
    if (DbgFileName)
    {
        cyg_mutex_unlock(&GlobalDebugMutex);
    }
})

    //*************************************************************************
    //* Name: print_token
    //*
    //* Description:  Function useful in debugging for printing a token.
    //*               Compiled out with DBGONLY macro. 
    //* In:           token *in
    //*
    //* Out:          prints the token. ("Token Size: %d\n (then actual token 
    //*               between ' '\n)
    //*
    //* Return Codes: None
    //* Error Codes:  None       
    //*************************************************************************
    DBGONLY(void print_token(token * in,Dbg_Level level, Dbg_Module Module,
                char *DbgFileName, int DbgLineNo)
    {
        int i=0;
        FILE *fd = GetDebugFile(level,Module);
        if (!fd)
            return;
        if (DbgFileName)
        {
            //get lock
            cyg_mutex_lock(&GlobalDebugMutex);
            UpnpDisplayFileAndLine(fd,DbgFileName,DbgLineNo);
        }

        fprintf(fd,"Token Size : %d \'",in->size);
        for (i=0;i<in->size;i++)
        {
            fprintf(fd,"%c",in->buff[i]);
        }
        fprintf(fd,"%c",'\'');
        fprintf(fd,"%c",'\n');
        if (DbgFileName)
        {
            cyg_mutex_unlock(&GlobalDebugMutex);
        }
    })

    //*************************************************************************
    //* Name: token_string_casecmp
    //*
    //* Description:  Compares a null terminated string to a token (irrespective of case)
    //*
    //* In:           token * in1,  char * in2
    //*
    //* Out:          returns 0 if equal
    //*
    //* Return Codes: 0 if equal
    //* Error Codes:  None       
    //*************************************************************************
    int token_string_casecmp( token * in1,  char * in2)
{
int in2_length=strlen(in2);
if (in1->size!=in2_length)
    return 1;
else
return strncasecmp(in1->buff,in2,in1->size); 
}

//*************************************************************************
//* Name: token_string_cmp
//*
//* Description:  Compares a null terminated string to a token (exact)
//*
//* In:           token *in1, char *in2
//*
//* Out:          0 if equal
//*
//* Return Codes: 0 if equal
//* Error Codes:  None       
//*************************************************************************
int token_string_cmp( token * in1,  char * in2)
{
    int in2_length=strlen(in2);
    if (in1->size!=in2_length)
        return 1;
    else
        return strncmp(in1->buff,in2,in1->size);
}

//*************************************************************************
//* Name: token_cmp
//*
//* Description:  Compares to tokens.
//*
//* In:           token *in1, token *in2
//*
//* Out:          0 if equal
//*
//* Return Codes: 0 if equal
//* Error Codes:  None       
//*************************************************************************
int token_cmp( token *in1,  token *in2)
{
  

    if (in1->size!=in2->size)
        return 1;
    else
        return memcmp(in1->buff,in2->buff,in1->size); 
}



//*************************************************************************
//* Name: is_reserved
//*
//* Description:  Returns a 1 if a char is a RESERVED char
//*               as defined in http://www.ietf.org/rfc/rfc2396.txt
//*               (RFC explaining URIs)
//*
//* In:           char in
//*
//* Out:          1 if reserved, 0 if not
//*
//* Return Codes: None
//* Error Codes:  None       
//*************************************************************************
int is_reserved(char in)
{
    if (strchr(RESERVED,in))
        return 1;
    else 
        return 0;
}

//*************************************************************************
//* Name: is_mark
//*
//* Description:  Returns a 1 if a char is a MARK char
//*               as defined in http://www.ietf.org/rfc/rfc2396.txt
//*               (RFC explaining URIs)
//*
//* In:           char in
//*
//* Out:          1 if a Mark, O if not
//*
//* Return Codes: None
//* Error Codes:  None       
//*************************************************************************
int is_mark(char in )
{
    if (strchr(MARK,in))
        return 1;
    else
        return 0;
}

//*************************************************************************
//* Name: is_unreserved
//*
//* Description:  Returns a 1 if a char is an unreserved char
//*               as defined in http://www.ietf.org/rfc/rfc2396.txt
//*               (RFC explaining URIs)
//*
//* In:           char in
//*
//* Out:          1 if unreserved, O if not
//*
//* Return Codes: None
//* Error Codes:  None       
//*************************************************************************
int is_unreserved(char in)
{
    if (isalnum(in) || (is_mark(in)))
        return 1;
    else 
        return 0;
}

//*************************************************************************
//* Name: is_escaped
//*
//* Description:  Returns a 1 if a char[3] sequence is escaped
//*               as defined in http://www.ietf.org/rfc/rfc2396.txt
//*               (RFC explaining URIs)
//*               size of array is NOT checked (MUST be checked by caller)
//*
//* In:           char *in
//*
//* Out:          1 if escaped, 0 if not
//*
//* Return Codes: None
//* Error Codes:  None       
//*************************************************************************
int is_escaped(  char * in)
{
  
    if ( ( in[0]=='%') && (isxdigit(in[1])) && isxdigit(in[2]))
    {
     
        return 1;
    }
    else
        return 0;
}

//*************************************************************************
//* Name: currentTmToHttpDate
//*
//* Description:  Returns the current date/time in the fixed length format 
//*               of RFC 822, updated by RFC 1123 (as described in HTTP 1.1
//*               http://www.w3.org/Protocols/rfc2616/rfc2616.htm 
//*               format is: 'DATE: Mon, 30 Apr 1979 08:49:37 GMT\r\n\0'
//*               ('\r\n\0' is added for convenience)
//*
//* In:           char * out (space for output , must be at least 38 
//*               characters)
//*
//* Out:          the current date is put in out
//*
//* Return Codes: None
//* Error Codes:  None       
//*************************************************************************

void currentTmToHttpDate(char *out)
{
    char *month=NULL;
    char *day=NULL;
    time_t current_time;
    struct tm * current_tm;
    time(&current_time);
    current_tm=gmtime(&current_time);

    switch (current_tm->tm_mon)
    {
        case 0: month="Jan";break;
        case 1: month="Feb";break;
        case 2: month="Mar";break;
        case 3: month="Apr";break;
        case 4: month="May";break;
        case 5: month="Jun";break;
        case 6: month="Jul";break;
        case 7: month="Aug";break;
        case 8: month="Sep";break;
        case 9: month="Oct";break;
        case 10: month="Nov";break;
        case 11: month="Dec";break;
    }
    switch (current_tm->tm_wday)
    {
        case 0:day="Sun";break;
        case 1:day="Mon";break;
        case 2:day="Tue";break;
        case 3:day="Wed";break;
        case 4:day="Thu";break;
        case 5:day="Fri";break;
        case 6:day="Sat";break;
    }
    strcpy(out,"DATE: ");
    strcat(out,day);
    strcat(out,", ");
    sprintf(&out[strlen(out)],"%02d ",current_tm->tm_mday);
    strcat(out,month);
    sprintf(&out[strlen(out)]," %04d %02d:%02d:%02d GMT\r\n",
        (current_tm->tm_year+1900), 
        current_tm->tm_hour,current_tm->tm_min,
        current_tm->tm_sec);
  
}

//*************************************************************************
//* Name: parse_header_value
//*
//* Description:  Parses the input for a header value.
//*               the header value is equal to all characters up to /r/n 
//*               (end of line)
//*               with beginning and trailing whitespace, removed  
//*               (TAB and ' ')
//*               The memory is NOT copied.  The returned token simply 
//*               points into the original char array. 
//*               Assumes that the whitespace has been stripped from
//*               the beginning of value
//* In:            char *in , int max_size (max_size to be considered 
//*               to avoid 
//*               running into bad memory)
//*
//* Out:          the size of the header value that was parsed, out.buff 
//*               points into the original string. There is NO new memory 
//*               created.
//*
//* Return Codes: None
//* Error Codes:  None       
//*************************************************************************
int parse_header_value(  char *in, token *out, int max_size)
{
    int counter =0;
    char * finger=in;
    counter = parse_http_line(in,max_size);
    if (!counter)
    { 
        out->size=0;
        out->buff=NULL;
        return 0;
    }

    counter-=2;

    finger+=(counter);

    //strip whitespace from end 
    while ( (counter>=0)
        && ( (*finger==' ')
            || (*finger==TAB)))
    {
        counter--;
        finger--;
    }

    out->buff=in;
    out->size=counter;
    return counter;

}

//*************************************************************************
//* Name: parse_http_line
//*
//* Description:  Returns the number of characters between 
//*               the first character of in, and the first \r\n
//*               sequence
//* In:            char *in , int max_size (max_size to be considered 
//*               to avoid 
//*               running into bad memory)
//*
//* Out:          the size of the line (including \r\n)
//* Return Codes: None
//* Error Codes:  0 if no \r\n is found within the max_size       
//*************************************************************************
int parse_http_line(  char * in, int max_size)
{
    int counter =0;
    while ( (counter+1<max_size)
        && ( (in[counter]!=CR)
            || (in[counter+1]!=LF)))
        counter++;
    if (counter>max_size-2 )
        return 0;
    return counter+2;
}

//*************************************************************************
//* Name: print_status_line
//*
//* Description:  For Debugging: prints out a parsed status line.
//*
//* In:            http_status *in
//*
//* Out:          None
//* Return Codes: None
//* Error Codes:  None       
//*************************************************************************
DBGONLY(void print_status_line(http_status *in, Dbg_Level level,
            Dbg_Module module, char *DbgFileName,
            int DbgLineNo)
{
    FILE *fd = GetDebugFile(level,module);
    if (!fd)
        return;
    if (DbgFileName)
    {
        //get lock
        cyg_mutex_lock(&GlobalDebugMutex);
        UpnpDisplayFileAndLine(fd,DbgFileName,DbgLineNo);
    }
    fprintf(fd,"STATUS LINE\n");
    fprintf(fd,"Status Code: \n");
    print_token(&in->status_code,level,module,NULL,0);
    fprintf(fd,"Reason Phrase: \n");
    print_token(&in->reason_phrase,level,module,NULL,0);
    fprintf(fd,"HTTP Version: \n");
    print_token(&in->http_version,level,module,NULL,0);
    if (DbgFileName)
    {
        cyg_mutex_unlock(&GlobalDebugMutex);
    }
})

    //*************************************************************************
    //* Name: print_request_line
    //*
    //* Description:  For Debugging: prints out a parsed request line.
    //*
    //* In:           http_request *in
    //*
    //* Out:          None
    //* Return Codes: None
    //* Error Codes:  None       
    //*************************************************************************
    DBGONLY(void print_request_line(http_request *in,Dbg_Level level,
                Dbg_Module module,char *DbgFileName,
                int DbgLineNo)
    {
        FILE *fd = GetDebugFile(level,module);
        if (!fd)
            return;
        if (DbgFileName)
        {
            //get lock
            cyg_mutex_lock(&GlobalDebugMutex);
            UpnpDisplayFileAndLine(fd,DbgFileName,DbgLineNo);
        }
        fprintf(fd,"HTTP request line:\n");
        fprintf(fd," Method: ");
        print_token(&in->method,level,module,NULL,0);
        fprintf(fd," Path URI: \n");
        print_uri(&in->request_uri,level,module,NULL,0);
        fprintf(fd," HTTP Version: ");
        print_token(&in->http_version,level,module,NULL,0);
        if (DbgFileName)
        {
            cyg_mutex_unlock(&GlobalDebugMutex);
        }

    })

    //*************************************************************************
    //* Name: parse_status_line
    //*
    //* Description:  parses the status line of an http response
    //* In:            char *in , http_status *out, int max_size 
    //*              ( to avoid running into bad memory)
    //*
    //* Out:          returns size of line (including /r/n)
    //* Return Codes: None
    //* Error Codes:  0 if any piece is missing (NOTE: NO LWS is expected at 
    //*                                          beginning of in) 
    //*                                         values are not to be trusted
    //*                                         if return code is zero      
    //*************************************************************************
    int parse_status_line( char * in, http_status * out, int max_size)
{
int temp=0;
int max_len=max_size;
char * finger=in;
int size=0;

out->http_version.buff=NULL;
out->http_version.size=0;
out->status_code.buff=NULL;
out->status_code.size=0;
out->reason_phrase.buff=NULL;
out->reason_phrase.size=0;

if (! (temp=parse_not_LWS(in,&out->http_version,max_size)))
{
    DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,
                "PARSING STATUS LINE \n COULDN'T GET HTTP VERSION"));
    return 0;
}
 finger+=temp;
 max_len-=temp;
 size+=temp;

 temp=parse_LWS(finger,max_len);

 max_len-=temp;
 finger+=temp;
 size+=temp;

 if (! (temp=parse_not_LWS(finger,&out->status_code,max_len)))
 { DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"PARSING STATUS LINE \n COULDN't GET STATUS CODE"));
 return 0;
 }
 finger+=temp;
 max_len-=temp;
 size+=temp;

 temp=parse_LWS(finger,max_len);

 max_len-=temp;
 finger+=temp;
 size+=temp;

 if (! (temp=parse_http_line(finger,max_len)))
 { DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"PARSING STATUS LINE \n COULDN'T GET REASON PHRASE"));
 return 0;
 }
 out->reason_phrase.buff=finger;

 //remove /r/n
 out->reason_phrase.size=temp-2;

 size+=temp;
 return size;
      
}

//*************************************************************************
//* Name: parse_request_line
//*
//* Description:  parses the request line of an http response
//* In:            char *in , http_status *out, int max_size 
//*              ( to avoid running into bad memory)
//*
//* Out:          returns size of line (including /r/n)
//* Return Codes: None
//* Error Codes:  0 if any piece is missing or bad (NOTE: 
//*                 NO LWS is expected at 
//*                                          beginning of in) 
//*                                          values are not to be trusted
//*                                          if return code is zero      
//*************************************************************************
int parse_request_line( char * in, http_request * out, int max_size)
{
    int temp=0;
    int max_len=max_size;
    char * finger=in;
    int size=0;
    token tempToken;

    out->http_version.buff=NULL;
    out->http_version.size=0;
    out->method.buff=NULL;
    out->method.size=0;
 

    if (! (temp=parse_token(in,&out->method,max_size)))
    {
        DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"PARSING REQUEST LINE \n COULDN'T GET METHOD"));
        return 0;
    }
    finger+=temp;
    max_len-=temp;
    size+=temp;

    temp=parse_LWS(finger,max_len);

    max_len-=temp;
    finger+=temp;
    size+=temp;

    //parse request URI (path)
    if (!(temp=parse_uri(finger,max_len,&out->request_uri)))
    {
        DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"PARSING REQUEST LINE \n COULDN't GET REQUEST URI"));
        return 0;
    }
    temp=parse_not_LWS(finger,&tempToken,max_len);

    max_len-=temp;
    finger+=temp;
    size+=temp;

    temp=parse_LWS(finger,max_len);

    max_len-=temp;
    finger+=temp;
    size+=temp;

    if (! (temp=parse_http_line(finger,max_len)))
    {
        DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"PARSING REQUEST LINE \n COULDN'T GET HTTP VERSION"));
        return 0;
    }
    //remove /r/n
    out->http_version.size=temp-2;
    out->http_version.buff=finger;
    size+=temp;
  
    return size;
      
}

//*************************************************************************
//* Name: parse_not_LWS
//*
//* Description:  Returns the number of characters between 
//*               the first character of in, and the first LWS (for this 
//*               function LWS: Tab 
//*               or space )
//*               sets the value of the token.buff=in, token.size= (num of 
//*               characters of nonLWS)
//*
//*               Note: does not handle multiple line white space  
//* In:           char *in , int max_size (max_size to be considered 
//*               to avoid 
//*               running into bad memory)
//*
//* Out:          out->size (length of non LWS at beginning of in)
//*
//* Return Codes: None
//* Error Codes:  None       
//*************************************************************************
int parse_not_LWS(  char *in, token *out, int max_size)
{
    int counter=0;

    while ( (counter<max_size)
        && (in[counter]!=' ')
        && (in[counter]!=TAB) )
        counter++;

    out->buff=in;
    out->size=counter;
    return counter;
  
}

//*************************************************************************
//* Name: parse_LWS
//*
//* Description:  Returns the number of characters between 
//*               the first character of in, and the first non LWS (LWS is 
//*               TAB or SPACE)
//*               for this function
//* 
//* In:            char *in , int max_size (max_size to be considered 
//*               to avoid 
//*               running into bad memory)
//*
//* Out:          length of LWS at beginning of in
//* Return Codes: None
//* Error Codes:  None       
//*************************************************************************
int parse_LWS( char * in, int max_size)
{
    int counter=0;
    while ((counter<max_size)
        && ( (in[counter]==' ')
            || (in[counter]==TAB) ))
    {
        counter++;
    }
    return counter;
}

//*************************************************************************
//* Name: parse_token
//*
//* Description:  parses a token as defined in HTTP/1.1 
//* In:            char *in , int max_size (max_size to be considered 
//*               to avoid 
//*               running into bad memory)
//*
//* Out:          out->size (length of token)
//*               Note: out->buff simply points to in. (no new memory)
//* Return Codes: length of token
//* Error Codes:  0 if no char is matched
//*************************************************************************
int parse_token( char * in, token * out, int max_size)
{
    int counter=0;

    while ( ( counter<max_size)
        && (in[counter]>31) 
        && (strchr(SEPARATORS,in[counter])==NULL) 
        && (in[counter]!=127))
    {
     
        counter++;
    }
    out->buff=in;
    out->size=counter;      
    return counter;
}

//*************************************************************************
//* Name: parse_uric
//*
//* Description:  parses a string of uric characters starting at in[0]
//*               as defined in http://www.ietf.org/rfc/rfc2396.txt
//*               (RFC explaining URIs)
//* In:            char *in , int max_size (max_size to be considered 
//*               to avoid 
//*               running into bad memory)
//*
//* Out:          out->size (length of uric string)
//*               Note: out->buff simply points to in.  (no new memory)
//* Return Codes: length of uric string
//* Error Codes:  0 if no char is matched.      
//*************************************************************************
int parse_uric(  char *in, int max, token *out)
{
    int i=0;

    while ( (i<max) && ( ( is_unreserved(in[i])) || (is_reserved(in[i])) ||
                ( (i+2<max) && (is_escaped(&in[i])) ) ) )
    {
        i++;
    }

    out->size=i;
    out->buff=in;
    return i;
}

//*************************************************************************
//* Name: parse_port
//*
//* Description:  parses a port (i.e. '4000') and converts it into a network
//*               ordered unsigned short int.
//* In:            char *port , int max (max_size to be considered 
//*               to avoid 
//*               running into bad memory)
//*
//* Out:          unsigned short int *out. (memory allocated by caller)
//*               
//* Return Codes: Number of characters contained in the port value
//* Error Codes:  0 if no char is matched.      
//*************************************************************************
int parse_port(int max,   char * port, unsigned short * out)
{

    char * finger=port;
    char * max_ptr=finger+max;
    unsigned short temp=0;

    while ( (finger<max_ptr) && ( isdigit(*finger) ) )
    {
        temp=temp*10;
        temp+=(*finger)-'0';
        finger++;
    }

    *out= htons(temp);
    return finger-port;
}

//*************************************************************************
//* Name: parse_hostport
//*
//* Description:  parses a string representing a host and port
//*               (e.g. "127.127.0.1:80" or "localhost")
//*               and fills out a hostport_type struct with internet
//*               address and a token representing the full host
//*               and port.  uses gethostbyname.
//*
//* In:           char *in , int max  (max size to be considered 
//*               to avoid 
//*               running into bad memory)
//*
//* Out:          out->text.size (size of text parsed as host and port)
//*               Note: out->text.buff points to in. No new memory is created.
//*
//* Return Codes: length of host port text, out->text.size
//* Error Codes:  UPNP_E_INVALID_URL (if either host or port can not be 
//*               resolved) 
//*               UPNP_E_OUTOF_MEMORY (if the function ran out
//*               of memory)
//*************************************************************************
int parse_hostport( char *in , int max, hostport_type *out)
{
    int i=0; 
    int begin_port;
    int hostport_size=0;
    int host_size=0;
    struct  hostent * h;
    char * temp_host_name;
  

    out->IPv4address.sin_port=htons(80); //default port is 80
    memset( &out->IPv4address.sin_zero,0,8);
  
    while ( (i<max) && (in[i]!=':') && (in[i]!='/') && ( (isalnum(in[i])) || (in[i]=='.') || (in[i]=='-') ))
        i++;
  
    host_size=i;

    if ( (i<max) && (in[i]==':'))
    {  
        begin_port=i+1;
        //convert port
        if (! ( hostport_size=parse_port(max-begin_port, &in[begin_port], &out->IPv4address.sin_port)))
            return UPNP_E_INVALID_URL;
        hostport_size+=begin_port;
    }
    else
        hostport_size=host_size;

    //convert to temporary null terminated string
    temp_host_name=(char * )malloc(host_size+1);

    if (temp_host_name==NULL)
        return UPNP_E_OUTOF_MEMORY;

    memcpy(temp_host_name,in,host_size);
    temp_host_name[host_size]='\0';

    if (inet_aton(temp_host_name, &(out->IPv4address.sin_addr)))
    {
        out->IPv4address.sin_family=AF_INET;
    }
    else if ( ( h=gethostbyname(temp_host_name)) && (h->h_addrtype==AF_INET) && (h->h_length==4) )
    {
        out->IPv4address.sin_addr= (* (struct in_addr *) h->h_addr);
        out->IPv4address.sin_family=AF_INET;
    }
    else
    {
        out->IPv4address.sin_addr.s_addr=0;
        out->IPv4address.sin_family=AF_INET;
        free(temp_host_name);
        return UPNP_E_INVALID_URL;
    }

    free(temp_host_name);

    out->text.size=hostport_size;
    out->text.buff=in;
    return hostport_size;
}

//*************************************************************************
//* Name: parse_scheme
//*
//* Description:  parses a uri scheme starting at in[0]
//*               as defined in http://www.ietf.org/rfc/rfc2396.txt
//*               (RFC explaining URIs)
//*               (e.g. "http:" -> scheme= "http")
//*               Note string MUST include ':' within the max charcters
//* In:           char *in , int max (max size to be considered 
//*               to avoid 
//*               running into bad memory)
//*
//* Out:          out->size (length of scheme)
//*               Note: out->buff simply points to in.  (no new memory)
//* Return Codes: length of scheme
//* Error Codes:  0 if scheme is not found   
//*************************************************************************
int parse_scheme(  char * in, int max, token * out)
{
    int i=0;
    out->size=0;
    out->buff=NULL;
 
 
    if ( (max==0) || ( ! isalpha(in[0])))
        return FALSE;

    i++;
    while ( (i<max) && (in[i]!=':'))
    {
 
        if (  !( isalnum(in[i]) || ( in[i]=='+') || (in[i]=='-')
                  || (in[i]=='.')))
            return FALSE;
      
        i++;
    }
    if (i<max) 
    {
        out->size=i;
        out->buff=&in[0];
        return i;
    }

    return FALSE;

}


//*************************************************************************
//* Name: remove_dots
//*
//* Description:  removes ".", and ".." from a path. If a ".." can not
//*               be resolved (i.e. the .. would go past the root of
//*               the path) an error is returned. The input IS
//*               modified in place.)
//* In:           char *in , int max (max size to be considered 
//*               to avoid 
//*               running into bad memory)
//*
//* Out:          char * in (with ".", and ".." rmeoved from the path)
//* Return Codes: UPNP_E_SUCCESS, UPNP_E_OUTOF_MEMORY, UPNP_E_INVALID_URL
//* Examples: 
//*          char path[30]="/../hello";
//*          remove_dots(path, strlen(path)) -> UPNP_E_INVALID_URL
//*          char path[30]="/./hello";
//*          remove_dots(path, strlen(path)) -> UPNP_E_SUCCESS, 
//*           in = "/hello"
//*          char path[30]="/./hello/foo/../goodbye" -> 
//*            UPNP_E_SUCCESS, in = "/hello/goodbye"
//*************************************************************************
int remove_dots(char *in, int size)
{
    char * copyTo=in;
    char * copyFrom=in;
    char * max=in+size;
    char **Segments=NULL;
    int lastSegment=-1;
  

    Segments=(char**)malloc(sizeof(char*) * size);

    if (Segments==NULL)
        return UPNP_E_OUTOF_MEMORY;

    Segments[0]=NULL;
  
    while  ( (copyFrom<max) && (*copyFrom!='?')
        && (*copyFrom!='#'))
    {
     
        if  ( ((*copyFrom)=='.')
            && ( (copyFrom==in) || (*(copyFrom-1)=='/') ))
        {
            if  ( (copyFrom+1==max)
                || ( *(copyFrom+1)=='/'))
            {
          
                copyFrom+=2;
                continue;
            }
            else
                if ( ( *(copyFrom+1)=='.')
                    && ( (copyFrom+2==max)
                        || ( *(copyFrom+2)=='/')))
                {
                    copyFrom+=3;
        
                    if (lastSegment>0)
                    {
                        copyTo=Segments[--lastSegment];
                    }
                    else
                    {
                        free(Segments);
                        DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"ERROR RESOLVING URL, ../ at ROOT"));
                        return UPNP_E_INVALID_URL;
                    }
                    continue;
                }
        }
      
        if ( (*copyFrom)=='/')
        {
      
            lastSegment++;
            Segments[lastSegment]=copyTo+1;
        }
        (*copyTo)=(*copyFrom);
        copyTo++;
        copyFrom++;
    }
    if (copyFrom<max)
    {
        while (copyFrom<max)
        {
            (*copyTo)=(*copyFrom);
            copyTo++;
            copyFrom++;
        }
    }
    (*copyTo)=0;
    free(Segments);
  
    return UPNP_E_SUCCESS;
}


//*************************************************************************
//* Name: resolve_rel_url
//*
//* Description:  resolves a relative url with a base url
//*               returning a NEW (dynamically allocated with malloc)
//*               full url.  Also frees (with free) the relative_url
//*               if the base_url is NULL, then the rel_url is passed back
//*               if the rel_url is absolute then the rel_url is passed back
//*               if neither the base nor the rel_url are Absolute then NULL 
//*                 is returned.
//*               otherwise it tries and resolves the relative url with the 
//*               base as described in: http://www.ietf.org/rfc/rfc2396.txt
//*               (RFC explaining URIs) 
//*               Note: resolution of '..' is NOT implemented, but '.' is 
//*               resolved 
//* In:           char *base_url, (can be fixed constant string) 
//*               char * rel_url (MUST be allocated with malloc, will be 
//*               freed on success)
//*
//* Out:          Dynamically allocated string (caller must free) 
//*               containing the full absolute url
//* Return Codes: 
//* Error Codes:  NULL in case of error      
//*************************************************************************
char * resolve_rel_url( char * base_url,  char * rel_url)
{
    uri_type base;
    uri_type rel;
    char temp_path='/';

    int i=0;
    char* finger=NULL;
  
    char* last_slash=NULL;
  

  
    char * out=NULL; 
    char * out_finger=NULL;
  
 

    if ( base_url && rel_url)
    {
        out = new char[strlen(base_url) + strlen(rel_url)+1];
        out_finger=out;
    }
    else
    {
        return rel_url;
    }
  
    if (out==NULL)
    {
        return NULL;
    }
 

    if ( (parse_uri(rel_url,strlen(rel_url),&rel))==HTTP_SUCCESS )
    {
      
        if (rel.type==ABSOLUTE)
        {
      
            strcpy(out,rel_url);
        }
        else
        {
      
            if ( (parse_uri(base_url, strlen(base_url),&base)==HTTP_SUCCESS) 
                && (base.type==ABSOLUTE) )
            {
          
                if (strlen(rel_url)==0)
                {
                    strcpy(out,base_url);
                }
                else
                { 
                    memcpy(out,base.scheme.buff,base.scheme.size);
                    out_finger+=base.scheme.size;
                    (*out_finger)=':';
                    out_finger++;
          
                    if (rel.hostport.text.size>0)
                    {
                        sprintf(out_finger,"%s",rel_url);
                    }
                    else
                    { 
                        if (base.hostport.text.size>0)
                        {
                            memcpy(out_finger,"//",2);
                            out_finger+=2;
                            memcpy(out_finger,base.hostport.text.buff,base.hostport.text.size);
                            out_finger+=base.hostport.text.size;
                        }
              
                        if (rel.path_type==ABS_PATH)
                        {
                            strcpy(out_finger,rel_url);
              
                        }
                        else
                        {
              
                            if (base.pathquery.size==0)
                            {
                                base.pathquery.size=1;
                                base.pathquery.buff=&temp_path;
                            }
              
                            finger =out_finger;
                            last_slash=finger;
                            i=0;
              
                            while ( (i<base.pathquery.size) && 
                                (base.pathquery.buff[i]!='?'))
                            {
                                (*finger)=base.pathquery.buff[i];
                                if (base.pathquery.buff[i]=='/')
                                    last_slash=finger+1;
                                i++;
                                finger++;
                  
                            }
                            i=0;
                            strcpy(last_slash,rel_url);
                            if (remove_dots(out_finger,
                                    strlen(out_finger))!=UPNP_E_SUCCESS)
                            {
                                delete [] out;
                                delete [] rel_url;
                                return NULL;
                            }
                        }
              
                    }
                }
            }
            else
            {
                delete [] out;
                delete [] rel_url;
                return NULL;
            }
        }
    }
    else
    {
        delete [] out;
        delete [] rel_url;            
        return NULL;
    }
  
    delete [] rel_url;
    return out;  
}


//*************************************************************************
//* Name: parse_uri
//*
//* Description:  parses a uri 
//*               as defined in http://www.ietf.org/rfc/rfc2396.txt
//*               (RFC explaining URIs)
//*               handles absolute, relative, and opaque uris
//*               Parses into the following pieces:
//*                  scheme, hostport, pathquery, fragment
//*                  (path and query are treated as one token)
//*               Caller should check for the pieces they require.
//* In:           char *in , int max (max size to be considered to avoid 
//*               running into bad memory)
//*
//* Out:          uri_type * out (memory allocated by caller)
//*               Each of the pieces that are present are filled in.
//*               Plus the type of uri (ABSOULTE, RELATIVE), and type of 
//*               path (ABS_PATH, REL_PATH, OPAQUE_PART)
//* Return Codes: HTTP_SUCCESS
//* Error Codes:  returns the error code returned from
//*               parse_hostport if an address is expected but not resolved   
//*************************************************************************
int parse_uri(  char * in, int max, uri_type * out)
{
    int begin_path=0;
    int begin_hostport=0;
    int begin_fragment=0;
  

    if ( (begin_hostport=parse_scheme(in, max,&out->scheme) ))
    {
        out->type=ABSOLUTE;
        out->path_type=OPAQUE_PART;
        begin_hostport++;
    }
    else
    {    
        out->type=RELATIVE;
        out->path_type=REL_PATH;
    }

    if ( ( (begin_hostport+1) <max) && (in[begin_hostport]=='/')
        && (in[begin_hostport+1]=='/'))
    {
        begin_hostport+=2;
      
        if ( ( begin_path=parse_hostport(&in[begin_hostport],
                   max-begin_hostport,
                   &out->hostport)) >= 0 )
        {
            begin_path+=begin_hostport;
        }
        else
            return begin_path;              
    
    }
    else
    {
        out->hostport.IPv4address.sin_port=0;
        out->hostport.IPv4address.sin_addr.s_addr=0;
        out->hostport.text.size=0;
        out->hostport.text.buff=0;
        begin_path=begin_hostport;
    }

    begin_fragment=parse_uric(&in[begin_path],max-begin_path,&out->pathquery);
  
    if ( (out->pathquery.size) && (out->pathquery.buff[0]=='/'))
    {
        out->path_type=ABS_PATH;
    }

    begin_fragment+=begin_path;
 
    if ( (begin_fragment<max) && ( in[begin_fragment]=='#'))
    {
        begin_fragment++;
        parse_uric(&in[begin_fragment],max-begin_fragment,&out->fragment);
    }
    else{
        out->fragment.buff=NULL;
        out->fragment.size=0;
    }
  
    return HTTP_SUCCESS;
}


//*************************************************************************
//* Name: concat_socket_buffers
//*
//* Description:  concats a list of socket buffers (used to read data of 
//*               unknown size) into one buffer (out)
//*               size of out is assumed to be large enough to hold the data
//*               the caller is responsible for maintaining this
//*
//* In:           socket_buffer * head (head of the list of buffers)
//*
//* Out:          the concatenated socket_buffers are copied into out
//*
//* Return Codes: None
//* Error Codes:  None   
//*************************************************************************
void concat_socket_buffers(socket_buffer * head, char * out)
{
    socket_buffer *finger=head;
    finger=head;
    while (finger)
    {
        memcpy(out,finger->buff,finger->size);
        out+=finger->size;
        finger=finger->next;
    }
  
}

//*************************************************************************
//* Name: free_socket_buffers
//*
//* Description:  frees the space allocated in a list of socket_buffers
//*              
//* In:           socket_buffer * head (head of list of buffers to free)
//*
//* Out:          None
//*           
//* Return Codes: None
//* Error Codes:  None  
//*************************************************************************
void free_socket_buffers(socket_buffer * head)
{
    socket_buffer * temp=NULL;
    socket_buffer * finger=NULL;
    if (head)
    {
        finger=head->next;
        while (finger)
        {
            temp=finger->next;
            free(finger);
            finger = temp;
        }
        head->next = NULL;
    }
}


//*************************************************************************
//* Name: getHeaders
//*
//* Description:  Reads the header section of a http message (used in 
//*               transfer http) 
//*               returns a list of socket buffers and the content length
//*               if a "CONTENT-LENGTH:" header is found. 
//*               reads until a line : "\r\n"
//*              
//* In:           socket_buffer * head (head of list of buffers)
//*               int fd (socket)
//*               int * ContentLength (space to place content length)
//*               socket_buffer **tail (space to place end of socket buffer 
//*                                     list)
//*               int * timeout (timeout for operation) (returns time 
//*                             remaining after operation)
//*               
//* Out:          total size of headers (memory contained in list of socket 
//*               buffers) memory in socket buffer list must be freed by
//*               caller
//*           
//* Return Codes: >=0 SUCCESS , time of operation is subtracted
//*                             from timeout
//* Error Codes:  UPNP_E_OUTOF_MEMORY (memory failure)
//*               UPNP_E_SOCKET_READ (error reading from socket , including 
//*                                   timeout) 
//*************************************************************************

int getHeaders(int fd, int *ContentLength, socket_buffer* head,
    socket_buffer **tail, int *timeout)
{
    socket_buffer *current=head;
    socket_buffer *prev=NULL;
    int total_size=0;
 
    char  * invalidchar;

    char * pat="CONTENT-LENGTH:";
    char * pat2="TRANSFER-ENCODING: CHUNKED";
    char * pat3="CONTENT";
    int pat_len=strlen(pat);
    int pat2_len=strlen(pat2);
    int pat3_len=strlen(pat3);
  

    head->next=NULL;

    (*ContentLength)=-3;
    (*tail)=NULL;

    current->size=0;
  

    while (  (current->size=readLine(fd,current->buff,SOCKET_BUFFER_SIZE,timeout)) >0)
    {

        total_size+=current->size;
      
        //check for continuation of header over multiple lines
        //this is a continuation if and only if the previous line was
        //properly ended AND this line begins with a space or tab
        //in this case the CRLF is removed from the previous line
        if ( (current->buff[0]==' ')
            || (current->buff[0]==TAB))
        {
            if (prev)
            {
                if ( (prev->size>=2) && (prev->buff[prev->size-1]=='\n')
                    && (prev->buff[prev->size-2]=='\r'))
                    prev->size-=2;
                DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"REMOVED CRLF FROM MULTILINE HEADER"));
                total_size-=2;
            }
        }

        //make sure we only check these values after new lines
        //just in case the previous value was so large that the CRLF was
        //not detected before the SOCKET_BUFFER was filled
        if  ( (!prev)
            || ( (prev) && (prev->size>=2) && (prev->buff[prev->size-1]=='\n')
                && (prev->buff[prev->size-2]=='\r')))
        {
            if (!strncasecmp(pat3,current->buff,pat3_len))
            {
                DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"EXPECTING CONTENT"));
                if ((*ContentLength)==-3)
                    (*ContentLength)=-2;
            }
            if (!strncasecmp(pat2,current->buff,pat2_len))
            {
                DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"CONTENT IS CHUNKED"));
                //transfer-encoding:chunked
                (*ContentLength)=-1;
            }
            //if it is "Content-Length:" then get the length
            if (!strncasecmp(pat,current->buff,pat_len))
            {
                //if transfer-encoding chunked is already found then
                //ignore content length header
                if ((*ContentLength)!=-1)
                {
                    DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"CONTENT LENGTH FOUND"));
                    (*ContentLength)=strtol(current->buff+pat_len
                        ,&invalidchar,10);
          
                    if (invalidchar==current->buff+pat_len)
                    {
                        (*ContentLength)=0;
                        DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"Trouble getting Content Length"));
                    }
                }
            }
            
            //last \r\n
            if ( ( current->size==2) && (current->buff[0]=='\r')) 
                break;
        }

      

        current->next= (socket_buffer *) malloc(sizeof(socket_buffer));
        if (!current->next)
        {
            free_socket_buffers(head);
            return UPNP_E_OUTOF_MEMORY;
        }      
        prev=current;
        current=current->next;
        current->next=NULL;
        current->size=0;
      
    }
  
    if (current->size<0)
    {
        (*ContentLength)=0;
        free_socket_buffers(head);
        DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"ERROR READING SOCKET"));
        return UPNP_E_SOCKET_READ; 
    }

    if ((*ContentLength)==-3)
        (*ContentLength)=0;
  
    (*tail)=current;
  
    return total_size;
}


//*************************************************************************
//* Name: write_timeout
//*
//* Description:  writes bytes to a socket with a timeout
//*              
//* In:           int fd (socket)
//*               void *buf (buffer to be sent)
//*               size_t count (number of bytes to be sent)
//*               int *timeout (timeout for operation)
//*
//* Out:          # of bytes sent. timeout is updated, time of operation
//*               is subtracted from it.
//*           
//* Return Codes: >=0 success
//* Error Codes:  -1 Timeout, <=0 Error return code of write()
//*************************************************************************
ssize_t write_timeout(int fd,  void *buf, size_t count, int *timeout)
{
    fd_set wset;
    struct timeval tv;
    time_t current_time;
    int writeable;
    time_t new_time;
    if ((*timeout)<=0)
    {
        return -1; //TIMEOUT
    }
    FD_ZERO(&wset);
    FD_SET(fd,&wset);
    tv.tv_sec=(*timeout);
    tv.tv_usec = 0 ;
    time(&current_time); //get current time
  
    writeable=select(fd+1,NULL, &wset,NULL,&tv);
    if (writeable>0)
    { 
        time(&new_time);
        (*timeout)-=(new_time-current_time);
     
        return send(fd,buf,count,0);
    }
    else
    {
        (*timeout)=0;
        return -1; //TIMEOUT
    }
}



//*************************************************************************
//* Name: read_timeout
//*
//* Description:  read bytes to a socket with a timeout
//*              
//* In:           int fd (socket)
//*               void *buf (buffer to store data)
//*               size_t count (number of bytes to be read t)
//*               int *timeout (timeout for operation)
//*
//* Out:          # of bytes read. timeout is updated, time of operation
//*               is subtracted from it.
//*           
//* Return Codes: >=0 success
//* Error Codes:  -1 Timeout, <=0 Error return code of read()
//*************************************************************************
ssize_t read_timeout(int fd,void *buf, size_t count, int * timeout)
{
    fd_set rset;
    struct timeval tv;
    time_t current_time;
    int readable;
    time_t new_time;

    if ((*timeout)<=0)
    {
        DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"TIMEOUT ON READ"));
        return -1; //TIMEOUT
    }
  
    FD_ZERO(&rset);
    FD_SET(fd, &rset);
    tv.tv_sec=(*timeout);
    tv.tv_usec = 0 ;
    time(&current_time); //get current time
  
    readable=select(fd+1,&rset, NULL,NULL,&tv);
    if (readable>0)
    { 
        time(&new_time);
        (*timeout)-=(new_time-current_time);
        return recv(fd,buf,count,0);
    }
    else
    {
        (*timeout)=0;
        DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"TIMEOUT ON READ"));
        return -1; //TIMEOUT
    }    
}

//*************************************************************************
//* Name: readLine
//*
//* Description:  reads characters from a socket until a newline is seen
//*               appends a null to the output
//* In:           int fd (socket)
//*               char *out(buffer to store data)
//*               int max (bytes in buffer)
//*               int *timeout (timeout for operation)
//*
//* Out:          # of bytes read. timeout is updated, time of operation
//*               is subtracted from it.
//*           
//* Return Codes: >=0 success
//* Error Codes:  -1 Error reading from socket
//*************************************************************************
ssize_t readLine(int fd, char *out, int max, int *timeout)
{
    char c;
    ssize_t count=0,rc=0;

    while ( (count<(max-1)) && ( (rc=read_timeout(fd,&c,1,timeout))==1) )
    {
        *out++=c;
        count++;
        if (c=='\n')
            break;
    }
    if (rc<0)//error
    {
        return -1;
    }
    *out=0;
    return count;
}




//*************************************************************************
//* Name: read_http_response
//*
//* Description:  reads the http response from a socket, with a timeout
//*               returns a dynamically allocated string (null terminated)
//*               must be freed by caller.      
//*        
//* In:           int fd (socket)
//*               char **out (memory allocated by function and pointer 
//*                           stored here)
//*               int timeout (timeout for operation)
//*
//* Out:          http response is read and returned in null terminated 
//*               string, (*out). http headers and content are read
//*               ONLY WORKS if "CONTENT LENGTH:" header is present.
//*               otherwise content is NOT returned.
//*           
//* Return Codes: HTTP_SUCCESS on success
//* Error Codes:  UPNP_E_OUTOF_MEMORY, memory error
//*               UPNP_E_SOCKET_READ, socket read error, including
//*               timeout
//*************************************************************************
int read_http_response(int fd, char **out, int timeout)
{
    socket_buffer head;
    socket_buffer *current=&head;
    socket_buffer * header_tail=NULL;
    int total_size=0;
    int contentLength=0;
    int Timeout=timeout;
    int done =0;
    int chunk_size=-1;
    socket_buffer chunkSizebuff;
    char *invalidchar;
    int nextToRead=0;
    head.next=NULL;
 

    if ( ( (total_size=getHeaders(fd,&contentLength,&head,&header_tail,&Timeout)) >0))
    {
      
        current=header_tail;
      
        if (contentLength==0)
        {
            DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"NO CONTENT EXPECTED"));
            done=1;
        }
        while (!done)
        {
            if ( (contentLength==-1)
                && ( chunk_size<=0))
            {
                DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"GETTING NEXT CHUNK SIZE"));
                //gobble up crlf
                if (chunk_size==0) {
                    chunkSizebuff.size = 
                        readLine(fd,chunkSizebuff.buff,SOCKET_BUFFER_SIZE,&Timeout);
                }              
          
                if ((chunkSizebuff.size=readLine(fd,chunkSizebuff.buff,SOCKET_BUFFER_SIZE,&Timeout)) >0)
                {
                    chunk_size = strtol(chunkSizebuff.buff
                        ,&invalidchar,16);
                    DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"chunk size: %d\n",chunk_size);)
                        if (invalidchar==chunkSizebuff.buff)
                            chunk_size=0;    
                }
                else
                    chunk_size=0;
                if (chunk_size==0)
                {
                    done=1;
                    break;
                }
            }

            current->next = (socket_buffer *) malloc(sizeof(socket_buffer));
            if (!current->next)
            {
                free_socket_buffers(&head);
                return UPNP_E_OUTOF_MEMORY;
            }
      
            current=current->next;
            current->next=NULL;
            current->size=0;

            nextToRead=SOCKET_BUFFER_SIZE;

            if (contentLength==-1)
            {
                if (chunk_size<SOCKET_BUFFER_SIZE)
                    nextToRead=chunk_size;
            }
            else
                if (contentLength>0)
                {
                    if (contentLength<SOCKET_BUFFER_SIZE)
                        nextToRead=contentLength;
                }

            if ((HTTP_READ_BYTES!=-1)
                && ( (total_size+(nextToRead))>HTTP_READ_BYTES))
            {
                done=1;
                break;
            }
            if ( (current->size=read_timeout(fd,current->buff,nextToRead,&Timeout))>0) 
            {
        
                if (contentLength>0)
                {
                    contentLength-=current->size;
                    if (contentLength==0)
                    {
                        done=1;
                    }
                }
                if (contentLength==-1)
                {
                    chunk_size-=current->size;
                }
                total_size+=current->size;
       
            }
            else
            {
                if (current->size<0)
                {
                    DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"socket read error, probably timed out"));
                    free_socket_buffers(&head);
                    return UPNP_E_SOCKET_READ;
                }
                done=1;
            }

        }
    
    }

    if (total_size<0) 
    {
        (*out)=NULL;
        free_socket_buffers(&head);
        return total_size;
    }

    (*out)=(char * ) malloc(total_size+1);
    concat_socket_buffers(&head,(*out));
    (*out)[total_size]=0;

    free_socket_buffers(&head);

    return HTTP_SUCCESS; 
}



//*************************************************************************
//* Name: read_http_binary_response
//*
//* Description:  reads the http response from a socket, with a timeout
//*               returns a dynamically allocated buffer
//*               must be freed by caller.      
//*        
//* In:           int fd (socket)
//*               char **out (memory allocated by function and pointer 
//*                           stored here)
//*               int timeout (timeout for operation)
//*
//* Out:          http response is read and returned in buffer
//*               string, (*out). http headers and content are read
//*               ONLY WORKS if "CONTENT LENGTH:" header is present.
//*               otherwise content is NOT returned.
//*           
//* Return Codes: HTTP_SUCCESS on success
//* Error Codes:  UPNP_E_OUTOF_MEMORY, memory error
//*               UPNP_E_SOCKET_READ, socket read error, including
//*               timeout
//*************************************************************************
int read_http_binary_response(int fd, char **out, int timeout, int *buflength)
{
    socket_buffer head;
    socket_buffer *current=&head;
    socket_buffer * header_tail=NULL;
    int total_size=0;
    int contentLength=0;
    int Timeout=timeout;
    int done =0;
    int chunk_size=-1;
    socket_buffer chunkSizebuff;
    char *invalidchar;
    int nextToRead=0;
    head.next=NULL;
 

    if ( ( (total_size=getHeaders(fd,&contentLength,&head,&header_tail,&Timeout)) >0))
    {
      
        current=header_tail;

      
        if (contentLength==0)
        {
            DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"NO CONTENT EXPECTED"));
            done=1;
        }
        while (!done)
        {
            if ( (contentLength==-1)
                && ( chunk_size<=0))
            {
                DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"GETTING NEXT CHUNK SIZE"));
                //gobble up crlf
                if (chunk_size==0)
                    chunkSizebuff.size=
                        readLine(fd,chunkSizebuff.buff,SOCKET_BUFFER_SIZE,&Timeout);
          
                if ((chunkSizebuff.size=readLine(fd,chunkSizebuff.buff,SOCKET_BUFFER_SIZE,&Timeout)) >0)
                {
                    chunk_size=strtol(chunkSizebuff.buff
                        ,&invalidchar,16);
                    DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"chunk size: %d\n",chunk_size);)
                        if (invalidchar==chunkSizebuff.buff)
                            chunk_size=0;    
                }
                else
                    chunk_size=0;
                if (chunk_size==0)
                {
                    done=1;
                    break;
                }
            }

            current->next= (socket_buffer *) malloc(sizeof(socket_buffer));
            if (!current->next)
            {
                free_socket_buffers(&head);
                return UPNP_E_OUTOF_MEMORY;
            }
      
            current=current->next;
            current->next=NULL;
            current->size=0;

            nextToRead=SOCKET_BUFFER_SIZE;

            if (contentLength==-1)
            {
                if (chunk_size<SOCKET_BUFFER_SIZE)
                    nextToRead=chunk_size;
            }
            else
                if (contentLength>0)
                {
                    if (contentLength<SOCKET_BUFFER_SIZE)
                        nextToRead=contentLength;
                }

            if ((HTTP_READ_BYTES!=-1)
                && ( (total_size+(nextToRead))>HTTP_READ_BYTES))
            {
                done=1;
                break;
            }
      
            if ( (current->size=read_timeout(fd,current->buff,nextToRead,&Timeout))>0) 
            {
        
                if (contentLength>0)
                {
                    contentLength-=current->size;
                    if (contentLength==0)
                    {
                        done=1;
                    }
                }
                if (contentLength==-1)
                {
                    chunk_size-=current->size;
                }
                total_size+=current->size;
       
            }
            else
            {
                if (current->size<0)
                {
                    DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"socket read error, probably timed out"));
                }
                done=1;
            }

        }
    
    }

    if (total_size<0) 
    {
        (*out) = NULL;
        free_socket_buffers(&head);
        return total_size;
    }

    (*out)=(char * ) malloc(total_size+1);
    concat_socket_buffers(&head,(*out));
    (*out)[total_size]=0;
    *buflength = total_size;

    free_socket_buffers(&head);

    return HTTP_SUCCESS;
}

//*************************************************************************
//* Name: write_bytes
//*
//* Description:  writes bytes to a socket with a timeout 
//*               writes until all bytes are written
//*              
//* In:           int fd (socket)
//*               char *buf (buffer to be sent)
//*               size_t n (number of bytes to be sent)
//*               int timeout (timeout for operation)
//*
//* Out:          # of bytes sent. 
//*           
//* Return Codes: >=0 success
//* Error Codes:  -1 (error writing socket, including
//*                                    timeout)
//*************************************************************************
int write_bytes(int fd,   char * bytes, size_t n, int timeout)
{
    int bytes_written=0;
    int bytes_left=n;
    char *ptr=bytes;
    int Timeout=timeout;

    while (bytes_left>0)
    {
        if  ( (bytes_written=write_timeout(fd,ptr,bytes_left,&Timeout))<=0)
            return -1;//error
        bytes_left-=bytes_written;
        ptr+=bytes_written;
    }
  
    return bytes_written;
}


//*************************************************************************
//* Name: transferHTTP
//*
//* Description:  sends an http request, and returns the output in a 
//*               null terminated string (allocated by function, freed by
//*               caller). automatically generates host line. if
//*               no path is given default path is '/'. HTTP version 1.1
//*               is used for everything except "GET"
//*              
//* In:           char * request (NULL terminated string, e.g. "GET")
//*               char * toSend (bytes to be sent AFTER the HOST header)
//*               int toSendSize (number of bytes in toSend)
//*               char ** out (output)
//*               URL (NULL terminated url)
//*
//* Out:          HTTP_SUCCESS (on success) , (*out) null terminated string
//*           
//* Return Codes: HTTP_SUCCESS
//* Error Codes:  UPNP_E_OUTOF_MEMORY
//*               UPNP_E_READ_SOCKET
//*               UPNP_E_WRITE_SOCKET
//*               UPNP_E_OUTOF_SOCKET
//*               UPNP_E_SOCKET_CONNECT
//*
//* Example:  transferHTTP("GET","\r\n",2,&temp,
//*                        "http://localhost:80/foo?silly#56");
//*           ------------------------------------------------------------
//*           A TCP conneciton is opened to localhost:80.
//*           The following is sent: 
//*          "GET /foo?silly HTTP/1.1\r\nHOST: localhost:80\r\n\r\n"
//*************************************************************************

int transferHTTP(  char * request,   char * toSend, 
    int toSendSize, char **out,  char * Url)
{
    uri_type parsed_url;

    int return_code=HTTP_SUCCESS;
  
    if ( (return_code=parse_uri(Url,strlen(Url),&parsed_url))==HTTP_SUCCESS )
    {
        return
            transferHTTPparsedURL(request,toSend,toSendSize,out,&parsed_url);
    }
    else return return_code;
}


//*************************************************************************
//* Name: transferHTTPRaw
//*
//* Description:  sends an http request, and returns the output in a 
//*               null terminated string (allocated by function, freed by
//*               caller). DOESN'T generate any headers
//*              
//* In:          
//*               char * toSend (bytes to be sent)
//*               int toSendSize (number of bytes in toSend)
//*               char ** out (output)
//*               URL (NULL terminated url)
//*
//* Out:          HTTP_SUCCESS (on success) , (*out) null terminated string
//*           
//* Return Codes: HTTP_SUCCESS
//* Error Codes:  UPNP_E_OUTOF_MEMORY
//*               UPNP_E_READ_SOCKET
//*               UPNP_E_WRITE_SOCKET
//*               UPNP_E_OUTOF_SOCKET
//*               UPNP_E_SOCKET_CONNECT
//*
//* Example:  transferHTTP(
//*  "GET /foo?silly HTTP/1.1\r\nHOST: localhost:80\r\nfoo\r\n\r\n",
//*  strlen("GET /foo?silly HTTP/1.1\r\nHOST: localhost:80\r\nfoo\r\n\r\n")
//*                         ,&temp,
//*                        "http://localhost:80");
//*           ------------------------------------------------------------
//*           A TCP conneciton is opened to localhost:80.
//*           The following is sent: 
//*          "GET /foo?silly HTTP/1.1\r\nHOST: localhost:80\r\nfoo\r\n\r\n"
//*************************************************************************
int transferHTTPRaw( char * toSend, int toSendSize, char **out,  char
    * Url)
{
    uri_type parsed_url;
  
    int return_code=HTTP_SUCCESS;
    int client_socket=0;
  
  
    if ( (return_code=parse_uri(Url,strlen(Url),&parsed_url))==HTTP_SUCCESS )
    {
        //set up tcp connection
        if (token_string_casecmp(&parsed_url.scheme,"http"))
        {
            return UPNP_E_INVALID_URL;
        }

        if (parsed_url.hostport.text.size==0)
        { 
            return UPNP_E_INVALID_URL;
        }
      
        if ( (client_socket=socket(AF_INET,SOCK_STREAM,0))==-1)
        {
      
            DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"OUT OF SOCKET"));
            return UPNP_E_OUTOF_SOCKET;
        }

        // dc- set a timeout on the connect socket
        struct timeval tv;
        tv.tv_sec  = 3;
        tv.tv_usec = 0;
        setsockopt( client_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv) );
        
        if (connect(client_socket,(struct sockaddr*) & 
                parsed_url.hostport.IPv4address,sizeof(struct sockaddr))==-1)
        {
            DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"CONNECT ERROR: %d %s", errno, strerror(errno)));
            close(client_socket);
            return UPNP_E_SOCKET_CONNECT;
        }
      
        //changed to transmit null character
        if ( write_bytes(client_socket,toSend,
                 toSendSize,RESPONSE_TIMEOUT)==-1) 
        {
      
            close(client_socket);
            DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"WRITE ERROR"));
            return UPNP_E_SOCKET_WRITE;
        }

        return_code = read_http_response(client_socket,out,RESPONSE_TIMEOUT);
      
        close(client_socket);    
      
        return return_code;
      
    }
    else return return_code; 
}



//*************************************************************************
//* Name: transferHTTPBinary
//*
//* Description:  sends an http request, and returns the output in a buffer
//*               (allocated by function, freed by
//*               caller). DOESN'T generate any headers
//*              
//* In:          
//*               char * toSend (bytes to be sent)
//*               int toSendSize (number of bytes in toSend)
//*               char ** out (output)
//*               URL (NULL terminated url)
//*
//* Out:          HTTP_SUCCESS (on success) , (*out) buffer  (*buflength)  size of buffer
//*           
//* Return Codes: HTTP_SUCCESS
//* Error Codes:  UPNP_E_OUTOF_MEMORY
//*               UPNP_E_READ_SOCKET
//*               UPNP_E_WRITE_SOCKET
//*               UPNP_E_OUTOF_SOCKET
//*               UPNP_E_SOCKET_CONNECT
//*
//* Example:  transferHTTP(
//*  "GET /foo?silly HTTP/1.1\r\nHOST: localhost:80\r\nfoo\r\n\r\n",
//*  strlen("GET /foo?silly HTTP/1.1\r\nHOST: localhost:80\r\nfoo\r\n\r\n")
//*                         ,&temp,
//*                        "http://localhost:80");
//*           ------------------------------------------------------------
//*           A TCP conneciton is opened to localhost:80.
//*           The following is sent: 
//*          "GET /foo?silly HTTP/1.1\r\nHOST: localhost:80\r\nfoo\r\n\r\n"
//*************************************************************************
int transferHTTPBinary( char * toSend, int toSendSize, char **out,  char * Url, int *buflength )
{
    uri_type parsed_url;
  
    int return_code=HTTP_SUCCESS;
    int client_socket=0;
  
  
    if ( (return_code=parse_uri(Url,strlen(Url),&parsed_url))==HTTP_SUCCESS )
    {
        //set up tcp connection
        if (token_string_casecmp(&parsed_url.scheme,"http"))
        {
            return UPNP_E_INVALID_URL;
        }

        if (parsed_url.hostport.text.size==0)
        { 
            return UPNP_E_INVALID_URL;
        }
      
        if ( (client_socket=socket(AF_INET,SOCK_STREAM,0))==-1)
        {
      
            DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"OUT OF SOCKET"));
            return UPNP_E_OUTOF_SOCKET;
        }
      
        // dc- set a timeout on the connect socket
        struct timeval tv;
        tv.tv_sec  = 3;
        tv.tv_usec = 0;
        setsockopt( client_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv) );
        
        if (connect(client_socket,(struct sockaddr*) & 
                parsed_url.hostport.IPv4address,sizeof(struct sockaddr))==-1)
        {
            DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"CONNECT ERROR: %d %s", errno, strerror(errno)));
            close(client_socket);
            return UPNP_E_SOCKET_CONNECT;
        }
      
        //changed to transmit null character
        if ( write_bytes(client_socket,toSend,
                 toSendSize,RESPONSE_TIMEOUT)==-1) 
        {
      
            close(client_socket);
            DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"WRITE ERROR"));
            return UPNP_E_SOCKET_WRITE;
        }
  
        return_code=read_http_binary_response(client_socket,out,RESPONSE_TIMEOUT,buflength);
      
        close(client_socket);    
      
        return return_code;
      
    }
    else return return_code; 
}



//*************************************************************************
//* Name: transferHTTPparsedURL (same as above but takes parsed url)
//*
//* Description:  sends an http request, and returns the output in a 
//*               null terminated string (allocated by function, freed by
//*               caller). automatically generates host line. if
//*               no path is given default path is '/' HTTP version 1.1
//*               is used for everything except "GET"
//*              
//* In:           char * request (NULL terminated string, e.g. "GET")
//*               char * toSend (bytes to be sent AFTER the HOST header)
//*               int toSendSize (number of bytes in toSend)
//*               char ** out (output)
//*               uri_type *URL(parsed url)
//*
//* Out:          HTTP_SUCCESS (on success) , (*out) null terminated string
//*           
//* Return Codes: HTTP_SUCCESS
//* Error Codes:  UPNP_E_OUTOF_MEMORY
//*               UPNP_E_READ_SOCKET
//*               UPNP_E_WRITE_SOCKET
//*               UPNP_E_OUTOF_SOCKET
//*               UPNP_E_SOCKET_CONNECT
//*   
//* Example:  Suppose parsed_url is the parsed form of 
//*           "http://localhost:80"
//*           transferHTTP("GET","\r\n",2,&temp,
//*                        &parsed_url);
//*           ------------------------------------------------------------
//*           A TCP conneciton is opened to localhost:80.
//*           The following is sent: 
//*          "GET /foo?silly HTTP/1.1\r\nHOST: localhost:80\r\n\r\n"
//*************************************************************************
int transferHTTPparsedURL(  char * request,   char * toSend, 
    int toSendSize, char **out, uri_type *URL)
{
    uri_type parsed_url=(*URL);
    int request_length=0;
    int host_length=0;
    char * message=NULL;
    char *  message_finger=NULL;
    int client_socket=0;
  
    char temp_path='/';
 
    int return_code=HTTP_SUCCESS;
    int message_size=0;

    if (token_string_casecmp(&parsed_url.scheme,"http"))
    {
        return UPNP_E_INVALID_URL;
    }
  
    if (parsed_url.hostport.text.size==0)
    { 
        return UPNP_E_INVALID_URL;
    }    
    if (parsed_url.pathquery.size==0)
    {
        parsed_url.pathquery.size=1;
        parsed_url.pathquery.buff=&temp_path;
    }
    //request_length="(request)<sp>(path)<sp>HTTP/1.1\r\n"
    request_length=strlen(request) + 1 + parsed_url.pathquery.size + 8 +1 + 2;
    //host_length = "HOST:<sp>(host:port)\r\n"
    host_length = 6 + parsed_url.hostport.text.size +2;
    message_size=request_length+host_length+toSendSize;
    message = (char * ) malloc(message_size);
  
    if (message==NULL)
        return UPNP_E_OUTOF_MEMORY;
  
    message_finger=message;
  
    sprintf(message, "%s ",request);
  
    message_finger+=strlen(request)+1; 
  
    memcpy(message_finger,parsed_url.pathquery.buff,
        parsed_url.pathquery.size);

    message_finger+=parsed_url.pathquery.size;
  
    //hack to force server to return content length
    if (!strcasecmp(request,"GET"))
        memcpy(message_finger," HTTP/1.0\r\nHOST: ",17);
    else
        memcpy(message_finger," HTTP/1.1\r\nHOST: ",17);
    message_finger+=17;
    memcpy(message_finger,parsed_url.hostport.text.buff,
        parsed_url.hostport.text.size);
    message_finger+=parsed_url.hostport.text.size;
    memcpy(message_finger,"\r\n",2);
    message_finger+=2;
    memcpy(message_finger,toSend,toSendSize);
  

    //set up tcp connection
  
    if ( (client_socket=socket(AF_INET,SOCK_STREAM,0))==-1)
    {
        free(message);
        DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"OUT OF SOCKET"));
        return UPNP_E_OUTOF_SOCKET;
    }
  
    // dc- set a timeout on the connect socket
    struct timeval tv;
    tv.tv_sec  = 3;
    tv.tv_usec = 0;
    setsockopt( client_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv) );
        
    if (connect(client_socket,(struct sockaddr*) & 
            parsed_url.hostport.IPv4address,sizeof(struct sockaddr))==-1)
    {
        DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"CONNECT ERROR: %d %s", errno, strerror(errno)));
        free(message);
        close(client_socket);
        return UPNP_E_SOCKET_CONNECT;
    }
  
  
    if ( write_bytes(client_socket,message,message_size,RESPONSE_TIMEOUT)==-1) 
    {
        free(message);
        close(client_socket);
        DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"WRITE ERROR"));
        return UPNP_E_SOCKET_WRITE;
    }
  
    free(message);

    return_code = read_http_response(client_socket,out,RESPONSE_TIMEOUT);
  
    close(client_socket);    

    return return_code;
}



void free_http_headers(http_header * list)
{
    http_header * next;
    while (list)
    {
        next=list->next;
        free(list);
        list=next;
    }
}

void free_http_message(http_message * message)
{
    free_http_headers(message->header_list);
}


DBGONLY(void print_http_response(http_message * message, Dbg_Level
            level,
            Dbg_Module module,
            char *DbgFileName, int DbgLineNo)
{
    FILE *fd = GetDebugFile(level,module);
 

    http_header *list=message->header_list;
    if (!fd)
        return;
    if (DbgFileName)
    {
        //get lock
        cyg_mutex_lock(&GlobalDebugMutex);
        UpnpDisplayFileAndLine(fd,DbgFileName,DbgLineNo);
    }

    fprintf(fd,"------------------------------------\n");
    fprintf(fd,"RESPONSE: ");
    print_status_line(&message->status,level,module,NULL,0);
    fprintf(fd,"HEADERS: \n");
    while (list)
    {
        print_token(&list->header,level,module,NULL,0);
        print_token(&list->value,level,module,NULL,0);
        list=list->next;
    }
    fprintf(fd,"CONTENT: \n");
    print_token(&message->content,level,module,NULL,0);
    fprintf(fd,"-----------------------------------\n");
    if (DbgFileName)
    {
        cyg_mutex_unlock(&GlobalDebugMutex);
    }
})

    DBGONLY(void print_http_request( http_message * message,Dbg_Level
                level,
                Dbg_Module module, char *DbgFileName,
                int DbgLineNo)
    {
        FILE *fd = GetDebugFile(level,module);
 
        http_header *list=message->header_list;
  
        if (!fd)
            return;
        if (DbgFileName)
        {
            //get lock
            cyg_mutex_lock(&GlobalDebugMutex);
            UpnpDisplayFileAndLine(fd,DbgFileName,DbgLineNo);
        }

        fprintf(fd,"------------------------------------\n");
        fprintf(fd,"HTTP REQUEST RECEIVED:\n");
        print_request_line(&message->request,level,module,NULL,0);
        fprintf(fd,"HEADERS: \n");
        while (list)
        {
            fprintf(fd," Header: ");
            print_token(&list->header,level,module,NULL,0);
            fprintf(fd," Value: ");
            print_token(&list->value,level,module,NULL,0);
            list=list->next;
        }
        fprintf(fd,"CONTENT: \n");
        print_token(&message->content,level,module,NULL,0);
        fprintf(fd,"-----------------------------------\n");
        if (DbgFileName)
        {
            cyg_mutex_unlock(&GlobalDebugMutex);
        }
    })



    int search_for_header(http_message * in, char * header, token *out)
{
http_header *temp=in->header_list;

while (temp)
{ 
    if (temp->header.size== (int)strlen(header))
        if ( !(strncasecmp(header,temp->header.buff,temp->header.size)))
        {
            out->size=temp->value.size;
            out->buff=temp->value.buff;
            return HTTP_SUCCESS;
        }
    temp=temp->next;
}
 return FALSE;

}

int parse_headers(char *in, http_message *out, int max_len)
{
    int counter;
    char * finger=in;
    char * max_finger=in+max_len;
    http_header *current=NULL;
    http_header *head =NULL;

    //parse headers

    while ( (finger<max_finger-1) &&
        ( ( (*finger)!=CR) || ((*(finger+1))!=LF)))
    {
      
        if (current)
        {
            current->next=(http_header *) malloc(sizeof(http_header));
            current=current->next;
        }
        else
        {
            head=(http_header *) malloc(sizeof(http_header));
            current=head;
        }
        if (!current)
        {
            free_http_headers(head);
            return UPNP_E_OUTOF_MEMORY;
        }
        current->next=NULL;
      
        //parse header
        if (!(counter=parse_token(finger, &current->header,max_len)))
        { free_http_headers(head);
        return UPNP_E_BAD_REQUEST;
    
        }
      
        finger+=counter;
        if ( (*finger!=':'))
        {
            free_http_headers(head);
            return UPNP_E_BAD_REQUEST;
        }
        counter++;
        finger++;
        max_len-=counter;

        //get rid of whitespace
        counter=parse_LWS(finger,max_len);
        finger+=counter;
        max_len-=counter;
      
        //parse value, allows empty values
        if (!(counter=parse_header_value(finger,&current->value,max_len)))
        {
            current->value.size=0;
            current->value.buff=NULL;
        }
      
        finger+=counter;
        max_len-=counter;
      
        counter=parse_http_line(finger,max_len);
        finger+=counter;
        max_len-=counter;
    }
  
    out->header_list=head;

    if (finger<max_finger-1)
        finger+=2;
    
    out->content.buff=finger;
    if (max_len>0)
        out->content.size=max_len-2;
    else
        out->content.size=0;

    return HTTP_SUCCESS;
}


int parse_http_response( char * in, http_message *out, int max_len)
{
    int temp;
    int max=max_len;
    char *finger=in;
  
    if ( !(temp=parse_status_line(in,&out->status,max_len)))
    {
        DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"BAD_STATUS LINE"));
        return UPNP_E_BAD_RESPONSE;
    }
    max-=temp;
    finger+=temp;
  
    if ( parse_headers(finger,out,max)!=HTTP_SUCCESS)
    {
        DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"BAD HEADERS"));
        return UPNP_E_BAD_REQUEST;
    }
    return HTTP_SUCCESS;

}

int parse_http_request(  char * in, http_message * out, int max_len)
{
    int temp;
    int max=max_len;
    char *finger=in;
  
    if ( !(temp=parse_request_line(in,&out->request,max_len)))
    {
        DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"BAD REQUEST"));
        return UPNP_E_BAD_REQUEST;
    }
    max-=temp;
    finger+=temp;
  
    if ( parse_headers(finger,out,max)!=HTTP_SUCCESS)
    {
        DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"BAD HEADERS"));
        return UPNP_E_BAD_REQUEST;
    }
    return HTTP_SUCCESS;
  
}
