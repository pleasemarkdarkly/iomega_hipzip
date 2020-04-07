// dpcodes.h: various codes for dataplay drive
// danc@iobjects.com 12/13/2000

#ifndef __DPCMD_H__
#define __DPCMD_H__

// *************** Command codes ***************
#define DPICMD_DEVICE_INFO         0x00
#define DPICMD_LOCKREL_MEDIA       0x01
#define DPICMD_SET_PARAMETERS      0x02
#define DPICMD_GET_ATTENTION_INFO  0x03
#define DPICMD_GET_EXTENDED_STATUS 0x04
#define DPICMD_LOADEJECT_MEDIA     0x05
#define DPICMD_POWER_CONTROL       0x06
#define DPICMD_MEDIA_STATE         0x07
#define DPICMD_DFS_COMMAND         0x10

// *************** DFS command codes ***************
#define DFSCMD_COMMIT       0x01
#define DFSCMD_CREATEDIR    0x02
#define DFSCMD_CREATEFILE   0x03
#define DFSCMD_DELETE       0x04
#define DFSCMD_GETATTR      0x05
#define DFSCMD_GETFILEINFO  0x06
#define DFSCMD_GETHANDLE    0x07
#define DFSCMD_GETMEDIAINFO 0x08
#define DFSCMD_MOVE         0x09
#define DFSCMD_READFILE     0x0A
#define DFSCMD_READDIR      0x0B
#define DFSCMD_RENAME       0x0C
#define DFSCMD_SETATTR      0x0D
#define DFSCMD_WRITEAPPEND  0x0E

// *************** Error Codes *******************

#define DP_NO_ERROR       0x00
#define DP_UNKNOWN_CMD    0x01
#define DP_NO_MEDIA       0x02
#define DP_BAD_CMD_SIZE   0x03
#define DP_BAD_PARAMETER  0x04
#define DP_MEDIA_LOCKED   0x05
#define DP_MEDIA_FULL     0x06
#define DP_ILLEGAL_CMD    0x07
#define DP_MEDIA_LOADED   0x08

#define DFS_ACCESS_DENIED   0x10
#define DFS_NO_WRITE        0x11
#define DFS_NO_DELETE       0x12
#define DFS_NO_RENAME       0x13
#define DFS_NO_READ         0x14
#define DFS_BAD_HANDLE      0x15
#define DFS_ALREADY_EXISTS  0x16
#define DFS_NAME_TOO_LONG   0x17
#define DFS_FILE_NOT_FOUND  0x18
#define DFS_WRITE_ERROR     0x19
#define DFS_ILLEGAL_BUFSIZE 0x1A
#define DFS_BAD_SESSION_ID  0x1B
#define DFS_ILLEGAL_DIRMOVE 0x1C
#define DFS_DIR_NOT_EMPTY   0x1D

#define DFS_MEDIA_FULL      0x50

#define DFS_GENERAL         0xFF

#endif // __DPCMD_H__