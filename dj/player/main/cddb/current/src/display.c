/*
 * Copyright (c) 2000, 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * display.c:	Sample code demonstrating simple display of disc information
 *				retrieve through various lookup functions and techniques.
 */

/*
 * Dependencies.
 */

#include <extras/cddb/gn_platform.h>
#include GN_STRING_H
#include GN_STDIO_H
#include GN_STDLIB_H
#include <extras/cddb/gn_errors.h>
#include <extras/cddb/gn_translator.h>
#include <extras/cddb/gn_char_map.h>
#include <extras/cddb/gn_lookup.h>
#include <extras/cddb/gn_memory.h>
#include "display.h"
#include "lookup.h"
#include "shell.h"
#include <extras/cddb/gn_utils.h>
#include <extras/cddb/gn_configmgr.h>
#include "branding.h"
#include <util/debug/debug.h>


/* Local functions */

/* validate the lookup data for the toc */
static int
validate_string(const char* string_type, const char* candidate);

/* get base64 decoded data into temporary buffer */
static void
get_base64_decoded(char *input, char *buffer, int size);

/* print raw UTF8 data */
static void
print_utf8_field(const char*  label, const char* utf8);


/*
 * Variables
 */

extern gn_bool_t	g_validate_lookup;
extern char*		g_artiststr;
extern char*		g_titlestr;
extern char*		g_tocstr;


/*
 *	Multiple choice selection helper functions.
 */

/* display the short info & get selection */
gn_uint32_t
get_selection(gn_sdiscinfo_t** discs, gn_uint32_t count)
{
	gn_uint32_t		n = 0;
	int				retval;
	char			*p, buffer[1024];

	/* display limited information for user to select */
	diag_printf("%d potential matches for this TOC\n# Artist/Title/Genre/Revision\n- ---------------------\n", count);
	for (n = 0; n < count; n++) {
		if (gEncoding) {
			diag_printf("%d ", n + 1);
			get_base64_decoded((*(discs+n))->di_artist, buffer, sizeof buffer);
			diag_printf("%s / ", buffer);
			get_base64_decoded((*(discs+n))->di_title, buffer, sizeof buffer);
			diag_printf("%s / ", buffer);
			get_base64_decoded((*(discs+n))->di_genre, buffer, sizeof buffer);
			diag_printf("%s / ", buffer);
			diag_printf("%d\n", (*(discs+n))->di_revision);
		}
		else {
			diag_printf("%d %s / %s / %s / %d\n", n + 1, (*(discs+n))->di_artist, (*(discs+n))->di_title, (*(discs+n))->di_genre, (*(discs+n))->di_revision);
		}
	}

	if (gAutoSelect == GN_FALSE) {
		/* prompt user for selection and retrieve it */
		diag_printf("\nEnter the number matching your selection (0 for none): ");
		p = fgets(buffer, sizeof(buffer), stdin);
		retval = atoi(buffer);

		/* bounds check */
		if (retval > (int)count)
			n = 0;
		else
			n = retval;
	}
	else
		n = 1;

	return n;
}


#if defined(GN_NO_LOCAL)
int
resolve_local_fuzzy(gn_tlu_result_t** results, gn_uint32_t nresults, gn_tlu_result_t** selection)
{
	return 0;
}
#else
/*
 * resolve_local_fuzzy()
 *
 * Description:
 *	Parse data from multiple/fuzzy match on local lookup for the user
 *	to pick.
 */

