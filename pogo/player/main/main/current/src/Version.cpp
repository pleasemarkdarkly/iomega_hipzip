#include <main/main/Version.h>

CVersion* CVersion::s_pInstance = 0;

CVersion* CVersion::GetInstance()
{
    if (!s_pInstance)
        s_pInstance = new CVersion;
    return s_pInstance;
}

void CVersion::Destroy()
{
    delete s_pInstance;
    s_pInstance = 0;
}

// player version
int CVersion::PlayerMajor()
{
    return 1;
}

int CVersion::PlayerMinor()
{
    return 2;
}

// player build
int CVersion::PlayerBuild()
{
    return 3;
}

// bootloader version
int CVersion::BootloaderMajor()
{
    return 4;
}

int CVersion::BootloaderMinor()
{
    return 5;
}

// copyright year
int CVersion::CopyrightYear()
{
    return 2002;
}

CVersion::CVersion()
{
}

CVersion::~CVersion()
{
}

