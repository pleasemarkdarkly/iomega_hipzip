#include <main/main/ProgressWatcher.h>
#include <main/main/AppSettings.h>
#include <datastream/fatfile/FileInputStream.h>
#include <datastream/fatfile/FileOutputStream.h>
#include <fs/fat/sdapi.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S(DBG_PROG_WATCH, DBGLEV_DEFAULT | DBGLEV_INFO | DBGLEV_TRACE );
DEBUG_USE_MODULE(DBG_PROG_WATCH);  // debugging prefix : (10) pw

// boot failure response thresholds
#define BF_CONTENT_REFRESH_THRESH 3
#define BF_SCANDISK_THRESH 4
#define BF_REFORMAT_THRESH 5
#define BF_NEW_HDD_THRESH 6

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
    DEBUGP( DBG_PROG_WATCH, DBGLEV_TRACE, "pw:SetBooting: %s\n", bBooting ? "true" : "false");
    m_tState.bBooting = bBooting;
}

bool CProgressWatcher::WasBooting()
{
    return m_tState.bBooting;
}

// is/was the player actively recording?
void CProgressWatcher::SetRecording(bool bRecording)
{
    DEBUGP( DBG_PROG_WATCH, DBGLEV_TRACE, "pw:SetRecording: %s\n", bRecording ? "true" : "false");
    m_tState.bRecording = bRecording;
}

bool CProgressWatcher::WasRecording()
{
    return m_tState.bRecording;
}

// how many times has the player failed to boot?
void CProgressWatcher::SetBootFails(int nFailures)
{
    DEBUGP( DBG_PROG_WATCH, DBGLEV_TRACE, "pw:SetBootFails: %d\n", nFailures);
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
    //    pc_unlink(SYSTEM_PROGRESS_PATH);
    if (SUCCEEDED(ffos.Open(SYSTEM_PROGRESS_PATH)))
    {
        ffos.Ioctl(FATFILE_OUTPUT_IOCTL_TRUNCATE, 0);
        // probably not needed due to truncate
        ffos.Seek(IOutputStream::SeekStart,0);
        ffos.Write((void*)&m_tState,sizeof(tPWState));
        ffos.Close();
    } else {
        // if we can't open it, try to delete it
        pc_unlink(SYSTEM_PROGRESS_PATH);
    }
}

static void PrintTasks(eCurrentTask task)
{
    if (!task)
        DEBUGP( DBG_PROG_WATCH, DBGLEV_CHATTER, "pw:TASK_GENERAL\n");
    else
    {
        if (task & TASK_LOADING_METAKIT)
            DEBUGP( DBG_PROG_WATCH, DBGLEV_CHATTER, "pw:TASK_LOADING_METAKIT\n");
        if (task & TASK_COMMITTING_METAKIT)
            DEBUGP( DBG_PROG_WATCH, DBGLEV_CHATTER, "pw:TASK_COMMITTING_METAKIT\n");
        if (task & TASK_RESTORING_PLAYLIST)
            DEBUGP( DBG_PROG_WATCH, DBGLEV_CHATTER, "pw:TASK_RESTORING_PLAYLIST\n");
        if (task & TASK_REFRESHING_CONTENT)
            DEBUGP( DBG_PROG_WATCH, DBGLEV_CHATTER, "pw:TASK_REFRESHING_CONTENT\n");
        if (task & TASK_LOADING_SETTINGS)
            DEBUGP( DBG_PROG_WATCH, DBGLEV_CHATTER, "pw:TASK_LOADING_SETTINGS\n");
        if (task & TASK_LOADING_CD_METADATA_CACHE)
            DEBUGP( DBG_PROG_WATCH, DBGLEV_CHATTER, "pw:TASK_LOADING_CD_METADATA_CACHE\n");
        if (task & TASK_COMMITTING_CD_METADATA_CACHE)
            DEBUGP( DBG_PROG_WATCH, DBGLEV_CHATTER, "pw:TASK_COMMITTING_CD_METADATA_CACHE\n");
        if (task & TASK_LOADING_CD_DIR_CACHE)
            DEBUGP( DBG_PROG_WATCH, DBGLEV_CHATTER, "pw:TASK_LOADING_CD_DIR_CACHE\n");
        if (task & TASK_REBUILDING_CD_DIR_CACHE)
            DEBUGP( DBG_PROG_WATCH, DBGLEV_CHATTER, "pw:TASK_REBUILDING_CD_DIR_CACHE\n");
        if (task & TASK_CD_DIR_CACHE)
            DEBUGP( DBG_PROG_WATCH, DBGLEV_CHATTER, "pw:TASK_CD_DIR_CACHE\n");
        if (task & TASK_CONTENT_UPDATE)
            DEBUGP( DBG_PROG_WATCH, DBGLEV_CHATTER, "pw:TASK_CONTENT_UPDATE\n");
    }
}

