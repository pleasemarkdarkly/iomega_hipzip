/*
 * microxml.c
 *
 * XML utilities
 */

#include <extras/cddb/microxml.h>
#include <extras/cddb/gn_platform.h>
#include GN_STRING_H
#include <extras/cddb/gn_memory.h>
#include <extras/cddb/gn_utils.h>

typedef struct Attribute
{
	gn_str_t attrName;
	gn_str_t attrData;
	struct Attribute* next;
}
Attribute;

typedef struct XMLTag
{
	gn_str_t tagName;
	gn_str_t tagData;
	Attribute* attrList;
	struct XMLTag* subTagList;
	struct XMLTag* next;
}
XMLTag;

static Attribute* CreateAttribute(gn_cstr_t attrNamePtr, gn_uint32_t attrNameLen, gn_cstr_t attrDataPtr, gn_uint32_t attrDataLen);
static void DisposeAttribute(Attribute* attr);
static XMLError AddAttribute(XMLTagRef xmlTag, Attribute* newAttr);
static void RemoveAttribute(XMLTagRef xmlTag, gn_cstr_t attrName, gn_uint32_t attrNameLen);

/*******************************************************************/

XMLTagRef CreateXMLTagFromStr(gn_cstr_t tagName, gn_cstr_t tagData)
{
	return CreateXMLTagFromBuf(tagName, tagName ? strlen(tagName) : 0,
	                           tagData, tagData ? strlen(tagData) : 0);
}

XMLTagRef CreateXMLTagFromBuf(gn_cstr_t tagName, gn_uint32_t tagNameLen, gn_cstr_t tagData, gn_uint32_t tagDataLen)
{
	XMLTag* xmlTag = 0;
	
	if (tagName == 0 || tagNameLen == 0 || *tagName == 0)
		return 0;
		
	xmlTag = CreateEmptyXMLTag();
	
	if (xmlTag != 0)
	{
		xmlTag->tagName = gn_makestr(tagName, tagNameLen);
		
		if (xmlTag->tagName == 0)
		{
			gnmem_free(xmlTag);
			return 0;
		}
		
		if (tagData != 0 && tagDataLen != 0 && *tagData != 0)
		{
			xmlTag->tagData = gn_makestr(tagData, tagDataLen);
			
			if (xmlTag->tagData == 0)
			{
				gnmem_free(xmlTag->tagName);
				gnmem_free(xmlTag);
				return 0;
			}
		}
	}
	
	return xmlTag;
}

XMLTagRef CloneXMLTag(const XMLTagRef xmlTag, gn_bool_t deep)
{
	Attribute* attr = 0;
	XMLTag* subTag = 0;

	/* copy the name and data */
	XMLTagRef clone = CreateXMLTagFromStr(GetXMLTagName(xmlTag), GetXMLTagData(xmlTag));

	if (clone == 0)
		return 0;

	/* copy the attributes */
	for (attr = xmlTag->attrList; attr != 0; attr = attr->next)
	{
		XMLError error = SetXMLTagAttrFromStr(clone, attr->attrName, attr->attrData);

		if (error != 0)
		{
			DisposeXMLTag(clone);
			return 0;
		}
	}

	if (deep == GN_TRUE)
	{
		/* copy the subtags */
		for (subTag = xmlTag->subTagList; subTag != 0; subTag = subTag->next)
		{
			XMLTagRef subTagClone = CloneXMLTag(subTag, deep);

			if (subTagClone != 0)
			{
				AddXMLSubTag(clone, subTagClone);
			}
			else
			{
				DisposeXMLTag(clone);
				return 0;
			}
		}
	}

	return clone;
}

void DisposeXMLTag(XMLTagRef xmlTag)
{
	if (xmlTag != 0)
	{
		/* free the tag name */
		if (xmlTag->tagName != 0)
			gnmem_free(xmlTag->tagName);
		
		/* free the tag data */
		if (xmlTag->tagData != 0)
			gnmem_free(xmlTag->tagData);
		
		/* free the attribute list */
		while (xmlTag->attrList != 0)
		{
			Attribute* this = xmlTag->attrList;
			Attribute* next = this->next;
			
			DisposeAttribute(this);
			
			xmlTag->attrList = next;
		}
		
		/* free the subtag list */
		while (xmlTag->subTagList != 0)
		{
			XMLTag* this = xmlTag->subTagList;
			XMLTag* next = this->next;
			
			DisposeXMLTag(this);
			
			xmlTag->subTagList = next;
		}
		
		/* finally, free the tag itself */
		gnmem_free(xmlTag);
	}
}

