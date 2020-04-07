#include <content/common/QueryableContentManager.h> // ContentKeyValueVector

class IFileNameRef;

struct CPlaylistConstraint
{
public:
    static CPlaylistConstraint* GetInstance();
    static void Destroy();
    void Constrain(int artist = CMK_ALL, int album = CMK_ALL, int genre = CMK_ALL, bool bRecordSession = false);
    void SetArtist(int artist);
    void SetAlbum(int album);
    void SetGenre(int genre);
    void SetTrack(IMediaContentRecord* pMCR);
    bool UpdatePlaylist(bool *pbCrntStillInList);
    void SetPlaylistFileNameRef(IFileNameRef* file);
    void ResyncCurrentPlaylistEntryById(int nCurrentEntryId, bool *pbCrntStillInList);
    void SyncPlayerToPlaylist();
    int RestoreFromRegistry();
    int SaveToRegistry();
    void HandleInvalidPlaylist();

private:
    void InitRegistry();
    void SaveState(void* buf, int len);
    void RestoreState(void* buf, int len);
    int GetStateSize();

    int m_iArtistKey;
    int m_iAlbumKey;
    int m_iGenreKey;
    bool m_bActiveRecordSession;
    IMediaContentRecord* m_pContentRecord;
    IFileNameRef* m_pPlaylistFileNameRef;
    CPlaylistConstraint();
    ~CPlaylistConstraint();
};