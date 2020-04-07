/*
 * Copyright (c) 2000, 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * read_cd_drive.cpp:	Wrapper routines to retrieve TOC from CD drive.
 */

/*
 * Dependencies.
 */

#include <extras/cddb/read_cd_drive.h>

#include <stdio.h>
#include <cyg/fileio/fileio.h>
#include <devs/storage/ata/atadrv.h>
#include <dirent.h>
#include <io/storage/blk_dev.h>


Cyg_ErrNo DoPacket(cyg_io_handle_t hCD, unsigned char *command, unsigned char *data, int data_length, int dir)
{
    ATAPICommand_T  cmd;
    cyg_uint32      len;
    int             status;

    memset(&cmd, 0, sizeof(cmd));
    cmd.Flags = dir; //(dir | ATAPI_AUTO_SENSE);
    cmd.Data = (char *)data;
    cmd.DataLength = data_length;
    cmd.Timeout = 100 * 30; /* 30s */
	
    memcpy(&cmd._SCSICmd, command, 12);
    cmd.SCSICmd = &cmd._SCSICmd;
    cmd.SCSICmdLength = 12;
	
    len = sizeof(cmd);
    status = cyg_io_set_config(hCD, IO_ATAPI_SET_CONFIG_EXEC_COMMAND, &cmd, &len);
    if (status) {
        static const unsigned char RequestSenseData[14]={0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        static const unsigned char packet_command_RequestSense[12]={0x03,0,0,0,14,0,0,0,0,0,0,0};	
		
        memset(&cmd, 0, sizeof(cmd));
        cmd.Flags = ATAPI_DATA_IN;
        cmd.Data = (char *)RequestSenseData;
        cmd.DataLength = sizeof(RequestSenseData);
        cmd.Timeout = 100 * 30; /* 30s */

        memcpy(&cmd._SCSICmd, packet_command_RequestSense, sizeof(packet_command_RequestSense));
        cmd.SCSICmd = &cmd._SCSICmd;
        cmd.SCSICmdLength = 12;

        len = sizeof(cmd);
        cyg_io_set_config(hCD, IO_ATAPI_SET_CONFIG_EXEC_COMMAND, &cmd, &len);
        status = RequestSenseData[12];
    }
    return status;
}

/* 
** Attempt to read the CD in the default drive.
*/
int read_toc_from_default_drive(char* toc_buffer, int toc_buffer_size)
{
    /* Use the default drive name. */
    return read_toc_from_drive("/dev/cda/", toc_buffer, toc_buffer_size);

}

/* 
** Attempt to read the CD in the specified drive.
*/
int read_toc_from_drive(const char * drive_spec, char* toc_buffer, 
                        int toc_buffer_size)
{
    cyg_io_handle_t hCD;
    cyg_uint32      len = 1;
    Cyg_ErrNo       ret;
    int             first, tracks, i;
    unsigned char   packet_command[12];
    unsigned char   packet_data[12];
    unsigned char   packet_command_TOC[12] = {0x43,2,0,0,0,0,0,0,12,0,0,0};

	char    buff[8192];
    char    str[32];

    memset(toc_buffer, 0, toc_buffer_size);
	*buff = 0;

    /* open the drive */
    ret = cyg_io_lookup(drive_spec, &hCD);
    if (ret == -ENOENT)
        return -1;

    ret = cyg_io_set_config(hCD, IO_BLK_SET_CONFIG_POWER_UP, 0, &len);
    if (ret != ENOERR)
        return -1;

    /* set up packet command to grab TOC geometry before anything else */
    memcpy(packet_command, packet_command_TOC, 12);

    ret = DoPacket(hCD, packet_command, packet_data, 12, ATAPI_DATA_IN);
    if (ret != ENOERR)
    {
        ret = DoPacket(hCD, packet_command, packet_data, 12, ATAPI_DATA_IN);
        if (ret != ENOERR)
            return -1;
    }

    /* how many tracks on this disc? */
    first = packet_data[2];
    tracks = packet_data[3] - first + 1;

    for (i = 0; i < tracks; ++i) {
        memcpy(packet_command, packet_command_TOC, 12);
        packet_command[6] = i + first;
		
        ret = DoPacket(hCD, packet_command, packet_data, 12, ATAPI_DATA_IN);

        if (ret == ENOERR) {
            sprintf(str, "%d ", packet_data[9] * 60 * 75 + packet_data[10] * 75 + packet_data[11]);
            strcat(buff, str);
        }
    }

    /* End with the leadout info */
    memcpy(packet_command, packet_command_TOC, 12);
    packet_command[6] = 0xAA;
    ret = DoPacket(hCD, packet_command, packet_data, 12, ATAPI_DATA_IN);
    if (ret == ENOERR) {
        sprintf(str, "%d", packet_data[9] * 60 * 75 + packet_data[10] * 75 + packet_data[11]);
        strcat(buff, str);
    }

    /* Make sure we copy no more than the caller can handle */
    strncpy(toc_buffer, buff, toc_buffer_size - 1);

    return 0;
}