void SmartDisposeXMLTag(XMLTagRef* xmlTag)
{
	if (xmlTag != 0 && *xmlTag != 0)
	{
		DisposeXMLTag(*xmlTag);
		*xmlTag = 0;
	}
}

gn_uint32_t GetXMLTagAttrCount(const XMLTagRef xmlTag)
{
	Attribute* attr = 0;
	gn_uint32_t count = 0;
	
	if (xmlTag == 0)
		return 0;
	
	for (attr = xmlTag->attrList; attr != 0; attr = attr->next)
		count++;

	return count;
}

gn_cstr_t GetXMLTagAttrName(const XMLTagRef xmlTag, gn_uint32_t index)
{
	Attribute* attr = 0;
	gn_uint32_t count = 0;
	
	if (xmlTag == 0)
		return 0;
	
	for (attr = xmlTag->attrList; attr != 0; attr = attr->next)
	{
		if (count == index)
			return attr->attrName;

		count++;
	}

	return 0;
}

gn_cstr_t GetXMLTagAttrValue(const XMLTagRef xmlTag, gn_uint32_t index)
{
	Attribute* attr = 0;
	gn_uint32_t count = 0;
	
	if (xmlTag == 0)
		return 0;
	
	for (attr = xmlTag->attrList; attr != 0; attr = attr->next)
	{
		if (count == index)
			return attr->attrData;

		count++;
	}

	return 0;
}

gn_cstr_t GetXMLTagAttrFromStr(const XMLTagRef xmlTag, gn_cstr_t attrName)
{
	return GetXMLTagAttrFromBuf(xmlTag, attrName, attrName ? strlen(attrName) : 0);
}

gn_cstr_t GetXMLTagAttrFromBuf(const XMLTagRef xmlTag, gn_cstr_t attrName, gn_uint32_t attrNameLen)
{
	Attribute* index = 0;
	
	if (xmlTag == 0 || attrName == 0 || attrNameLen == 0 || *attrName == 0)
		return 0;
	
	for (index = xmlTag->attrList; index != 0; index = index->next)
	{
		if (gn_bufcmp(index->attrName, strlen(index->attrName), attrName, attrNameLen) == 0)
			return index->attrData;
	}
	
	return 0;
}

XMLError SetXMLTagAttrFromStr(XMLTagRef xmlTag, gn_cstr_t attrName, gn_cstr_t attrValue)
{
	XMLError	error;

	error = SetXMLTagAttrFromBuf(xmlTag, attrName, attrName ? strlen(attrName) : 0,
	                            attrValue, attrValue ? strlen(attrValue) : 0);

	return GNERR_ERR_CODE(error);
}

XMLError SetXMLTagAttrFromBuf(XMLTagRef xmlTag, gn_cstr_t attrName, gn_uint32_t attrNameLen, gn_cstr_t attrValue, gn_uint32_t attrValueLen)
{
	if (xmlTag == 0 || attrName == 0 || attrNameLen == 0 || *attrName == 0)
	{
		return GNERR_ERR_CODE(kXMLInvalidParamError);
	}

	if (attrValue == 0 || attrValueLen == 0)
	{
		RemoveAttribute(xmlTag, attrName, attrNameLen);

		return GNERR_ERR_CODE(kXMLNoError);
	}
	else
	{
		Attribute* newAttr = CreateAttribute(attrName, attrNameLen, attrValue, attrValueLen);
		
		if (newAttr == 0)
			return GNERR_ERR_CODE(kXMLOutOfMemoryError);
		
		return GNERR_ERR_CODE((AddAttribute(xmlTag, newAttr)));
	}
}

gn_uint32_t GetXMLSubTagCount(const XMLTagRef xmlTag)
{
	XMLTag* tag = 0;
	gn_uint32_t count = 0;
	
	if (xmlTag == 0)
		return 0;
	
	for (tag = xmlTag->subTagList; tag != 0; tag = tag->next)
		count++;

	return count;
}

