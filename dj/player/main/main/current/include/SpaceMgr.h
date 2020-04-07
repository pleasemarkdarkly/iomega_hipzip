#ifndef __SPACEMGR_H__
#define __SPACEMGR_H__

typedef enum tSpaceStatus { SPACE_OK, SPACE_WARN, SPACE_LOW };

class CSpaceMgr {
public:
    static CSpaceMgr* GetInstance();
    static void Destroy();
    // bytes till warning status
    long long BytesFromWarning();
    // bytes from low status
    long long BytesFromLow();
    // current status
    tSpaceStatus Status();
private:
    CSpaceMgr();
    ~CSpaceMgr();
    static CSpaceMgr* m_pInstance;
    int m_nBytesReserved;
};

#endif // __SPACEMGR_H__