void CProgressWatcher::SetTask(eCurrentTask task)
{
    eCurrentTask eNewTask = (eCurrentTask)(m_tState.eTask | task);
    if (eNewTask != m_tState.eTask)
    {
        m_tState.eTask = eNewTask;
        // this preprocesses out to an if( 0 ) if debugging is disabled
        if ( DEBUG_TEST( DBG_PROG_WATCH, DBGLEV_CHATTER ) )
        {
            DEBUGP( DBG_PROG_WATCH, DBGLEV_CHATTER, "pw:CProgressWatcher::SetTask:\n");
            PrintTasks(task);
            DEBUGP( DBG_PROG_WATCH, DBGLEV_CHATTER, "pw:Tasks:\n");
            PrintTasks(m_tState.eTask);
        }
        Save();
    }
}

void CProgressWatcher::UnsetTask(eCurrentTask task)
{
    eCurrentTask eNewTask = (eCurrentTask)(m_tState.eTask & ~task);
    if (eNewTask != m_tState.eTask)
    {
        m_tState.eTask = eNewTask;
        // this preprocesses out to an if( 0 ) if debugging is disabled
        if ( DEBUG_TEST( DBG_PROG_WATCH, DBGLEV_CHATTER ) )
        {
            DEBUGP( DBG_PROG_WATCH, DBGLEV_CHATTER, "pw:CProgressWatcher::UnsetTask:\n");
            PrintTasks(task);
            DEBUGP( DBG_PROG_WATCH, DBGLEV_CHATTER, "pw:Tasks:\n");
            PrintTasks(m_tState.eTask);
        }
        Save();
    }
}

// clear all tasks
void CProgressWatcher::ClearTasks()
{
    if (m_tState.eTask != TASK_GENERAL)
    {
        m_tState.eTask = TASK_GENERAL;
        Save();
    }
}

