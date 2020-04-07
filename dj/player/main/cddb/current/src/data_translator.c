/*
 * Copyright (c) 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * data_translator.c - Implementation for the "Data Structure
 *						Translator" module.
 */

#include <extras/cddb/gn_platform.h>
#include GN_STRING_H
#include GN_STDLIB_H
#include <extras/cddb/gn_translator.h>
#include <extras/cddb/gn_memory.h>
#include <extras/cddb/gn_utils.h>
#include <extras/cddb/microxml.h>


static gn_xlt_error_t
do_convert_xml(XMLTagRef xml, gn_udiscinfo_t *info, gn_bool_t full);

static gn_xlt_error_t
do_copy_xml_tag(XMLTagRef xml_src, XMLTagRef xml_dst, gn_cstr_t tag_name);


/* Initialize Data Translator subsystem. */
gn_xlt_error_t
gnxlt_initialize(void)
{
	return XLTERR_NoError;
}

/* Shutdown Data Translator subsystem. */
gn_xlt_error_t
gnxlt_shutdown(void)
{
	return XLTERR_NoError;
}

/* Convert raw data (XML) to discinfo format. */
gn_xlt_error_t
gnxlt_convert(void *rawdata, gn_size_t data_size, gn_discinfo_t *info)
{
	gn_xlt_error_t error = XLTERR_NoError;
	XMLTagRef xml = 0;
	XMLError xmlError = kXMLNoError;

	if (rawdata == 0 || data_size == 0 || info == 0) {
		return XLTERR_InvalidParam;
	}

	xmlError = ParseBufToXMLTag(rawdata, data_size, &xml);

	if (xmlError == kXMLNoError) {
		error = gnxlt_convert_xml(xml, info);
	} else {
		error = XLTERR_BadFormat;
	}

	if (xml != 0) {
		DisposeXMLTag(xml);
	}

	return error;
}

/* Convert raw data (XML) to discinfo format, dynamically allocation structure and strings. */
gn_xlt_error_t
gnxlt_convert_alloc(void *rawdata, gn_size_t data_size, gn_discinfo_t **info)
{
	gn_xlt_error_t error = XLTERR_NoError;

	if (rawdata == 0 || data_size == 0 || info == 0) {
		return XLTERR_InvalidParam;
	}

	*info = (gn_discinfo_t*)gnmem_malloc(sizeof(gn_discinfo_t));

	if (*info == 0) {
		return XLTERR_OutOfMemory;
	}

	error = gnxlt_convert(rawdata, data_size, *info);

	if (error != XLTERR_NoError) {
		gnmem_free(*info);
		*info = 0;
	}

	return error;
}

/* Convert raw data (XML text) to short discinfo format, dynamically allocating structure and strings. */
gn_xlt_error_t
gnxlt_sconvert_alloc(void *rawdata, gn_size_t data_size, gn_sdiscinfo_t **info)
{
	gn_xlt_error_t error = XLTERR_NoError;
	XMLTagRef		xml = 0;
	XMLError		xmlError = kXMLNoError;


	if (rawdata == 0 || data_size == 0 || info == 0) {
		return XLTERR_InvalidParam;
	}

	*info = (gn_sdiscinfo_t*)gnmem_malloc(sizeof(gn_sdiscinfo_t));

	xmlError = ParseBufToXMLTag(rawdata, data_size, &xml);

	if (xmlError == kXMLNoError) {
		error = do_convert_xml(xml, (gn_udiscinfo_t*)*info, GN_FALSE);
	} else {
		error = XLTERR_BadFormat;
	}

	if (xml != 0) {
		DisposeXMLTag(xml);
	}

	return error;
}

/* Convert structured XML to short discinfo format, dynamically allocating structure and strings. */
gn_xlt_error_t
gnxlt_sconvert_xml_alloc(XMLTagRef xml, gn_sdiscinfo_t **info)
{
	if (xml == 0 || info == 0) {
		return XLTERR_InvalidParam;
	}

	*info = (gn_sdiscinfo_t*)gnmem_malloc(sizeof(gn_sdiscinfo_t));

	if (*info == NULL)
		return kXMLOutOfMemoryError;

	return do_convert_xml(xml, (gn_udiscinfo_t*)*info, GN_FALSE);
}


