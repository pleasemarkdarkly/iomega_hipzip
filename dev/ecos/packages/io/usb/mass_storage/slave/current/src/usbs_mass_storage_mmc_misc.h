#ifndef CYGONCE_USBS_MASS_STORAGE_MMC_MISC_H_
#define  CYGONCE_USBS_MASS_STORAGE_MMC_MISC_H_
//==========================================================================
//
//      usbs_mass_storage_mmc_misc.h
//
//      Support for USB-mass storage MMC devices, slave-side.
//
//==========================================================================
//####COPYRIGHTBEGIN####
//                                                                          
// -------------------------------------------                              
// -------------------------------------------                              
//                                                                          
//####COPYRIGHTEND####
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    toddm@iobjects.com
// Contributors: toddm@iobjects.com
// Date:         2001-21-02
//
//####DESCRIPTIONEND####
//
//==========================================================================

#define MAX_LUNS 1

/* Commands */
#define TEST_UNIT_READY              0x00
#define REQUEST_SENSE                0x03
#define INQUIRY                      0x12
#define MODE_SENSE_6                 0x1a
#define START_STOP_UNIT              0x1b
#define PREVENT_ALLOW_MEDIUM_REMOVAL 0x1e
#define READ_FORMAT_CAPACITIES       0x23
#define READ_CAPACITY                0x25
#define READ_10                      0x28
#define WRITE_10                     0x2a
#define VERIFY_10                    0x2f

/* Sense keys */
#define NO_SENSE                                0x00
#define RECOVERED_ERROR                         0x01
#define NOT_READY                               0x02
#define MEDIUM_ERROR                            0x03
#define HARDWARE_ERROR                          0x04
#define ILLEGAL_REQUEST                         0x05
#define UNIT_ATTENTION                          0x06
#define DATA_PROTECT                            0x07
#define BLANK_CHECK                             0x08
#define VENDOR_SPECIFIC                         0x09
#define COPY_ABORTED                            0x0a
#define ABORTED_COMMAND                         0x0b
#define VOLUME_OVERFLOW                         0x0d
#define MISCOMPARE                              0x0e

/* Sense data parameters */
#define CURRENT_ERRORS                          0x70

/* ASCs */
#define POWER_ON_BUS_RESET                    0x29
#define INVALID_FIELD_IN_CDB                  0x24
#define NO_ADDITIONAL_SENSE_INFORMATION       0x00
#define INVALID_COMMAND_OPERATION_CODE        0x20
#define MEDIUM_NOT_PRESENT                    0x3a
#define MEDIUM_CHANGED                        0x28
#define WRITE_ERROR                           0x0c
#define READ_ERROR                            0x11
#define LOGICAL_UNIT_NOT_SUPPORTED            0x25

/* Read format capcities descriptor codes */
#define UNFORMATTED_MEDIA                     0x01
#define FORMATTED_MEDIA                       0x02
#define NO_MEDIA_PRESENT                      0x03

/* Vital product data pages */
#define SUPPORTED_VPD_PAGES                   0x00
#define UNIT_SERIAL_NUMBER_PAGE               0x80

typedef struct 
{
    cyg_uint8 operation_code;
    cyg_uint8 evpd      :1;
    cyg_uint8 cmddt     :1;
    cyg_uint8 reserved0 :6;
    cyg_uint8 page_op_code;
    cyg_uint8 reserved1;
    cyg_uint8 allocation_length;
    cyg_uint8 control;
} __attribute__((packed)) inquiry_cmd_t;

typedef struct
{
    cyg_uint8 peripheral_device_type :5;
    cyg_uint8 peripheral_qualifier   :3;

    cyg_uint8 reserved_0             :7;
    cyg_uint8 rmb                    :1;

    cyg_uint8 version;

    cyg_uint8 response_data_format   :4;
    cyg_uint8 hisup                  :1;
    cyg_uint8 normaca                :1;
    cyg_uint8 obsolete_0             :1;
    cyg_uint8 aerc                   :1;

    cyg_uint8 additional_length;

    cyg_uint8 reserved_1             :7;
    cyg_uint8 sccs                   :1;

    cyg_uint8 addr16                 :1;
    cyg_uint8 obsolete_1             :1;
    cyg_uint8 obsolete_2             :1;
    cyg_uint8 mchngr                 :1;
    cyg_uint8 multip                 :1;
    cyg_uint8 vs_0                   :1;
    cyg_uint8 encserv                :1;
    cyg_uint8 bque                   :1;

    cyg_uint8 vs_1                   :1;
    cyg_uint8 cmdque                 :1;
    cyg_uint8 trandis                :1;
    cyg_uint8 linked                 :1;
    cyg_uint8 sync                   :1;
    cyg_uint8 wbus16                 :1;
    cyg_uint8 obsolete_3             :1;
    cyg_uint8 reladr                 :1;

    cyg_int8 vendor_identification[8];
    cyg_int8 product_identification[16];
    cyg_int8 product_revision_level[4];
} __attribute__((packed)) inquiry_data_t;

