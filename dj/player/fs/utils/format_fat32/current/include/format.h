// format.h
// fat32 format utility
// temancl@fullplaymedia.com 04/16/02

#ifndef _FDISK_H
#define _FDISK_H

#ifdef __cplusplus
extern "C" {
#endif

int format_drive(char* fname,void (*statuscb)(int,int));

#ifdef __cplusplus
}
#endif

#endif // _FDISK_H