gn_xlt_error_t
gnxlt_convert_xml(XMLTagRef xml, gn_discinfo_t *info)
{
	return do_convert_xml(xml, (gn_udiscinfo_t*)info, GN_TRUE);
}


gn_xlt_error_t
gnxlt_convert_xml_alloc(XMLTagRef xml, gn_discinfo_t **info)
{
	gn_xlt_error_t error = XLTERR_NoError;

	if (xml == 0 || info == 0) {
		return XLTERR_InvalidParam;
	}

	*info = (gn_discinfo_t*)gnmem_malloc(sizeof(gn_discinfo_t));

	if (*info == 0) {
		return XLTERR_OutOfMemory;
	}

	error = gnxlt_convert_xml(xml, *info);

	if (error != XLTERR_NoError) {
		gnmem_free(*info);
		*info = 0;
	}

	return error;
}

/* Reduce raw XML data into more compact format. */
gn_xlt_error_t
gnxlt_reduce_raw(void *rawdata, gn_size_t data_size, void **rawdata_out, gn_size_t *data_size_out, gn_xlt_options_t options)
{
	gn_xlt_error_t	error = XLTERR_NoError;
	XMLTagRef		xml = NULL, xml_out = NULL;

	if (rawdata_out == NULL || data_size_out == NULL) {
		return XLTERR_InvalidParam;
	}
	*rawdata_out = NULL;
	*data_size_out = 0;

	error = ParseBufToXMLTag(rawdata, data_size, &xml);
	if (error != XLTERR_NoError)
		return error;

	if (options == XLTCVT_Default)
		options = XLTCVT_DefaultMask;

	error = gnxlt_reduce_xml(xml, &xml_out, options);

	if (error == XLTERR_NoError && xml_out) {
		*rawdata_out = RenderXMLTagToStrEx(xml_out,
										  (gn_bool_t)((options & XLTCVT_XMLHeader) ? GN_TRUE : GN_FALSE),
										  (gn_bool_t)((options & XLTCVT_XMLIndent) ? GN_TRUE : GN_FALSE),
										  (gn_bool_t)((options & XLTCVT_XMLNewlines) ? GN_TRUE : GN_FALSE));
		if (!*rawdata_out)
			error = XLTERR_OutOfMemory;
		else
			*data_size_out = strlen(*rawdata_out) + 1;
	}

	SmartDisposeXMLTag(&xml_out);
	SmartDisposeXMLTag(&xml);

	return error;
}

