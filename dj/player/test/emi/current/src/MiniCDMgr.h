//
// MiniCDMgr.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef MINICDMGR_H_
#define MINICDMGR_H_

#include <io/storage/blk_dev.h>
#include <util/timer/Timer.h>

class CMiniCDMgr;

typedef struct media_record_info_s media_record_info_t;
typedef struct playlist_record_s playlist_record_t;
typedef struct cdda_toc_entry_s cdda_toc_entry_t;
typedef struct cdda_toc_s
{
    int entries;
    cdda_toc_entry_t    *entry_list;
} cdda_toc_t;


typedef struct cdda_toc_entry_s
{
    int audio_p;
    int channels;
    int copyok_p;
    int preemp_p;
    int track;
    long lba_startsector;
    long lba_length;
} cdda_toc_entry_t;

#define CD_DATA_SOURCE_CLASS_ID  1

//! Implements the IDataSource class for CD drives.
class CMiniCDMgr 
{
public:

    //! Attempts to open the drive with the given device name.
    //! Returns a pointer to a new CCDataSource object if successful, 0 otherwise.
    //! If bStartMediaStatusTimer is true, then the media status test thread will be
    //! started on creation.
    //! If false, then ResumeMediaStatusTimer must be called to start the check.
    
	CMiniCDMgr( cyg_io_handle_t hCD, const char* szDeviceName, 	const char* szMountDirectory, bool bStartMediaStatusTimer );

	~CMiniCDMgr();

	bool IsAudioCD();
	void ClearTOC();

	Cyg_ErrNo FetchTOC();

	Cyg_ErrNo DoPacket(unsigned char *command, unsigned char *data, int iDataLength,int dir);
	static CMiniCDMgr* GetInstance();

    //! Queries the media status to see if a disc has been removed or inserted.
    Cyg_ErrNo GetMediaStatus(bool bSendEvent);

    //! Pause/resume the media status test thread.
    void SuspendMediaStatusTimer();
    void ResumeMediaStatusTimer();

    //! Ejects the CD.
    //! Returns true if the CD was ejected, false otherwise.
    bool OpenTray();

    //! Closes the CD tray.
    //! Returns true if the CD tray was closed, false otherwise.
    bool CloseTray();

    //! Toggles the CD tray open/closed.
    void ToggleTray();


	//! Suspends and spins down drive
	bool SuspendDrive();

	//! wakes up drive for reads
	bool WakeupDrive();

    void TrayIsClosed()
        { m_bTrayOpen = false; }
    
    //! Returns true if the tray is open, false otherwise.
    bool IsTrayOpen() const
        { return m_bTrayOpen; }

private:

     cdda_toc_t      m_toc;      //!< Record that stores the CD's table of contents.

    // Obtains the media geometry and caches it locally
    int GetMediaGeometry();
    
    // Used for checking media status at regular intervals.
    timer_handle_t  m_TimerHandle;
    static void CheckMediaStatusCB(void* arg);
    bool            m_bNoMedia;
	bool m_bStartMediaStatusTimer;
    bool            m_bMediaBad;
	int				m_iLastUnitStatus;
   char*           m_szDeviceName;
    char*           m_szMountDirectory;
  cyg_io_handle_t	m_hCD;      //!< Handle to the CD drive.
    bool    m_bTrayOpen;        //!< True if the CD tray is open, false otherwise.
  
    drive_geometry_t m_dgGeometry;
    

};

#endif	// MINICDMGR_H_
