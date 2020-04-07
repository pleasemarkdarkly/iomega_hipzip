#include <main/main/AppSettings.h>     // warn_mbytes and low_mbytes
#include <main/main/SpaceMgr.h>
#include <fs/fat/sdapi.h>

#include <util/debug/debug.h>          // debugging hooks
DEBUG_MODULE_S( DBG_SPACE_MGR, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_SPACE_MGR );

#define NULL 0
#define HD_DRIVE_NUM 0

#define WARN_THRESH_BYTES ((long long)SPACEMGR_WARN_MBYTES * DJ_KILOBYTES_PER_MEGABYTE * DJ_BYTES_PER_KILOBYTE)
#define LOW_THRESH_BYTES ((long long)SPACEMGR_LOW_MBYTES * DJ_KILOBYTES_PER_MEGABYTE * DJ_BYTES_PER_KILOBYTE)

CSpaceMgr* CSpaceMgr::m_pInstance = NULL;

CSpaceMgr::CSpaceMgr() : m_nBytesReserved(0)
{}

CSpaceMgr::~CSpaceMgr()
{}

CSpaceMgr* CSpaceMgr::GetInstance()
{
    if (!m_pInstance)
        m_pInstance = new CSpaceMgr;
    return m_pInstance;
}

void CSpaceMgr::Destroy()
{
    delete m_pInstance;
    m_pInstance = NULL;
}

inline long long HDBytesFree()
{
    return pc_free64(HD_DRIVE_NUM);
}

// bytes till warning status
long long CSpaceMgr::BytesFromWarning()
{
    long long free = HDBytesFree();
    if (free > WARN_THRESH_BYTES)
    {
        DEBUGP( DBG_SPACE_MGR , DBGLEV_INFO, "sm:KBytesFromWarn %d\n",(free - WARN_THRESH_BYTES)/DJ_BYTES_PER_KILOBYTE); 
        return free - WARN_THRESH_BYTES;
    }
    DEBUGP( DBG_SPACE_MGR , DBGLEV_INFO, "sm:BytesFromWarn 0\n"); 
    return 0;
}

// bytes from low status
long long CSpaceMgr::BytesFromLow()
{
    long long free = HDBytesFree();
    if (free > LOW_THRESH_BYTES)
    {
        DEBUGP( DBG_SPACE_MGR , DBGLEV_INFO, "sm:KBytesFromLow %d\n",(free - LOW_THRESH_BYTES)/DJ_BYTES_PER_KILOBYTE); 
        return free - LOW_THRESH_BYTES;
    }
    DEBUGP( DBG_SPACE_MGR , DBGLEV_INFO, "sm:BytesFromLow 0\n"); 
    return 0;
}

// current status
tSpaceStatus CSpaceMgr::Status()
{
    long long free = HDBytesFree();
    if (free > WARN_THRESH_BYTES)
    {
        DEBUGP( DBG_SPACE_MGR , DBGLEV_TRACE, "sm:StatusOk\n"); 
        return SPACE_OK;
    }
    if (free > LOW_THRESH_BYTES)
    {
        DEBUGP( DBG_SPACE_MGR , DBGLEV_INFO, "sm:StatusWarn\n"); 
        return SPACE_WARN;
    }
    DEBUGP( DBG_SPACE_MGR , DBGLEV_INFO, "sm:StatusLow\n"); 
    return SPACE_LOW;
}
