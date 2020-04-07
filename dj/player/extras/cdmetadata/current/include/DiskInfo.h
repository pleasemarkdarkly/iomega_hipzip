//
// DiskInfo.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef DISKINFO_H_
#define DISKINFO_H_

#include <util/datastructures/SimpleVector.h>
#include <util/tchar/tchar.h>

// fdecl
class CCDDataSource;
class IDiskInfo;
class IContentManager;
typedef SimpleVector<IDiskInfo*> DiskInfoVector;

//! Prints the given disks to debug output.
void PrintDiskList(const DiskInfoVector& svDisks);

//! Clears the list of disks and frees the associated memory.
void ClearDiskList(DiskInfoVector& svDisks);

//! Uses the given disk info to iterate over the audio tracks in the CD data source and
//! assign metadata to them.
void SetCDRecordMetadata(IContentManager* pContentManager, CCDDataSource* pCDDS, IDiskInfo* pDisk);

//! Populates a IDiskInfo object with metadata:
//! Album title:    "CD <session #>"
//! Artist:         "Artist <session #>"
//! Genre:          "Genre <session #>"
//! Track x:        "Track <x>"
void PopulateNewDiskInfo(IDiskInfo* pDiskInfo, int iCDSessionIndex, int iTrackCount);

class CTrackInfo
{
public:

	CTrackInfo();
	~CTrackInfo();

	void SetFrameOffset(unsigned long ulFrameOffset)
        { m_ulFrameOffset = ulFrameOffset; }
	unsigned long GetFrameOffset() const
        { return m_ulFrameOffset; }

	void SetTrackName(const char* szTrackName);
	const TCHAR* GetTrackName() const
        { return m_szTrackName; }

private:

	unsigned long m_ulFrameOffset;
	TCHAR* m_szTrackName;
};

typedef SimpleVector<CTrackInfo*> TrackInfoVector;

class IDiskInfo
{
public:

	IDiskInfo();
	virtual ~IDiskInfo() = 0;

	virtual void SetTitle(const char* szTitle);
	virtual const TCHAR* GetTitle() const
        { return m_szTitle; }

	virtual void SetArtist(const char* szArtist);
	virtual const TCHAR* GetArtist() const
        { return m_szArtist; }

	virtual void SetGenre(const char* szGenre);
	virtual const TCHAR* GetGenre() const
        { return m_szGenre; }

    virtual void AddTrack(CTrackInfo* pTrackInfo) { m_svTracks.PushBack(pTrackInfo); }
	virtual TrackInfoVector& GetTrackList() { return m_svTracks;}

protected:

	TCHAR*  m_szTitle;
	TCHAR*  m_szArtist;
	TCHAR*  m_szGenre;
    TrackInfoVector m_svTracks;     //!< List of tracks on this disk.
};


#endif // DISKINFO_H_