/* Reduce structured XML data into more compact format. */
gn_xlt_error_t
gnxlt_reduce_xml(XMLTagRef xml, XMLTagRef* xml_out, gn_xlt_options_t options)
{
	gn_xlt_error_t	error = XLTERR_NoError;
	XMLTagRef		short_xml = NULL;
	XMLTagRef		xml_tag = NULL;

	if (xml_out == NULL) {
		return XLTERR_InvalidParam;
	}
	*xml_out = NULL;

	if (options == XLTCVT_Default)
		options = XLTCVT_DefaultMask;

	short_xml = CloneXMLTag(xml, GN_FALSE);
	if (short_xml) {

		if (options & XLTCVT_Revision) {
			error = do_copy_xml_tag(xml, short_xml, GN_STR_REVISION);
		}
		if (options & XLTCVT_Title) {
			error = do_copy_xml_tag(xml, short_xml, GN_STR_TITLE);
		}
		if (error == XLTERR_NoError && (options & XLTCVT_Artist)) {
			error = do_copy_xml_tag(xml, short_xml, GN_STR_ARTIST);
		}
		if (error == XLTERR_NoError && (options & XLTCVT_Genre)) {
			error = do_copy_xml_tag(xml, short_xml, GN_STR_GENRE);
		}
		if (error == XLTERR_NoError && (options & XLTCVT_Year)) {
			error = do_copy_xml_tag(xml, short_xml, GN_STR_YEAR);
		}
		if (error == XLTERR_NoError && (options & XLTCVT_DiscIds)) {
			error = do_copy_xml_tag(xml, short_xml, GN_STR_DISCID);
			if (error == XLTERR_NoError)
				error = do_copy_xml_tag(xml, short_xml, GN_STR_TUID);
		}

		if (error == XLTERR_NoError && (options & XLTCVT_TrackMask)) {
			XMLTagRef		trks_tag = NULL;
			XMLTagRef		short_trks_tag = NULL;

			trks_tag = GetXMLSubTagFromStr(xml, GN_STR_TRACKS);
			if (trks_tag != NULL)
				short_trks_tag = CloneXMLTag(trks_tag, GN_FALSE);

			if (short_trks_tag != NULL ) {
				gn_uint32_t sub_tag_count = GetXMLSubTagCount(trks_tag);
				gn_uint32_t i;

				AddXMLSubTag(short_xml, short_trks_tag);

				for (i = 0; i < sub_tag_count && error == XLTERR_NoError; i++) {

					XMLTagRef trk_tag = GetXMLSubTag(trks_tag, i);

					if ((trk_tag != 0) && gn_str_eq(GetXMLTagName(trk_tag), GN_STR_TRACK)) {
						XMLTagRef	short_trk_tag = CloneXMLTag(trk_tag, GN_FALSE);

						if (short_trk_tag == NULL) {
							error = XLTERR_OutOfMemory;
							continue;
						}

						AddXMLSubTag(short_trks_tag, short_trk_tag);

						if (error == XLTERR_NoError && (options & XLTCVT_TrackTitle)) {
							error = do_copy_xml_tag(trk_tag, short_trk_tag, GN_STR_TITLE);
						}
						if (error == XLTERR_NoError && (options & XLTCVT_TrackArtist)) {
							error = do_copy_xml_tag(trk_tag, short_trk_tag, GN_STR_ARTIST);
						}
						if (error == XLTERR_NoError && (options & XLTCVT_TrackGenre)) {
							error = do_copy_xml_tag(trk_tag, short_trk_tag, GN_STR_GENRE);
						}
						if (error == XLTERR_NoError && (options & XLTCVT_TrackYear)) {
							error = do_copy_xml_tag(trk_tag, short_trk_tag, GN_STR_YEAR);
						}
						if (error == XLTERR_NoError && (options & XLTCVT_TrackIds)) {
							error = do_copy_xml_tag(trk_tag, short_trk_tag, GN_STR_TUID);
						}
					}
				}
			}
			else
				error = XLTERR_OutOfMemory;
		}
	}
	else
		error = XLTERR_OutOfMemory;

	if (error != XLTERR_NoError) {
		SmartDisposeXMLTag(&short_xml);
		short_xml = NULL;
	}

	*xml_out = short_xml;

	return error;
}


/* Release results from previous conversion. */
gn_xlt_error_t
gnxlt_release(gn_discinfo_t *info)
{
	if (info == 0) {
		return XLTERR_InvalidParam;
	}

	if (info->di_title != 0) {
		gnmem_free(info->di_title);
		info->di_title = 0;
	}

	if (info->di_artist != 0) {
		gnmem_free(info->di_artist);
		info->di_artist = 0;
	}

	if (info->di_genre != 0) {
		gnmem_free(info->di_genre);
		info->di_genre = 0;
	}

	if (info->di_numtracks > 0) {
		unsigned int i;
		for (i = 0; i < info->di_numtracks; i++) {
			if (info->di_tracktitles[i] != 0) {
				gnmem_free(info->di_tracktitles[i]);
				info->di_tracktitles[i] = 0;
			}
		}
		info->di_numtracks = 0;
	}

	return XLTERR_NoError;
}

/* Release results from previous conversion and free structure. */
gn_xlt_error_t
gnxlt_release_free(gn_discinfo_t *info)
{
	if (info == 0) {
		return XLTERR_InvalidParam;
	}

	gnxlt_release(info);
	gnmem_free(info);
	return XLTERR_NoError;
}



/* Release results from previous conversion. */
gn_xlt_error_t
gnxlt_srelease(gn_sdiscinfo_t *info)
{
	if (info == 0) {
		return XLTERR_InvalidParam;
	}

	if (info->di_title != 0) {
		gnmem_free(info->di_title);
		info->di_title = 0;
	}

	if (info->di_artist != 0) {
		gnmem_free(info->di_artist);
		info->di_artist = 0;
	}

	if (info->di_genre != 0) {
		gnmem_free(info->di_genre);
		info->di_genre = 0;
	}

	return XLTERR_NoError;
}

