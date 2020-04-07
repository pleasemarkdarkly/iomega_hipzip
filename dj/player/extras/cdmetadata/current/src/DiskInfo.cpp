//
// DiskInfo.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <extras/cdmetadata/DiskInfo.h>

#include <content/common/ContentManager.h>
#include <datasource/cddatasource/CDDataSource.h>
#include <extras/cdmetadata/CDMetadataEvents.h>
#include <util/debug/debug.h>
#include <util/utils/utils.h>

// This is kinda ugly.  In theory this should be separate from the main app, but
// when things get down to the wire it's better to sacrifice theory for functionality.
#include <main/main/AppSettings.h>
extern void SetMainThreadPriority(int nPrio);
extern int GetMainThreadPriority();

#include <stdio.h>

DEBUG_MODULE_S(DISKINFO, DBGLEV_DEFAULT | DBGLEV_INFO);
DEBUG_USE_MODULE(DISKINFO);

// Converts a source string from UTF-8 to Latin-1 encoding.
// All charaters that can't be converted to Latin-1 will be replaced by '*'.
static TCHAR*
UTF8ToLatin1(const char* szSource)
{
    int i = 0, j = 0;
    TCHAR* szDest = new TCHAR[strlen(szSource) + 1];
    while (szSource[i])
    {
        // If the high bit is 1, then this is UTF-8.
        if (szSource[i] & 0x80)
        {
            // Check the range.  Latin-1 has a high byte of 1100001x.
            if (szSource[i + 1] && (szSource[i] == 0xC2) || (szSource[i] == 0xC3))
            {
                // UTF-8: 110000xx 10yyyyyy Latin-1: xxyyyyyy
                szDest[j++] = ((szSource[i] & 0x03) << 6) + (szSource[i + 1] & 0x3F);
                i += 2;
            }
            else
            {
                // This character isn't in the Latin-1 range, so skip all UTF-8 bytes.
                while (szSource[++i] & 0x80)
                    ;
                szDest[j++] = '*';
            }
        }
        else
            szDest[j++] = szSource[i++];
    }
    szDest[j] = '\0';
    return szDest;
}

CTrackInfo::CTrackInfo() :
	m_ulFrameOffset(0),
	m_szTrackName(NULL)
{
}

CTrackInfo::~CTrackInfo()
{
	delete [] m_szTrackName;
}

void
CTrackInfo::SetTrackName(const char* szTrackName)
{
    delete [] m_szTrackName;
    m_szTrackName = UTF8ToLatin1(szTrackName);
}


IDiskInfo::IDiskInfo() :
	m_szTitle(NULL),
	m_szArtist(NULL),
	m_szGenre(NULL)
{
}

IDiskInfo::~IDiskInfo()
{
	delete [] m_szTitle;
	delete [] m_szArtist;
    delete [] m_szGenre;
    while (!m_svTracks.IsEmpty())
        delete m_svTracks.PopBack();
}

void
IDiskInfo::SetTitle(const char* szTitle)
{
    delete [] m_szTitle;
    m_szTitle = UTF8ToLatin1(szTitle);
}

void
IDiskInfo::SetArtist(const char* szArtist)
{
    delete [] m_szArtist;
    m_szArtist = UTF8ToLatin1(szArtist);
}

void
IDiskInfo::SetGenre(const char* szGenre)
{
    delete [] m_szGenre;
    m_szGenre = UTF8ToLatin1(szGenre);
}

//! Prints the given disks to debug output.
void PrintDiskList(const DiskInfoVector& svDisks)
{
    for (int i = 0; i < svDisks.Size(); ++i)
	{
        DEBUGP(DISKINFO, DBGLEV_INFO, "Artist: %w\n", svDisks[i]->GetArtist());
        DEBUGP(DISKINFO, DBGLEV_INFO, "Title: %w\n", svDisks[i]->GetTitle());
        DEBUGP(DISKINFO, DBGLEV_INFO, "Genre: %w\n", svDisks[i]->GetGenre());
        for (int j = 0; j < svDisks[i]->GetTrackList().Size(); ++j)
            DEBUGP(DISKINFO, DBGLEV_INFO, "    Track %d: %w - offset: %d\n", j + 1, svDisks[i]->GetTrackList()[j]->GetTrackName(), svDisks[i]->GetTrackList()[j]->GetFrameOffset());
	}
}

//! Clears the list of disks and frees the associated memory.
void ClearDiskList(DiskInfoVector& svDisks)
{
    while (!svDisks.IsEmpty())
        delete svDisks.PopBack();
}

//! Uses the given disk info to iterate over the audio tracks in the CD data source and
//! assign metadata to them.
void SetCDRecordMetadata(IContentManager* pContentManager, CCDDataSource* pCDDS, IDiskInfo* pDisk)
{
    DBASSERT(DISKINFO, pContentManager, "Invalid content manager pointer\n");
    DBASSERT(DISKINFO, pCDDS, "Invalid CD data source pointer\n");
    DBASSERT(DISKINFO, pDisk, "Invalid disk info pointer\n");

    int priority = GetMainThreadPriority();
    SetMainThreadPriority(UI_THREAD_BUSY_PRIORITY);

    int iTrackCount = pDisk->GetTrackList().Size();
    for (int i = 0; i < iTrackCount; ++i)
        if (IMediaContentRecord* pRecord = pCDDS->GetAudioTrackRecord(pContentManager, i))
        {
            TCHAR tszScratch[256];
            IMetadata* pMetadata = pContentManager->CreateMetadataRecord();
            pMetadata->SetAttribute(MDA_TITLE, (void*)pDisk->GetTrackList()[i]->GetTrackName());
            pMetadata->SetAttribute(MDA_ARTIST, (void*)pDisk->GetArtist());
            pMetadata->SetAttribute(MDA_ALBUM, (void*)pDisk->GetTitle());
            pMetadata->SetAttribute(MDA_ALBUM_TRACK_NUMBER, (void*)(i + 1));
            pMetadata->SetAttribute(MDA_GENRE, (void*)pDisk->GetGenre());
            pRecord->MergeAttributes(pMetadata, true);
        }

    SetMainThreadPriority(priority);
}

//! Populates a IDiskInfo object with metadata:
//! Album title:    "CD <session #>"
//! Artist:         "Artist <session #>"
//! Genre:          "Genre <session #>"
//! Track x:        "Track <x>"
void PopulateNewDiskInfo(IDiskInfo* pDiskInfo, int iCDSessionIndex, int iTrackCount)
{
    char szScratch[64];
    sprintf(szScratch, "Unknown Album %d", iCDSessionIndex);
    pDiskInfo->SetTitle(szScratch);

    szScratch[0] = '\0';
    pDiskInfo->SetArtist(szScratch);
    pDiskInfo->SetGenre(szScratch);

    for (int i = 0; i < iTrackCount; ++i)
    {
        CTrackInfo* pTrackInfo = new CTrackInfo;
        sprintf(szScratch, "Track %d", i + 1);
        pTrackInfo->SetTrackName(szScratch);
        pDiskInfo->AddTrack(pTrackInfo);
    }
}
