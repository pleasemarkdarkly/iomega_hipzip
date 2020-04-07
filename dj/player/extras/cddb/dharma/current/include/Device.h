/*
 * Copyright (c) 2000, 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * Device.h -	Interface to CDevice class - used for reading TOCs.
 *				Adapted from MS sample code.
 */

#ifndef __cplusplus
#error C++ must be used for this header file.
#endif 

#ifndef	_DEVICE_H_
#define	_DEVICE_H_

#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

/*
 * Dependencies.
 */

#ifdef _WINDOWS
#include <mmsystem.h>
#endif
#ifdef macintosh
#include "CdMac.h"
#endif


/*
 * Constants.
 */

#define MAX_TRACKS 100


/*
 * Classes.
 */

/* Simple class to extract audio CD information */
class CDevice  
{
public:
	CDevice(long MciDeviceId = 0);
    CDevice(const char * driveSpec);
	virtual ~CDevice();

	MCIERROR	Close(long MciDeviceId = 0);
	long		DeviceId(void) { return m_lDeviceId; }
	MCIERROR	Open();
	void		Play(short nTrack);
	short		Read();

	short	GetNumberOfTracks(void)
			{
				return m_nNumberOfTracks;
			}
	void	SetNumberOfTracks(short nTracks)
			{
				m_nNumberOfTracks = nTracks;
			}
	long	GetLeadOut(void)
			{
				return m_nLeadOut;
			}
	long	GetTrackLength(short nTrack)
			{
				if (nTrack>0 && nTrack<=m_nNumberOfTracks)
					return m_nTrackLength[nTrack-1];
				else
					return 0;
			}
	long	GetTrackStart(short nTrack)
			{
				if (nTrack>0 && nTrack<=m_nNumberOfTracks)
					return m_nTrackStart[nTrack-1];
				else
					return 0;
			}
	void	SetTrackLength(short nTrack, long nNewLength)
			{
				if (nTrack>0 && nTrack<=m_nNumberOfTracks)
					m_nTrackLength[nTrack-1] = nNewLength;
			}
	long	GetTotalLength()
			{
				long	nTotalLength = 0;
				short	nTrack;
				for (nTrack=0; nTrack<m_nNumberOfTracks; nTrack++)
					nTotalLength = (nTotalLength + m_nTrackLength[nTrack]);
				return nTotalLength;
			}
	static inline long	MSF2Frames(long nMSFUnits)
			{
				return ((nMSFUnits & 0xff) * 60 * 75) + ((nMSFUnits & 0xff00) >> 8) * 75 + ((nMSFUnits & 0xff0000) >> 16);
			}

private:
	short				m_nNumberOfTracks;
	long				m_nStartFrame;
	unsigned long		m_nLeadOut;
	unsigned long		m_nTrackLength[MAX_TRACKS]; 
	unsigned long		m_nTrackStart[MAX_TRACKS];
	long				m_lDeviceId;
    char                m_driveSpec[4];     /* drive to open, as "x:" */
    
	MCI_STATUS_PARMS	m_MCIStatus;
	MCI_OPEN_PARMS		m_MCIOpen;
};

#endif /* _DEVICE_H_ */
