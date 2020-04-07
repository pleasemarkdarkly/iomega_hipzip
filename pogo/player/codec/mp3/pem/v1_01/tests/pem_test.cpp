#include <cyg/infra/diag.h>
#include <cyg/kernel/kapi.h>            /* All the kernel specific stuff */
#include <content/simplecontentmanager/SimpleContentManager.h>
#include <core/mediaplayer/MediaPlayer.h>
#include <core/playmanager/PlayManager.h>
#include <datasource/fatdatasource/FatDataSource.h>
#include <datasource/fatdatasource/FatDataSource.h>
#include <datastream/fatfile/FileInputStream.h>
#include <datastream/fatfile/FileOutputStream.h>
#include <fs/fat/sdapi.h>
#include <playlist/simpleplaylist/SimplePlaylist.h>
#include <playlist/simpleplaylist/SimplePlaylist.h>
#include <string.h>
#include <util/debug/debug.h>
#include <util/thread/ThreadedObject.h>
#include <extras/idlecoder/IdleCoder.h>

size_t strtrunk(const char *s, size_t maxlen)
{
	size_t x;
	for(x=0; (s[x]) && (s[x] != ' ') && (x < maxlen); x++)
		;
	return x;
}

#include <content/simplemetadata/SimpleMetadata.h>
IMetadata* CreateSimpleMetadata()
{
    return new CSimpleMetadata;
}

void
SDKInit()
{
    CPlayManager* pPlayManager = CPlayManager::GetInstance();
	
	CSimpleContentManager* pCM = new CSimpleContentManager( CreateSimpleMetadata );
    pPlayManager->SetContentManager( pCM );
    pPlayManager->SetPlaylist( new CSimplePlaylist("") );
	
    ////////////////////////
    // Create data sources
    //
    diag_printf("Opening hard drive\n");
    if (CFatDataSource* pFatDS = CFatDataSource::Open(0))
    {
        pPlayManager->AddDataSource( pFatDS );
        diag_printf("Opened hard drive\n");
    }
    else {
        diag_printf("Failed to open hard drive\n");
    }
}


static
char *MakePath(char *target, const char *path, const char *name, const char *ext)
{
	int pathlen = strtrunk(path,EMAXPATH);
	int namelen = strtrunk(name,8);
	int extlen = strtrunk(ext,3);
	char* str = target;
	memcpy(str, "file://",7);
	str += 7;
	memcpy(str, path, pathlen);
	str += pathlen;
	memcpy(str, name, namelen);
	str += namelen;
	*str++ = '.';
	memcpy(str, ext, extlen);
	str += extlen;
	*str = '\0';
	for(str=target;*str;str++) {
		if (*str=='\\') *str = '/';
	}
	return target;
}

void
IdleCoderCallback(CIdleCoder::job_rec_t *j, CIdleCoder::state_t s)
{

	static cyg_tick_count_t t=0,t_old = 0;
	static unsigned long bytes_old = 0;

	switch(s) {
	case CIdleCoder::kStarting:
		diag_printf("Starting %s\n",j->in_url);
		t_old = t = cyg_current_time();
		bytes_old=0;
		j->in->Seek(IInputStream::SeekStart, 44);
		break;
	case CIdleCoder::kEncoding:
		{
			t = cyg_current_time();
			if (t-t_old > 200) {
				long workdone = 10*(j->bytes_encoded - bytes_old)/1764;
				long timetaken = 10*(t-t_old);
				diag_printf("%s: %3d ms encoded in %4d ms (%d.%02d realtime)\n", j->in_url,
					workdone, timetaken, workdone/timetaken, (100 * workdone/timetaken)%100);
				bytes_old = j->bytes_encoded;
				t_old = t;
			}
		}
		break;
	case CIdleCoder::kFinishing:
		break;
	default:
		;
	}
}


class IdleCoderTest : public IThreadedObject {
public:
	IdleCoderTest() : IThreadedObject(10, "MainThread", 32768) {}
	virtual void ThreadBody() {
		SDKInit();

		CIdleCoder *idle = CIdleCoder::GetInstance();
		diag_printf("+%s\n",__FUNCTION__);
		char inFile[EMAXPATH+20];
		char outFile[EMAXPATH+20];

		cyg_thread_delay(100);

		idle->SetCallback(IdleCoderCallback);

		diag_printf("Scanning drive...\n");

		DSTAT ds;
		if (pc_gfirst(&ds, "a:\\*.wav")) do {
			diag_printf("infile: %s ",MakePath(inFile,ds.path,ds.fname,ds.fext));
			diag_printf("outfile: %s\n",MakePath(outFile,ds.path,ds.fname,"MP3"));


			//idle->EncodeFatFile(inFile,outFile);
			idle->Enqueue(inFile,outFile,256);

		} while (pc_gnext(&ds));
		pc_gdone(&ds);
		diag_printf("Starting encoder:\n");
		idle->Run();
	}
};


extern "C" {

void
cyg_user_start(void)
{
    diag_printf("+%s\n", __FUNCTION__);
    
	diag_printf("Dynamic\n");
	IThreadedObject *t = new IdleCoderTest;
	t->Start();
	cyg_scheduler_start();
    diag_printf("-%s\n", __FUNCTION__);
}

};

