//
// Restore.cpp
// load restore file from cd
// format drive
// copy freedb
// copy sample content
//
//
//
// Teman Clark-Lindh 
// temancl@fullplaymedia.com
// (c) 2002 Fullplay Media
//
#include <cyg/kernel/kapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <main/main/DJPlayerState.h>
#include <main/main/FatHelper.h>
#include <datastream/fatfile/FatFile.h>
#include <main/util/update/UpdateApp.h>
#include <main/util/update/ParseConfig.h>
#include <fs/utils/format_fat32/format.h>
#include <cyg/fileio/fileio.h>
#include <datastream/isofile/IsoFileInputStream.h>
#include <util/datastructures/SimpleList.h>
#include <fs/flash/flashmanager.h>
#include <cyg/infra/diag.h>
#include <fs/fat/sdapi.h>
#include <cyg/hal/hal_cache.h>


#include <util/debug/debug.h>          // debugging hooks
DEBUG_MODULE_S( DBG_RESTORE_CD, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_RESTORE_CD );


#define CDDB_GROUP_NAME "cddb"
#define CDDB_VERSION "version"
#define CONTENT_GROUP_NAME "content"
#define APP_GROUP_NAME "app"
#define APP_VERSION "version"
#define RESTORE_CD_CONFIG_PATH "/restore.cfg"
// conversion helper
char* cdpathtohdpath(char *cd, char* hd)
{
	strcpy(hd,"a:");
	strcat(hd,cd);

	DEBUGP( DBG_RESTORE_CD, DBGLEV_INFO, "converted %s to %s\n",cd,hd);

	return hd;

}

// buffered, incremental copy
// cddb can be very large, don't just buffer everything

#define COPY_BUFF_SIZE 64*1024

bool CopyCDtoHD(const char *source, const char *target)
{

	CIsoFileInputStream cdis;
	CFatFile hdis;
	int rcount = 0;

	
	char *buff;
	buff = new char[COPY_BUFF_SIZE];
	
	if(INPUTSTREAM_NO_ERROR != cdis.Open(source))
	{
		DEBUGP( DBG_RESTORE_CD, DBGLEV_INFO, "cd open fail\n");
		delete []buff;
		return false;
	}

	
	char temppath[MAX_PATH_LEN];	
	strcpy(temppath,target);
	int templen = strlen(temppath);
	templen--;
	while(temppath[templen] != '/' && templen > 0)
		templen--;

	if(temppath[templen] != '/')
	{
		DEBUGP( DBG_RESTORE_CD, DBGLEV_INFO, "path convert fail\n");
		delete []buff;
		return false;
	}

	// terminate at last directory
	temppath[templen] = '\0';


	DEBUGP( DBG_RESTORE_CD, DBGLEV_INFO, "creating path %s\n",temppath);
	if(!VerifyOrCreateDirectory(temppath))
	{
		DEBUGP( DBG_RESTORE_CD, DBGLEV_INFO, "error creating path %s\n",temppath);
		delete []buff;
		return false;
	}


	
	if(!hdis.Open(target,CFatFile::WriteCreate ))
	{
		cdis.Close();
		DEBUGP( DBG_RESTORE_CD, DBGLEV_INFO, "hd open fail\n");
		delete []buff;
		return false;
	}

	while(0 != (rcount = cdis.Read(buff,COPY_BUFF_SIZE)))
	{
		hdis.Write(buff,rcount);
	}

	hdis.Close();
	cdis.Close();
	delete []buff;

	DEBUGP( DBG_RESTORE_CD, DBGLEV_INFO, "Copied %s to %s\n",source,target);
	return true;

}


// placeholder, contents will be merged into ui scheme
// call prior to HD intialization

void format_status(int current, int total)
{
	diag_printf("formating progress %d of %d\r",current,total);
}

