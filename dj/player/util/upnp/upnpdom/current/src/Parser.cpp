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

//	$Revision: 1.20 $
//	$Date: 2000/10/06 16:03:49 $
#include <util/upnp/api/config.h>
#if EXCLUDE_DOM == 0
#include <stdlib.h>
#include <string.h>
#include <util/upnp/upnpdom/DOMException.h>
#include <util/upnp/upnpdom/Parser.h>
//#include "autoprofile.h"

#ifdef _WIN32
#define strncasecmp strnicmp
#endif

#include <util/upnp/genlib/mystring.h>
#include <cyg/infra/diag.h>


static const char *WHITESPACE	= "\n\t\r\f ";
//static const char *NEWLINE		= "\r";
static const char *NAMECHARS	= "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.:/";
//static const char *NAMECHARS	= "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.:";
static const char *LETTERS		= "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const char *LESSTHAN		= "<";
static const char *GREATERTHAN	= ">";
//static const char *QUESTION		= "?";
//static const char *EXCLAMATION	= "!";
static const char *SLASH		= "/";
static const char *EQUALS		= "=";
//static const char *DASH			= "-";
static const char *UNDERSCORE	= "_";
static const char *QUOTE		= "\"";
static const char *COMPLETETAG	= "/>";
static const char *ENDTAG		= "</";
static const char *BEGIN_COMMENT= "<!--";
static const char *END_COMMENT  = "-->";
static const char *BEGIN_PROCESSING="<?";
static const char *END_PROCESSING="?>";
static const char *BEGIN_DOCTYPE= "<!";

