#include <fs/dataplay/dfs.h>
#include <fs/dataplay/dp.h>
#include <datastream/dataplayfile/DPFile.h>
#include <cyg/infra/diag.h>                    // for DEBUG
#include <util/debug/debug.h>
#include <string.h>

// declare the debug module here
DEBUG_MODULE(DP);
DEBUG_USE_MODULE(DP);

/*  Class CDPFile  */
static DFSHANDLE s_hRootDir = -1;

CDPFile::CDPFile() : m_FD(-1), m_iPosition(0), m_hParentDir(-1)
{
	dfs_media_info_t media_info;
	int err;
	
    if( (err = dp_load_media() ) ) {
        DEBUG(DP,DBGLEV_ERROR,  "Error loading DP media: %s\n", dp_get_error( err ) );
		//      return ;
    }
    if( (err = dp_lock_media() ) ) {
		DEBUG(DP,DBGLEV_ERROR,  "Error locking DP media: %s\n", dp_get_error( err ) );
		//    return ;
    }
    if( (err = dfs_get_media_info( &media_info ) ) ) {
		DEBUG(DP,DBGLEV_ERROR,  "Error reading dfs media info: %s\n", dp_get_error( err ) );
		//    return ;
    }
	// init parent to root?
	s_hRootDir = media_info.handle;
	DEBUG(DP,DBGLEV_ERROR, "root directory handle is %d\n", m_hParentDir );
    if( (err = dp_release_media() ) ) {
		DEBUG(DP,DBGLEV_ERROR,  "Error releasing media: %s\n", dp_get_error( err ) );
		//    return ;
    }
	if( (err = dp_eject_media() ) ) {
		DEBUG(DP,DBGLEV_ERROR, "Error ejecting media (fuck): %s\n", dp_get_error( err ) );
		return ;
	}
}

DFSHANDLE HandleFromPath(const char* path)
{
	char path_leg[128];
	char *p;
	int len;
	DFSHANDLE h = s_hRootDir;
	if (strncmp(path,"DFS:\\",5))
		return -1;
	p = (char*)path + 5; // skip 'DFS:\'
	while (p)
	{
		len = 0;
		// point p + len at next '\\' or '\0'.
		while (*(p + len) && *(p + len) != '\\')
			++len;
		strncpy (path_leg,p,len);
		path_leg[len] = 0;
		h = dfs_get_handle( h, 0, path_leg );
		// fail the path was invalid
		if (h < 0)
			return h;
		// skip handled leg
		p += len;
		// if there is a char at *p, it is a backslash, so skip it for the next round.
		if (*p)
			++p;
	}
	return h;
}

int CDPFile::Open(const char* filename)
{
	if (m_hParentDir == -1)
	{
		DEBUG(DP,DBGLEV_ERROR, "no parent dir handle in DPFile::Open\n");
		return -1;
	}
	m_FD = HandleFromPath( filename );
	if (m_FD < 0)
		return -1;
	m_iPosition = 0;
	return (int) m_FD;
}

int CDPFile::Close()
{
	m_FD = -1;
	return 1;
}

CDPFile::~CDPFile()
{
}

int CDPFile::Length()
{
	dfs_file_info_t file_info;
	if (m_FD < 0)
		return 0;
	int nFileSize = -1;					// how many chars total?	
	if( dfs_get_file_info( m_FD, &file_info ) == 0 )
		nFileSize = (unsigned int) file_info.file_size;
	else
	{
		DEBUG(DP,DBGLEV_ERROR, "pc_fstat unable to determine stats on input file\n");
	}
	return nFileSize;
}

int CDPFile::Read(void* pBuffer, unsigned int nBytes)
{
	if(m_FD < 0)
		return -1;
	if( dfs_read( m_FD, (char*)pBuffer, m_iPosition, nBytes, 0 ) != 0 )
		return -1;
	else
	{
		m_iPosition += nBytes;
		return (int) nBytes;
	}
}

int CDPFile::Write( const void* Buffer, int Count )
{ 
	return -1;
}

bool CDPFile::Flush()
{
	DEBUG(DP,DBGLEV_ERROR, "FLUSH\n");
	return false;
}

int CDPFile::Ioctl( int Key, void* Value )
{
    return 0;
}

int CDPFile::GetOffset() const
{
	DEBUG(DP,DBGLEV_ERROR, "GET OFFSET\n");
	return -1;
}

int CDPFile::Seek( FileSeekPos Origin, int Offset )
{
	int nFileSize=0;					// how many chars total?	
	if(m_FD < 0)
		return -1;
	switch(Origin)
	{
	case SeekStart:
		if(Offset >= 0)
		{
			// (epg,8/8/2001): DP doesn't maintain file cursor, so we just remember that next time we'll request data from a certain offset
			m_iPosition = Offset;
			return m_iPosition;
		}
		else
		{
			DEBUG(DP,DBGLEV_ERROR, "FIL:Invalid negative offset from bof requested of seek\n");
			return -1;
		}
	case SeekCurrent:
		{
			
			m_iPosition += Offset;
			return m_iPosition;
		}
		
	case SeekEnd:
		// (epg,6/7/2000): ok, dammit.  po_lseek adds the offset to the file size and seeks from the beginning of the file.
		// so the sense of the offset from eof is that we want it to always be negative.  however, po_lseek wants a ulong
		// offset, so that is never possible.  what we therefore have to do is figure out in advance what the offset is from
		// the bof and send it in that way.  
		if(Offset <= 0)
		{
			dfs_file_info_t file_info;
			if( dfs_get_file_info( m_FD, &file_info ) == 0 )
				nFileSize = (unsigned int) file_info.file_size;
			else
			{
				DEBUG(DP,DBGLEV_ERROR, "pc_fstat unable to determine stats on input file\n");
			}
			Offset += nFileSize;
			
			m_iPosition = Offset;
			return m_iPosition;
		}
		else
		{
			DEBUG(DP,DBGLEV_ERROR, "FIL:Invalid positive offset from eof requested of seek\n");
			return -1;
		}
	}
	return -1;
}
