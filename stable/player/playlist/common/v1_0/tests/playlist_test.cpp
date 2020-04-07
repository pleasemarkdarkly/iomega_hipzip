#include <pkgconf/kernel.h>
#include <cyg/kernel/kapi.h>            /* All the kernel specific stuff */
#include <cyg/hal/hal_arch.h>           /* CYGNUM_HAL_STACK_SIZE_TYPICAL */
#include <cyg/infra/diag.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/hal/drv_api.h>

#include <playlist/simpleplaylist/SimplePlaylist.h>

#define METAKIT_CM
#if defined (METAKIT_CM)
#include <content/metakitcontentmanager/MetakitContentManager.h>
#else
#include <content/simplecontentmanager/SimpleContentManager.h>
#endif

#include <util/debug/debug.h>

#include <stdio.h>  /* sprintf */
#include <stdlib.h>

/* DEFINES */

#define DEBUG(s...) diag_printf(##s)
//#define DEBUG(s...) /**/

#define NTHREADS 1
#define STACKSIZE (CYGNUM_HAL_STACK_SIZE_TYPICAL + (16 * 4096))

/* STATICS */

static cyg_handle_t _ThreadH[NTHREADS];
static cyg_thread _Thread[NTHREADS];
static char _ThreadStack[NTHREADS][STACKSIZE];

static void
mem_stats(void)
{
	struct mallinfo mem_info;		    
	mem_info = mallinfo();
	diag_printf("Memory system: Total=0x%08x Used = 0x%08x Free=0x%08x Max=0x%08x\n",
		    mem_info.arena, mem_info.arena - mem_info.fordblks, mem_info.fordblks,
		    mem_info.maxfree);
}

char*
NextURL()
{
    static int s_iNextID = 1;
    char* szURL = (char*)malloc(10);
    sprintf(szURL, "URL%d", s_iNextID++);
    return szURL;
}

bool CompareContentRecords(const cm_key_value_record_t& a, const cm_key_value_record_t& b)
{
    return tstrncmp(a.szValue, b.szValue, PLAYLIST_STRING_SIZE) <= 0;
}

void
PrintWide(const TCHAR* wsz)
{
    char szScratch[PLAYLIST_STRING_SIZE];
    TcharToCharN(szScratch, wsz, PLAYLIST_STRING_SIZE);
    DEBUG("%s", szScratch);
}

void
PrintRecord(const IMediaContentRecord& cCR)
{
//    PrintWide(cCR.GetURL());
    DEBUG("%s - ", cCR.GetURL());
    PrintWide(cCR.GetTitle());
    DEBUG(" - ");
    PrintWide(cCR.GetArtist());
    DEBUG(" - ");
    PrintWide(cCR.GetAlbum());
    DEBUG(" - ");
    PrintWide(cCR.GetGenre());
    DEBUG("\n");
}

void
PrintAllRecords(const IContentManager& cCM)
{
    DEBUG("Artists:\n");
    ContentKeyValueVector artists = cCM.GetArtists();
    artists.Sort(CompareContentRecords);
    for (int i = 0; i < artists.Size(); ++i)
    {
        DEBUG("Artist ID %d: ", artists[i].iKey);
        PrintWide(artists[i].szValue);
        DEBUG("\n");
        ContentKeyValueVector albums = cCM.GetArtistAlbums(artists[i].iKey);
        albums.Sort(CompareContentRecords);
        for (int j = 0; j < albums.Size(); ++j)
        {
            DEBUG("   Album ID %d: ", albums[j].iKey);
            PrintWide(albums[j].szValue);
            DEBUG("\n");
            MediaRecordList albumTracks;
            cCM.GetMediaRecordsByAlbum(albumTracks, albums[j].iKey);
            for (MediaRecordIterator it = albumTracks.GetHead(); it != albumTracks.GetEnd(); ++it)
            {
                DEBUG("       Track: ");
                PrintRecord(*(*it));
            }
        }
    }
}

#define METADATA_STRING_SIZE 256

