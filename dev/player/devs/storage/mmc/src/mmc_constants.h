// mmc_constants.h: constant values per the MMC protocol
// Dan Conti 02/20/01 danc@iobjects.com
// (c) Interactive Objects. It's only sometimes funny.

#ifndef __MMC_CONSTANTS_H__
#define __MMC_CONSTANTS_H__

#define MMC_RESPONSE_LENGTH  6  // bytes in response length
#define MMC_CID_LENGTH      17  // bytes in card ID
#define MMC_COMMAND_LENGTH   6  // bytes in an MMC command

#define MMC_RESP1_3_BITLEN  48
#define MMC_RESP2_BITLEN   136


#define MMC_RESET_DELAY     30

// all commands are preceded by the start bit
#define MMC_CMD_STARTBIT   0x40

// MMC low level commands, all in hex because it makes it easier to interpret serial output
#define MMC_CMD_GO_IDLE_STATE           (0x00 + MMC_CMD_STARTBIT)
#define MMC_CMD_SEND_OP_COND            (0x01 + MMC_CMD_STARTBIT)
#define MMC_CMD_ALL_SEND_CID            (0x02 + MMC_CMD_STARTBIT)
#define MMC_CMD_SET_RELATIVE_ADDR       (0x03 + MMC_CMD_STARTBIT)
#define MMC_CMD_SET_DSR                 (0x04 + MMC_CMD_STARTBIT)
#define MMC_CMD_SELECT_DESELECT_CARD    (0x07 + MMC_CMD_STARTBIT)
#define MMC_CMD_SEND_CSD                (0x09 + MMC_CMD_STARTBIT)
#define MMC_CMD_SEND_CID                (0x0a + MMC_CMD_STARTBIT)
#define MMC_CMD_READ_DAT_UNTIL_STOP     (0x0b + MMC_CMD_STARTBIT)
#define MMC_CMD_STOP_TRANSMISSION       (0x0c + MMC_CMD_STARTBIT)
#define MMC_CMD_SEND_STATUS             (0x0d + MMC_CMD_STARTBIT)
#define MMC_CMD_SET_BUS_WIDTH_REGISTER  (0x0e + MMC_CMD_STARTBIT)
#define MMC_CMD_GO_INACTIVE_STATE       (0x0f + MMC_CMD_STARTBIT)
#define MMC_CMD_SET_BLOCKLEN            (0x10 + MMC_CMD_STARTBIT)
#define MMC_CMD_READ_BLOCK              (0x11 + MMC_CMD_STARTBIT)
#define MMC_CMD_READ_MULTIPLE_BLOCK     (0x12 + MMC_CMD_STARTBIT)
#define MMC_CMD_WRITE_DAT_UNTIL_STOP    (0x14 + MMC_CMD_STARTBIT)
#define MMC_CMD_WRITE_BLOCK             (0x18 + MMC_CMD_STARTBIT)
#define MMC_CMD_WRITE_MULTIPLE_BLOCK    (0x19 + MMC_CMD_STARTBIT)
#define MMC_CMD_PROGRAM_CID             (0x1a + MMC_CMD_STARTBIT)
#define MMC_CMD_PROGRAM_CSD             (0x1b + MMC_CMD_STARTBIT)
#define MMC_CMD_SET_WRITE_PROT          (0x1c + MMC_CMD_STARTBIT)
#define MMC_CMD_CLR_WRITE_PROT          (0x1d + MMC_CMD_STARTBIT)
#define MMC_CMD_SEND_WRITE_PROT         (0x1e + MMC_CMD_STARTBIT)
#define MMC_CMD_TAG_SECTOR_START        (0x20 + MMC_CMD_STARTBIT)
#define MMC_CMD_TAG_SECTOR_END          (0x21 + MMC_CMD_STARTBIT)
#define MMC_CMD_UNTAG_SECTOR            (0x22 + MMC_CMD_STARTBIT)
#define MMC_CMD_TAG_ERASE_GROUP_START   (0x23 + MMC_CMD_STARTBIT)
#define MMC_CMD_TAG_ERASE_GROUP_END     (0x24 + MMC_CMD_STARTBIT)
#define MMC_CMD_UNTAG_ERASE_GROUP       (0x25 + MMC_CMD_STARTBIT)
#define MMC_CMD_ERASE_SECTORS           (0x26 + MMC_CMD_STARTBIT)
#define MMC_CMD_CRC_ON_OFF              (0x3b + MMC_CMD_STARTBIT) // probably not really supported

// there are 4 types of responses

#define MMC_RESP_0  0
#define MMC_RESP_1  1
#define MMC_RESP_2  2
#define MMC_RESP_3  3

// error codes, i use a cyg_uint32 for these
#define MMC_ERR_NONE             0x00  // no error
#define MMC_ERR_NOT_RESPONDING   0x01  // card not responding
#define MMC_ERR_TIMEOUT          0x02  // timed out while referring to card
#define MMC_ERR_NO_DEVICES       0x03  // no devices found on MMC bus
#define MMC_ERR_ADDRESS          0x04  // attempt to reference an invalid address
#define MMC_ERR_BAD_DATA         0x05  // bad data, or a data xfer error
#define MMC_ERR_NO_CRC           0x06  // no crc was received after a transmit
#define MMC_ERR_CRC_INVALID      0x07  // an invalid CRC was received
#define MMC_ERR_MEDIACHANGE      0x08  // the card has been swapped out
#define MMC_ERR_GENERAL          0x09  // general error

// states for the card
#define MMC_CARDSTATE_IDLE      0
#define MMC_CARDSTATE_READY     1
#define MMC_CARDSTATE_IDENT     2
#define MMC_CARDSTATE_STANDBY   3
#define MMC_CARDSTATE_TRANSFER  4
#define MMC_CARDSTATE_DATA      5
#define MMC_CARDSTATE_RCV       6
#define MMC_CARDSTATE_PRG       7
#define MMC_CARDSTATE_DIS       8

// WP bit control
//#define MMC_TMPWPBIT_ON     0x10
//#define MMC_TMPWPBIT_OFF    0xEF

// default block length
#define MMC_DEFAULT_BLOCK_LEN  512

#endif // __MMC_CONSTANTS_H__
