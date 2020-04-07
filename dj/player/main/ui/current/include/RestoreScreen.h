// IPAddressString.h: This class is derived from the CScreen class, and
//  contains the various windows that make up the main play screen display
// danb@iobjects.com 09/04/2001
// (c) Interactive Objects

#ifndef RESTORESCREEN_H_
#define RESTORESCREEN_H_

//#include <gui/peg/peg.hpp>
#include <main/ui/Screen.h>
#include <main/ui/SystemMessageString.h>

enum eRestoreOption {
    EVERYTHING = 0,
    FORMAT_HD,
    CHECK_DISK,
    UPDATE_CDDB, 
    UPDATE_SOFTWARE,
    DELETE_CONTENT,
    RESET_CDDB,
    CLEAR_CD_CACHE,
    CLEAR_LOCAL_CONTENT_DATABASE,
    CLEAR_SAVED_SETTINGS
};

class CRestoreScreen : public CScreen
{
public:
    static CRestoreScreen* GetInstance();
    static void Destroy() {
        if (s_pRestoreScreen)
            delete s_pRestoreScreen;
        s_pRestoreScreen = 0;
    }
    
    SIGNED Message(const PegMessage &Mesg);
    
    void Draw();
    void ForceRedraw();

    void SetTitleText(const TCHAR* szText);
    void SetActionText(const TCHAR* szText);
    
    // timeout length is in peg ticks.  no value, or zero, means there's no timeout, and the message stays until it's changed.
    void SetMessageText(const char* szText, CSystemMessageString::SysMsgType iMessageType = CSystemMessageString::INFO);
    void SetMessageText(const TCHAR* szText, CSystemMessageString::SysMsgType iMessageType = CSystemMessageString::INFO);
    void ResetProgressBar(int iProgress = 0, int iTotal = 0);
    void UpdateProgressBar(int iProgress = 0);

    bool SetRestoreOption(eRestoreOption eOption, bool bEnable);
    bool GetRestoreOption(eRestoreOption eOption);

    void DoSelectedOptions();

    bool DoCDCheck(CScreen* pScreen, const TCHAR* TitleText);
    void NotifyMediaInserted();

    void DoChkdsk();
    void DoReset(const TCHAR* MessageText = 0);

    void DoRestoreCDDB();

private:
    
    CRestoreScreen(CScreen* pParent);
    virtual ~CRestoreScreen();
    
    static void ProgressCallback(int iCurrent, int iTotal);
    void ProgressCB(int iCurrent, int iTotal);

    static void FileCopyCallback(const char* szFilename, int iCurrent, int iTotal);
    void FileCopyCB(const char* szFilename, int iCurrent, int iTotal);
    
    static bool ChkdskStatusCallback(int iPass, int iPhase, int iCurrent, int iTotal);
	bool ChkdskStatusCB(int iPass, int iPhase, int iCurrent, int iTotal);	

    void DoFormat();
    void DoRestore();
    void DoDeleteLocalContent();
    void DoClearLocalContentDatabase();
    void DoClearCDCache();
    void DoClearSavedSettings();
    void DoResetCDDB();
    void DoSummary();
    bool IsRestoreSuccessful();
    
    void BuildScreen();

    // Called in Draw() and used to calculate the drawing of the progress bar
    void DrawProgressBar();
    
    PegString *m_pScreenTitle;
    PegString *m_pActionTextString;
    
    CSystemMessageString *m_pMessageTextString;
    
    PegIcon *m_pTopScreenHorizontalDottedBarIcon;
    PegIcon *m_pScreenHorizontalDottedBarIcon;
    
    PegRect m_ProgressBarRect;
    
    int     m_iProgressBarTotal;

    bool    m_bFormat, m_bFormatStatus;
    bool    m_bCheckDisk, m_bCheckDiskStatus;
    bool    m_bUpdateCDDB, m_bUpdateCDDBStatus;
    bool    m_bUpdateSoftware, m_bUpdateSoftwareStatus;
    bool    m_bDeleteLocalContent, m_bDeleteLocalContentStatus;
    bool    m_bClearCDCache, m_bClearCDCacheStatus;
    bool    m_bClearLocalContentDatabase, m_bClearLocalContentDatabaseStatus;
    bool    m_bClearSavedSettings, m_bClearSavedSettingsStatus;
    bool    m_bResetCDDB, m_bResetCDDBStatus;
    bool    m_bRestoreCDDB;

    static CRestoreScreen* s_pRestoreScreen;

    int m_iTryCount;
};

#endif  // RESTORESCREEN_H_

