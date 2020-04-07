/*
 * parser.c
 *
 * parses text to an XML object
 */

#include <extras/cddb/gn_platform.h>
#include GN_STRING_H
#include GN_CTYPE_H
#include <extras/cddb/gn_utils.h>
#include <extras/cddb/microxml.h>
#include <extras/cddb/gn_memory.h>


/*
 * Constants
 */

	/* Scan States */
#define	kStartState				0
#define	kGotLessThan			1
#define	kAfterGreaterThan		2
#define	kBeginScanningWord		3
#define	kContinueScanningWord	4
#define	kBeginQuotedString		5
#define	kContinueQuotedString	6
#define	kBeginDataString		7
#define	kContinueDataString		8
#define	kGotSlash				9

	/* Token Types */
#define	kNoToken				0
#define	kEndOfInput				1
#define	kLessThan				2
#define	kGreaterThan			3
#define	kLessThanSlash			4
#define	kEquals					5
#define	kString					6
#define	kSlash					7
#define	kSlashGreaterThan		8


/*
 * Typedefs
 */

typedef gn_int16_t	ScanState;
typedef gn_int16_t	TokenType;

typedef struct
{
	const char* buffer;
	int maxChars;
	int index;
	ScanState state;
}
ScanBuffer;

typedef struct
{
	TokenType type;
	const char* value;
	int valueLen;
}
Token;

static XMLError InitializeScanBuffer(ScanBuffer* sb, gn_cstr_t buf, gn_uint32_t bufLen);
static XMLError ParseXMLHeader(ScanBuffer* sb);
static int IsWordChar(char ch);
static XMLError GetNextToken(ScanBuffer* sb, Token* t);
static XMLError ParseXMLTag(XMLTagRef xmlTag, ScanBuffer* sb);
static XMLError UnescapeSpecialCharacters(const char* rawData, int rawDataLen, char** filteredData, int* filteredDataLen);

/*******************************************************************/

XMLError ParseStrToXMLTag(gn_cstr_t str, XMLTagRef* xmlResult)
{
	return GNERR_ERR_CODE((ParseBufToXMLTag(str, str ? strlen(str) : 0, xmlResult)));
}

XMLError ParseBufToXMLTag(gn_cstr_t buf, gn_uint32_t bufLen, XMLTagRef* xmlResult)
{
	XMLError result = kXMLNoError;
	XMLTagRef xmlTag = 0;
	
	*xmlResult = 0;
	
	if (buf == 0 || bufLen == 0 || xmlResult == 0)
		return GNERR_ERR_CODE(kXMLInvalidParamError);
	
	xmlTag = CreateEmptyXMLTag();
	
	if (xmlTag != 0)
	{
		ScanBuffer sb;
		
		result = InitializeScanBuffer(&sb, buf, bufLen);
		
		if (result == kXMLNoError)
			result = ParseXMLHeader(&sb);

		if (result == kXMLNoError)
		{
			Token t;
			
			result = GetNextToken(&sb, &t);
			
			if (result == kXMLNoError)
			{
				if (t.type == kLessThan)
					result = ParseXMLTag(xmlTag, &sb);
				else
					result = kXMLSyntaxError;
			}
		}
		
		if (result != kXMLNoError)
		{
			DisposeXMLTag(xmlTag);
			xmlTag = 0;
		}
		
		*xmlResult = xmlTag;
	}
	else
		result = kXMLOutOfMemoryError;
	
	return GNERR_ERR_CODE(result);
}

/*******************************************************************/

static XMLError InitializeScanBuffer(ScanBuffer* sb, gn_cstr_t buf, gn_uint32_t bufLen)
{
	if (buf == 0 || bufLen == 0)
		return GNERR_ERR_CODE(kXMLInvalidParamError);
	
	sb->buffer = buf;
	sb->maxChars = bufLen;
	sb->index = 0;
	sb->state = kStartState;
	
	return GNERR_ERR_CODE(kXMLNoError);
}

