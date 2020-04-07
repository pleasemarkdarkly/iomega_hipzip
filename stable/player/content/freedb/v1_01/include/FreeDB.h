//
// FreeDB.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef FREEDB_H_
#define FREEDB_H_

#include <extras/cdmetadata/DiskInfo.h>

//! Assigns a root path where the freedb "db.tab" and "db.dat" files can be found.
//! The default path is "a:/iofreedb".
void FreeDBSetRootPath(const char* szDBRootPath);

//! Populates the list of disks that match the given ID in freedb.
bool FreeDBGetDiskList(unsigned int iDiskID, DiskInfoVector& svDisks);


class CFreeDBDiskInfo : public IDiskInfo
{
public:

	CFreeDBDiskInfo(unsigned int uiDiskID);
	~CFreeDBDiskInfo();

    unsigned int GetID() const { return m_uiDiskID; }

private:

    unsigned int m_uiDiskID;        //!< freedb ID of this disk.
};

#endif // FREEDB_H_
