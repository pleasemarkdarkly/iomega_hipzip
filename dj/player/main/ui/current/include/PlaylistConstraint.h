#include <content/common/QueryableContentManager.h> // ContentKeyValueVector

struct CPlaylistConstraint
{
public:
    static CPlaylistConstraint* GetInstance();
    static void Destroy();
    void Constrain(int artist = CMK_ALL, int album = CMK_ALL, int genre = CMK_ALL);
    void SetArtist(int artist);
    void SetAlbum(int album);
    void SetGenre(int genre);
    void SetTrack(IMediaContentRecord* pMCR);
    void UpdatePlaylist();
    void SetPlaylistURL(const char* url);
private:
    int m_iArtistKey;
    int m_iAlbumKey;
    int m_iGenreKey;
    IMediaContentRecord* m_pContentRecord;
    char* m_szPlaylistURL;
    CPlaylistConstraint();
    ~CPlaylistConstraint();
};