static XMLError ParseXMLHeader(ScanBuffer* sb)
{
	/* "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n" */

	/* skip whitespace */
	while ((sb->index < sb->maxChars) && isspace(sb->buffer[sb->index]))
		sb->index++;

	/* match "<", fail if not found */
	if (sb->index < sb->maxChars && sb->buffer[sb->index] == '<')
		sb->index++;
	else
		return GNERR_ERR_CODE(kXMLSyntaxError);

	/* match "?", back up and exit if not found */
	if (sb->index < sb->maxChars)
	{
		if (sb->buffer[sb->index] == '?')
		{
			sb->index++;
		}
		else
		{
			sb->index--;
			return GNERR_ERR_CODE(kXMLNoError);
		}
	}
	else
		return GNERR_ERR_CODE(kXMLSyntaxError);

	/* match "xml", fail if not found */
	if (sb->index < sb->maxChars && strncmp(sb->buffer + sb->index, "xml", 3) == 0)
		sb->index += 3;
	else
		return GNERR_ERR_CODE(kXMLSyntaxError);

	/* skip until "?" TBD REC: embedded '?'s in the header could break this */
	while (sb->index < sb->maxChars && sb->buffer[sb->index] != '?')
		sb->index++;

	/* match "?>", fail if not found */
	if (sb->index < sb->maxChars && strncmp(sb->buffer + sb->index, "?>", 2) == 0)
		sb->index += 2;
	else
		return GNERR_ERR_CODE(kXMLSyntaxError);

	return GNERR_ERR_CODE(kXMLNoError);
}

static int IsWordChar(char ch)
{
	/* TBD REC: extend this to better comply with the XML spec */
	return isalnum(ch) || ch == '_';
}

static XMLError GetNextToken(ScanBuffer* sb, Token* t)
{
	XMLError result = kXMLNoError;
	
	t->type = kNoToken;
	t->value = 0;
	t->valueLen = 0;
	
	while (sb->index < sb->maxChars && result == kXMLNoError && t->type == kNoToken)
	{
		char ch = sb->buffer[sb->index];
		
		switch (sb->state)
		{
			case kStartState:
				if (ch == '<')
				{
					sb->state = kGotLessThan;
					sb->index++;
				}
				else if (ch == '>')
				{
					t->type = kGreaterThan;
					t->value = sb->buffer + sb->index;
					t->valueLen = 1;
					sb->index++;
					sb->state = kAfterGreaterThan;
				}
				else if (ch == '=')
				{
					t->type = kEquals;
					t->value = sb->buffer + sb->index;
					t->valueLen = 1;
					sb->index++;
				}
				else if (ch == '\"')
				{
					sb->state = kBeginQuotedString;
					sb->index++;
				}
				else if (ch == '/')
				{
					sb->state = kGotSlash;
					sb->index++;
				}
				else if (IsWordChar(ch))
				{
					sb->state = kBeginScanningWord;
				}
				else if (isspace(ch))
				{
					sb->index++;
				}
				else
				{
					result = kIllegalXMLCharacterError;
				}
				break;
			
			case kGotLessThan:
				if (ch == '/')
				{
					t->type = kLessThanSlash;
					t->value = sb->buffer + sb->index - 1;
					t->valueLen = 2;
					sb->index++;
				}
				else
				{
					t->type = kLessThan;
					t->value = sb->buffer + sb->index - 1;
					t->valueLen = 1;
				}
				sb->state = kStartState;
				break;
			
			case kGotSlash:
				if (ch == '>')
				{
					t->type = kSlashGreaterThan;
					t->value = sb->buffer + sb->index - 1;
					t->valueLen = 2;
					sb->index++;
				}
				else
				{
					t->type = kSlash;
					t->value = sb->buffer + sb->index - 1;
					t->valueLen = 1;
				}
				sb->state = kStartState;
				break;
			
			case kAfterGreaterThan:
				if (isspace(ch))
				{
					sb->index++;
				}
				else if (ch != '<' /*IsWordChar(ch)*/)
				{
					sb->state = kBeginDataString;
				}
				else
				{
					sb->state = kStartState;
				}
				break;
			
			case kBeginScanningWord:
				if (IsWordChar(ch))
				{
					t->value = sb->buffer + sb->index;
					sb->index++;
					sb->state = kContinueScanningWord;
				}
				else
				{
					result = kIllegalXMLCharacterError;
				}
				break;
			
			case kContinueScanningWord:
				if (IsWordChar(ch))
				{
					sb->index++;
				}
				else
				{
					t->type = kString;
					t->valueLen = (sb->buffer + sb->index) - t->value;
					sb->state = kStartState;
				}
				break;
			
			case kBeginQuotedString:
				if (ch == '\"')
				{
					t->type = kString;
					t->valueLen = 0;
					sb->state = kStartState;
					sb->index++;
				}
				else
				{
					t->value = sb->buffer + sb->index;
					sb->index++;
					sb->state = kContinueQuotedString;
				}
				break;
			
			case kContinueQuotedString:
				if (ch != '\"')
				{
					sb->index++;
				}
				else
				{
					t->type = kString;
					t->valueLen = (sb->buffer + sb->index) - t->value;
					sb->state = kStartState;
					sb->index++;
				}
				break;
			
			case kBeginDataString:
				if (ch != '<' /*IsWordChar(ch)*/)
				{
					t->value = sb->buffer + sb->index;
					sb->index++;
					sb->state = kContinueDataString;
				}
				else
				{
					result = kIllegalXMLCharacterError;
				}
				break;
			
			case kContinueDataString:
				if (ch != '<')
				{
					sb->index++;
				}
				else
				{
					t->type = kString;
					t->valueLen = (sb->buffer + sb->index) - t->value;
					sb->state = kStartState;
				}
				break;
		}
	}
	
	if (t->type == kNoToken && sb->index >= sb->maxChars && result == kXMLNoError)
		result = kXMLUnexpectedEndOfInputError;
	
	return GNERR_ERR_CODE(result);
}