void
AddAlbum(IContentManager& CM, const char* szArtist, const char* szGenre, const char* szAlbum, int iTitles, const char** aryTitles)
{
#ifdef UNICODE
    TCHAR tszArtist[METADATA_STRING_SIZE];
    TCHAR tszAlbum[METADATA_STRING_SIZE];
    TCHAR tszGenre[METADATA_STRING_SIZE];
    CharToTcharN(tszArtist, szArtist, METADATA_STRING_SIZE); 
    CharToTcharN(tszAlbum, szAlbum, METADATA_STRING_SIZE); 
    CharToTcharN(tszGenre, szGenre, METADATA_STRING_SIZE); 
#endif  // UNICODE

    media_record_info_t mri;
    mri.iCodecID = 0;
    mri.iDataSourceID = 0;

    for (int i = 0; i < iTitles; ++i)
    {
        mri.pMetadata = CM.CreateMetadataRecord();

        mri.szURL = NextURL();
#ifdef UNICODE
        TCHAR tszTitle[METADATA_STRING_SIZE];
        CharToTcharN(tszTitle, aryTitles[i], METADATA_STRING_SIZE);
        mri.pMetadata->SetAttribute(MDA_TITLE, (void*)tszTitle);
        mri.pMetadata->SetAttribute(MDA_ARTIST, (void*)tszArtist);
        mri.pMetadata->SetAttribute(MDA_ALBUM, (void*)tszAlbum);
        mri.pMetadata->SetAttribute(MDA_GENRE, (void*)tszGenre);
#else   // UNICODE
        mri.pMetadata->SetAttribute(MDA_TITLE, (void*)aryTitles[i]);
        mri.pMetadata->SetAttribute(MDA_ARTIST, (void*)szArtist);
        mri.pMetadata->SetAttribute(MDA_ALBUM, (void*)szAlbum);
        mri.pMetadata->SetAttribute(MDA_GENRE, (void*)szGenre);
#endif  // UNICODE
        CM.AddMediaRecord(mri);
    }
}

