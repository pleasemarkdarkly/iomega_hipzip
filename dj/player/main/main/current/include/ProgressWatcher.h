#ifndef __PROGRESS_WATCHER_H_
#define __PROGRESS_WATCHER_H_

// different critical tasks that may fail
typedef enum eCurrentTask {
    TASK_GENERAL                        = 0x000,
    TASK_LOADING_METAKIT                = 0x001,
    TASK_COMMITTING_METAKIT             = 0x002,
    TASK_RESTORING_PLAYLIST             = 0x004,
    TASK_REFRESHING_CONTENT             = 0x008,
    TASK_LOADING_SETTINGS               = 0x010,
    TASK_LOADING_CD_METADATA_CACHE      = 0x020,
    TASK_COMMITTING_CD_METADATA_CACHE   = 0x040,
    TASK_LOADING_CD_DIR_CACHE           = 0x080,
    TASK_REBUILDING_CD_DIR_CACHE        = 0x100,
    TASK_CD_DIR_CACHE                   = 0x200,
    TASK_CONTENT_UPDATE                 = 0x400,
};

// steps taken to restore working order
typedef enum eErrorResponse {
    ER_NO_RESPONSE,
    ER_CLEAR_PLAYLIST,
    ER_DELETE_CD_METADATA_CACHE,
    ER_REBUILD_CD_DIR_CACHE,
    ER_DELETE_CD_DIR_CACHE,
    ER_DELETE_SETTINGS,
    ER_REFRESH_CONTENT,
    ER_SCANDISK,
    ER_REFORMAT_HDD,
    ER_NEW_HDD
};

// maintain a sense of what is going on in the player, to allow targeted recovery.
class CProgressWatcher {
public:
    // singleton access
    static CProgressWatcher* GetInstance();
    static void Destroy();
    // set/unset the active task, and save it to disk.
    void SetTask(eCurrentTask);
    void UnsetTask(eCurrentTask);
    // clear all tasks
    void ClearTasks();
    // what was the player doing exactly?
    eCurrentTask GetLastTask();
    // is/was the player trying to boot?
    void SetBooting(bool bBooting);
    bool WasBooting();
    // how many times has the player failed to boot?
    void SetBootFails(int nFailures);
    int GetBootFails();
    // is/was the player actively recording
    void SetRecording(bool bRecording);
    bool WasRecording();
    // serialization
    void Save();
    void Load();

    // see what was last in progress, if the last boot failed, and set response flags accordingly.
    eErrorResponse AnalyzeShutdown();

private:
    // serialization layout
    struct tPWState {
        enum eCurrentTask eTask;
        int nBootFails;
        bool bBooting;
        bool bRecording;
    } m_tState;
    // private instantiation
    CProgressWatcher();
    ~CProgressWatcher();
    static CProgressWatcher* s_pInstance;
};

#endif // __PROGRESS_WATCHER_H_