static const char *DEC_NUMBERS	= "0123456789";
static const char *HEX_NUMBERS	= "0123456789ABCDEFabcdef";
static const char *UPPERCASE	= "ABCDEFGHIJKLMNOPQRSTUVWXYZ";


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Parser::Parser(char *ParseStr, bool bCopyString)
{
#ifdef PROFILE
	FunctionProfiler fp("Parser::Parser");
#endif	// PROFILE

	if(ParseStr == NULL)
	{
#ifndef FORCE_NO_EXCEPTIONS
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
		throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
#else	// FORCE_NO_EXCEPTIONS
		DEBUG_THROW_DOM_FATAL_ERROR_DURING_PARSING_EXCEPTION;
#endif	// FORCE_NO_EXCEPTIONS
	}
	if(*ParseStr == '\0')
	{
#ifndef FORCE_NO_EXCEPTIONS
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
		throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
#else	// FORCE_NO_EXCEPTIONS
		DEBUG_THROW_DOM_FATAL_ERROR_DURING_PARSING_EXCEPTION;
#endif	// FORCE_NO_EXCEPTIONS
	}
	int iParseStrLen = strlen(ParseStr);
	if (bCopyString)
	{
		ParseBuff = new char[iParseStrLen + 1];
		if(!ParseBuff)
	   		{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
		strcpy(ParseBuff, ParseStr);
	}
	else
		ParseBuff = ParseStr;

	TokenBuff = new char[iParseStrLen + 1];
	if(!TokenBuff)
	   	{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	LastElement = new char[iParseStrLen + 1];
	if(!LastElement)
	   	{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	CurrPtr=ParseBuff;
	TagVal=false;
	inAttrib=false;
	attrFound = true;
	atLeastOneAttrFound=false;

}

Parser::~Parser()
{
	delete [] ParseBuff;
	delete [] TokenBuff;
	delete [] LastElement;
}

void Parser::IgnoreWhiteSpaces()
{
#ifdef PROFILE
	FunctionProfiler fp("Parser::IgnoreWhiteSpaces");
#endif	// PROFILE
	while(charMatch (*CurrPtr, WHITESPACE))
		getNextToken();
}

//Rewinds Current Ptr by n bytes
void Parser::rewindCurrentPtr(int n)
{
#ifdef PROFILE
	FunctionProfiler fp("Parser::rewindCurrentPtr");
#endif	// PROFILE
	CurrPtr=CurrPtr - n;
}

//Finds the Next Match
//Finds the next character in strSearch that contains strMatch.
// dc- per usage this returns NULL if it can't find the match, or the point within
//     strSearch where it finds a match
char * Parser::findNextMatch(char *strSearch, const char *strMatch)
{
#ifdef PROFILE
	FunctionProfiler fp("Parser::findNextMatch");
#endif	// PROFILE
	char *strIndex;		// Current character match position

	if ((strSearch == NULL) || (strMatch == NULL))
		return NULL;

	strIndex = strSearch;

    //	while ( !(charMatch (*strIndex, strMatch)) && (*strIndex != '\0'))
    // dc- check to see if the string is null first
	while ( (*strIndex != '\0') && !(charMatch (*strIndex, strMatch)) )
		strIndex++;

    // dc- return NULL if the match was not found
	return (*strIndex ? strIndex : NULL);
}

//Copy's the token from source to destination
int Parser::copyToken(char *strDest, char *strSource, int iLength)
{
#ifdef PROFILE
	FunctionProfiler fp("Parser::copyToken");
#endif	// PROFILE
	int iSource = 0;		// index used for copying from source buffer
    //	unsigned int uDest = 0;			// index used for copying to destination buffer
	char cEsc;				// Escape character

	if (!strDest || !strSource || !iLength)
		return -1;
	while (iSource < iLength)
	{
		if (getEscapeCharacter (strSource, (unsigned int *) &iSource, &cEsc))
			*strDest++ = cEsc;
		else
			*strDest++ = *(strSource + iSource);

		iSource++;
        //		uDest++;
	}

	*strDest = '\0'; //add a null terminator

	return 0; //success
}

//#define strncasecmp strncmp

//Get Escape Character
//Returns true if an escape character was found. Returns false otherwise.
//Returns the value of the escape character in pcEsc and updates the source pointer
//to point past the	escape character.
bool Parser::getEscapeCharacter(char *strSource, unsigned int *puSource, char *pcEsc)
{
#ifdef PROFILE
	FunctionProfiler fp("Parser::getEscapeCharacter");
#endif	// PROFILE
	bool	bReturn= false;		// return value
	unsigned int uSum = 0;			// sum value for escape character
	char *pConvert = NULL;	// pointer for converting strings

	if (!strSource || !puSource || !pcEsc)
		return false;

	if (*(strSource + *puSource) == '&')
		bReturn = false;  // for debugging only

	if (!strncasecmp (strSource + *puSource, QUOT, strlen(QUOT)))
	{
		*pcEsc = '\"';
		*puSource += strlen(QUOT) - 1;
		bReturn = true;
	}
	else if (!strncasecmp (strSource + *puSource, LT, strlen(LT)))
	{
		*pcEsc = '<';
		*puSource += strlen(LT) - 1;
		bReturn = true;
	}
	else if (!strncasecmp (strSource + *puSource, GT, strlen(GT)))
	{
		*pcEsc = '>';
		*puSource += strlen(GT) - 1;
		bReturn = true;
	}
	else if (!strncasecmp (strSource + *puSource, APOS, strlen(APOS)))
	{
		*pcEsc = '\'';
		*puSource += strlen(APOS) - 1;
		bReturn = true;
	}
	else if (!strncasecmp (strSource + *puSource, AMP, strlen(AMP)))
	{
		*pcEsc = '&';
		*puSource += strlen(AMP) - 1;
		bReturn = true;
	}
	// Read in escape characters of type &#xnn where nn is a hexadecimal value
	else if (!strncasecmp (strSource + *puSource, ESC_HEX, strlen(ESC_HEX)))
	{
		uSum = 0;
		if (charMatch (*(strSource + *puSource + strlen(ESC_HEX)), HEX_NUMBERS))
		{
			// convert first digit
			if (charMatch (*(strSource + *puSource + strlen(ESC_HEX)), DEC_NUMBERS))
				uSum = *(strSource + *puSource + strlen(ESC_HEX)) - '0'; // 0-9
			else
				if (charMatch (*(strSource + *puSource + strlen(ESC_HEX)), UPPERCASE))
					uSum = *(strSource + *puSource + strlen(ESC_HEX)) - 'A' + 10; // A-F
				else
					uSum = *(strSource + *puSource + strlen(ESC_HEX)) - 'a' + 10; // a-f
		}
		else
			return false;
		
		pConvert = strSource + *puSource + strlen(ESC_HEX) + 1;
		while (charMatch (*pConvert, HEX_NUMBERS))
		{
			if (charMatch (*pConvert, DEC_NUMBERS)) // 0-9
				uSum = uSum * 16 + (*pConvert - '0');
			else
				if (charMatch (*pConvert, UPPERCASE))
					uSum = uSum * 16 + (*pConvert - 'A' + 10); // A-F
				else
					uSum = uSum * 16 + (*pConvert - 'a' + 10); // a-f

			pConvert++;
		}
		if (!charMatch (*pConvert, SEMICOLON))
			bReturn = false;
		else if ((uSum <= 0) || (uSum > 255))
			bReturn = false;
		else
		{
			bReturn = true;
			*pcEsc = uSum;
			*puSource = pConvert - strSource;
		}
	}
	// Read in escape characters of type &#nn where nn is a decimal value
	else if (!strncasecmp (strSource + *puSource, ESC_DEC, strlen(ESC_DEC)))
	{
		uSum = 0;
		if (charMatch (*(strSource + *puSource + strlen(ESC_DEC)), DEC_NUMBERS))
			uSum = *(strSource + *puSource + strlen(ESC_DEC)) - '0'; // convert first digit
		else
			return false;
		
		pConvert = strSource + *puSource + strlen(ESC_DEC) + 1;
		while (charMatch (*pConvert, DEC_NUMBERS))
		{
			uSum = uSum * 10 + (*pConvert - '0');
			pConvert++;
		}

        //		if (!charMatch (*pConvert, SEMICOLON))
        if( *pConvert != ':' )
			bReturn = false;
		else if ((uSum <= 0) || (uSum > 255))
			bReturn = false;
		else
		{
			bReturn = true;
			*pcEsc = uSum;
			*puSource = pConvert - strSource;
		}
	}
	
	return bReturn;
}

#undef strncasecmp

//Skip String
//Skips all characters in the fragment string that are contained within
//strSkipChars until some other character is found.  Useful for skipping
//over whitespace.

long Parser::skipString(char **pstrFragment, const char *strSkipChars)
{
#ifdef PROFILE
	FunctionProfiler fp("Parser::skipString");
#endif	// PROFILE
	if (!pstrFragment || !strSkipChars)
		return -1;

	while ((**pstrFragment != '\0') && (charMatch (**pstrFragment, strSkipChars)))
	{
		(*pstrFragment)++;
	}

	return 0; //success
}

//Skip Until String
//Skips all characters in the string until it finds the skip key.
//Then it skips the skip key and returns.
long Parser::skipUntilString(char **pstrSource, const char *strSkipKey)
{
#ifdef PROFILE
	FunctionProfiler fp("Parser::skipUntilString");
#endif	// PROFILE
	if (!pstrSource || !strSkipKey)
		return -1;

	while (**pstrSource && strncmp (*pstrSource, strSkipKey, strlen(strSkipKey)))
		(*pstrSource)++;

	*pstrSource = *pstrSource + strlen (strSkipKey);

	return 0; //success
}

//Character Match
//Returns true if character cUnknown is in the match set.
//Returns false otherwise.

// dc- this is the simplified inline version
//while( *strMatchSet ) {
//  if( *strMatchSet++ == cUnknown )
//    return true;
//}
//return false;
#if 0
bool Parser::charMatch(char cUnknown, const char * strMatchSet)
{
#ifdef PROFILE
	FunctionProfiler fp("Parser::charMatch");
#endif	// PROFILE
	int iMatchLen = 0;		// Length of the match string
	int iIndex = 0;			// Index through match set looking for match with cUnknown
	bool bReturn = false;	// Return value: true if cUnknown is in strMatchSet

	iMatchLen = strlen (strMatchSet);

	if ((strMatchSet == NULL) || (iMatchLen <= 0))
		return false;

	while ((iIndex < iMatchLen) && (!bReturn))
	{
		if (*(strMatchSet + iIndex) == cUnknown)
			bReturn = true;

		iIndex++;
	}

	return bReturn;
}
#endif
#include <cyg/infra/diag.h>

//This will return the string of the next token in the TokenBuff
int Parser::getNextToken()
{
#ifdef PROFILE
	FunctionProfiler fp("Parser::getNextToken");
#endif	// PROFILE
	int TokenLength=0;
	// Check for white space
	if(*CurrPtr=='\0')
	{
		TokenBuff[0]='\0';					
		return 0;
	}

	// Attribute value logic must come first, since all text untokenized until end-quote
    //	if (inAttrib && (!charMatch (*CurrPtr, QUOTE)))
    if( inAttrib && *CurrPtr != '\"' )
	{
		char *strEndQuote = findNextMatch (CurrPtr, QUOTE);
		if (strEndQuote == NULL)
		{
			TokenLength = 1;
			*TokenBuff = '\0';
			 // return a single space for whitespace
			copyToken (TokenBuff, CurrPtr, TokenLength);
			return 0; // serious problem - no matching end-quote found for attribute
		}

		TokenLength = strEndQuote - CurrPtr; // BUGBUG: conversion issue if using more than simple strings
		copyToken (TokenBuff, CurrPtr, TokenLength);
		CurrPtr = CurrPtr+TokenLength;
		return 0; // must return now, so it doesn't go into name processing
	}
	
	if (charMatch (*CurrPtr, WHITESPACE))
	{
		TokenLength = 1;
		copyToken (TokenBuff, " ", TokenLength); // return a single space for whitespace
		CurrPtr = CurrPtr+TokenLength;
		return 0;
	}
#define PARSE_DOCTYPE
#ifdef PARSE_DOCTYPE
	// Skip <? .. ?> , <!-- .. -->
	while (!strncmp (CurrPtr, BEGIN_COMMENT, strlen(BEGIN_COMMENT))  // <!--
//			|| !strncmp (CurrPtr, "<\\", 2)
			|| !strncmp (CurrPtr, BEGIN_PROCESSING, strlen(BEGIN_PROCESSING))) // <?
#else	// PARSE_DOCTYPE
	// Skip <? .. ?> , <! .. >, <!-- .. -->
	while (!strncmp (CurrPtr, BEGIN_COMMENT, strlen(BEGIN_COMMENT))  // <!--
			|| !strncmp (CurrPtr, BEGIN_PROCESSING, strlen(BEGIN_PROCESSING)) // <?
			|| !strncmp (CurrPtr, BEGIN_DOCTYPE, strlen(BEGIN_DOCTYPE)))  // <!
#endif	// PARSE_DOCTYPE
	{
		if (!strncmp (CurrPtr, BEGIN_COMMENT, strlen(BEGIN_COMMENT)))
			skipUntilString (&CurrPtr, END_COMMENT);
		else if (!strncmp (CurrPtr, BEGIN_PROCESSING, strlen(BEGIN_PROCESSING)))
			skipUntilString (&CurrPtr, END_PROCESSING);
		else
			skipUntilString (&CurrPtr, GREATERTHAN);

		skipString (&CurrPtr, WHITESPACE);
		TagVal=false;
	}

#ifdef PARSE_DOCTYPE
	if (!strncmp (CurrPtr, BEGIN_DOCTYPE, strlen(BEGIN_DOCTYPE)))  // <!
	{
		char* pch = CurrPtr;
		if (*(pch + 2) == '[')
		{
			pch += 3;
			int iBracketCount = 1;
			while (iBracketCount && *pch)
			{
				if (*pch == ']')
					--iBracketCount;
				else if (*pch == '[')
					++iBracketCount;
				++pch;
			}
		}
		char *strEndTag = findNextMatch (pch, GREATERTHAN);
		if (strEndTag == NULL)
		{
			TokenLength = 1;
			*TokenBuff = '\0';
			 // return a single space for whitespace
			copyToken (TokenBuff, CurrPtr, TokenLength);
			return 0; // serious problem - no matching end-quote found for attribute
		}

		TokenLength = strEndTag - CurrPtr + 1;
		copyToken (TokenBuff, CurrPtr, TokenLength);
		CurrPtr = CurrPtr+TokenLength;
		return 0; // must return now, so it doesn't go into name processing
	}


#endif	// PARSE_DOCTYPE

	// Check for start tags
    if( *CurrPtr == '<' )
    //	if (charMatch (*CurrPtr, LESSTHAN))
	{
		if ((charMatch (*(CurrPtr + 1), LETTERS))
			|| (charMatch (*(CurrPtr + 1), UNDERSCORE))
			|| (charMatch (*(CurrPtr + 1), WHITESPACE)))
		{
			// Begin tag found, so return '<' token
			TokenLength = 1; //strlen(LESSTHAN);
		}
        else if( *(CurrPtr + 1) == '/' )
        //		else if (charMatch (*(CurrPtr + 1), SLASH))  
		{
			TokenLength = 2;  // token is '</' end tag
		}
		else
		{
			strcpy(TokenBuff, "\0");
			return 1;
		}
		TagVal=false;
	}

		// Check for opening/closing attribute value quotation mark
    //	if (charMatch (*CurrPtr, QUOTE) && !TagVal)
    if( *CurrPtr == '\"' && !TagVal )
	{
		// Quote found, so return it as token
		TokenLength = 1; //strlen(QUOTE);
	}

	// Check for '=' token
    //	if (charMatch (*CurrPtr, EQUALS) && !TagVal)
    if( *CurrPtr == '=' && !TagVal )
	{
		// Equals found, so return it as a token
		TokenLength = 1; //strlen(EQUALS);
	}

	if( *CurrPtr == '/' ) //charMatch (*CurrPtr, SLASH))
	{
        if( *(CurrPtr+1) == '>' )
            //		if (charMatch (*(CurrPtr + 1), GREATERTHAN))
		{
			// token '/>' found
			TokenLength = 2;
			TagVal=true;
		}
		//Content may begin with a /
    	else if (TagVal)
    	{
    		TagVal=false;
    		CurrPtr=SavePtr+1;//SavePtr whould not have have already moved.
    		char *pEndContent = CurrPtr;

    		// Read content until a < is found that is not a comment <!--
    		bool bReadContent = true;

    		while (bReadContent)
    		{
                while( *pEndContent && *pEndContent != '<' )
                    pEndContent++;
                
                //    			while (!charMatch (*pEndContent, LESSTHAN) && *pEndContent)
                //    				pEndContent++;
    			if( pEndContent && !strncmp (pEndContent, BEGIN_COMMENT, strlen (BEGIN_COMMENT)) )
    				skipUntilString( &pEndContent, END_COMMENT );
    			else
    				bReadContent = false;

    			if (!(*pEndContent))
    				bReadContent = false;
    		}
    		TokenLength = pEndContent - CurrPtr;
    	}
	}
	// Check for '>' token
    //	else if (charMatch (*CurrPtr, GREATERTHAN))
    else if( *CurrPtr == '>' )
	{
		// Equals found, so return it as a token
		TokenLength = 1; //strlen(GREATERTHAN);
		SavePtr=CurrPtr; // Saving this ptr for not ignoring the leading and trailing spaces.
		TagVal=true;
	}

	// Check for Content e.g.  <tag>content</tag>
	else if (TagVal)
	{
		TagVal=false;
		CurrPtr=SavePtr+1;//SavePtr whould not have have already moved.
		char *pEndContent = CurrPtr;

		// Read content until a < is found that is not a comment <!--
		bool bReadContent = true;

		while (bReadContent)
		{
            //			while (!charMatch (*pEndContent, LESSTHAN) && *pEndContent)
            while( *pEndContent && *pEndContent != '<' )
				pEndContent++;

			if( pEndContent && !strncmp (pEndContent, BEGIN_COMMENT, strlen (BEGIN_COMMENT)) )
				skipUntilString (&pEndContent, END_COMMENT);
			else
				bReadContent = false;

			if (!(*pEndContent))
				bReadContent = false;
		}
		TokenLength = pEndContent - CurrPtr;
	}
	// Check for name tokens
	else if ((charMatch (*CurrPtr, LETTERS))
		|| (charMatch (*CurrPtr, UNDERSCORE)))
	{
		// Name found, so find out how long it is
		int iIndex = 0;
		while (charMatch (*(CurrPtr + iIndex), NAMECHARS))
		{
            //			if (charMatch(*(CurrPtr + iIndex), SLASH) && charMatch(*(CurrPtr + iIndex + 1), GREATERTHAN))
            if( *(CurrPtr + iIndex) == '/' && *(CurrPtr + iIndex + 1) == '>' )
				break;
			iIndex++;
		}

		TokenLength = iIndex;
	}

	// Copy the token to the return string
	if (TokenLength > 0)
		copyToken (TokenBuff, CurrPtr, TokenLength);
	else
	{
		
		// return the unrecognized token for error information
		TokenLength = 1;
		copyToken (TokenBuff, CurrPtr, TokenLength);
		//Check for end of file
		if(*CurrPtr == '\0')
			return 0;
		TokenLength = 0;
		return 1;
	}
	CurrPtr = CurrPtr+TokenLength;
	return 0;
}

#ifndef FORCE_NO_EXCEPTIONS
//Will return a handle to the tree structure
//The tree structure indicates to which parent the node belongs.
int Parser::getNextNode(NODE_TYPE &NodeType, char **NodeName, char **NodeValue, bool &IsEnd,  bool IgnoreWhiteSpace)
{
#ifdef PROFILE
	FunctionProfiler fp("Parser::getNextNode");
#endif	// PROFILE
	while(*CurrPtr!='\0')
	{
		if(IgnoreWhiteSpace)
			IgnoreWhiteSpaces();
		if(getNextToken()!=0)
		{
			*NodeValue=NULL;
			*NodeName=NULL;
			NodeType=INVALID_NODE;
			IsEnd=false;
			return 1;
		}
		if(!strcmp(TokenBuff, LESSTHAN))
		{
			if(IgnoreWhiteSpace)
				IgnoreWhiteSpaces();
			if(getNextToken()!=0)
			{
				*NodeValue=NULL;
				*NodeName=NULL;
				NodeType=INVALID_NODE;
				IsEnd=false;
				return 1;
			}
			if(TokenBuff==NULL)
			{
				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
				throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
			}
			if(*TokenBuff=='\0')
			{
				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
				throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
			}
			*NodeName=(char *)malloc(strlen(TokenBuff)+1);
			if(!*NodeName)
	   			{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
			strcpy(*NodeName,TokenBuff);
			strcpy(LastElement,TokenBuff);
			*NodeValue=NULL;
			NodeType=ELEMENT_NODE;
			attrFound=true;
			atLeastOneAttrFound=false;
			IsEnd=false;
		}
		else if(!strcmp(TokenBuff, GREATERTHAN))
		{
			attrFound=false;
			if(atLeastOneAttrFound)//forget the greater than
			{
				atLeastOneAttrFound=false;
				continue;
			}
			else
				return 0;
		}
		else if(!strcmp(TokenBuff, ENDTAG))
		{
			if(IgnoreWhiteSpace)
				IgnoreWhiteSpaces();
			if(getNextToken()!=0)
			{
				*NodeValue=NULL;
				*NodeName=NULL;
				NodeType=INVALID_NODE;
				IsEnd=false;
				return 1;
			}
			if(TokenBuff==NULL)
			{
				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
				throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
			}
			if(*TokenBuff=='\0')
			{
				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
				throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
			}
			*NodeName=(char *)malloc(strlen(TokenBuff)+1);
			if(!*NodeName)
	   			{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
			strcpy(*NodeName,TokenBuff);
			*NodeValue=NULL;
			NodeType=ELEMENT_NODE;
			IsEnd=true;
		}
		else if(!strcmp(TokenBuff, COMPLETETAG))
		{
			if(NodeType==ELEMENT_NODE || (NodeType==ATTRIBUTE_NODE))
			{
				IsEnd=false;
				rewindCurrentPtr(strlen(TokenBuff));
				return 0;
			}
			//store the last element tag and return back as end element node
			*NodeName=(char *)malloc(strlen(LastElement)+1);
			if(!*NodeName)
	   			{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
			strcpy(*NodeName,LastElement);
			*NodeValue=NULL;
			NodeType=ELEMENT_NODE;
			attrFound=true;
			atLeastOneAttrFound=false;
			IsEnd=true;
			return 0;
		}
		else if(TokenBuff[0] == '\0')
		{
			IsEnd=false;
			continue;
		}
		else if(!attrFound)
		{
			if(TokenBuff!=NULL)
			{
				if(*TokenBuff!='\0')
				{
					*NodeValue=(char *)malloc(strlen(TokenBuff)+1);
					if(!*NodeValue)
	   					{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
					strcpy(*NodeValue,TokenBuff);
				}
    		}
			*NodeName=(char *)malloc(strlen("#text")+1);
			if(!*NodeName)
	   			{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
			strcpy(*NodeName,"#text");
			NodeType=TEXT_NODE;
			IsEnd=false;
			return 0;
		}
		else
		{
			if(TokenBuff==NULL)
			{
				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
				throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
			}
			if(*TokenBuff=='\0')
			{
				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
				throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
			}
			if(NodeType==ELEMENT_NODE)
			{
				rewindCurrentPtr(strlen(TokenBuff)+1);
				return 0;
			}
			*NodeName=(char *)malloc(strlen(TokenBuff)+1);
			if(!*NodeName)
	   			{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
			strcpy(*NodeName,TokenBuff);
			if(IgnoreWhiteSpace) 
				IgnoreWhiteSpaces(); 
			//gets rid of equals
			if(getNextToken()!=0)
			{
				*NodeValue=NULL;
				*NodeName=NULL;
				NodeType=INVALID_NODE;
				IsEnd=false;
				return 1;
			}
			if(IgnoreWhiteSpace)
				IgnoreWhiteSpaces(); 
			if(getNextToken()!=0)
			{
				*NodeValue=NULL;
				*NodeName=NULL;
				NodeType=INVALID_NODE;
				IsEnd=false;
				return 1;
			}
			//gets rid of beginning quotes			
			inAttrib=true;
			if(getNextToken()!=0)
			{
				*NodeValue=NULL;
				*NodeName=NULL;
				NodeType=INVALID_NODE;
				IsEnd=false;
				return 1;
			}
			inAttrib=false;
			if(TokenBuff==NULL)
			{
				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
				throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
			}
			if(*TokenBuff=='\0')
			{
				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
				throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
			}
			*NodeValue=(char *)malloc(strlen(TokenBuff)+1);
			if(!*NodeValue)
	   			{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
			strcpy(*NodeValue,TokenBuff);
			// gets rid of ending quotes
			if(getNextToken()!=0)
			{
				*NodeValue=NULL;
				*NodeName=NULL;
				NodeType=INVALID_NODE;
				IsEnd=false;
				return 1;
			}
			NodeType=ATTRIBUTE_NODE;
			IsEnd=false;
			atLeastOneAttrFound=true;
			return 0;
		}
	}
	return 0;
}
#endif	// FORCE_NO_EXCEPTIONS

//Will return a handle to the tree structure
//The tree structure indicates to which parent the node belongs.
EDERRCODE Parser::getNextNodeNoEx(NODE_TYPE &NodeType, char **NodeName, char **NodeValue, bool &IsEnd, bool IgnoreWhiteSpace, int& oldretval)
{
#ifdef PROFILE
	FunctionProfiler fp("Parser::getNextNodeNoEx");
#endif	// PROFILE
	while(*CurrPtr!='\0')
	{
		if(IgnoreWhiteSpace)
			IgnoreWhiteSpaces();
		if(getNextToken()!=0)
		{
			*NodeValue=NULL;
			*NodeName=NULL;
			NodeType=INVALID_NODE;
			IsEnd=false;
			oldretval = 1;
			return ED_OK;
		}
		if(!strcmp(TokenBuff, LESSTHAN))
		{
			if(IgnoreWhiteSpace)
				IgnoreWhiteSpaces();
			if(getNextToken()!=0)
			{
				*NodeValue=NULL;
				*NodeName=NULL;
				NodeType=INVALID_NODE;
				IsEnd=false;
				oldretval = 1;
				return ED_OK;
			}
			if(TokenBuff==NULL)
			{
				DEBUG_THROW_DOM_FATAL_ERROR_DURING_PARSING_EXCEPTION;
				return EDERR_DOM_FATAL_ERROR_DURING_PARSING_EXCEPTION;
//				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//				throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
			}
			if(*TokenBuff=='\0')
			{
//				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//				throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
				DEBUG_THROW_DOM_FATAL_ERROR_DURING_PARSING_EXCEPTION;
				return EDERR_DOM_FATAL_ERROR_DURING_PARSING_EXCEPTION;
			}
			*NodeName=(char *)malloc(strlen(TokenBuff)+1);
			if(!*NodeName)
	   			{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
			strcpy(*NodeName,TokenBuff);
			strcpy(LastElement,TokenBuff);
			*NodeValue=NULL;
			NodeType=ELEMENT_NODE;
			attrFound=true;
			atLeastOneAttrFound=false;
			IsEnd=false;
		}
		else if(!strcmp(TokenBuff, GREATERTHAN))
		{
			attrFound=false;
			if(atLeastOneAttrFound)//forget the greater than
			{
				atLeastOneAttrFound=false;
				continue;
			}
			else
			{
				oldretval = 0;
				return ED_OK;
			}
		}
		else if(!strcmp(TokenBuff, ENDTAG))
		{
			if(IgnoreWhiteSpace)
				IgnoreWhiteSpaces();
			if(getNextToken()!=0)
			{
				*NodeValue=NULL;
				*NodeName=NULL;
				NodeType=INVALID_NODE;
				IsEnd=false;
				oldretval = 1;
				return ED_OK;
			}
			if(TokenBuff==NULL)
			{
//				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//				throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
				DEBUG_THROW_DOM_FATAL_ERROR_DURING_PARSING_EXCEPTION;
				return EDERR_DOM_FATAL_ERROR_DURING_PARSING_EXCEPTION;
			}
			if(*TokenBuff=='\0')
			{
//				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//				throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
				DEBUG_THROW_DOM_FATAL_ERROR_DURING_PARSING_EXCEPTION;
				return EDERR_DOM_FATAL_ERROR_DURING_PARSING_EXCEPTION;
			}
			*NodeName=(char *)malloc(strlen(TokenBuff)+1);
			if(!*NodeName)
	   			{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
			strcpy(*NodeName,TokenBuff);
			*NodeValue=NULL;
			NodeType=ELEMENT_NODE;
			IsEnd=true;
		}
		else if(!strcmp(TokenBuff, COMPLETETAG))
		{
			if(NodeType==ELEMENT_NODE || (NodeType==ATTRIBUTE_NODE))
			{
				IsEnd=false;
				rewindCurrentPtr(strlen(TokenBuff));
				oldretval = 0;
				return ED_OK;
			}
			//store the last element tag and return back as end element node
			*NodeName=(char *)malloc(strlen(LastElement)+1);
			if(!*NodeName)
	   			{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
			strcpy(*NodeName,LastElement);
			*NodeValue=NULL;
			NodeType=ELEMENT_NODE;
			attrFound=true;
			atLeastOneAttrFound=false;
			IsEnd = true;
			oldretval = 0;
			return ED_OK;
		}
		else if(TokenBuff[0] == '\0')
		{
			IsEnd=false;
			continue;
		}
		else if(!attrFound)
		{
			if(TokenBuff!=NULL)
			{
				if(*TokenBuff!='\0')
				{
					*NodeValue=(char *)malloc(strlen(TokenBuff)+1);
					if(!*NodeValue)
	   					{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
					strcpy(*NodeValue,TokenBuff);
				}
    		}
			*NodeName=(char *)malloc(strlen("#text")+1);
			if(!*NodeName)
	   			{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
			strcpy(*NodeName,"#text");
			NodeType=TEXT_NODE;
			IsEnd=false;
			oldretval = 0;
			return ED_OK;
		}
		else
		{
			if(TokenBuff==NULL)
			{
//				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//				throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
				DEBUG_THROW_DOM_FATAL_ERROR_DURING_PARSING_EXCEPTION;
				return EDERR_DOM_FATAL_ERROR_DURING_PARSING_EXCEPTION;
			}
			if(*TokenBuff=='\0')
			{
//				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//				throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
				DEBUG_THROW_DOM_FATAL_ERROR_DURING_PARSING_EXCEPTION;
				return EDERR_DOM_FATAL_ERROR_DURING_PARSING_EXCEPTION;
			}
			if(NodeType==ELEMENT_NODE)
			{
				rewindCurrentPtr(strlen(TokenBuff)+1);
				oldretval = 0;
				return ED_OK;
			}
			*NodeName=(char *)malloc(strlen(TokenBuff)+1);
			if(!*NodeName)
	   			{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
			strcpy(*NodeName,TokenBuff);
			if(IgnoreWhiteSpace) 
				IgnoreWhiteSpaces(); 
			//gets rid of equals
			if(getNextToken()!=0)
			{
				*NodeValue=NULL;
				*NodeName=NULL;
				NodeType=INVALID_NODE;
				IsEnd=false;
				oldretval = 1;
				return ED_OK;
			}
			if(IgnoreWhiteSpace)
				IgnoreWhiteSpaces(); 
			if(getNextToken()!=0)
			{
				*NodeValue=NULL;
				*NodeName=NULL;
				NodeType=INVALID_NODE;
				IsEnd=false;
				oldretval = 1;
				return ED_OK;
			}
			//gets rid of beginning quotes			
			inAttrib=true;
			if(getNextToken()!=0)
			{
				*NodeValue=NULL;
				*NodeName=NULL;
				NodeType=INVALID_NODE;
				IsEnd=false;
				oldretval = 1;
				return ED_OK;
			}
			inAttrib=false;
			if(TokenBuff==NULL)
			{
//				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//				throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
				DEBUG_THROW_DOM_FATAL_ERROR_DURING_PARSING_EXCEPTION;
				return EDERR_DOM_FATAL_ERROR_DURING_PARSING_EXCEPTION;
			}
			if(*TokenBuff=='\0')
			{
//				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//				throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
				DEBUG_THROW_DOM_FATAL_ERROR_DURING_PARSING_EXCEPTION;
				return EDERR_DOM_FATAL_ERROR_DURING_PARSING_EXCEPTION;
			}
			*NodeValue=(char *)malloc(strlen(TokenBuff)+1);
			if(!*NodeValue)
	   			{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
			strcpy(*NodeValue,TokenBuff);
			// gets rid of ending quotes
			if(getNextToken()!=0)
			{
				*NodeValue=NULL;
				*NodeName=NULL;
				NodeType=INVALID_NODE;
				IsEnd=false;
				oldretval = 1;
				return ED_OK;
			}
			NodeType=ATTRIBUTE_NODE;
			IsEnd=false;
			atLeastOneAttrFound=true;
			oldretval = 0;
			return ED_OK;
		}
	}
	oldretval = 0;
	return ED_OK;
}

#endif			
		
		