int
resolve_local_fuzzy(gn_tlu_result_t** results, gn_uint32_t nresults, gn_tlu_result_t** selection)
{
	gn_xlt_error_t		xlate_err = XLTERR_NoError;
	gn_error_t			lookup_err = SUCCESS;
	gn_error_t			our_err = SUCCESS;
	gn_uint32_t			choice = 0;
	gn_uint32_t			n;
	void				*info = NULL;
	gn_size_t			info_size;
	gn_sdiscinfo_t		*matches[FUZZY_CHOICES];

	/* for each potential match, save up to FUZZY_CHOICES artist/title strings */
	for (n = 0; n < nresults && n < FUZZY_CHOICES; n++) {

		/* get the raw data */
		lookup_err = gntlu_result_info_alloc(*(results + n), &info, &info_size);
		if (lookup_err != TLERR_NoError) {
			our_err = lookup_err;
			break;
		}

		/* translate into the short form of the discinfo struct */
		xlate_err = gnxlt_sconvert_alloc(info, info_size, &matches[n]);

		gnmem_free(info);
		info = NULL;
		info_size = 0;

		if (xlate_err != XLTERR_NoError) {
			our_err = xlate_err;
			break;
		}

		/* save the toc_id - this can be used to retrieve the disc info */
		matches[n]->di_toc_id = gntlu_result_toc_id(*(results + n));
	}

	if (our_err != SUCCESS) {
		/* free the short info structs */
		if (n > 0) {
			while (n-- >= 0)
				gnxlt_srelease_free(matches[n]);
		}

		return our_err;
	}

	/* let the user choose */
	choice = get_selection(&matches[0], n);

	/* set the appropriate selection */
	if (choice == 0)
		*selection = NULL;
	else
		*selection = *(results + choice - 1);

	/* free intermediate selection data */
	for (n = 0; n < nresults && n < FUZZY_CHOICES; n++) {
		gnxlt_srelease_free(matches[n]);
	}
	return SUCCESS;
}
#endif /* #if !defined(GN_NO_LOCAL) */


#if defined(GN_NO_ONLINE)
int
resolve_online_fuzzy(XMLTagRef xml, XMLTagRef* xml_choice)
{
	return 0;
}
#else
/*
 * resolve_online_fuzzy()
 *
 * Description:
 *	Parse data from multiple/fuzzy match on online lookup for the user
 *	to pick.
 */

int
resolve_online_fuzzy(XMLTagRef xml, XMLTagRef* xml_choice)
{
	gn_error_t			xlate_err;
	gn_uint32_t			choice = 0;
	gn_uint32_t			matchcnt = 0;
	gn_uint32_t			n;
	gn_cstr_t			match_str;
	XMLTagRef			alb_tags[FUZZY_CHOICES];
	gn_sdiscinfo_t		*matches[FUZZY_CHOICES];

	/* multiple matches are containing in an ALBS element */
	if (strcmp(GetXMLTagName(xml), GN_STR_ALBUMS))
		return kXMLInvalidParamError;

	/* first extract MATCHES attribute from ALBS tag */
	match_str = GetXMLTagAttrFromStr(xml, GN_STR_MATCHES);
	if (match_str == NULL)
		return kXMLInvalidParamError;

	matchcnt = parse_digits_to_uint32(match_str, strlen(match_str), GN_FALSE);
	if (matchcnt <= 0)
		return kXMLInvalidParamError;

	/* get the subtags for the ALB elements */
	for (n = 0; n < matchcnt && n < FUZZY_CHOICES; n++) {
		alb_tags[n] = GetXMLSubTag(xml, n);
		if (alb_tags[n] == NULL) {
			if (n == 0)
				return kXMLInvalidParamError;
			matchcnt = n;
			break;
		}
	}

	for (n = 0; n < FUZZY_CHOICES; n++)
		matches[n] = NULL;

	/* convert the ALB elements to the sdiscinfo structs */
	for (n = 0; n < matchcnt && n < FUZZY_CHOICES; n++) {
		xlate_err = gnxlt_sconvert_xml_alloc(alb_tags[n], &matches[n]);
		if (xlate_err)
			break;
	}

	/* clean up on error */
	if (xlate_err != SUCCESS) {
		/* free the short info structs */
		if (n > 0) {
			while (n-- >= 0)
				gnxlt_srelease_free(matches[n]);
		}

		return xlate_err;
	}


	/* let the user choose */
	choice = get_selection(&matches[0], n);

	/* set the appropriate selection */
	if (choice == 0)
		*xml_choice = NULL;
	else
		*xml_choice = alb_tags[choice - 1];

	/* free intermediate selection data */
	for (n = 0; n < matchcnt && n < FUZZY_CHOICES; n++) {
		gnxlt_srelease_free(matches[n]);
	}
	return SUCCESS;
}
#endif /* #if !defined(GN_NO_ONLINE) */


/*
 *	Display helper functions.
 */

/*
 * display_discinfo()
 *
 * Description:
 *	Dump disc info to standard output.
 *
 */
