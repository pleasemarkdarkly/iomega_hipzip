#ifndef XMLDOCS_H_
#define XMLDOCS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define DJ_SERVICE_SERVCOUNT	1
#define DJ_SERVICE_1			0
#define DJ_SERVICE_2			1
#define DJ_SERVICE_3			2



#define DJ_SERVICE_1_VARCOUNT	1
#define TV_CONTROL_POWER	2
#define TV_CONTROL_CHANNEL	1
//#define DJ_VOLUME	0
#define DJ_VOLUME	0

#define DJ_SERVICE_2_VARCOUNT	0
#define AV_TRANSPORT_TITLE	0
#define AV_TRANSPORT_ARTIST	1

#define DJ_SERVICE_3_VARCOUNT	0


#define TV_MAX_VAL_LEN 128

/* This should be the maximum VARCOUNT from above */
#define DJ_MAXVARS	DJ_SERVICE_1_VARCOUNT

void BuildFakeFileTable(const char* szIPAddress, int iPort);
char* FakeGetFile(const char* szFilename);
int FakeGetFileLength(const char* szFilename);
int FakeDriveFileGetRequestIndex(const char* szFilename);

extern char DJDeviceType[];
extern char *DJServiceType[];
extern char *DJServiceName[];



#ifdef __cplusplus
}
#endif

#endif	// XMLDOCS_H_
