#ifndef GN_CHECKSUM_H
#define GN_CHECKSUM_H

/*
 * Copyright (c) 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * Gracenote Embedded Database Checksum Functions
 *
 * 
 */

/*
 * Dependencies
 */

#include <extras/cddb/gn_defines.h>
#include <extras/cddb/gn_errors.h>


#ifdef __cplusplus
extern "C" {
#endif

extern gn_cstr_t CHECKSUM_CRC32;
extern gn_cstr_t CHECKSUM_NONE;

typedef struct gn_checksum_alg_data_t* gn_checksum_alg_data_ref_t;

/*
 * Prototypes
 */

/* gn_verify_file_checksum
 *
 * Given a filename, a checksum algorithm descriptor string, and a checksum
 * value string, this function computes the checksum for the file using the
 * given algorithm and compares its result with the given expected value.
 * If the values match, UTILERR_NoError is returned. If the values do not
 * match, UTILERR_ChecksumMismatch is returned. If the algorithm descriptor
 * is not supported, UTILERR_UnknownChecksumAlg is returned. If
 * file_checksum_value is not null, the computed checksum value string for
 * the file is returned there. It is the caller's responsibility to release
 * that memory.
 */
gn_error_t
gn_verify_file_checksum(gn_cstr_t filename, gn_cstr_t checksum_alg,
                        gn_cstr_t checksum_value, gn_str_t* file_checksum_value);

/* gn_verify_buffer_checksum
 *
 * Given a buffer location and size, a checksum algorithm descriptor string, 
 * and a checksum value string, this function computes the checksum for the
 * buffer using the given algorithm and compares its result with the given
 * expected value. If the values match, UTILERR_NoErr is returned. If the
 * values do not match, UTILERR_ChecksumMismatch is returned. If the algorithm
 * descriptor is not supported, UTILERR_UnknownChecksumAlg is returned. If
 * file_checksum_value is not null, the computed checksum value string for
 * the file is returned there. It is the caller's responsibility to release
 * that memory.
 */
gn_error_t
gn_verify_buffer_checksum(gn_cstr_t buffer, gn_size_t length, gn_cstr_t checksum_alg,
                          gn_cstr_t checksum_value, gn_str_t* buffer_checksum_value);

/* gn_begin_stream_checksum
 *
 * Initializes alg_data for computing the checksum of an upcoming data stream
 * using the given algorithm. This call is typically followed by
 * gn_continue_stream_checksum. To deallocate the memory used by the algorithm
 * data structure, call gn_end_stream_checksum. If the algorithm descriptor is
 * not supported, UTILERR_UnknownChecksumAlg is returned.
 */
gn_error_t
gn_begin_stream_checksum(gn_cstr_t checksum_alg, gn_checksum_alg_data_ref_t* alg_data);

/* gn_continue_stream_checksum
 *
 * Continues checksum computation for the given buffer of data using the 
 * given algorithm and algorithm data structure. If the algorithm descriptor
 * is not supported, UTILERR_UnknownChecksumAlg is returned.
 */
gn_error_t
gn_continue_stream_checksum(gn_cstr_t checksum_alg, gn_checksum_alg_data_ref_t alg_data,
                            gn_cstr_t buffer, gn_size_t length);

/* gn_end_stream_checksum
 *
 * Call this function when there is no more data in the stream, passing the 
 * checksum algorithm, an optional checksum value that the stream is expected
 * to match, the algorithm data structure, and an optional location where
 * the stream checksum value will be kept. This function will deallocate any 
 * memory used by the algorithm data structure. If the caller does not pass
 * null for stream_checksum_value, the caller must deallocate that memory
 * using gnmem_free. If the algorithm descriptor is not supported,
 * UTILERR_UnknownChecksumAlg is returned. If checksum_value is not null, and
 * it does not match the stream checksum, UTILERR_ChecksumMismatch is returned.
 */
gn_error_t
gn_end_stream_checksum(gn_cstr_t checksum_alg, gn_cstr_t checksum_value,
                       gn_checksum_alg_data_ref_t* alg_data, gn_str_t* stream_checksum_value);

#ifdef __cplusplus
}
#endif

#endif
