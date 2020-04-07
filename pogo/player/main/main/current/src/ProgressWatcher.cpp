#include <main/main/ProgressWatcher.h>
#include <main/main/AppSettings.h>
#include <datastream/fatfile/FileInputStream.h>
#include <datastream/fatfile/FileOutputStream.h>
#include <fs/fat/sdapi.h>

#define NULL 0
CProgressWatcher* CProgressWatcher::s_pInstance = NULL;

CProgressWatcher* CProgressWatcher::GetInstance()
{
    if (!s_pInstance)
        s_pInstance = new CProgressWatcher;
    return s_pInstance;
}

void CProgressWatcher::Destroy()
{
    delete s_pInstance;
    s_pInstance = NULL;
}

void CProgressWatcher::SetCurrentTask(eCurrentTask task)
{
    m_tState.eTask = task;
}

eCurrentTask CProgressWatcher::GetLastTask()
{
    return m_tState.eTask;
}

CProgressWatcher::CProgressWatcher()
{

}

CProgressWatcher::~CProgressWatcher()
{

}

// is/was the player trying to boot?
void CProgressWatcher::SetBooting(bool bBooting)
{
    m_tState.bBooting = bBooting;
}

bool CProgressWatcher::WasBooting()
{
    return m_tState.bBooting;
}

// how many times has the player failed to boot?
void CProgressWatcher::SetBootFails(int nFailures)
{
    m_tState.nBootFails = nFailures;
}

int CProgressWatcher::GetBootFails()
{
    return m_tState.nBootFails;
}

// serialization
void CProgressWatcher::Save()
{
    CFatFileOutputStream ffos;
    pc_unlink(SYSTEM_PROGRESS_PATH);
    if (SUCCEEDED(ffos.Open(SYSTEM_PROGRESS_PATH)))
    {
        ffos.Seek(IOutputStream::SeekStart,0);
        ffos.Write((void*)&m_tState,sizeof(tPWState));
        ffos.Close();
    }
}

void CProgressWatcher::Load()
{
    CFatFileInputStream ffis;
    if (SUCCEEDED(ffis.Open(SYSTEM_PROGRESS_PATH)))
    {
        ffis.Seek(IInputStream::SeekStart,0);
        ffis.Read((void*)&m_tState,sizeof(tPWState));
        ffis.Close();
    }
    else
    {
        // if there wasn't a previous state, just init to all-ok.
        m_tState.bBooting = false;
        m_tState.eTask = TASK_GENERAL;
        m_tState.nBootFails = 0;
    }
}