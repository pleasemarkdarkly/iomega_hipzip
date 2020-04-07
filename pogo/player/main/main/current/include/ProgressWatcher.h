#ifndef __PROGRESS_WATCHER_H_
#define __PROGRESS_WATCHER_H_

typedef enum eCurrentTask {
    TASK_GENERAL,
    TASK_LOADING_METAKIT,
    TASK_COMMITTING_METAKIT,
    TASK_LOADING_SETTINGS,
    TASK_REFRESHING_CONTENT
};

// maintain a sense of what is going on in the player, to allow targeted recovery.
class CProgressWatcher {
public:
    // different critical tasks that may fail
    // singleton access
    static CProgressWatcher* GetInstance();
    static void Destroy();
    // what is/was the player doing exactly?
    void SetCurrentTask(eCurrentTask);
    eCurrentTask GetLastTask();
    // is/was the player trying to boot?
    void SetBooting(bool bBooting);
    bool WasBooting();
    // how many times has the player failed to boot?
    void SetBootFails(int nFailures);
    int GetBootFails();
    // serialization
    void Save();
    void Load();
private:
    // serialization layout
    struct tPWState {
        enum eCurrentTask eTask;
        int nBootFails;
        bool bBooting;
    } m_tState;
    // private instantiation
    CProgressWatcher();
    ~CProgressWatcher();
    static CProgressWatcher* s_pInstance;
};

#endif // __PROGRESS_WATCHER_H_