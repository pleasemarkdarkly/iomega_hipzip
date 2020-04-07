#include <main/upnpservices/XMLDocs.h>

#include <util/debug/debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cyg/infra/diag.h>
#include <io/net/Net.h>
#include <network.h>  //for bootp record

DEBUG_MODULE_S( DEVICESERVICES, DBGLEV_DEFAULT );
DEBUG_USE_MODULE( DEVICESERVICES );

extern const char* gc_szDJDeviceDescXMLBase;
char* g_szDJDeviceDescXML = 0;

extern const char* gc_szSCPD;
/*
extern const char* gc_szDJAVTransportSCPDXML;
extern const char* gc_szDJConnectorSCPDXML;
extern const char* gc_szDJDevicePresentationHTML;
*/

char DJDeviceType[] = "urn:www-fullplaymedia-com:device:darwinJukebox:1";
char *DJServiceType[] = {"urn:www-fullplaymedia-com:service:X_FullPlayDeviceServices:1"};
char *DJServiceName[] = {"DeviceServices"};

#define CHECK_ALLOC(x)


/* Global arrays for storing audio variable names, values, and defaults 
char *iops1_varname[] = {"Volume"};
char iops1_varval[DJ_SERVICE_1_VARCOUNT][TV_MAX_VAL_LEN];
char *iops1_varval_def[] = {"15"};

/* Global arrays for storing AV Transport Service variable names, values, and defaults 
char *iops2_varname[] = { "Title", "Artist" };
char iops2_varval[DJ_SERVICE_2_VARCOUNT][TV_MAX_VAL_LEN];
char *iops2_varval_def[] = { "<none>", "<nobody>" };

/* Global arrays for storing Connector Service variable names, values, and defaults 
char *iops3_varname[] = { "Title", "Artist" };
char iops3_varval[DJ_SERVICE_3_VARCOUNT][TV_MAX_VAL_LEN];
char *iops3_varval_def[] = { "<none>", "<nobody>" };
*/


typedef struct
{
	char*	szFile;
	int		iFileLength;
} FakeFileRec;

static FakeFileRec* s_Files;

static const char* sc_aryFilenames[] =
{
	"DJdesc.xml",
	"SCPD.xml"/*,
	"avtransportSCPD.xml",
	"connectorSCPD.xml",
	"tvdevicepres.html"*/
};


void
BuildFakeFileTable(const char* szIPAddress, int iPort)
{
	s_Files = (FakeFileRec*)malloc(sizeof(FakeFileRec) * (sizeof(sc_aryFilenames) / sizeof(char*)));
	CHECK_ALLOC(s_Files);

	char szUid[13];
    char mac[6];
    GetInterfaceAddresses( "eth0", NULL, mac );
	sprintf(szUid,"%02x%02x%02x%02x%02x%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

	// Create the device description doc file record.
	s_Files[0].szFile = (char*)malloc(strlen(gc_szDJDeviceDescXMLBase) + 4*strlen(szIPAddress)+ 2*strlen(szUid) +3*5/*iPort*/);
	sprintf(s_Files[0].szFile, gc_szDJDeviceDescXMLBase, szUid, szUid, szIPAddress, iPort,szIPAddress, iPort,szIPAddress, iPort, szIPAddress);
	s_Files[0].iFileLength = strlen(s_Files[0].szFile) + 1;

	// Add the device control doc file record.
	s_Files[1].szFile = const_cast<char*>(gc_szSCPD);
	s_Files[1].iFileLength = strlen(s_Files[1].szFile) + 1;

/*	// Add the device control doc file record.
	s_Files[2].szFile = const_cast<char*>(gc_szDJAVTransportSCPDXML);
	s_Files[2].iFileLength = strlen(s_Files[2].szFile) + 1;

	// Add the connector control doc file record.
	s_Files[3].szFile = const_cast<char*>(gc_szDJConnectorSCPDXML);
	s_Files[3].iFileLength = strlen(s_Files[3].szFile) + 1;

	// Add the presentation web page file record.
	s_Files[4].szFile = const_cast<char*>(gc_szDJDevicePresentationHTML);
	s_Files[4].iFileLength = strlen(s_Files[4].szFile) + 1;
*/
}

// Returns the 0-based index of the file with the given filename, or -1 if the file is not found.
int
GetFakeFileIndex(const char* szFilename)
{
	int i;
    DEBUG(DEVICESERVICES, DBGLEV_INFO, "Looking for file %s\n", szFilename);

	for (i = 0; i < sizeof(sc_aryFilenames) / sizeof(char*); ++i)
		// Do a brain-dead substring search.
		if (strstr(szFilename, sc_aryFilenames[i]))
			return i;

    DEBUG(DEVICESERVICES, DBGLEV_INFO, "Unable to find file %s\n", szFilename);
	return -1;
}

char*
FakeGetFile(const char* szFilename)
{
	int index = GetFakeFileIndex(szFilename);
    DEBUG(DEVICESERVICES, DBGLEV_INFO, "Getting file %s, index %d\n", szFilename, index);
	if (index == -1)
		return 0;
	else
		return s_Files[index].szFile;
}

int
FakeGetFileLength(const char* szFilename)
{
	int index = GetFakeFileIndex(szFilename);
    DEBUG(DEVICESERVICES, DBGLEV_INFO, "Getting length of file %s, index %d\n", szFilename, index);
	if (index == -1)
		return 0;
	else
		return s_Files[index].iFileLength;
}


int
FakeDriveFileGetRequestIndex(const char* szFilename)
{
	const char* pchFinder = strrchr(szFilename, '?');
	if (pchFinder)
	{
		if (!strncmp(pchFinder + 1, "index", 5))
		{
            DEBUG(DEVICESERVICES, DBGLEV_INFO, "File request TOC\n");
			return 0;
		}
		int index = atoi(pchFinder + 1);
		if (index > 0)
		{
            DEBUG(DEVICESERVICES, DBGLEV_INFO, "File request index: %d\n", index);
			return index;
		}
	}
	return -1;
}
