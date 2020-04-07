//
// UpdateApp.h
//
// Copyright (c) 1998 - 2002 Fullplay

#ifndef UPDATEAPP_H_
#define UPDATEAPP_H_


#include <fs/fat/sdapi.h>  // for EMAXPATH


// available updates flags
#define AV_CDDB		1
#define AV_APP		2
#define AV_CDDBFULL 4

#if 0
// Used to hold a list of CDDB update files and
// their update levels found on the CD.
typedef struct cddb_update_info_s
{
	char    path[EMAXPATH];
    int     update_level;
} cddb_update_info_t;

class CUpdateApp;
#endif

class CUpdateApp
{
public:
    static CUpdateApp* GetInstance();

    static void Destroy();
    
	CUpdateApp();
	~CUpdateApp();

	// check the version of the firmware update
	int CheckFileVersion(const char* buff, int buffsize);

    // verify the specified buffer
	bool VerifyImage(const char* buff, unsigned long size);

    // update the firmware from the specified buffer
    bool UpdateImage(const char* buff, unsigned long size, void (*progress_cb)() =0);

	// delete updates on hard drive
	void DeleteUpdates();

private:
    static CUpdateApp* s_pInstance;

    void _xor( const char* src, char* dest, int len, unsigned int val );
    
	char m_szNewImagePath[EMAXPATH];
	unsigned long m_ulNewImageSize;

	// cd update specific information
	char m_szCDUpdatePath[EMAXPATH];
	int m_iCDUpdateVer; 
	bool m_bCDUpdateAvailable;

};

#endif	// UPDATEAPP_H_
