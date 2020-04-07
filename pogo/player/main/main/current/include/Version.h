#ifndef __POGO_VERSION_H
#define __POGO_VERSION_H

#define VERSION_NUM_SIZE 40

class CVersion {
public:
    static CVersion* GetInstance();
    static void Destroy();
    // player version
    int PlayerMajor();
    int PlayerMinor();
    // player build
    int PlayerBuild();
    // bootloader version
    int BootloaderMajor();
    int BootloaderMinor();
    // copyright year
    int CopyrightYear();
private:
    CVersion();
    ~CVersion();

    static CVersion* s_pInstance;
};

#endif //__POGO_VERSION_H