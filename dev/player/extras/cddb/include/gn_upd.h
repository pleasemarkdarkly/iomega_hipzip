/*
 * Copyright (c) 2000, 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gn_upd.h - general API for data updates.
 */

#ifndef	_GN_UPD_H_
#define _GN_UPD_H_


/*
 * Dependencies.
 */

#include <extras/cddb/gn_defines.h>
#include <extras/cddb/gn_errors.h>
#include <extras/cddb/gn_upd_constants.h>
#include <extras/cddb/gn_transporter.h>
#include <extras/cddb/gn_registrar.h>

#ifdef __cplusplus
extern "C"{
#endif 


/*
 * Structures and typedefs.
 */

/* This structure represents the update summary information
 * found in the package file header block. */
typedef struct upd_update_summary
{
	gn_uint16_t		data_revision;
	gn_uint16_t		update_level;
	gn_uint32_t		package_offset;
	gn_uint32_t		envelope_file_size;
	gn_char_t		file_name[FS_FNAME_MAX+1];
}
gn_upd_update_summary_t;

/* This structure contains information on the data installed on a target device. */
typedef struct upd_install_profile
{
	gn_uint16_t		data_revision;
	gn_uint16_t		update_level_element_count;
	gn_uint16_t		last_update_level;
	gn_uint16_t		last_update_record;
	gn_uint16_t*	update_level_list;
}
gn_upd_install_profile_t;


/*
 * Prototypes.
 */

/* Callback proc used for monitoring update progress */
typedef		gn_upd_error_t (*gnupd_callback_t)(gn_upd_state state, gn_uint32_t count1, gn_uint32_t total1, gn_uint32_t count2, gn_uint32_t total2);


/*******************************
 * HIGH LEVEL UPDATE INTERFACE
 *******************************/

gn_upd_error_t
gn_upd_init(void);

void
gn_upd_set_callback(gnupd_callback_t callback);

void
gn_upd_shutdown(void);

gn_upd_error_t
gn_upd_update(gn_cstr_t package_file);

void
gn_upd_cleanup_profile(gn_upd_install_profile_t* profile);

/* error recover routine which cleans up temporary installation files */
void
gn_upd_cleanup(gn_cstr_t package_file);


/*******************************
 * INSTALLATION
 *******************************/

gn_upd_error_t
gn_upd_load_install_profile(gn_upd_install_profile_t* profile);

/*
 * This function installs all necessary update files on the target device hard drive.
 * It requires a single update package file, an install profile, a target installation
 * directory path on the device hard drive, and prior initialization of the cryptographic system.
 * The function expects the install directory path has the appropriate trailing separator character
 * (e.g., '\' on Windows).
 * This function combines all necessary steps in the installation process. In a perfect world,
 * this is the only function required by a package installation application; it is implemented
 * with most of the other functions in the Install API.
 */
gn_upd_error_t
gn_upd_install(void);

/*
 * This function takes an update package file,
 * and returns a package summary of the required update envelopes.
 */
gn_upd_error_t
gn_upd_determine_required_updates(gn_cstr_t package_file_path, gn_upd_update_summary_t** summary_list, gn_uint16_t* summary_element_count);

/*
 * This function extracts update envelope files from the update package file,
 * as indicated by the contents of the summary list, and transfers them to the
 * hard drive location identified by the install directory path.
 * This function is implemented with gnupd_extract_from_package.
 * The function expects the install directory path has the appropriate trailing separator character
 * (e.g., '\' on Windows).
 */
gn_upd_error_t
gn_upd_extract_from_package_batch(gn_cstr_t package_file_path, gn_cstr_t install_directory_path, gn_upd_update_summary_t* summary_list, gn_uint16_t element_count);

/*
 * This function extracts an update envelope file from the update package file
 * and transfers that envelope file to the device hard drive. The target location
 * on the hard drive is determined by the value of the envelope file path parameter.
 */
gn_upd_error_t
gn_upd_extract_from_package(gn_handle_t package_file_fd, gn_upd_update_summary_t* summary, gn_cstr_t envelope_file_path);

/*
 * This function extracts a decrypted update archive file from an update envelope file.
 * The decrypted archive file is transferred to the location on the device hard drive
 * indicated by the parameter value.
 * This function performs all necessary decryption internally, and requires the caller
 * to first call gn_crypt_init to initialize the necessary cryptographic facilities.
 * It is implemented with upd_open_envelope_decrypt.
 */
gn_upd_error_t
gn_upd_open_envelope(gn_cstr_t envelope_file_path, gn_cstr_t archive_file_path);


/*******************************
 * INTEGRATION
 *******************************/

/*
 * This function integrates all installed updated files into the embedded database on the target device.
 * It requires an installation directory path on the device hard drive, and prior initialization of the
 * database integrity system.
 * The progress of the integration is stored in an internal profile. Call gn_upd_load_install_profile
 * if you wish details of what the integration accomplishes.
 * This function combines all necessary steps in the integration process. In a perfect world,
 * this is the only function required by an update integration application; it is implemented with most of
 * the other functions in the Integration API.
*/

gn_upd_error_t
gn_upd_integrate(void);

gn_upd_error_t
gn_upd_integrate_update_file(gn_cstr_t update_file, gn_upd_install_profile_t* profile);


/*******************************
 * ONLINE
 *******************************/

typedef struct gn_upd_file_list_t* gn_upd_file_list_ref_t;

gn_size_t
gn_upd_get_file_count(gn_upd_file_list_ref_t file_list);

void
gn_upd_dispose_file_list(gn_upd_file_list_ref_t file_list);

gn_error_t
gn_upd_configure_online_update(gn_cstr_t new_server_url);

gn_cstr_t
gn_upd_get_server_url(void);

/* Caller must free update_files */
gn_error_t
gn_upd_get_remote_update_files(gn_upd_install_profile_t* profile,
                               gn_registrar_t* registrar,
                               gn_transporter_t* transporter,
                               gn_upd_file_list_ref_t* update_files);

/* Caller must free file_url */
gn_error_t
gn_upd_extract_update_file_url(gn_upd_file_list_ref_t update_files,
                               gn_size_t index, gn_cstr_t* file_url);

gn_error_t
gn_upd_get_update_file_date(gn_upd_file_list_ref_t update_files,
                            gn_size_t index, gn_uint32_t* date);

gn_error_t
gn_upd_get_update_file_size(gn_upd_file_list_ref_t update_files,
                            gn_size_t index, gn_size_t* size);

/* Caller must free alg and value */
gn_error_t
gn_upd_extract_update_file_checksum(gn_upd_file_list_ref_t update_files,
                                    gn_size_t index, gn_cstr_t* alg, gn_cstr_t* value);

/* Caller must free update_file, as dictated by transporter */
gn_error_t
gn_upd_download_update_file(gn_cstr_t file_url, gn_cstr_t* update_file,
                            gn_transporter_t* transporter);

/* caller must free update_requests */
gn_error_t
gn_upd_make_portable_update_requests(gn_upd_install_profile_t* profile,
                                     gn_cstr_t* update_requests);

#ifdef __cplusplus
}
#endif 


#endif /* _GN_UPD_H_ */


