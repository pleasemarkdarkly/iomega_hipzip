# default.dcl: default configuration for the cddb include module
# edwardm@iobjects.com 4/04/2002

name cddb
type extras

#requires cddb_common_extras fat_fs

#build_flags -DGN_NO_ONLINE

export crossplatform.h gn_build.h gn_cache.h gn_cachex.h gn_char_map.h gn_checksum.h
export gn_comm.h gn_configmgr.h gn_crypt.h gn_ctype.h gn_debug.h gn_defines.h gn_dyn_buf.h
export gn_edb_integ_mgr.h gn_error_codes.h gn_error_values.h gn_errors.h gn_flash.h 
export gn_fs.h gn_log.h gn_lookup.h gn_memory.h gn_platform.h gn_registrar.h gn_string.h
export gn_system.h gn_tagging.h gn_thl.h gn_timer.h gn_tocinfo.h gn_translator.h
export gn_transporter.h gn_upd.h gn_upd_constants.h gn_utils.h microxml.h port.h
export read_cd_drive.h template.h toc_util.h

# Use this to link a library
#link libecddblib-nozlib.a
# Or use the below to link directly against the object files
compile adler32.c db_freelist.c ec_vlong.c gn_cddbmsg.c inftrees.c tag.c
compile compress.c db_index.c edb_cache.c gn_checksum.c infutil.c tea128.c
compile crc_32.c db_interface.c edb_crc.c gn_encryption.c keys.c toc_convert.c
compile crypt_high.c db_io.c edb_init.c gn_payload.c libecddblib-nozlib.a toc_lookup.c
compile crypt_high_enc.c db_pack.c edb_integ_mgr.c gn_protocol.c md5.c trees.c
compile crypt_low.c db_util.c edb_lookup.c gn_register.c mid.c upd_integrate.c
compile crypt_low_enc.c deflate.c edb_tagging.c gn_transmit.c online_lookup.c upd_online.c
compile crypt_low_enc_fs.c ec_crypt.c edb_univ_header.c gn_upd.c otp.c upd_package.c
compile crypt_low_fs.c ec_curve.c edb_update.c infblock.c pegwit.c upd_unpackage.c
compile db_block.c ec_field.c endian.c infcodes.c sha1.c zipglue.c
compile db_cache.c ec_param.c fuzzy.c inffast.c sys_manager.c zutil.c