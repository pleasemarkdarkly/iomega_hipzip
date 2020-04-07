//
// FreeDB.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <content/freedb/FreeDB.h>
#include <devs/storage/ata/atadrv.h>
#include <fs/fat/sdapi.h>
#include <util/debug/debug.h>
#include <ctype.h>
#include <stdio.h>

DEBUG_MODULE_S(FREEDB, DBGLEV_DEFAULT | DBGLEV_INFO);
DEBUG_USE_MODULE(FREEDB);

static const char* sc_szDefaultDBRootPath = "a:/iofreedb";
static char* s_szDBRootPath = 0;


//! Assigns a root path where the freedb "db.tab" and "db.dat" files can be found.
//! The default path is "a:/iofreedb".
void FreeDBSetRootPath(const char* szDBRootPath)
{
    delete [] s_szDBRootPath;
    s_szDBRootPath = new char[strlen(s_szDBRootPath) + 1];
    strcpy(s_szDBRootPath, s_szDBRootPath);
}

unsigned long Reorder(char* Start)
{
	return ((unsigned char)*Start) | ((unsigned char)(*(Start + 1)) << 8) | ((unsigned char)(*(Start + 2)) << 16) | ((unsigned char)(*(Start + 3)) << 24);
}

struct DiskAddress
{
	unsigned long DiskID;
	unsigned long Offset;
};

long BinSearch(DiskAddress*, unsigned long, unsigned long, unsigned long);

void BuildDiskList(DiskInfoVector& svDisks, unsigned int uiDiskID, char* InfoBlock, unsigned long size)
{
    unsigned int Offset = 1;    // Skip the number of discs.
    unsigned char ReadLen;
    CFreeDBDiskInfo* pCurrent;
    while (Offset < size)
    {
        if(((unsigned char)(InfoBlock[Offset])) == 0xFF)
        {
            Offset++;
            pCurrent = new CFreeDBDiskInfo(uiDiskID);
            unsigned long DiskLength = Reorder(InfoBlock + Offset);
//            pCurrent->SetDiskLength(DiskLength);
            Offset+=4;
            ReadLen = InfoBlock[Offset++];
            char* szSeparator = strstr((InfoBlock + Offset), " / ");
            char* szDiskTitle;
            char* szArtist;
            if(szSeparator)
            {
                unsigned int StringLength = szSeparator - (InfoBlock + Offset);
                szArtist = new char[StringLength + 1];
                strncpy(szArtist, (InfoBlock + Offset), StringLength);
                szArtist[StringLength] = '\0';
                Offset+= (StringLength + 3);
                StringLength = ReadLen - (StringLength + 3);
                szDiskTitle = new char[StringLength + 1];
                strncpy(szDiskTitle, (InfoBlock + Offset), StringLength);
                szDiskTitle[StringLength] = '\0';
                Offset+= StringLength;
            }
            else
            {
                szArtist = new char[ReadLen + 1];
                strncpy(szArtist, (InfoBlock + Offset), ReadLen);
                szArtist[ReadLen] = '\0';
                szDiskTitle = new char[ReadLen + 1];
                strncpy(szDiskTitle, (InfoBlock + Offset), ReadLen);
                szDiskTitle[ReadLen] = '\0';
                Offset+= ReadLen;
            }
			ReadLen = InfoBlock[Offset++];
			char* szGenre = new char[ReadLen + 1];
			strncpy(szGenre, (InfoBlock + Offset), ReadLen);
			szGenre[ReadLen] = '\0';
			Offset+= ReadLen;
            pCurrent->SetArtist(szArtist);
            pCurrent->SetTitle(szDiskTitle);
			pCurrent->SetGenre(szGenre);
            svDisks.PushBack(pCurrent);
            delete [] szArtist;
            delete [] szDiskTitle;
            delete [] szGenre;
        }
        if(((unsigned char)InfoBlock[Offset]) == 0xFE)
        {
            Offset++;
            CTrackInfo* pCurrentTrack = new CTrackInfo;
            pCurrentTrack->SetFrameOffset(Reorder(InfoBlock + Offset));
            Offset+=4;
            ReadLen = InfoBlock[Offset++];
            char* szTrackTitle = new char[ReadLen + 1];
            strncpy(szTrackTitle, (InfoBlock + Offset), ReadLen);
            szTrackTitle[ReadLen] = '\0';
            pCurrentTrack->SetTrackName(szTrackTitle);
            delete [] szTrackTitle;
            Offset+= ReadLen;
            pCurrent->AddTrack(pCurrentTrack);
        }
    }
}


#define FAT_ERROR 65535


