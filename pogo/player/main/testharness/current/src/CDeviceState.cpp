//
// CDeviceState.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//


#include <stdlib.h>
#include <stdio.h>
#include <cyg/infra/diag.h>

#include <stdarg.h>


#include <main/testharness/testharness.h>
#include <core/playmanager/playmanager.h>

#define IDLE_TIME_DETECT 100*60*1	//1 minute

CDeviceState* CDeviceState::s_pSingleton = 0;


CDeviceState::CDeviceState()
{
	m_eDeviceState = CDeviceState::BUSY;
	m_LastBusyTime = cyg_current_time();
}

CDeviceState::~CDeviceState()
{
}


// Returns a pointer to the global debug router.
CDeviceState*
CDeviceState::GetInstance()
{
    if (!s_pSingleton)
        s_pSingleton = new CDeviceState;
    return s_pSingleton;
}

// Destroy the singleton debug router.
void
CDeviceState::Destroy()
{
    delete s_pSingleton;
    s_pSingleton = 0;
}

void CDeviceState::NotifyNotIdle()
{
	if (m_eDeviceState == CDeviceState::IDLE)
	{
		m_eDeviceState = CDeviceState::BUSY;
	}
	m_LastBusyTime = cyg_current_time();
}

void CDeviceState::SetDeviceState(EDeviceState eDeviceState)
{
	m_eDeviceState = eDeviceState; 
}

char * CDeviceState::GetDeviceState()
{
	if (m_eDeviceState == CDeviceState::BUSY)
	{
		unsigned int uCurrentTime = cyg_current_time();

		if ((uCurrentTime - m_LastBusyTime) > IDLE_TIME_DETECT)
		{

			CMediaPlayer::PlayState ps = CPlayManager::GetInstance()->GetPlayState();

			if ((ps == CMediaPlayer::STOPPED) || (ps == CMediaPlayer::NOT_CONFIGURED))
			{
				m_eDeviceState = CDeviceState::IDLE;
			}
		}
	}

	char * ret = "No idea";

	switch (m_eDeviceState)
	{
	case CDeviceState::BUSY:
		ret = "Interacting with user";
		break;
	case CDeviceState::IDLE:
		ret = "Idle";
		break;
	case CDeviceState::RUNNING_SCRIPT:
		ret = "Running Script";
		break;
	case CDeviceState::DOING_MONKEY:
		ret = "Running Monkey Test";
		break;
	}
	return ret;

}