XMLTagRef GetXMLSubTag(const XMLTagRef xmlTag, gn_uint32_t index)
{
	XMLTag* tag = 0;
	gn_uint32_t count = 0;
	
	if (xmlTag == 0)
		return 0;
	
	for (tag = xmlTag->subTagList; tag != 0; tag = tag->next)
	{
		if (count == index)
			return tag;

		count++;
	}

	return 0;
}

XMLTagRef GetXMLSubTagFromStr(const XMLTagRef xmlTag, gn_cstr_t subTagName)
{
	return GetXMLSubTagFromBuf(xmlTag, subTagName, subTagName ? strlen(subTagName) : 0);
}

XMLTagRef GetXMLSubTagFromBuf(const XMLTagRef xmlTag, gn_cstr_t subTagName, gn_uint32_t subTagNameLen)
{
	XMLTag* index = 0;
	
	if (xmlTag == 0 || subTagName == 0 || subTagNameLen == 0 || *subTagName == 0)
		return 0;
	
	for (index = xmlTag->subTagList; index != 0; index = index->next)
	{
		if (gn_bufcmp(index->tagName, strlen(index->tagName), subTagName, subTagNameLen) == 0)
			return index;
	}
	
	return 0;
}

void AddXMLSubTag(XMLTagRef xmlTag, XMLTagRef subTag)
{
	if (xmlTag == 0 || subTag == 0)
		return;
	
	if (xmlTag->subTagList != 0)
	{
		XMLTag* index = 0;
		
		/* append the new subtag to the end of the list */
		for (index = xmlTag->subTagList; index != 0; index = index->next)
		{
			if (index->next == 0)
			{
				index->next = subTag;
				break;
			}
		}
	}
	else
		xmlTag->subTagList = subTag;
}

void RemoveXMLSubTag(XMLTagRef xmlTag, XMLTagRef subTag, gn_bool_t disposeSubTag)
{
	if (xmlTag == 0 || subTag == 0)
		return;
	
	if (xmlTag->subTagList != 0)
	{
		XMLTag* index = 0;
		XMLTag* lastIndex = 0;

		for (index = xmlTag->subTagList; index != 0; index = index->next)
		{
			if (index == subTag)
				break;
			
			lastIndex = index;
		}
		
		if (index != 0)
		{
			if (lastIndex == 0)
				xmlTag->subTagList = index->next;
			else
				lastIndex->next = index->next;
			
			if (disposeSubTag)
				DisposeXMLTag(subTag);
		}
	}
}

gn_cstr_t GetXMLTagData(const XMLTagRef xmlTag)
{
	return (xmlTag != 0) ? xmlTag->tagData : 0;
}

gn_cstr_t GetXMLSubTagData(const XMLTagRef xmlTag, gn_cstr_t path)
{
	gn_str_t delim = 0;

	if (xmlTag == 0) {
		return 0;
	}

	if (path == 0 || *path == 0) {
		return GetXMLTagData(xmlTag);
	}

	delim = strchr(path, '/');

	if (delim == 0) {

		XMLTagRef subTag = GetXMLSubTagFromStr(xmlTag, path);

		return GetXMLTagData(subTag);
	} else {

		XMLTagRef subTag = GetXMLSubTagFromBuf(xmlTag, path, delim - path);

		return GetXMLSubTagData(subTag, delim + 1);
	}
}

XMLError SetXMLTagDataFromStr(XMLTagRef xmlTag, gn_cstr_t tagData)
{
	XMLError	error;

	error = SetXMLTagDataFromBuf(xmlTag, tagData, tagData ? strlen(tagData) : 0);

	return GNERR_ERR_CODE(error);
}

XMLError SetXMLTagDataFromBuf(XMLTagRef xmlTag, gn_cstr_t tagData, gn_uint32_t tagDataLen)
{
	if (xmlTag == 0)
		return GNERR_ERR_CODE(kXMLInvalidParamError);
	
	if (xmlTag->tagData != 0)
	{
		gnmem_free(xmlTag->tagData);
		xmlTag->tagData = 0;
	}
	
	if (tagData != 0 && tagDataLen != 0 && *tagData != 0)
	{
		xmlTag->tagData = gn_makestr(tagData, tagDataLen);
		
		return GNERR_ERR_CODE(((xmlTag->tagData != 0) ? kXMLNoError : kXMLOutOfMemoryError));
	}
	else
		return GNERR_ERR_CODE(kXMLNoError);
}

