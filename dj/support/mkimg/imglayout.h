// imglayout.h: structures and format for DJ firmware update process
// (c) fullplay media systems

#ifndef __IMGLAYOUT_H__
#define __IMGLAYOUT_H__

#define XOR_VALUE 0xCFDBDCDD

#define IMG_MAGIC_BYTES     "DJFWIMG#"
#define IMG_NUM_MAGIC_BYTES 8

static const char szMagicBytes[] = IMG_MAGIC_BYTES;

typedef struct img_file_header_s
{
    // magic bytes at the beginning of the file to identify the file as a proper img
    unsigned char magic_bytes[IMG_NUM_MAGIC_BYTES];
    // crc32 to verify integrity of the file
    unsigned int  crc32;
    // initial offset from which the crc is run (usually the following word)
    unsigned int  crc_offset;
    // number of firmware update entries
    unsigned int  fw_entries;
    // offset to the firmware update table
    unsigned int  fw_offset;
    // version number for this image
    unsigned int  version;
    // next x bytes are a comment string
    char          comment[0];
} img_file_header_t;

typedef struct fw_image_header_s 
{
    // magic bytes
    unsigned int magic;
#define FW_MAGIC  0xCFDBDCFF
    // address to burn to in flash table (-f)
    unsigned int fis_burnaddr;
    // entry point for the image (-e)
    unsigned int fis_entry;
    // load address for the image (-r)
    unsigned int fis_loadaddr;
    // length in bytes of the image (-l)
    unsigned int fis_length;
    // name of the entry
    char fis_name[16];
    // offset of the image within this file
    unsigned int offset;
    // flags for this entry
    unsigned int flags;
#define FW_COMPRESSED   0x01   // image is compressed
#define FW_CRYPT_XOR1   0x02   // image uses 32bit xor with XOR_VALUE
#define FW_CRYPT_XOR2   0x04   // image uses 32bit xor with (XOR_VALUE + (image_index<<image_index))
#define FW_CRYPT_RES1   0x08   // reserved type 1
#define FW_CRYPT_RES2   0x10   // reserved type 2
#define FW_CRYPT_RES3   0x20   // reserved type 3
#define FW_CRYPT_RES4   0x40   // reserved type 4
#define FW_CRYPT_RESERVED (FW_CRYPT_RES1|FW_CRYPT_RES2|FW_CRYPT_RES3|FW_CRYPT_RES4)
} fw_image_header_t;

#endif // __IMGLAYOUT_H__