int
display_discinfo(gn_discinfo_t* info)
{
	gn_uint32_t		n;

	if (info == NULL) {
		diag_printf("No match found.\n");
		if (g_tocstr)
			diag_printf("For TOC: %s\n", g_tocstr);
	}
	else {
		if (gEncoding) {
			char	buffer[1024];

			get_base64_decoded(info->di_title, buffer, sizeof buffer);
			diag_printf("Title: %s\n", buffer);
			get_base64_decoded(info->di_artist, buffer, sizeof buffer);
			diag_printf("Artist: %s\n", buffer);

			get_base64_decoded(info->di_genre, buffer, sizeof buffer);
			diag_printf("Genre: %s\n", buffer);

			diag_printf("Revision: %d\n", info->di_revision);

			for (n = 0; n < info->di_numtracks; n++) {
				get_base64_decoded(info->di_tracktitles[n], buffer, sizeof buffer);
				diag_printf("Track %d Title: %s\n", (int)n+1, buffer);
			}
		}
		else {
#if defined(PLATFORM_WIN32)
			gn_str_t	cnvt_buff;
			gn_size_t	cnvt_len;
			gn_error_t	cnvt_error;
#if defined(ARTIST_TITLE_ONLY)
			gn_char_t	temp_fmt[256];
#endif

			cnvt_error = gn_map_chars(info->di_title, strlen(info->di_title), &cnvt_buff, &cnvt_len);
			if (cnvt_error == SUCCESS) {
#if defined(ARTIST_TITLE_ONLY)
				sprintf(temp_fmt, "%-48s", cnvt_buff);
				diag_printf(temp_fmt);
#else
				diag_printf("Title: %s\n", cnvt_buff);
#endif
				gnmem_free(cnvt_buff);
			}
			else
				diag_printf("Error (%X) converting Title.\n", cnvt_error);
			if (gRawUTF8)
				print_utf8_field("Title", info->di_title);

			cnvt_error = gn_map_chars(info->di_artist, strlen(info->di_artist), &cnvt_buff, &cnvt_len);
			if (cnvt_error == SUCCESS) {
#if defined(ARTIST_TITLE_ONLY)
				diag_printf("%s\n", cnvt_buff);
#else
				diag_printf("Artist: %s\n", cnvt_buff);
#endif
				gnmem_free(cnvt_buff);
			}
			else
				diag_printf("Error (%X) converting Artist.\n", cnvt_error);
			if (gRawUTF8)
				print_utf8_field("Title", info->di_title);

#if !defined(ARTIST_TITLE_ONLY)
			cnvt_error = gn_map_chars(info->di_genre, strlen(info->di_genre), &cnvt_buff, &cnvt_len);
			if (cnvt_error == SUCCESS) {
				diag_printf("Genre: %s\n", cnvt_buff);
				gnmem_free(cnvt_buff);
			}
			else
				diag_printf("Error (%X) converting Genre.\n", cnvt_error);

			diag_printf("Revision: %d\n", info->di_revision);
	   		for (n = 0; n < info->di_numtracks; n++) {
				cnvt_error = gn_map_chars(info->di_tracktitles[n], strlen(info->di_tracktitles[n]), &cnvt_buff, &cnvt_len);
				if (cnvt_error == SUCCESS) {
			   		diag_printf("Track %d Title: %s\n", (int)n+1, cnvt_buff);
					gnmem_free(cnvt_buff);
				}
				else
					diag_printf("Error (%X) converting Track #%d.\n", cnvt_error, n + 1);
				if (gRawUTF8) {
					diag_printf("Track %d Title: ", n + 1);
					print_utf8_field("", info->di_tracktitles[n]);
				}
			}
#endif
#else
			diag_printf("Title: %s\n", info->di_title);
		    diag_printf("Artist: %s\n", info->di_artist);
	   		diag_printf("Genre: %s\n", info->di_genre);
			diag_printf("Revision: %d\n", info->di_revision);

	   		for (n = 0; n < info->di_numtracks; n++) {
		   		diag_printf("Track %d Title: %s\n", (int)n+1, info->di_tracktitles[n]);
			}
#endif
		}	    
	}

	return 0;
}


