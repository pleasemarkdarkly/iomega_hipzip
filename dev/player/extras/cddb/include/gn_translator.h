/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * data_translator.h - Interface definition for the "Data Structure
 *						Translator" module.
 */


#ifndef _DATA_TRANSLATOR_H_
#define _DATA_TRANSLATOR_H_

/*
 * Dependencies.
 */

#include <extras/cddb/gn_defines.h>
#include <extras/cddb/microxml.h>

#ifdef __cplusplus
extern "C"{
#endif 


/*
 * Constants.
 */

#if !defined(MAXTRKS)
#define		MAXTRKS		99
#endif

/* The following strings are the XML tags used for disc information */
static const char* GN_STR_ALBUM      = "ALB";
static const char* GN_STR_ALBUMS     = "ALBS";
static const char* GN_STR_DISCID	 = "DISCID";
static const char* GN_STR_MUID		 = "MUID";
static const char* GN_STR_MEDIAID	 = "MEDIAID";
static const char* GN_STR_REVISION   = "REV";
static const char* GN_STR_TITLE      = "TIT";
static const char* GN_STR_ARTIST     = "ART";
static const char* GN_STR_GENRE      = "GEN";
static const char* GN_STR_TRACKS     = "TRKS";
static const char* GN_STR_YEAR       = "YEAR";
static const char* GN_STR_TRACK      = "TRK";
static const char* GN_STR_TUID       = "TID";

/* The following strings are attribute names in the XML */
static const char* GN_STR_ID         = "ID";
static const char* GN_STR_TAG        = "TAG";
static const char* GN_STR_LNG        = "LNG";
static const char* GN_STR_CONTENTS   = "CONTENTS";
static const char* GN_STR_MATCHES    = "MATCHES";
static const char* GN_STR_MATCH      = "MATCH";
static const char* GN_STR_COUNT      = "CNT";
static const char* GN_STR_NUMBER     = "N";

/* The following strings are non-numeric attribute values in the XML */
static const char* GN_STR_PARTIAL    = "PARTIAL";
static const char* GN_STR_FULL       = "FULL";

/* Options for reduction/compaction of XML data */
#define	XLTCVT_Default		0	/* Titles, Artists, Genres */
#define XLTCVT_Revision		0x1
#define XLTCVT_Title		0x2
#define XLTCVT_Artist		0x4
#define XLTCVT_Genre		0x8
#define XLTCVT_Year			0x10
#define XLTCVT_DiscIds		0x20
#define XLTCVT_TrackTitle	0x40
#define XLTCVT_TrackArtist	0x80
#define XLTCVT_TrackGenre	0x100
#define XLTCVT_TrackYear	0x200
#define XLTCVT_TrackIds		0x400
#define XLTCVT_XMLHeader	0x1000
#define XLTCVT_XMLIndent	0x2000
#define XLTCVT_XMLNewlines	0x4000
#define XLTCVT_TrackMask	(XLTCVT_TrackTitle|XLTCVT_TrackArtist|XLTCVT_TrackGenre|XLTCVT_TrackIds)
#define XLTCVT_DefaultMask	(XLTCVT_XMLHeader|XLTCVT_Revision|XLTCVT_Title|XLTCVT_Artist|XLTCVT_Genre|XLTCVT_TrackTitle|XLTCVT_TrackArtist|XLTCVT_TrackGenre)

/*
 * Structures and typedefs.
 */

typedef gn_error_t		gn_xlt_error_t;
typedef gn_uint32_t		gn_xlt_options_t;

/* Disc lookup results structure returned from Data Translator. */
typedef struct gn_discinfo {
	gn_uint32_t	di_revision;				/* The revision number for the data record. */
	gn_str_t	di_title;					/* The title of the disc. */
	gn_str_t	di_artist;					/* The artist for the disc. */
	gn_str_t	di_genre;					/* The genre for the disc. */
	gn_uint32_t	di_numtracks;				/* The number of valid track titles in following array. */
	gn_str_t	di_tracktitles[MAXTRKS];	/* The track titles for the disc. */
} gn_discinfo_t;


/* Short disc lookup results structure returned from Data Translator. */
typedef struct gn_sdiscinfo {
	gn_uint32_t	di_revision;				/* The revision number for the data record. */
	gn_str_t	di_title;					/* The title of the disc. */
	gn_str_t	di_artist;					/* The artist for the disc. */
	gn_str_t	di_genre;					/* The genre for the disc. */
	gn_uint32_t	di_toc_id;					/* The toc_id for looking up full results. */
} gn_sdiscinfo_t;

/* Union of two structures */
typedef union gn_udiscinfo {
	gn_sdiscinfo_t	dis;
	gn_discinfo_t	dif;
} gn_udiscinfo_t;


/*
 * Prototypes.
 */

/* Initialize Data Translator subsystem. */
gn_xlt_error_t
gnxlt_initialize(void);

/* Shutdown Data Translator subsystem. */
gn_xlt_error_t
gnxlt_shutdown(void);

/* Convert raw data (XML text) to discinfo format. */
gn_xlt_error_t
gnxlt_convert(void *rawdata, gn_size_t data_size, gn_discinfo_t *info);

/* Convert raw data (XML text) to discinfo format, dynamically allocating structure and strings. */
gn_xlt_error_t
gnxlt_convert_alloc(void *rawdata, gn_size_t data_size, gn_discinfo_t **info);

/* Convert raw data (XML text) to short discinfo format, dynamically allocating structure and strings. */
gn_xlt_error_t
gnxlt_sconvert_alloc(void *rawdata, gn_size_t data_size, gn_sdiscinfo_t **info);

/* Convert structured XML to short discinfo format, dynamically allocating structure and strings. */
gn_xlt_error_t
gnxlt_sconvert_xml_alloc(XMLTagRef xml, gn_sdiscinfo_t **info);

/* Convert structured XML data to discinfo format. */
gn_xlt_error_t
gnxlt_convert_xml(XMLTagRef xml, gn_discinfo_t *info);

/* Convert structured XML data to discinfo format, dynamically allocating structure and strings. */
gn_xlt_error_t
gnxlt_convert_xml_alloc(XMLTagRef xml, gn_discinfo_t **info);

/* Reduce raw XML data into more compact format. */
gn_xlt_error_t
gnxlt_reduce_raw(void *rawdata, gn_size_t data_size, void **rawdata_out, gn_size_t *data_size_out, gn_xlt_options_t options);

/* Reduce structured XML data into more compact format. */
gn_xlt_error_t
gnxlt_reduce_xml(XMLTagRef xml, XMLTagRef* xml_out, gn_xlt_options_t options);

/* Release results from previous conversion. */
gn_xlt_error_t
gnxlt_release(gn_discinfo_t *info);

/* Release results from previous conversion and free structure. */
gn_xlt_error_t
gnxlt_release_free(gn_discinfo_t *info);

/* Release results from previous conversion. */
gn_xlt_error_t
gnxlt_srelease(gn_sdiscinfo_t *info);

/* Release results from previous conversion and free structure. */
gn_xlt_error_t
gnxlt_srelease_free(gn_sdiscinfo_t *info);

#ifdef __cplusplus
}
#endif 

#endif /* _DATA_TRANSLATOR_H_ */