void RestoreDJ()
{
	

	 DEBUGP( DBG_RESTORE_CD, DBGLEV_INFO, "Restoring DJ\n");
          

	 DEBUGP( DBG_RESTORE_CD, DBGLEV_INFO, "Polling on CD\n");
	 int err;
	 while((err = mount("/dev/cda/", "/", "cd9660")) < 0)
	 {
		 DEBUGP( DBG_RESTORE_CD, DBGLEV_INFO, "Error mounting CD, try inserting a disc\n");
	 }

	 
          

	// open cd
	char pathtemp[MAX_PATH_LEN];


	// look for restore file
	CIsoFileInputStream ifis;
	char *buff;
	ifis.Open(RESTORE_CD_CONFIG_PATH);
	buff = new char[ifis.Length()];
	ifis.Read(buff,ifis.Length());
	
	// parse restore file
	CParseConfig *parser = new CParseConfig();
	
	if(parser->ParseBuffer(buff,ifis.Length()))
	{

		DEBUGP( DBG_RESTORE_CD, DBGLEV_INFO, "Formatting HD\n");

		// format drive
		diag_printf("\n");
		format_drive("/dev/hda/",&format_status);
		diag_printf("\n");

		// intialize fat fs
		pc_system_init(0);


		// copy cddb database files

		
		const char* tmp;
		int cddbver;
		if((tmp = parser->FindVariable(CDDB_GROUP_NAME,CDDB_VERSION)) != NULL)
		{

			DEBUGP( DBG_RESTORE_CD, DBGLEV_INFO, "Copying CDDB\n");

			// get file version
			cddbver = atoi(tmp);

			if(cddbver != 0)
			{
				
				SimpleList<FileInfo> cddbfiles;
				if(parser->GetFileList(CDDB_GROUP_NAME,cddbfiles))
				{

					// FIXME: total and progress bars here
					for (SimpleListIterator<FileInfo> itFiles = cddbfiles.GetHead();			
					itFiles != cddbfiles.GetEnd(); ++itFiles)				
					{
						CopyCDtoHD((*itFiles).path,cdpathtohdpath((*itFiles).path,pathtemp));
					}
				}

			}

		}


		// update content
		if(parser->FindGroup(CONTENT_GROUP_NAME) != NULL)
		{

			DEBUGP( DBG_RESTORE_CD, DBGLEV_INFO, "Copying Content\n");
			SimpleList<FileInfo> content;
			if(parser->GetFileList(CONTENT_GROUP_NAME,content))
			{

				// FIXME: total and progress bars here
				for (SimpleListIterator<FileInfo> itFiles = content.GetHead();			
				itFiles != content.GetEnd(); ++itFiles)				
				{
					CopyCDtoHD((*itFiles).path,cdpathtohdpath((*itFiles).path,pathtemp));
				}
			}
		}


		// check update image, load into ram and flash if new
		if((tmp = parser->FindVariable(APP_GROUP_NAME,APP_VERSION)) != NULL)
		{
			int ver = atoi(tmp);
			if(ver > CDJPlayerState::GetInstance()->GetPlayerVersion())
			{
				DEBUGP( DBG_RESTORE_CD, DBGLEV_INFO, "Updating Flash\n");
	
				// perform a flash update
				SimpleList<FileInfo> appfiles;
				
				if(parser->GetFileList(APP_GROUP_NAME,appfiles))
				{
					
					if(appfiles.Size() == 1)
					{
						CIsoFileInputStream ifisApp;
						char *appbuff;
						int appsize;
						ifisApp.Open(appfiles.PopFront().path);
						appsize = ifisApp.Length();
						appbuff = new char[appsize];
						ifisApp.Read(appbuff,ifisApp.Length());
						ifisApp.Close();

						CUpdateApp *pUpdate = CUpdateApp::GetInstance();	

						if(pUpdate->VerifyImage(appbuff,appsize))
						{
							int oldints;
							// does this shit work at all?
							// HAL_DISABLE_INTERRUPTS(oldints);
						
							CFlashManager *pfm = CFlashManager::GetInstance();
	   
							// rewrite that image
							pfm->UpdateImage(IMAGE_NAME,appbuff,appsize);


							// HAL_ENABLE_INTERRUPTS(oldints);
							delete[] appbuff;

						}
			
		

					}
					else
					{
							DEBUGP( DBG_RESTORE_CD, DBGLEV_INFO, "Flash Config Parse Error\n");
					}
	

				}

			}
		}


	}
	else
	{
		DEBUGP( DBG_RESTORE_CD, DBGLEV_INFO, "Flash Config Parse Error\n");
	}


	delete parser;
	delete[] buff;

	ifis.Close();

	DEBUGP( DBG_RESTORE_CD, DBGLEV_INFO, "Restore Complete\n");
}

