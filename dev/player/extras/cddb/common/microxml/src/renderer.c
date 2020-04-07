/*
 * renderer.c
 *
 * renders an XML object to text
 */

#include <extras/cddb/microxml.h>
#include <extras/cddb/gn_dyn_buf.h>

static gn_dyn_buf_error_t
AppendTagToDynBuf(gn_dyn_buf_ref_t output, XMLTagRef xmlTag,
                  int indentationLevel, int indent, int newlines);

static gn_dyn_buf_error_t
AppendAttrsToDynBuf(gn_dyn_buf_ref_t output, XMLTagRef xmlTag);

static gn_dyn_buf_error_t
AppendSubTagsToDynBuf(gn_dyn_buf_ref_t output, XMLTagRef xmlTag,
                      int indentationLevel, int indent, int newlines);

static gn_dyn_buf_error_t
do_indentation(gn_dyn_buf_ref_t dyn_buf, int indentation);

/*******************************************************************/

gn_str_t RenderXMLTagToStr(const XMLTagRef xmlTag, gn_bool_t header)
{
	return RenderXMLTagToStrEx(xmlTag, header, 1, 1);
}

gn_str_t RenderXMLTagToStrEx(const XMLTagRef xmlTag, gn_bool_t header, gn_bool_t indent, gn_bool_t newlines)
{
	gn_dyn_buf_error_t result = DYN_BUF_NO_ERROR;
	gn_dyn_buf_ref_t output = 0;
	gn_char_t null = '\0';

	if (xmlTag == 0)
		return 0;
	
	output = gn_dyn_buf_create(256, 128);
	
	if (output == 0)
		return 0;

	if (header)
	{
		result = gn_dyn_buf_append_str(output, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>");

		if ((result == DYN_BUF_NO_ERROR) && newlines)
			result = gn_dyn_buf_append_str(output, "\n");
	}
	
	if (result == DYN_BUF_NO_ERROR)
		result = AppendTagToDynBuf(output, xmlTag, 0, indent, newlines);

	if (result == DYN_BUF_NO_ERROR)
		result = gn_dyn_buf_append_buf(output, &null, sizeof(null));
	
	if (result == DYN_BUF_NO_ERROR)
		return gn_dyn_buf_dispose_and_return_buffer(output);
	
	gn_dyn_buf_dispose(output);

	return 0;
}

/*******************************************************************/

static gn_dyn_buf_error_t
AppendEscapedString(gn_dyn_buf_ref_t output, gn_cstr_t str)
{
	gn_dyn_buf_error_t error = DYN_BUF_NO_ERROR;

	while (*str != 0 && error == DYN_BUF_NO_ERROR)
	{
		char ch = *str++;

		/* TBD REC: eventually add support for 7-bit only output (&#12345;) */

		if (ch == '<')
			error = gn_dyn_buf_append_str(output, "&lt;");
		else if (ch == '>')
			error = gn_dyn_buf_append_str(output, "&gt;");
		else if (ch == '&')
			error = gn_dyn_buf_append_str(output, "&amp;");
		else if (ch == '\'')
			error = gn_dyn_buf_append_str(output, "&apos;");
		else if (ch == '"')
			error = gn_dyn_buf_append_str(output, "&quot;");
		else
			error = gn_dyn_buf_append_buf(output, &ch, sizeof(ch));
	}

	return error;
}

static gn_dyn_buf_error_t
AppendAttrsToDynBuf(gn_dyn_buf_ref_t output, XMLTagRef xmlTag)
{
	gn_dyn_buf_error_t result = DYN_BUF_NO_ERROR;
	gn_uint32_t attrCount = GetXMLTagAttrCount(xmlTag);
	gn_uint32_t i;
	
	for (i = 0; i < attrCount && result == DYN_BUF_NO_ERROR; i++)
	{
		result = gn_dyn_buf_append_str(output, " ");
		
		if (result == DYN_BUF_NO_ERROR)
			result = gn_dyn_buf_append_str(output, GetXMLTagAttrName(xmlTag, i));
		
		if (result == DYN_BUF_NO_ERROR)
			result = gn_dyn_buf_append_str(output, "=\"");
		
		if (result == DYN_BUF_NO_ERROR)
			result = AppendEscapedString(output, GetXMLTagAttrValue(xmlTag, i));
		
		if (result == DYN_BUF_NO_ERROR)
			result = gn_dyn_buf_append_str(output, "\"");
	}
	
	return result;
}

static gn_dyn_buf_error_t
do_indentation(gn_dyn_buf_ref_t dyn_buf, int indentation)
{
	gn_dyn_buf_error_t result = DYN_BUF_NO_ERROR;
	int i = 0;
	
	for (i = 0; i < indentation && result == DYN_BUF_NO_ERROR; i++)
		result = gn_dyn_buf_append_str(dyn_buf, "  ");
	
	return result;
}

static gn_dyn_buf_error_t
AppendSubTagsToDynBuf(gn_dyn_buf_ref_t output, XMLTagRef xmlTag,
                      int indentationLevel, int indent, int newlines)
{
	gn_dyn_buf_error_t result = DYN_BUF_NO_ERROR;
	gn_uint32_t subTagCount = GetXMLSubTagCount(xmlTag);
	gn_uint32_t i;
	
	for (i = 0; i < subTagCount && result == DYN_BUF_NO_ERROR; i++)
	{
		result = AppendTagToDynBuf(output, GetXMLSubTag(xmlTag, i),
		                           indentationLevel, indent, newlines);
	}
	
	return result;
}

static gn_dyn_buf_error_t
AppendTagToDynBuf(gn_dyn_buf_ref_t output, XMLTagRef xmlTag,
                  int indentation, int indent, int newlines)
{
	gn_dyn_buf_error_t result = DYN_BUF_NO_ERROR;
	
	if (indent)
		result = do_indentation(output, indentation);

	if (result == DYN_BUF_NO_ERROR)
		result = gn_dyn_buf_append_str(output, "<");
	
	if (result == DYN_BUF_NO_ERROR)
		result = gn_dyn_buf_append_str(output, GetXMLTagName(xmlTag));
	
	if (result == DYN_BUF_NO_ERROR)
		result = AppendAttrsToDynBuf(output, xmlTag);
	
	if (result == DYN_BUF_NO_ERROR)
		result = gn_dyn_buf_append_str(output, ">");
	
	if (GetXMLSubTagCount(xmlTag) > 0)
	{
		/* if there is a sublist, start a new line */
		
		if ((result == DYN_BUF_NO_ERROR) && newlines)
			result = gn_dyn_buf_append_str(output, "\n");
		
		if (GetXMLTagData(xmlTag) != 0)
		{
			if (indent)
				result = do_indentation(output, indentation + 1);
		
			if (result == DYN_BUF_NO_ERROR)
				result = AppendEscapedString(output, GetXMLTagData(xmlTag));
			
			if ((result == DYN_BUF_NO_ERROR) && newlines)
				result = gn_dyn_buf_append_str(output, "\n");
		}
		
		if (result == DYN_BUF_NO_ERROR)
			result = AppendSubTagsToDynBuf(output, xmlTag, indentation + 1, indent, newlines);
		
		if (indent)
			result = do_indentation(output, indentation);
	}
	else
	{
		if (GetXMLTagData(xmlTag) != 0)
		{
			if (result == DYN_BUF_NO_ERROR)
				result = AppendEscapedString(output, GetXMLTagData(xmlTag));
		}
	}
	
	if (result == DYN_BUF_NO_ERROR)
		result = gn_dyn_buf_append_str(output, "</");
	if (result == DYN_BUF_NO_ERROR)
		result = gn_dyn_buf_append_str(output, GetXMLTagName(xmlTag));
	if (result == DYN_BUF_NO_ERROR)
		result = gn_dyn_buf_append_str(output, ">");
	if ((result == DYN_BUF_NO_ERROR) && newlines)
		result = gn_dyn_buf_append_str(output, "\n");
	
	return result;
}