/* Release results from previous conversion and free structure. */
gn_xlt_error_t
gnxlt_srelease_free(gn_sdiscinfo_t *info)
{
	if (info == 0) {
		return XLTERR_InvalidParam;
	}

	gnxlt_srelease(info);
	gnmem_free(info);
	return XLTERR_NoError;
}

/* fetch named tag, clone it, and add to src */
static gn_xlt_error_t
do_copy_xml_tag(XMLTagRef xml_src, XMLTagRef xml_dst, gn_cstr_t tag_name)
{
	gn_xlt_error_t	error = XLTERR_NoError;
	XMLTagRef		tag = NULL;
	XMLTagRef		new_tag = NULL;

	tag = GetXMLSubTagFromStr(xml_src, tag_name);
	if (tag == NULL)
		return error;

	new_tag = CloneXMLTag(tag, GN_TRUE);
	if (new_tag == NULL)
		return XLTERR_OutOfMemory;

	AddXMLSubTag(xml_dst, new_tag);
	return error;
}


static gn_xlt_error_t
do_convert_xml(XMLTagRef xml, gn_udiscinfo_t *info, gn_bool_t full)
{
	gn_xlt_error_t error = XLTERR_NoError;

	if (xml == 0 || info == 0) {
		return XLTERR_InvalidParam;
	}

	gnmem_memset(info, 0, full == GN_TRUE ? sizeof(gn_discinfo_t) : sizeof(gn_sdiscinfo_t));

	if (gn_str_eq(GetXMLTagName(xml), GN_STR_ALBUM)) {

		XMLTagRef sub_tag = GetXMLSubTagFromStr(xml, GN_STR_TITLE);
		XMLTagRef tracks_tag = 0;

		if (sub_tag != 0) {
			info->dis.di_title = gn_strdup(GetXMLTagData(sub_tag));
		}

		sub_tag = GetXMLSubTagFromStr(xml, GN_STR_ARTIST);

		if (sub_tag != 0) {
			info->dis.di_artist = gn_strdup(GetXMLTagData(sub_tag));
		}

		sub_tag = GetXMLSubTagFromStr(xml, GN_STR_GENRE);

		if (sub_tag != 0) {
			info->dis.di_genre = gn_strdup(GetXMLTagData(sub_tag));
		}

		sub_tag = GetXMLSubTagFromStr(xml, GN_STR_REVISION);

		if (sub_tag != 0) {
			gn_cstr_t	str = GetXMLTagData(sub_tag);

			info->dis.di_revision = parse_digits_to_uint32(str, strlen(str), GN_FALSE);
		}
		else {
			info->dis.di_revision = 0;
		}

		if (full == GN_TRUE) {
			tracks_tag = GetXMLSubTagFromStr(xml, GN_STR_TRACKS);

			if (tracks_tag != 0) {

				/* This code adds the track titles based on the "N" attributes of */
				/* the TRK tags. */
				gn_cstr_t track_count_str = GetXMLTagAttrFromStr(tracks_tag, GN_STR_COUNT);
				gn_uint32_t sub_tag_count = GetXMLSubTagCount(tracks_tag);
				gn_uint32_t i;

				info->dif.di_numtracks = track_count_str != 0 ? atoi(track_count_str) : 0;

				for (i = 0; i < sub_tag_count; i++) {

					XMLTagRef sub_tag = GetXMLSubTag(tracks_tag, i);

					if ((sub_tag != 0) && gn_str_eq(GetXMLTagName(sub_tag), GN_STR_TRACK)) {

						gn_cstr_t track_num_str = GetXMLTagAttrFromStr(sub_tag, GN_STR_NUMBER);

						gn_int32_t track_num = (track_num_str != 0)
											 ? (atoi(track_num_str) - 1)
											 : -1;

						XMLTagRef title_tag = GetXMLSubTagFromStr(sub_tag, GN_STR_TITLE);

						if (title_tag != 0 && GetXMLTagData(title_tag) != 0 &&
							track_num >= 0 && track_num < MAXTRKS) {
							info->dif.di_tracktitles[track_num] = gn_strdup(GetXMLTagData(title_tag));
						}
					}
				}
			}
		}
	}
	else {
		error = XLTERR_BadFormat;
	}

	return error;
}