static XMLError ParseXMLTag(XMLTagRef xmlTag, ScanBuffer* sb)
{
	XMLError result = kXMLNoError;
	Token t;
	int tagHasContent = 1;
	
	/* scan the first word into the tag name */
	if (result == kXMLNoError)
	{
		result = GetNextToken(sb, &t);
		
		if (result == kXMLNoError)
		{
			if (t.type == kString && t.value != 0 && t.valueLen > 0)
				result = SetXMLTagNameFromBuf(xmlTag, t.value, t.valueLen);
			else
				result = kXMLSyntaxError;
		}
	}
	
	/* try to scan the attribute list */
	while (result == kXMLNoError)
	{
		result = GetNextToken(sb, &t);
		
		if (result != kXMLNoError)
			break;
		
		if (t.type == kGreaterThan)
			break;

		if (t.type == kSlashGreaterThan)
		{
			tagHasContent = 0;
			break;
		}
		
		if (t.type == kString && t.value != 0 && t.valueLen > 0)
		{
			const char* attrNamePtr = t.value;
			int attrNameLen = t.valueLen;
			
			result = GetNextToken(sb, &t);
			
			if (result != kXMLNoError)
				break;
			
			if (t.type != kEquals)
			{
				result = kXMLSyntaxError;
				break;
			}
			
			result = GetNextToken(sb, &t);
			
			if (result == kXMLNoError)
			{
				if (t.type == kString)	/* we tolerate empty attribute values */
				{
					char* attrDataPtr = 0;
					int attrDataLen = 0;

					result = UnescapeSpecialCharacters(t.value, t.valueLen, &attrDataPtr, &attrDataLen);
					
					if (result == kXMLNoError)
					{
						result = SetXMLTagAttrFromBuf(xmlTag, attrNamePtr, attrNameLen,
													  attrDataPtr, attrDataLen);
					}

					if (attrDataPtr != 0)
						gnmem_free((void*)attrDataPtr);
				}
				else
				{
					result = kXMLSyntaxError;
					break;
				}
			}
		}
		else
		{
			result = kXMLSyntaxError;
			break;
		}
	}
	
	if (tagHasContent != 0)
	{
		/* deal with the three things that can come after the tag opening */
		while (result == kXMLNoError)
		{
			result = GetNextToken(sb, &t);
			
			if (result != kXMLNoError)
				break;
				
			if (t.type == kString && GetXMLTagData(xmlTag) == 0)
			{
				if (t.value != 0 && t.valueLen > 0)
				{
					char* dataPtr = 0;
					int dataLen = 0;

					result = UnescapeSpecialCharacters(t.value, t.valueLen, &dataPtr, &dataLen);
					
					if (result == kXMLNoError)
						result = SetXMLTagDataFromBuf(xmlTag, dataPtr, dataLen);	/* might want to trim trailing whitespace from this... */

					if (dataPtr != 0)
						gnmem_free((void*)dataPtr);
				}
				else
				{
					result = kXMLSyntaxError;
				}
			}
			else if (t.type == kLessThan)
			{
				XMLTagRef subTag = CreateEmptyXMLTag();
				
				result = ParseXMLTag(subTag, sb);
				
				if (result == kXMLNoError)
					AddXMLSubTag(xmlTag, subTag);
				else
					DisposeXMLTag(subTag);
			}
			else if (t.type == kLessThanSlash)
			{
				result = GetNextToken(sb, &t);
				
				if (result != kXMLNoError)
					break;
				
				if (t.type != kString)
				{
					result = kXMLSyntaxError;
					break;
				}
				
				if (gn_bufcmp(t.value, t.valueLen, GetXMLTagName(xmlTag), strlen(GetXMLTagName(xmlTag))) != 0)
				{
					result = kXMLSyntaxError;
					break;
				}
				
				result = GetNextToken(sb, &t);
				
				if (result != kXMLNoError)
					break;
				
				if (t.type != kGreaterThan)
				{
					result = kXMLSyntaxError;
					break;
				}
				else
					break;
			}
			else
			{
				result = kXMLSyntaxError;
				break;
			}
		}
	}
	
	return GNERR_ERR_CODE(result);
}