static void
_MainThread(CYG_ADDRESS Data)
{       
    DEBUG("+%s\n", __FUNCTION__);

    mem_stats();

    {
#if defined (METAKIT_CM)
    CMetakitContentManager CM;
#else
    CSimpleContentManager CM;
#endif
    }
    mem_stats();

    {
#if defined (METAKIT_CM)
    CMetakitContentManager CM;
#else
    CSimpleContentManager CM;
#endif
    media_record_info_t mri;

    mri.iCodecID = 0;
    mri.iDataSourceID = 0;

    mem_stats();
    mri.pMetadata = CM.CreateMetadataRecord();
    mem_stats();
    mri.szURL = (char*)malloc(5);
    strcpy(mri.szURL, "URL");
    mri.pMetadata->SetAttribute(MDA_ARTIST, (void*)"TestArtist");
    mri.pMetadata->SetAttribute(MDA_ALBUM, (void*)"TestAlbum");
    mri.pMetadata->SetAttribute(MDA_GENRE, (void*)"TestGenre");
    CM.AddMediaRecord(mri);

    }
    mem_stats();

    {

#if defined (METAKIT_CM)
    CMetakitContentManager CM;
#else
    CSimpleContentManager CM;
#endif

    const char* pHOTDTracks[] = { "The Walk", "Let's Go to Bed", "The Upstairs Room" };
    AddAlbum(CM, "The Cure", "Rock", "Head on the Door", 3, pHOTDTracks);

    }

    mem_stats();

    {

#if defined (METAKIT_CM)
    CMetakitContentManager CM;
#else
    CSimpleContentManager CM;
#endif

    const char* pDTracks[] = { "Pictures of You", "Fascination Street" };
    AddAlbum(CM, "", "Rock", "Disintegration", 2, pDTracks);

    }

    mem_stats();

    {

#if defined (METAKIT_CM)
    CMetakitContentManager CM;
#else
    CSimpleContentManager CM;
#endif

    const char* pGVTracks[] = { "Aria", "Presto", "Allegro" };
    AddAlbum(CM, "Bach", "Classical", "Goldberg Variations", 3, pGVTracks);

    const char* pSFETracks[] = { "Dumb Motha Fucka", "You Are Now About to Witness" };
    AddAlbum(CM, "DJ Spooky", "", "Sonic Fury EP", 2, pSFETracks);

    IPlaylist* pPlaylist = new CSimplePlaylist("");
    MediaRecordList mrl;
    CM.GetAllMediaRecords(mrl);
    pPlaylist->AddEntries(mrl);
    int iPlaylistEntryCount = pPlaylist->GetSize();
    DEBUG("Playlist size: %d\n", iPlaylistEntryCount);
    for (int j = 0; j < iPlaylistEntryCount; ++j)
    {
        IPlaylistEntry* pEntry = pPlaylist->GetEntry(j);
        if (pEntry)
        {
            IMediaContentRecord* pRecord = pEntry->GetContentRecord();
            if (pRecord)
            {
                DEBUG("Entry %d: ");
                PrintRecord(*pRecord);
            }
        }
        else
            DEBUG("Entry %d: :(\n", j);
    }
    delete pPlaylist;

    DEBUG("\n");
    PrintAllRecords(CM);

    DEBUG("\nAlbums:\n");
    ContentKeyValueVector albums = CM.GetAlbums();
    albums.Sort(CompareContentRecords);
    for (int i = 0; i < albums.Size(); ++i)
    {
        DEBUG("Album %d: ", albums[i].iKey);
        PrintWide(albums[i].szValue);
        DEBUG("\n");
    }

    DEBUG("\nGenres:\n");
    ContentKeyValueVector genres = CM.GetGenres();
    genres.Sort(CompareContentRecords);
    for (int i = 0; i < genres.Size(); ++i)
    {
        DEBUG("Genre %d: ", genres[i].iKey);
        PrintWide(genres[i].szValue);
        DEBUG("\n");
    }

    TCHAR tchScratch[PLAYLIST_STRING_SIZE];
    int iArtistKey = CM.GetArtistKey(CharToTchar(tchScratch, "The Cure"));
    DEBUG("\nCure tracks (ID = %d):\n", iArtistKey);
    MediaRecordList mrlTracks;
    CM.GetMediaRecordsByArtist(mrlTracks, iArtistKey);
    for (MediaRecordIterator it = mrlTracks.GetHead(); it != mrlTracks.GetEnd(); ++it)
    {
        DEBUG("Entry: ");
        PrintRecord(*(*it));
        CM.DeleteMediaRecord((*it)->GetID());
    }

    DEBUG("\nDisintegration tracks:\n");
    mrlTracks.Clear();
    CM.GetMediaRecordsByAlbum(mrlTracks, CM.GetAlbumKey(CharToTchar(tchScratch, "Disintegration")));
    CharToTchar(tchScratch, "The Cure");
    for (MediaRecordIterator it = mrlTracks.GetHead(); it != mrlTracks.GetEnd(); ++it)
    {
        DEBUG("Entry: ");
        PrintRecord(*(*it));
        (*it)->SetAttribute(MDA_ARTIST, tchScratch);
    }

    mrlTracks.Clear();
    iArtistKey = CM.GetArtistKey(CharToTchar(tchScratch, "The Cure"));
    DEBUG("\nCure tracks (ID = %d):\n", iArtistKey);
    CM.GetMediaRecordsByArtist(mrlTracks, iArtistKey);
    for (MediaRecordIterator it = mrlTracks.GetHead(); it != mrlTracks.GetEnd(); ++it)
    {
        DEBUG("Entry: ");
        PrintRecord(*(*it));
    }

    DEBUG("\n");
    PrintAllRecords(CM);

    DEBUG("\nSonic Fury EP tracks:\n");
    mrlTracks.Clear();
    CM.GetMediaRecordsByAlbum(mrlTracks, CM.GetAlbumKey(CharToTchar(tchScratch, "Sonic Fury EP")));
    for (MediaRecordIterator it = mrlTracks.GetHead(); it != mrlTracks.GetEnd(); ++it)
    {
        DEBUG("Entry: ");
        PrintRecord(*(*it));
        TCHAR tszIllbient[] = {'I','l','l','b','i','e','n','t',0};
        (*it)->SetGenre(tszIllbient);
    }

    DEBUG("\nGenres:\n");
    genres = CM.GetGenres();
    genres.Sort(CompareContentRecords);
    for (int i = 0; i < genres.Size(); ++i)
    {
        DEBUG("Genre %d: ", genres[i].iKey);
        PrintWide(genres[i].szValue);
        DEBUG("\n");
    }

    DEBUG("\nClassical tracks:\n");
    mrlTracks.Clear();
    CM.GetMediaRecordsByGenre(mrlTracks, CM.GetGenreKey(CharToTchar(tchScratch, "Classical")));
    for (MediaRecordIterator it = mrlTracks.GetHead(); it != mrlTracks.GetEnd(); ++it)
    {
        DEBUG("Entry: ");
        PrintRecord(*(*it));
    }

    }

    mem_stats();

    DEBUG("-%s\n", __FUNCTION__);
}

extern "C" {

void
cyg_user_start(void)
{
    DEBUG("+%s\n", __FUNCTION__);
    
    cyg_thread_create(10, _MainThread, (cyg_addrword_t) 0, "MainThread",
		      (void *)_ThreadStack[0], STACKSIZE, &_ThreadH[0], &_Thread[0]);
    cyg_thread_resume(_ThreadH[0]);

    diag_printf("-%s\n", __FUNCTION__);
}

};