bool FreeDBGetDiskList(unsigned int uiDiskID, DiskInfoVector& svDisks)
{
    char szDBTableFilename[EMAXPATH];
    sprintf(szDBTableFilename, "%s/db.tab", s_szDBRootPath ? s_szDBRootPath : sc_szDefaultDBRootPath);

    PCFD fdTable = po_open((TEXT *)szDBTableFilename, PO_RDONLY, PS_IREAD);
    if (fdTable < 0)
    {
        DEBUG(FREEDB, DBGLEV_ERROR, "Unable to open freedb table file %s\n", szDBTableFilename);
	    return false;
    }

    unsigned long ulDataOffset = ((uiDiskID & 0xFF000000) >> 22);
    short err;
    po_lseek(fdTable, ulDataOffset, PSEEK_SET, &err);
    if (err != 0)
    {
        DEBUG(FREEDB, DBGLEV_ERROR, "Error seeking to %d in freedb table file %s\n", ulDataOffset, szDBTableFilename);
        po_close(fdTable);
	    return false;
    }

	unsigned long ulTableOffset;
    if (po_read(fdTable, (UCHAR*)&ulTableOffset, 4) == FAT_ERROR)
    {
        DEBUG(FREEDB, DBGLEV_ERROR, "Error reading from freedb table file %s\n", szDBTableFilename);
        po_close(fdTable);
	    return false;
    }

	unsigned long ulNextOffset;
	if (ulDataOffset < 0x3F8)
	{
        if (po_read(fdTable, (UCHAR*)&ulNextOffset, 4) == FAT_ERROR)
        {
            DEBUG(FREEDB, DBGLEV_ERROR, "Error reading from freedb table file %s\n", szDBTableFilename);
            po_close(fdTable);
	        return false;
        }
	}
	else
	{
        ulNextOffset = po_lseek(fdTable, 0, PSEEK_END, &err);
        if (err != 0)
        {
            DEBUG(FREEDB, DBGLEV_ERROR, "Error seeking to the end of freedb table file %s\n", ulDataOffset, szDBTableFilename);
            po_close(fdTable);
	        return false;
        }
	}
    po_lseek(fdTable, ulTableOffset, PSEEK_SET, &err);
    if (err != 0)
    {
        DEBUG(FREEDB, DBGLEV_ERROR, "Error seeking to %d in freedb table file %s\n", ulTableOffset, szDBTableFilename);
        po_close(fdTable);
	    return false;
    }

    unsigned long ulNumDisks = (ulNextOffset - ulTableOffset) >> 3;
	DiskAddress* pOffsetTree = new DiskAddress[ulNumDisks];

    if (po_read(fdTable, (UCHAR*)pOffsetTree, sizeof(DiskAddress) * ulNumDisks) == FAT_ERROR)
    {
        DEBUG(FREEDB, DBGLEV_ERROR, "Error reading from freedb table file %s\n", szDBTableFilename);
        po_close(fdTable);
	    return false;
    }
    po_close(fdTable);

	long lAddressIndex = BinSearch(pOffsetTree, uiDiskID, 0, (ulNumDisks - 1));
	if (lAddressIndex == -1)
	{
		delete [] pOffsetTree;
        DEBUGP(FREEDB, DBGLEV_INFO, "No disc found for ID %x\n", uiDiskID);
	}
	else
	{
		ulDataOffset = pOffsetTree[lAddressIndex].Offset;
		delete [] pOffsetTree;
        char szDBDataFilename[EMAXPATH];
        sprintf(szDBDataFilename, "%s/db.dat", s_szDBRootPath ? s_szDBRootPath : sc_szDefaultDBRootPath);
        PCFD fdData = po_open((TEXT *)szDBDataFilename, PO_RDONLY, PS_IREAD);
        if (fdData < 0)
        {
            DEBUG(FREEDB, DBGLEV_ERROR, "Unable to open freedb data file %s\n", szDBDataFilename);
	        return false;
        }

        po_lseek(fdData, ulDataOffset, PSEEK_SET, &err);
        if (err != 0)
        {
            DEBUG(FREEDB, DBGLEV_ERROR, "Error seeking to %d in freedb data file %s\n", ulDataOffset, szDBDataFilename);
            po_close(fdData);
	        return false;
        }

        if (po_read(fdData, (UCHAR*)&ulTableOffset, 4) == FAT_ERROR)
        {
            DEBUG(FREEDB, DBGLEV_ERROR, "Error reading from freedb data file %s\n", szDBDataFilename);
            po_close(fdData);
	        return false;
        }

		char* szInfoBlock = new char[ulTableOffset];
        if (po_read(fdData, (UCHAR*)szInfoBlock, ulTableOffset) == FAT_ERROR)
        {
            DEBUG(FREEDB, DBGLEV_ERROR, "Error reading from freedb data file %s\n", szDBDataFilename);
            po_close(fdData);
	        return false;
        }
        po_close(fdData);

        BuildDiskList(svDisks, uiDiskID, szInfoBlock, ulTableOffset);
        delete [] szInfoBlock;
	}

    return true;
}


long BinSearch(DiskAddress* AddressTable, unsigned long DiskID, unsigned long Min, unsigned long Max)
{
	if(AddressTable[Min].DiskID == DiskID)
		return Min;
	if(AddressTable[Max].DiskID == DiskID)
		return Max;
	if(Max < (Min+2))
		return -1;
	unsigned long Search = (Max - Min);
	if(Search)
		Search = Min + (Search/2);
	if(DiskID < AddressTable[Min].DiskID || DiskID > AddressTable[Max].DiskID)
		return -1;
	if(DiskID == AddressTable[Search].DiskID)
		return Search;
	if(DiskID < AddressTable[Search].DiskID)
		return BinSearch(AddressTable, DiskID, Min, Search);
	return BinSearch(AddressTable, DiskID, Search, Max);
}

CFreeDBDiskInfo::CFreeDBDiskInfo(unsigned int uiDiskID)
    : m_uiDiskID(uiDiskID)
{
}

CFreeDBDiskInfo::~CFreeDBDiskInfo()
{
}