/* display terse validation information */
int
display_validation_discinfo(gn_discinfo_t* info)
{
	gn_bool_t	verbose = GN_FALSE;

	if (info == NULL)	{
		diag_printf("No exact match found for TOC\n");
		if (g_tocstr)
			diag_printf("%s\n%s\n%s\n",g_tocstr,g_artiststr,g_titlestr);
		/*
		if (g_misfile) {
			fprintf(g_misfile, "%s\n%s\n%s\n", g_tocstr, g_artiststr, g_titlestr);
		}
		*/
		return 0;
	}

	if (gEncoding) {
		char	title_buffer[1024];
		char	artist_buffer[1024];

		get_base64_decoded(info->di_title, title_buffer, sizeof title_buffer);
		if (validate_string("Title",title_buffer) != 0) {
		/*
			if (g_num_mismatch)
				(*g_num_mismatch)++;
		*/
			return 1;
		}
		get_base64_decoded(info->di_artist, artist_buffer, sizeof artist_buffer);
		if (validate_string("Artist", artist_buffer) != 0) {
		/*
			if (g_num_mismatch)
				(*g_num_mismatch)++;
		*/
			return 1;
		}

		if (verbose == GN_TRUE)
			diag_printf("== Valid lookup.\nArtist: %s\nTitle: %s\n", artist_buffer, title_buffer);
	}
	else {
		if (validate_string("Title", info->di_title) != 0) {
		/*
			if (g_num_mismatch)
				(*g_num_mismatch)++;
		*/
			return 1;
		}
		if (validate_string("Artist", info->di_artist) != 0) {
		/*
			if (g_num_mismatch)
				(*g_num_mismatch)++;
		*/
			return 1;
		}

		if (verbose == GN_TRUE)
			diag_printf("== Valid lookup.\nArtist: %s\nTitle: %s\n", info->di_artist, info->di_title);
	}

	return 0;
}

/* print limited disc information to standard output */
int
display_artist_title(void* info, gn_size_t info_size, XMLTagRef xml_info)
{
	gn_error_t			error = SUCCESS;
	gn_sdiscinfo_t*		disc_info = NULL;

	if (xml_info == NULL)
		error = gnxlt_sconvert_alloc(info, info_size, &disc_info);
	else
		error = gnxlt_sconvert_xml_alloc(xml_info, &disc_info);

	if (error == SUCCESS) {
		/* TODO: map chars */
		diag_printf("TITLE: %s\n", disc_info->di_title);
		diag_printf("ARTIST: %s\n", disc_info->di_artist);
	}

	if (disc_info)
		gnxlt_srelease_free(disc_info);

	return error;
}


/* validate the lookup data for the toc */
static int
validate_string(const char* string_type, const char* candidate)
{
	if (g_validate_lookup == GN_FALSE)
		return 0;

	if (!string_type)
		return 1;

	if (!strcmpi(string_type, "Title")) {
		if (!candidate || (strcmp(g_titlestr, candidate) && gStrict == GN_TRUE) || strcmpi(g_titlestr, candidate)) {
			diag_printf("\n*** Lookup error:\nTOC: %s\nExpected title: %s\n  Actual title: %s\n\n",
				g_tocstr, g_titlestr, candidate);

			return 1;
		}
	}
	else if (!strcmpi(string_type, "Artist")) {
		if (!candidate || (strcmp(g_artiststr, candidate) && gStrict == GN_TRUE) || strcmpi(g_artiststr, candidate)) {
			diag_printf("\n*** Lookup error:\nTOC: %s\nExpected artist: %s\n  Actual artist: %s\n\n",
				g_tocstr, g_artiststr, candidate);

			return 1;
		}
	}
	else
		return 1;

	return 0;
}



/*
 *	Conversion helper functions.
 */


/* get base64 decoded data into temporary buffer */
static void
get_base64_decoded(char *input, char *buffer, int size)
{
	char	*decoded = NULL;
	int		decode_size = 0;

	*buffer = 0;
	base64_decode(input, strlen(input), &decoded, &decode_size, 1);
	if (decoded && decode_size)
		strncpy(buffer, decoded, size);

	if (decode_size < size)
		*(buffer + decode_size) = 0;
	else
		*(buffer + size - 1) = 0;

	if (decoded)
		gnmem_free(decoded);
}


static void
print_utf8_field(const char*  label, const char* utf8)
{
	int		bytes;
	unsigned char*	p = (unsigned char*)utf8;

	if (label && *label)
		diag_printf("%s: ", label);

	while (*p) {
		if ((*p & 0x80)) {
			/* see how many bytes to process */
			if ((*p & 0xF0) == 0xF0)
				bytes = 4;
			else if ((*p & 0xE0) == 0xE0)
				bytes = 3;
			else if ((*p & 0xC0) == 0xC0)
				bytes = 2;
			else {
				diag_printf("(EEK)Too many bytes\n");
				return;
			}
			while (bytes > 0) {
				diag_printf("%X", *p);
				bytes--;
				p++;
			}
		}
		else {
			diag_printf("%c", *p);
			p++;
		}
	}
	diag_printf("\n");
}

