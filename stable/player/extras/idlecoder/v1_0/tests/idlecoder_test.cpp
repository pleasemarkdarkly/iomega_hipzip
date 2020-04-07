#include <cyg/infra/diag.h>
#include <cyg/kernel/kapi.h>            /* All the kernel specific stuff */
#include <content/simplecontentmanager/SimpleContentManager.h>
#include <core/mediaplayer/MediaPlayer.h>
#include <core/playmanager/PlayManager.h>
#include <datasource/fatdatasource/FatDataSource.h>
#include <datasource/fatdatasource/FatDataSource.h>
#include <datastream/fatfile/FatFile.h>
#include <datastream/fatfile/FileInputStream.h>
#include <datastream/fatfile/FileOutputStream.h>
#include <fs/fat/sdapi.h>
#include <extras/idlecoder/IdleCoder.h>
#include <playlist/simpleplaylist/SimplePlaylist.h>
#include <playlist/simpleplaylist/SimplePlaylist.h>
#include <stdio.h>
#include <string.h>
#include <util/debug/debug.h>
#include <util/datastructures/SimpleList.h>

#include <content/simplemetadata/SimpleMetadata.h>
IMetadata* CreateSimpleMetadata()
{
    return new CSimpleMetadata;
}

char *
make_url(DSTAT *ds) {
	int len = 0;
	char *path;
	if (*ds->longFileName) { // long filename is present
		len = strlen(ds->path);
		len += strlen(ds->longFileName);
		len += 7;	// "file://"
		path = new char[len+1];
		sprintf(path,"file://%s%s", ds->path, ds->longFileName);
	} else {  // only short filename is present. Yuck
		int j;
		len = strlen(ds->path);
		len += 7;	// "file://"
		j = len;
		len += 13;	// 8 plus dot plus 3 plus \0
		path = new char[len+1];
		strcpy(path, "file://");
		strcat(path, ds->path);
		for (int i=0; i < 8 && ds->fname[i] != ' '; i++, j++)
			path[j] = ds->fname[i];
		path[j++] = '.';
		for (int i=0; i < 3 && ds->fext[i] != ' '; i++, j++)
			path[j] = ds->fext[i];
		path[j] = '\0';
	}
	diag_printf("%s\n",path);
	return path;
}


static CFatFileOutputStream *out;


void
StatusCallback(CIdleCoder::job_rec_t *j, CIdleCoder::state_t s)
{
	switch(s) {
	case CIdleCoder::kStarting:
		diag_printf("kStarting: %s -> %s\n", j->in_url, j->out_url);
		j->in->Seek(IInputStream::SeekCurrent,44);
		break;
	case CIdleCoder::kEncoding:
		diag_printf(".");
		break;
	case CIdleCoder::kFinishing:
		diag_printf("\nkFinishing: %s -> %s\n", j->in_url, j->out_url);
		out->Close();
		break;
	case CIdleCoder::kError:
		diag_printf("kError\n");
	default:
		diag_printf("Unknown callback\n");
	}
}

class IdleCoderTest : public IThreadedObject {
private:
	SimpleList<char *> files;
public:
	IdleCoderTest() : IThreadedObject(10, "MainThread", 32768) {}
	virtual void ThreadBody() {
		diag_printf("+%s\n",__FUNCTION__);
		CIdleCoder *idle = CIdleCoder::GetInstance();

		out = new CFatFileOutputStream();
		out->Open("a:\\pem.xr");

		diag_printf("Scanning drive...\n");

		DSTAT ds;
		if (pc_gfirst(&ds, "a:\\*.wav")) do {
			files.PushBack(make_url(&ds));
		} while (pc_gnext(&ds));
		pc_gdone(&ds);

		diag_printf("Scan complete\n");
		diag_printf("%d files found\n", files.Size());
		
		CPlayManager* pPlayManager = CPlayManager::GetInstance();
		CSimpleContentManager* pCM = new CSimpleContentManager( CreateSimpleMetadata );
		pPlayManager->SetContentManager( pCM );
		pPlayManager->SetPlaylist( new CSimplePlaylist("") );
		
		if (CFatDataSource* pFatDS = CFatDataSource::Open(0)) {
			pPlayManager->AddDataSource( pFatDS );
			diag_printf("Opened hard drive\n");
		}
		else {
			diag_printf("Failed to open hard drive\n");
		}
		
		idle->SetCallback(&StatusCallback);
		idle->Run();

		int count = 0;
		char *inFile;
		char suffix[16];
		char outFile[EMAXPATH];
		while (!files.IsEmpty()) {
			inFile = files.PopFront();
			strcpy(outFile,inFile);
			outFile[strlen(outFile)-4] = '\0'; // chop off the extension
			snprintf(suffix,16,"-%d.mp3",128);
			strcat(outFile,suffix);
			diag_printf("infile: %30s outfile:%s\n",inFile, outFile);

			idle->Enqueue(inFile, outFile, 128);
			cyg_thread_delay(300);
			count++;
		}
		diag_printf("Enqueued %d files\n", count);
	}
};

extern "C" void dump_poly(long *left, long *right)
{
	out->Write(left,576*sizeof(long));
	out->Flush();
}

extern "C" void dump_xr(long *xr)
{
	out->Write(xr,576*sizeof(xr));
	out->Flush();
}

RUN_AS_STARTUP_THREAD(IdleCoderTest)