void CProgressWatcher::Load()
{
    DEBUGP( DBG_PROG_WATCH, DBGLEV_INFO, "pw:loading\n"); 
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

// load from file, run integrity checks, respond to any perceived problems
eErrorResponse CProgressWatcher::AnalyzeShutdown()
{
    DEBUGP( DBG_PROG_WATCH, DBGLEV_INFO, "pw:AnalyzeShutdown\n");      
    enum eCurrentTask eTask = GetLastTask();
    eErrorResponse erReturn = ER_NO_RESPONSE;
    int nBootFails = GetBootFails();
    // track boot failure count
    if (WasBooting())
    {
        DEBUGP( DBG_PROG_WATCH, DBGLEV_TRACE, "pw:was booting\n"); 
        ++nBootFails;
        DEBUGP( DBG_PROG_WATCH, DBGLEV_TRACE, "pw:%d prev failures\n",nBootFails); 
        SetBootFails(nBootFails);
    }

    // handle repeated boot failures with escalating responses.
    if (nBootFails > 2)
    {
        DEBUGP( DBG_PROG_WATCH, DBGLEV_TRACE, "pw:%d boot consecutive failures, taking evasive action\n"); 
        switch (nBootFails)
        {
            case BF_CONTENT_REFRESH_THRESH:
                erReturn = ER_REFRESH_CONTENT;
                DEBUGP( DBG_PROG_WATCH, DBGLEV_TRACE, "pw:bootfail: refresh content\n"); 
                break;
            case BF_SCANDISK_THRESH:
                erReturn = ER_SCANDISK;
                DEBUGP( DBG_PROG_WATCH, DBGLEV_TRACE, "pw:bootfail: scan disk\n"); 
                break;
            case BF_REFORMAT_THRESH:
                erReturn = ER_REFORMAT_HDD;
                DEBUGP( DBG_PROG_WATCH, DBGLEV_TRACE, "pw:bootfail: reformant hdd\n"); 
                break;
            case BF_NEW_HDD_THRESH:
            default:
                erReturn = ER_NEW_HDD;
                DEBUGP( DBG_PROG_WATCH, DBGLEV_TRACE, "pw:bootfail: buy new hdd\n"); 
                break;
        }
    }
    // or take targeted action based on the (low-count) failure type
    else
    {
        if (eTask & TASK_LOADING_METAKIT)
        {
            DEBUGP( DBG_PROG_WATCH, DBGLEV_TRACE, "pw:was loading mk\n"); 
            if (nBootFails > 1)
            {
                DEBUGP( DBG_PROG_WATCH, DBGLEV_TRACE, "pw:more than one failure, was loading mk, chkdsk.\n"); 
                erReturn = ER_REFRESH_CONTENT;
            }
            else
            {
                DEBUGP( DBG_PROG_WATCH, DBGLEV_TRACE, "pw:only one failure, was loading mk, let slide\n"); 
                erReturn = ER_NO_RESPONSE;
            }
        }
        else if (eTask & TASK_COMMITTING_METAKIT)
        {
            DEBUGP( DBG_PROG_WATCH, DBGLEV_TRACE, "pw:was saving mk, chkdsk\n"); 
            erReturn = ER_REFRESH_CONTENT;
        }
        else if (eTask & TASK_RESTORING_PLAYLIST)
        {
            DEBUGP( DBG_PROG_WATCH, DBGLEV_TRACE, "pw:failed to restore playlist\n"); 
            erReturn = ER_CLEAR_PLAYLIST;
        }
        else if (eTask & TASK_CONTENT_UPDATE)
        {
            DEBUGP( DBG_PROG_WATCH, DBGLEV_TRACE, "pw:content was added but not saved, rescan.\n"); 
            erReturn = ER_REFRESH_CONTENT;
        }
        else if (eTask & TASK_REFRESHING_CONTENT)
        {
            DEBUGP( DBG_PROG_WATCH, DBGLEV_TRACE, "pw:was rescanning content, chkdsk.\n"); 
            erReturn = ER_SCANDISK;
        }
        else if (eTask & TASK_LOADING_SETTINGS)
        {
            DEBUGP( DBG_PROG_WATCH, DBGLEV_TRACE, "pw:was loading settings, nuke\n"); 
            erReturn = ER_DELETE_SETTINGS;
        }
        else if (eTask & TASK_LOADING_CD_METADATA_CACHE)
        {
            DEBUGP( DBG_PROG_WATCH, DBGLEV_TRACE, "pw:was loading cd metadata cache, nuke\n"); 
            erReturn = ER_DELETE_CD_METADATA_CACHE;
        }
        else if (eTask & TASK_COMMITTING_CD_METADATA_CACHE)
        {
            DEBUGP( DBG_PROG_WATCH, DBGLEV_TRACE, "pw:was committing cd metadata cache, nuke\n"); 
            erReturn = ER_DELETE_CD_METADATA_CACHE;
        }
        else if (eTask & TASK_LOADING_CD_DIR_CACHE)
        {
            DEBUGP( DBG_PROG_WATCH, DBGLEV_TRACE, "pw:was loading/committing cd dir cache, nuke and rebuild\n");
            erReturn = ER_REBUILD_CD_DIR_CACHE;
        }
        else if (eTask & TASK_REBUILDING_CD_DIR_CACHE)
        {
            DEBUGP( DBG_PROG_WATCH, DBGLEV_TRACE, "pw:was rebuilding cd dir cache, nuke and chkdsk\n");
            erReturn = ER_DELETE_CD_DIR_CACHE;
        }
        else if (!eTask)
        {
            // nothing in progress, and the boot count didn't threshold for extreme action, so just boot normally.
            DEBUGP( DBG_PROG_WATCH, DBGLEV_TRACE, "pw:nothing in progress, low failure count, ok.\n");
        }
        else
        {
            DEBUGP( DBG_PROG_WATCH, DBGLEV_WARNING, "pw:task %d unrecognized\n",(int)eTask); 
        }
    }
    return erReturn;
}