/* TranslateSymbol
 *
 * If inBuf + *inIndex matches str, write symbol to outBuf + *outIndex.
 */
static int TranslateSymbol(const char* str, char symbol, const char* inBuf,
                           int* inIndex, int maxInIndex, char* outBuf, int* outIndex)
{
	int str_len = strlen(str);

	if ((*inIndex + str_len) <= maxInIndex && strncmp(inBuf + *inIndex, str, str_len) == 0)
	{
		outBuf[(*outIndex)++] = symbol;
		*inIndex += str_len;
		return 1;
	}

	return 0;
}

/* TranslateGenericChars
 *
 * Converts &#1234; and &#x1A2B3C strings into UTF-8.
 */
static int TranslateGenericChars(const char* inBuf, int* inIndex, int maxIndex, char* outBuf, int* outIndex)
{
	int index = *inIndex;
	char isHex = 0;
	int startIndex = 0;
	int endIndex = 0;
	unsigned int charValue = 0;

	if ((maxIndex - index) < 4 || inBuf[index] != '&' || inBuf[index + 1] != '#')
		return 0;
	
	index += 2;	/* skip past the &# */

	if (inBuf[index] == 'x')
	{
		isHex = 1;
		index++;
	}

	startIndex = index;

	while (index < maxIndex)
	{
		char ch = inBuf[index];

		if (ch == ';')
		{
			endIndex = index++;
			break;
		}
		else if (isdigit(ch) || (isHex && isxdigit(ch)))
		{
			index++;
		}
		else
		{
			break;
		}
	}

	if (endIndex == 0)
		return 0;

	charValue = parse_digits_to_uint32(inBuf + startIndex, endIndex - startIndex, isHex);

	*outIndex += write_UTF8(charValue, (unsigned char*)(outBuf + *outIndex));
	*inIndex = index;

	return 1;
}

/* UnescapeSpecialCharacters
 *
 * Convert XML escaped characters ("&amp; &12345;" etc.) into raw characters ("&", etc.).
 */
static XMLError UnescapeSpecialCharacters(const char* rawData, int rawDataLen, char** filteredData, int* filteredDataLen)
{
	XMLError error = kXMLNoError;
	int inIndex = 0;
	int outIndex = 0;

	*filteredData = NULL;
	*filteredDataLen = 0;
	if (rawDataLen == 0)
		return GNERR_ERR_CODE(kXMLNoError);

	*filteredData = gnmem_malloc(rawDataLen);

	if (*filteredData == 0)
		return GNERR_ERR_CODE(kXMLOutOfMemoryError);

	while (inIndex < rawDataLen)
	{
		int handled = TranslateSymbol("&lt;", '<', rawData, &inIndex, rawDataLen, *filteredData, &outIndex);

		if (!handled)
			handled = TranslateSymbol("&gt;", '>', rawData, &inIndex, rawDataLen, *filteredData, &outIndex);

		if (!handled)
			handled = TranslateSymbol("&amp;", '&', rawData, &inIndex, rawDataLen, *filteredData, &outIndex);

		if (!handled)
			handled = TranslateSymbol("&apos;", '\'', rawData, &inIndex, rawDataLen, *filteredData, &outIndex);

		if (!handled)
			handled = TranslateSymbol("&quot;", '"', rawData, &inIndex, rawDataLen, *filteredData, &outIndex);

		if (!handled)
			handled = TranslateGenericChars(rawData, &inIndex, rawDataLen, *filteredData, &outIndex);

		if (!handled)
			(*filteredData)[outIndex++] = rawData[inIndex++];
	}

	*filteredDataLen = outIndex;

	return GNERR_ERR_CODE(kXMLNoError);
}
