/*
 * Copyright (c) 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * microxml.h - XML parsing and rendering utilities.
 */

#ifndef MICROXML_H
#define MICROXML_H


/*
 * Dependencies
 */

#include <extras/cddb/gn_defines.h>
#include <extras/cddb/gn_errors.h>


#ifdef __cplusplus
extern "C" {
#endif


/*
 * Typedefs
 */

typedef gn_error_t XMLError;

typedef struct XMLTag* XMLTagRef;


/*
 * Prototypes
 */

/* XML Tags */
XMLTagRef CreateEmptyXMLTag(void);
XMLTagRef CreateXMLTagFromStr(gn_cstr_t tagName, gn_cstr_t tagData);
XMLTagRef CreateXMLTagFromBuf(gn_cstr_t tagName, gn_uint32_t tagNameLen, gn_cstr_t tagData, gn_uint32_t tagDataLen);
XMLTagRef CloneXMLTag(const XMLTagRef xmlTag, gn_bool_t deep);
#if 0
/* not implemented yet... */
gn_bool_t EqualXMLTags(const XMLTagRef a, const XMLTagRef b);
#endif
void DisposeXMLTag(XMLTagRef xmlTag);
void SmartDisposeXMLTag(XMLTagRef* xmlTag);

/* XML Tag Attributes */
gn_uint32_t GetXMLTagAttrCount(const XMLTagRef xmlTag);
gn_cstr_t GetXMLTagAttrName(const XMLTagRef xmlTag, gn_uint32_t index);
gn_cstr_t GetXMLTagAttrValue(const XMLTagRef xmlTag, gn_uint32_t index);
gn_cstr_t GetXMLTagAttrFromStr(const XMLTagRef xmlTag, gn_cstr_t attrName);
gn_cstr_t GetXMLTagAttrFromBuf(const XMLTagRef xmlTag, gn_cstr_t attrName, gn_uint32_t attrNameLen);
XMLError SetXMLTagAttrFromStr(XMLTagRef xmlTag, gn_cstr_t attrName, gn_cstr_t attrValue);
XMLError SetXMLTagAttrFromBuf(XMLTagRef xmlTag, gn_cstr_t attrName, gn_uint32_t attrNameLen, gn_cstr_t attrValue, gn_uint32_t attrValueLen);

/* XML Subtags */
gn_uint32_t GetXMLSubTagCount(const XMLTagRef xmlTag);
XMLTagRef GetXMLSubTag(const XMLTagRef xmlTag, gn_uint32_t index);
XMLTagRef GetXMLSubTagFromStr(const XMLTagRef xmlTag, gn_cstr_t subTagName);
XMLTagRef GetXMLSubTagFromBuf(const XMLTagRef xmlTag, gn_cstr_t subTagName, gn_uint32_t subTagNameLen);
void AddXMLSubTag(XMLTagRef xmlTag, XMLTagRef subTag);
void RemoveXMLSubTag(XMLTagRef xmlTag, XMLTagRef subTag, gn_bool_t disposeSubTag);

/* XML Tag Data */
gn_cstr_t GetXMLTagData(const XMLTagRef xmlTag);
gn_cstr_t GetXMLSubTagData(const XMLTagRef xmlTag, gn_cstr_t path);
XMLError SetXMLTagDataFromStr(XMLTagRef xmlTag, gn_cstr_t tagData);
XMLError SetXMLTagDataFromBuf(XMLTagRef xmlTag, gn_cstr_t tagData, gn_uint32_t tagDataLen);

/* XML Tag Name */
gn_cstr_t GetXMLTagName(const XMLTagRef xmlTag);
XMLError SetXMLTagNameFromStr(XMLTagRef xmlTag, gn_cstr_t tagName);
XMLError SetXMLTagNameFromBuf(XMLTagRef xmlTag, gn_cstr_t tagName, gn_uint32_t tagNameLen);

/* Parsing and Rendering */
gn_str_t RenderXMLTagToStr(const XMLTagRef xmlTag, gn_bool_t header);
gn_str_t RenderXMLTagToStrEx(const XMLTagRef xmlTag, gn_bool_t header, gn_bool_t indent, gn_bool_t newlines);
XMLError ParseStrToXMLTag(gn_cstr_t str, XMLTagRef* xmlResult);
XMLError ParseBufToXMLTag(gn_cstr_t buf, gn_uint32_t bufLen, XMLTagRef* xmlResult);

#ifdef __cplusplus
}
#endif

#endif