typedef struct 
{
    cyg_uint8 peripheral_device_type : 4;
    cyg_uint8 peripheral_qualifier   : 4;
    
    cyg_uint8 page_code;
    cyg_uint8 reserved;
    cyg_uint8 page_length;

    cyg_uint8 supported_page_list[0];
}__attribute__((packed)) supported_vpd_t;

typedef struct
{
    cyg_uint8 peripheral_device_type : 4;
    cyg_uint8 peripheral_qualifier   : 4;
    
    cyg_uint8 page_code;
    cyg_uint8 reserved;
    cyg_uint8 page_length;

    cyg_uint8 product_serial_number[0];
}__attribute__((packed)) unit_serial_number_t;

typedef struct
{
    cyg_uint8 response_code   :7;
    cyg_uint8 valid           :1;
    
    cyg_uint8 segment_number;
    
    cyg_uint8 sense_key       :4;
    cyg_uint8 reserved0       :1;
    cyg_uint8 ili             :1;
    cyg_uint8 eom             :1;
    cyg_uint8 filemark        :1;
    
    cyg_int32 information;
    cyg_uint8 additional_length;
    cyg_int32 command_specific_information;
    cyg_uint8 asc;
    cyg_uint8 ascq;
    cyg_uint8 field_replacable_unit_code;
    cyg_uint8 sense_key_specific[3];
} __attribute__((packed)) sense_data_t;

typedef struct
{
    unsigned char operation_code;
    
    unsigned char reserved0 :3;
    unsigned char dbd       :1;
    unsigned char reserved1 :4;
    
    unsigned char page_code :6;
    unsigned char pc        :2;
    
    unsigned char reserved2;
    unsigned char allocation_length;
    unsigned char control;
}__attribute__((packed)) mode_sense_6_cmd_t;

typedef struct
{
    unsigned char mode_data_length;
    unsigned char medium_type;
    unsigned char device_specific;
    unsigned char block_descriptor_length;
}__attribute__((packed)) mode_param_header_6_t;

typedef struct
{
    unsigned int number_of_blocks;
    unsigned char density_code;
    unsigned char block_length_msb;
    unsigned short block_length;
}__attribute__((packed)) block_descriptor_t;

typedef struct
{
    unsigned char operation_code;
    unsigned char reserved0[6];
    short allocation_length;
    unsigned char control;
}__attribute__((packed)) read_format_capacities_cmd_t;

typedef struct
{
    unsigned char operation_code;

    unsigned char reladr    :1;
    unsigned char reserved0 :7;

    int lba;

    unsigned char reserved1[2];

    unsigned char pmi       :1;
    unsigned char reserved2 :7;
    
    unsigned char control;
}__attribute__((packed)) read_capacity_cmd_t;

typedef struct
{
    int lba;			/* this is big-endian */
    int blk_len;		/* this is big-endian */
}__attribute__((packed)) read_capacity_data_t;

typedef struct
{
    unsigned char operation_code;
    unsigned char reserved0;
    int lba;			/* this is big-endian */
    unsigned char reserved1;
    short transfer_length;	/* this is big-endian */
    unsigned char control;
}__attribute__((packed)) read_10_cmd_t;

typedef struct
{
    unsigned char operation_code;
    
    unsigned char reserved0 :3;
    unsigned char fua       :1;
    unsigned char reserved1 :4;
    
    int lba;			/* this is big-endian */
    unsigned char reserved2;
    short transfer_length;	/* this is big-endian */
    unsigned char control;
}__attribute__((packed)) write_10_cmd_t;

typedef struct
{
    unsigned char operation_code;
    unsigned char reserved[3];
    unsigned char allocation_length;
    unsigned char control;
}__attribute__((packed)) request_sense_cmd_t;

typedef struct
{
    unsigned char reserved0[3];
    unsigned char list_length;
}__attribute__((packed)) capacity_list_header_t;

typedef struct
{
    int number_of_blocks;    
    unsigned char descriptor_code :2;
    unsigned char reserved0       :6;
    unsigned char block_length_msb; /* block length is 3 bytes */
    short block_length;
}__attribute__((packed)) current_capacity_t;

typedef struct
{
    int number_of_blocks;
    unsigned char reserved0   :2; 
    unsigned char format_type :6;
    unsigned char param_msb; /* param is 3 bytes and the meaning depends on format_type */
    short param;
}__attribute__((packed)) formattable_capacity_t;

#endif // CYGONCE_USBS_MASS_STORAGE_MMC_MISC_H_