gn_cstr_t GetXMLTagName(const XMLTagRef xmlTag)
{
	return (xmlTag != 0) ? xmlTag->tagName : 0;
}

XMLError SetXMLTagNameFromStr(XMLTagRef xmlTag, gn_cstr_t tagName)
{
	XMLError	error;

	error = SetXMLTagNameFromBuf(xmlTag, tagName, tagName ? strlen(tagName) : 0);

	return GNERR_ERR_CODE(error);
}

XMLError SetXMLTagNameFromBuf(XMLTagRef xmlTag, gn_cstr_t tagName, gn_uint32_t tagNameLen)
{
	if (xmlTag == 0 || tagName == 0 || tagNameLen == 0 || *tagName == 0)
		return GNERR_ERR_CODE(kXMLInvalidParamError);

	if (xmlTag->tagName != 0)
	{
		gnmem_free(xmlTag->tagName);
		xmlTag->tagName = 0;
	}

	xmlTag->tagName = gn_makestr(tagName, tagNameLen);

	return GNERR_ERR_CODE(((xmlTag->tagName != 0) ? kXMLNoError : kXMLOutOfMemoryError));
}

/*******************************************************************/

XMLTagRef CreateEmptyXMLTag()
{
	XMLTag* xmlTag = (XMLTag*)gnmem_malloc(sizeof(XMLTag));
	
	if (xmlTag != 0)
	{
		xmlTag->tagName = 0;
		xmlTag->tagData = 0;
		xmlTag->attrList = 0;
		xmlTag->subTagList = 0;
		xmlTag->next = 0;
	}
	
	return xmlTag;
}

static Attribute* CreateAttribute(gn_cstr_t attrNamePtr, gn_uint32_t attrNameLen, gn_cstr_t attrDataPtr, gn_uint32_t attrDataLen)
{
	Attribute* newAttr = (Attribute*)gnmem_malloc(sizeof(Attribute));
	
	if (newAttr != 0)
	{
		newAttr->attrName = gn_makestr(attrNamePtr, attrNameLen);
		
		if (newAttr->attrName == 0)
		{
			gnmem_free(newAttr);
			return 0;
		}
		
		newAttr->attrData = gn_makestr(attrDataPtr, attrDataLen);
		
		if (newAttr->attrData == 0)
		{
			gnmem_free(newAttr->attrName);
			gnmem_free(newAttr);
			return 0;
		}
		
		newAttr->next = 0;
	}
	
	return newAttr;
}

static void DisposeAttribute(Attribute* attr)
{
	if (attr->attrName != 0)
		gnmem_free(attr->attrName);
	
	if (attr->attrData != 0)
		gnmem_free(attr->attrData);
	
	gnmem_free(attr);
}

static XMLError AddAttribute(XMLTagRef xmlTag, Attribute* newAttr)
{
	if (newAttr == 0)
		return GNERR_ERR_CODE(kXMLInvalidParamError);
	
	if (xmlTag->attrList != 0)
	{
		Attribute* index = 0;

		/* remove any existing attribute */
		RemoveAttribute(xmlTag, newAttr->attrName, strlen(newAttr->attrName));
		
		/* append the attribute to the end of the list */
		for (index = xmlTag->attrList; index != 0; index = index->next)
		{
			if (index->next == 0)
			{
				index->next = newAttr;
				break;
			}
		}
	}
	else
		xmlTag->attrList = newAttr;
	
	return GNERR_ERR_CODE(kXMLNoError);
}

static void RemoveAttribute(XMLTagRef xmlTag, gn_cstr_t attrName, gn_uint32_t attrNameLen)
{
	Attribute* index = 0;
	Attribute* lastIndex = 0;
	
	if (xmlTag == 0 || attrName == 0 || attrNameLen == 0)
		return;
	
	for (index = xmlTag->attrList; index != 0; index = index->next)
	{
		if (gn_bufcmp(index->attrName, strlen(index->attrName), attrName, attrNameLen) == 0)
			break;
		
		lastIndex = index;
	}
	
	if (index != 0)
	{
		if (lastIndex == 0)
			xmlTag->attrList = index->next;
		else
			lastIndex->next = index->next;
		
		DisposeAttribute(index);
	}
}
