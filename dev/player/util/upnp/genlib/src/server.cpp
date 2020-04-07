///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000 Intel Corporation
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
// * Neither name of Intel Corporation nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////

// $Revision: 1.8 $
// $Date: 2000/10/06 16:37:57 $

// readwrite.cpp

#include <util/upnp/api/config.h>


#ifdef INTERNAL_WEB_SERVER
#if EXCLUDE_WEB_SERVER == 0
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <util/upnp/genlib/util.h>
#include <util/upnp/genlib/xstring.h>
#include <util/upnp/genlib/dbllist.h>
#include <util/upnp/genlib/statuscodes.h>
#include <util/upnp/genlib/parseutil2.h>
#include <util/upnp/genlib/readwrite.h>
#include <util/upnp/genlib/server.h>
#include <util/upnp/genlib/http_client.h>

#include <util/upnp/genlib/mystring.h>
extern "C"
{
#include <network.h>	// - ecm
//#include <cyg/posix/signal.h>
#include <main/upnpservices/XMLDocs.h>
}

#ifndef _WIN32
#include <unistd.h>
#ifdef USE_PTHREADS
#include <pthread.h>
#else
#include <cyg/kernel/kapi.h>
#endif
#else
#include <io.h>
#define S_ISDIR(x) (x & _S_IFDIR)
#define S_ISREG(x) (x & _S_IFREG)
#endif


#define SVRERR_FILE_NOT_FOUND -2
#define SVRERR_BAD_FORMAT     -3

CREATE_NEW_EXCEPTION_TYPE( HttpServerException, BasicException, "HttpServerException" )

//////////////////////////////////////////
struct AliasedEntity
{
    xstring alias;
    xstring contentType;
    xstring contentSubtype;
    time_t modifiedTime;
    HttpEntity* entity;
    int count;
    bool stopServing;
};

//////////////////////////////////////////////
CREATE_NEW_EXCEPTION_TYPE( AliasedListException, BasicException, "AliasedListException" )

//////////////////////////////////////////////
class AliasedEntityList
{
public:
    enum { E_SUCCESS = 0, E_NOT_FOUND = -2, E_DUPLICATE = -3,
        E_CANT_ADD = -5 };

public:
    // throws OutOfMemoryException
    AliasedEntityList();

    virtual ~AliasedEntityList();

    // adds the specified aliased entity to the list
    //
    // NOTE: 'entity' will be owned by the list
    // throws:
    //      OutOfMemoryException
    EDERRCODE addEntity( IN const char* alias, IN HttpEntity* entity,
        OUT xstring& actualAlias );

    // returns entity and its attributes
    //
    // throws:
    //      AliasListException.E_NOT_FOUND
	bool grabEntity( IN const char* alias, OUT HttpEntity** entity,
		OUT xstring& contentType, OUT xstring& contentSubtype,
		OUT time_t& lastModifiedTime );

    // releases entity denoted by the alias
    //
    // stopServing: if true, future requests will return NOT FOUND
    //
    // throws:
    //      AliasListException.E_NOT_FOUND
    EDERRCODE releaseEntity( IN const char* alias, bool stopServing = false );

    // returns true if the aliased entity is in the list
    bool isAliasPresent( IN const char* alias );

private:
    // returns NULL if list node associated with alias is not found
    dblListNode* find( IN const char* alias );

private:
    dblList aliasList;      // = list<AliasedEntity>
#ifdef USE_PTHREADS
	pthread_mutex_t mutex;
#else
	cyg_mutex_t mutex;
#endif
};

// *******************************************************
// module vars //////////////
// root directory of http server; must not have '/' at the end
static xstring gRootDocumentDir;
static bool    gServerActive = false;       // serve requests only when active
static AliasedEntityList gAliasList;
/////////////////////////////


///////////////////////////////////////////
static void Free_AliasedEntity( void* al_ent )
{
    AliasedEntity* ae = (AliasedEntity *)al_ent;

    assert( ae != NULL );
    assert( ae->entity != NULL );

    DBG(
        UpnpPrintf( UPNP_INFO, MSERV, __FILE__, __LINE__,
            "Deleting aliased entity %s\n", ae->alias.c_str()); )

    delete ae->entity;
    delete ae;
}

//////////////////////////////////////////////
static bool Compare_AliasedEntity( void *entityA, void* entityB )
{
    AliasedEntity *a = (AliasedEntity*) entityA;
    AliasedEntity *b = (AliasedEntity*) entityB;

    //DBG(
    //    printf("Compare alias: a = %s, b = %s\n",
    //        a->alias.c_str(), b->alias.c_str() );
    //)

    return a->alias == b->alias;
}

////////////////////////////////////////
// 1) adds leading '/' to entityName if it is absent
// 2) returns index of extension in string after the dot
//     index = -1 if no extension found
static int GetExtensionIndex( INOUT xstring& entityName )
{
    const char *ptr;
    const char *dot_ptr;
    const char *save_dot_ptr;
    int extensionIndex;

    // doc name starts after last '/'
    ptr = strrchr( entityName.c_str(), '/' );
    if ( ptr == NULL )
    {
        // no '/'; insert one at start
        entityName.insert( '/', 0 );

        ptr = entityName.c_str();
    }

    // precond: ptr points to last '/'
    assert( *ptr == '/' );

    //
    // look for last '.'
    //

    dot_ptr = ptr;
    save_dot_ptr = NULL;

    // scan left-to-right so that a dot before '/' is
    //  not detected accidentally
    while ( true )
    {
        dot_ptr = strchr( dot_ptr, '.' );
        if ( dot_ptr == NULL )
        {
            break;
        }
        save_dot_ptr = dot_ptr;
        dot_ptr++;  // skip dot
    }

    if ( save_dot_ptr == NULL )
    {
        // dot is not present
        extensionIndex = -1;
    }
    else
    {
        // determine index of dot in string
        extensionIndex = save_dot_ptr - entityName.c_str();
    }

    return extensionIndex;
}

struct FileExtensionType
{
    char* ext;
    char* type;
    char* subtype;
};

static char* AudioStr = "audio";
static char* VideoStr = "video";
static char* ImageStr = "image";
static char* ApplicationStr = "application";
static char* TextStr = "text";

#define NUM_MEDIA_TYPES 35
static FileExtensionType MediaTypeList[ NUM_MEDIA_TYPES ] =
{
    { "aif",  AudioStr,         "x-aiff" },
    { "au",   AudioStr,         "basic" },
    { "avi",  VideoStr,         "msvideo" },

    { "gif",  ImageStr,         "gif" },

    { "htm",  TextStr,          "html" },
    { "html", TextStr,          "html" },

    { "jar",  ApplicationStr,   "java-archive" },
    { "jpeg", ImageStr,         "jpeg" },
    { "jpg",  ImageStr,         "jpeg" },
    { "jpe",  ImageStr,         "jpeg" },
    { "jfif", ImageStr,         "jpeg" },

    { "mid",  AudioStr,         "midi" },
    { "midi", AudioStr,         "midi" },
    { "mp3",  AudioStr,         "mp3"  },
    { "mpeg", VideoStr,         "mpeg" },
    { "mpg",  VideoStr,         "mpeg" },
    { "mpv",  VideoStr,         "mpeg" },
    { "mpe",  VideoStr,         "mpeg" },
    { "mov",  VideoStr,         "quicktime" },

    { "pdf",  ApplicationStr,   "pdf" },
    { "pjp",  ImageStr,         "jpeg" },
    { "pjpeg",ImageStr,         "jpeg" },
    { "png",  ImageStr,         "png" },

    { "qt",   VideoStr,         "quicktime" },

    { "ram",  AudioStr,         "x-pn-realaudio" },
    { "rmi",  AudioStr,         "mid"  },
    { "rmm",  AudioStr,         "x-pn-realaudio" },

    { "shtml",TextStr,          "html" },

    { "tar",  ApplicationStr,   "tar" },
    { "text", TextStr,          "plain" },
    { "txt",  TextStr,          "plain" },

    { "wav",  AudioStr,         "wav" },

    { "xml",  TextStr,          "xml" },
    { "xsl",  TextStr,          "xml" },

    { "zip",  ApplicationStr,   "zip" },
};

static int SearchExtension( IN const char* fileExtension,
    IN FileExtensionType* table, IN int size )
{
    int top, mid, bot;
    int cmp;
    top = 0;
    bot = size - 1;

    while ( top <= bot )
    {
        mid = ( top + bot ) / 2;
        cmp = strcasecmp( fileExtension, table[mid].ext );

        if ( cmp > 0 )
        {
            top = mid + 1;      // look below mid
        }
        else if ( cmp < 0 )
        {
            bot = mid - 1;      // look above mid
        }
        else    // cmp == 0
        {
            return mid; // match
        }
    }
    return -1;
}

// if fileExtension not found, type is application/octet-stream
static void GetMediaType( IN const char* fileExtension,
    OUT xstring& contentType, OUT xstring& contentSubtype )
{
    int index;

    index = SearchExtension( fileExtension, MediaTypeList,
        NUM_MEDIA_TYPES );

    if ( index == -1 )
    {
        contentType = ApplicationStr;
        contentType = "octet-stream";
    }
    else
    {
        FileExtensionType* ext;

        assert( index >= 0 && index < NUM_MEDIA_TYPES );
        ext = &MediaTypeList[index];
        contentType = ext->type;
        contentSubtype = ext->subtype;
    }
}


////////////////////////////////////////
// AliasedEntityList
////////////////////////////////////////
AliasedEntityList::AliasedEntityList()
    : aliasList( Free_AliasedEntity, Compare_AliasedEntity )
{
#ifdef USE_PTHREADS
    if ( pthread_mutex_init(&mutex, NULL) != 0 )
    {
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
        throw OutOfMemoryException( "AliasedEntityList::AliasedEntityList()" );
    }
#else
	cyg_mutex_init(&mutex);
#endif
}

////////////////////////////////////////
AliasedEntityList::~AliasedEntityList()
{
#ifdef USE_PTHREADS
    while ( pthread_mutex_destroy(&mutex) == EBUSY )
    {
        // highly unlikely -- but still possible to return EBUSY
        sleep( 1 );
    }
#else
	cyg_mutex_destroy(&mutex);
#endif
}

////////////////////////////////////////
EDERRCODE AliasedEntityList::addEntity( IN const char* alias,
    IN HttpEntity* entity, OUT xstring& actualAlias )
{
    dblListNode *node;
    AliasedEntity* ae;
    int eCode = ED_OK;
    int dot_index;
    xstring contentType, contentSubtype;

    assert( alias != NULL );
    assert( entity != NULL );

#ifdef USE_PTHREADS
	pthread_mutex_lock( &mutex );
#else
	cyg_mutex_lock( &mutex );
#endif
//	try
//	{
		ae = new AliasedEntity;
		if ( ae == NULL )
		{
//			diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//			throw -5;       // no mem
			DEBUG_THROW_OUT_OF_MEMORY_EXCEPTION("AliasedEntityList::addEntity()");
			eCode = -5;
			goto CatchError;
		}

		actualAlias = alias;

		dot_index = GetExtensionIndex( actualAlias );

		node = find( actualAlias.c_str() );
		if ( node != NULL )
		{
			// duplicate alias
			// add id to make it unique

			// allocated entity -- to -- integer string
			char buf[50];
			sprintf( buf, "-%u", (unsigned)ae );

			if ( dot_index == -1 )
			{
				actualAlias += buf; // no dot; append
			}
			else
			{
				actualAlias.insert( buf, dot_index );

				// determine new dot index
				dot_index = GetExtensionIndex( actualAlias );
			}

			assert( find(actualAlias.c_str()) == NULL );
		}

		DBG(
			UpnpPrintf( UPNP_INFO, MSERV, __FILE__, __LINE__,
				"actual alias = %s\n", actualAlias.c_str()); )

		// fill values
		ae->alias = actualAlias;
		ae->entity = entity;
		ae->modifiedTime = time( NULL );
		ae->count = 1;
		ae->stopServing = false;
		if ( dot_index == -1 )
		{
			ae->contentType = ApplicationStr;
			ae->contentSubtype = "octet-stream";
		}
		else
		{
			GetMediaType( actualAlias.c_str() + dot_index + 1,
				ae->contentType,
				ae->contentSubtype );
		}
		aliasList.addAfterTail( ae );
/*
	}
	catch ( int excepCode )
	{
		eCode = excepCode;
	}
*/
CatchError:

#ifdef USE_PTHREADS
	pthread_mutex_unlock( &mutex );
#else
	cyg_mutex_unlock( &mutex );
#endif

    if ( eCode == -5 )
    {
/*
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
        throw OutOfMemoryException("AliasedEntityList::addEntity()" );
*/
		return EDERR_OUT_OF_MEMORY;
    }
	return eCode;
}

////////////////////////////////////////
bool AliasedEntityList::grabEntity( IN const char* alias,
    OUT HttpEntity** entity, OUT xstring& contentType,
    OUT xstring& contentSubtype,
    OUT time_t& lastModifiedTime )
{
	dblListNode *node;
	AliasedEntity* ae;
	int eCode = 0;

#ifdef USE_PTHREADS
	pthread_mutex_lock( &mutex );
#else
	cyg_mutex_lock( &mutex );
#endif

	node = find( alias );
	if ( node == NULL )
	{
		eCode = -2;		// alias not found -- E_NOT_FOUND
		goto CatchError;
	}

	ae = (AliasedEntity*) (node->data);
	assert( ae != NULL );

	// if not serving anymore - fake not found
	if ( ae->stopServing )
	{
		eCode = -2;		// alias not found -- E_NOT_FOUND
		goto CatchError;
	}

	*entity = ae->entity;
	contentType = ae->contentType;
	contentSubtype = ae->contentSubtype;
	lastModifiedTime = ae->modifiedTime;
	ae->count++;

CatchError:

#ifdef USE_PTHREADS
	pthread_mutex_unlock( &mutex );
#else
	cyg_mutex_unlock( &mutex );
#endif

	return (eCode == 0);
}

////////////////////////////////////////
EDERRCODE AliasedEntityList::releaseEntity( IN const char* alias,
    bool stopServing )
{
    dblListNode * node;
    AliasedEntity* ae;
    int eCode;

#ifdef USE_PTHREADS
	pthread_mutex_lock( &mutex );
#else
	cyg_mutex_lock( &mutex );
#endif

//	try
//	{
		node = find( alias );

		if ( node == NULL )
		{
//			diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//			throw -2;       // alias not found
			eCode = -2;
			goto CatchError;
		}

		ae = (AliasedEntity *)(node->data);

		ae->stopServing = stopServing;

		assert( ae->count > 0 );
		if ( --ae->count <= 0 )
		{
			aliasList.remove( node );
		}

		eCode = ED_OK;
/*
	}
	catch ( int excepCode )
	{
		eCode = excepCode;
	}
*/
CatchError:

#ifdef USE_PTHREADS
	pthread_mutex_unlock( &mutex );
#else
	cyg_mutex_unlock( &mutex );
#endif

	if ( eCode == -2 )
	{
//		AliasedListException e("releaseEntity(): alias not found");
//		e.setErrorCode( E_NOT_FOUND );
//		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//		throw e;
		return -2;
	}

	return eCode;
}

////////////////////////////////////////
bool AliasedEntityList::isAliasPresent( IN const char* alias )
{
    dblListNode *node;

#ifdef USE_PTHREADS
	pthread_mutex_lock( &mutex );
#else
	cyg_mutex_lock( &mutex );
#endif
    node = find(alias);
//	pthread_mutex_lock( &mutex );
#ifdef USE_PTHREADS
	pthread_mutex_unlock( &mutex );
#else
	cyg_mutex_unlock( &mutex );
#endif

    return node != NULL;
}

////////////////////////////////////////
dblListNode* AliasedEntityList::find( IN const char* alias )
{
    AliasedEntity key;
    dblListNode* result;

    key.alias = alias;
    result = aliasList.find( &key );

    return result;
}
///////////////////////////////////////
void http_SetRootDir( const char* httpRootDir )
{
    if ( httpRootDir == NULL )
    {
        // deactivate server
        gServerActive = false;
        gRootDocumentDir = "/###***%////";
    }
    else
    {
        xstring &s = gRootDocumentDir;

        s = httpRootDir;

        // remove trailing '/', if any
        if ( s.length() > 0 )
        {
            int index = s.length() - 1;
            if ( s[index] == '/' )
            {
                s.deleteSubstring( index, 1 );
            }
        }
        DBG(
            UpnpPrintf( UPNP_INFO, MSERV, __FILE__, __LINE__,
                "http_SetRootDir(): root = '%s'\n", s.c_str()); )

        gServerActive = true;
    }
}

////////////////////////////////////////////
// returns:
//  1: 'filename' is a regular file
//  2: 'filename' is a directory (fileLength, last_modified not returned)
// -1: filename in neither a file nor a directory
//
#ifndef USE_FILESYSTEM
//#include "XMLDocs.h"
#endif	// USE_FILESYTEM

static int GetFileInfo( IN const char* filename,
    OUT int& fileLength, OUT time_t& last_modified )
{
    int code;
    struct stat s;
    int fileType;

#ifdef USE_FILESYSTEM
    code = stat( filename, &s );

    if ( code == -1 )
    {
        return -1;
    }

    if ( S_ISREG(s.st_mode) )
    {
        fileType = 1;   // reg file
        // DBG( printf("is regular file\n"); )
    }
    else if ( S_ISDIR(s.st_mode) )
    {
        fileType = 2;
        //DBG( printf("is directory\n"); )
        return fileType;
    }
    else
    {
        //DBG( printf("invalid file type\n"); )
        return -1;
    }

    fileLength = s.st_size;
    //DBG( printf("fileLength = %d\n", fileLength); )

    last_modified = s.st_mtime;


    //DBG(
    //struct tm* last_mod;
    //last_mod = gmtime( &s.st_mtime );
    //printf("last mod = %s\n", asctime(last_mod) );
    //)

    return fileType;

#else	// USE_FILESYSTEM

	fileLength = FakeGetFileLength(filename) - 1;
	if (fileLength > 0)
		return 1;
	else
		return -1;

#endif	// USE_FILESYSTEM
}

////////////////////////////////////////////
// determines resource filename and its length from given request
//
// throws HttpServerException
//    HTTP_BAD_REQUEST,
//    HTTP_NOT_FOUND,
//    HTTP_INTERNAL_SERVER_ERROR
//    HTTP_FORBIDDEN
static EDERRCODE GetFilename( IN HttpMessage& request,
    OUT xstring& filename, OUT int& fileLength,
    OUT time_t& last_modified )
{
	xstring resourceName;
	uri_type* uri;
	HttpServerException e;

	uri = &request.requestLine.uri.uri;

	resourceName.copyLimited( uri->pathquery.buff, uri->pathquery.size );

//	try
//	{
		// security: do not allow access to files above the
		//   http root dir
		//if ( strstr(resourceName.c_str(), "..") != NULL )
		//{
		//    throw HTTP_FORBIDDEN;
		//}

		if ( request.requestLine.pathIsStar ||
			 resourceName.length() == 0 ||
			 resourceName[0] != '/' )
		{
//			diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//			throw HTTP_BAD_REQUEST;
			DEBUG_THROW_HTTP_BAD_REQUEST_EXCEPTION;
			return EDERR_HTTP_BAD_REQUEST_EXCEPTION;
		}

		// get file info
		{
			const char* tempResPtr;
			xstring resName;
			char *tempBuf;
			int code;

			resName = resourceName;
			tempBuf = resName.detach();
			code = remove_dots( tempBuf, strlen(tempBuf) );
			resourceName =  tempBuf;
			free( tempBuf );
			if ( code != 0 )
			{
//				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//				throw HTTP_FORBIDDEN;
				DEBUG_THROW_HTTP_FORBIDDEN_EXCEPTION;
				return EDERR_HTTP_FORBIDDEN_EXCEPTION;
			}

			filename = gRootDocumentDir;

			// append '/' suffix
			if ( (gRootDocumentDir.length() > 0 &&
				  gRootDocumentDir[gRootDocumentDir.length()-1] != '/') ||
				 (gRootDocumentDir.length() == 0)
			   )
			{
				filename += '/';
			}

			// skip leading '/'
			tempResPtr = resourceName.c_str();
			tempResPtr += (resourceName[0] == '/' ? 1 : 0);

			filename += tempResPtr;
		}

		int fileType;

		fileType = GetFileInfo( filename.c_str(), fileLength, last_modified );
		if ( fileType == -1 )
		{
//			diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//			throw HTTP_NOT_FOUND;   // file not found
			DEBUG_THROW_HTTP_NOT_FOUND_EXCEPTION;
			return EDERR_HTTP_NOT_FOUND_EXCEPTION;
		}
		if ( fileType == 2 )
		{
			// handle dir; use default index.html

			// add '/' suffix
			if ( filename[filename.length()-1] != '/' )
			{
				filename += '/';
			}
			filename += "index.html";

			fileType = GetFileInfo( filename.c_str(), fileLength,
				last_modified );

			if ( fileType != 1 )
			{
				// not a file
//				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//				throw HTTP_NOT_FOUND;
				DEBUG_THROW_HTTP_NOT_FOUND_EXCEPTION;
				return EDERR_HTTP_NOT_FOUND_EXCEPTION;
			}
		}

		// make sure file is readable
		{
#ifdef USE_FILESYSTEM
			FILE *fp;

			fp = fopen( filename.c_str(), "r" );
			if ( fp == NULL )
			{
//				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//				throw HTTP_FORBIDDEN;
				DEBUG_THROW_HTTP_FORBIDDEN_EXCEPTION;
				return EDERR_HTTP_FORBIDDEN_EXCEPTION;
			}

			fclose( fp );
#endif	// USE_FILESYSTEM
		}

		assert( fileType == 1 );    // regular file

		DBG(
			UpnpPrintf( UPNP_INFO, MSERV, __FILE__, __LINE__,
				"GetFileName(): file: %s\n", filename.c_str()); )
//	}
/*
	catch ( int code )
	{
		//DBG( perror("GetFilename()"); )
		DBG(
			UpnpPrintf( UPNP_INFO, MSERV, __FILE__, __LINE__,
				"GetFileName() error: %s\n", strerror(errno)); )

		e.setErrorCode( code );
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
		throw e;        
	}
*/

	return ED_OK;
}

////////////////////////////////////////////
// Sees if request doc is an alias. If it is, the entity is
//  loaded into the response.
//
// returns:
//   0: success; loaded alias in request
//  -1: did not find alias
static int TryToGetAlias( IN HttpMessage& request,
    OUT HttpMessage& response, OUT xstring& aliasOut )
{
    xstring resourceName;
    uri_type* uri;
    int retCode = 0;
    char *tempBuf;

    uri = &request.requestLine.uri.uri;

    resourceName.copyLimited( uri->pathquery.buff, uri->pathquery.size );

    tempBuf = resourceName.detach();
    retCode = remove_dots( tempBuf, strlen(tempBuf) );
    resourceName = tempBuf;
    free( tempBuf );
    if ( retCode != 0 )
    {
        return -1;
    }
    aliasOut = resourceName;


	HttpEntity* entity;
	xstring contentType;
	xstring contentSubtype;
	time_t lastModified;

	if (!gAliasList.grabEntity( resourceName.c_str(), &entity, contentType, contentSubtype, lastModified ))
		return -1;

// TODO: convert rest from exception handling.
	assert( entity != NULL );
	assert( entity->type == HttpEntity::TEXT );

	response.isRequest = false;

	//response.addContentLengthHeader( entity->getEntityLen() );
	response.addNumTypeHeader( HDR_CONTENT_LENGTH,
		entity->getEntityLen() );

	response.addContentTypeHeader( contentType.c_str(),
		contentSubtype.c_str() );

	//response.addLastModifiedHeader( lastModified );
	response.addDateTypeHeader( HDR_LAST_MODIFIED, lastModified );

	if ( request.requestLine.method == HTTP_GET )
	{
		response.entity.setTextPtrEntity( entity->getEntity(),
			entity->getEntityLen() );
	}
	else
	{
		response.entity.type = HttpEntity::EMPTY;
	}

    return retCode;
}

////////////////////////////////////////////
// throws OutOfMemoryException
static EDERRCODE AddContentType( IN const char* fileName,
    OUT HttpMessage& response )
{
    char *extPtr;
    char *type = NULL;
    char *subtype = NULL;
    int index;

    // find last dot
    bool found = false;
        
    extPtr = strrchr( fileName, '.' );
    if ( extPtr != NULL )
    {
        extPtr++;   // skip dot
        
        index = SearchExtension( extPtr, MediaTypeList, NUM_MEDIA_TYPES );
        if ( index != -1 )
        {
            type = MediaTypeList[index].type;
            subtype = MediaTypeList[index].subtype;
            found = true;
        }
    }
        
    if ( !found )
    {
        type = ApplicationStr;
        subtype = "octet-stream";
    }
        
    return response.addContentTypeHeader( type, subtype );
}

////////////////////////////////////////////
// throws HttpServerException
//    HTTP_HTTP_VERSION_NOT_SUPPORTED
static EDERRCODE GetReplyVersion( IN HttpRequestLine& reqLine,
    OUT int& majorVersion, OUT int& minorVersion )
{
	int reqMajor, reqMinor;
	int respMajor = 1, respMinor = 1;

	reqMajor = reqLine.majorVersion;
	reqMinor = reqLine.minorVersion;

	if ( reqMajor == 0 )
	{
		respMajor = reqMajor;
		if ( reqMinor != 9 )
		{
//			HttpServerException e("can't handle http version" );
//			e.setErrorCode( HTTP_HTTP_VERSION_NOT_SUPPORTED );
//			diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//			throw e;
			DEBUG_THROW_HTTP_VERSION_NOT_SUPPORTED_EXCEPTION;
			return EDERR_HTTP_VERSION_NOT_SUPPORTED_EXCEPTION;
		}
		respMinor = reqMinor;
	}
	else if ( reqMajor == 1 )
	{
		respMajor = reqMajor;
		if ( reqMinor == 0 )
		{
			respMinor = reqMinor;
		}
		else
		{
			respMinor = 1;
		}
	}
	else
	{
		respMajor = 1;
		respMinor = 1;
	}

	majorVersion = respMajor;
	minorVersion = respMinor;

	return ED_OK;
}

////////////////////////////////////////////
void free_alias( IN bool usingAlias, IN const char* alias )
{
	if ( !usingAlias )
	{
		return;
	}

//	try
//	{
		if (ED_FAILED(gAliasList.releaseEntity( alias )))
			assert( 0 );    // aliased entity not found
//	}
//	catch ( AliasedListException& /* e */ )
//	{
//		assert( 0 );    // aliased entity not found
//	}
}


////////////////////////////////////////////
// processes GET and HEAD methods
// throws HttpServerException
//    HTTP_BAD_REQUEST
//    HTTP_NOT_FOUND
//    HTTP_INTERNAL_SERVER_ERROR
//    HTTP_NOT_IMPLEMENTED
//    HTTP_HTTP_VERSION_NOT_SUPPORTED
static EDERRCODE ProcessRequest( IN HttpMessage& request,
    OUT HttpMessage& response, OUT bool& usingAlias,
    OUT xstring& aliasOut )
{
	int method;
	int respMajor, respMinor;

	usingAlias = false;

//	try
//	{
		// check method
		method = request.requestLine.method;
		if ( !(method == HTTP_GET || method == HTTP_HEAD) )
		{
//			diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//			throw HTTP_NOT_IMPLEMENTED; // unknown method
			DEBUG_THROW_HTTP_NOT_IMPLEMENTED_EXCEPTION;
			return EDERR_HTTP_NOT_IMPLEMENTED_EXCEPTION;
		}
    
		ED_RETURN_EXCEPTION(GetReplyVersion( request.requestLine, respMajor, respMinor ));

		xstring filename;
		int fileLength;
		time_t lastModified;

		usingAlias = true;
		if ( TryToGetAlias(request, response, aliasOut) != 0 )
		{
			// alias not found -- try files
			usingAlias = false;

			ED_RETURN_EXCEPTION(GetFilename( request, filename, fileLength, lastModified ));

			ED_RETURN_EXCEPTION(AddContentType( filename.c_str(), response ));
        
			// add std response headers
    
			// content-length
			//response.addContentLengthHeader( fileLength );
			ED_RETURN_EXCEPTION(response.addNumTypeHeader( HDR_CONTENT_LENGTH, fileLength ));

			// last modified
			//response.addLastModifiedHeader( lastModified );
			ED_RETURN_EXCEPTION(response.addDateTypeHeader( HDR_LAST_MODIFIED, lastModified ));

			if ( method == HTTP_GET )
			{
				// set http body to be sent
				response.entity.type = HttpEntity::FILENAME;
				response.entity.filename = filename;
			}
			else
			{
				response.entity.type = HttpEntity::EMPTY;
			}
		}

		// date
		//response.addDateHeader();
		ED_RETURN_EXCEPTION(response.addDateTypeHeader( HDR_DATE, time(NULL) ));
    
		// server
		ED_RETURN_EXCEPTION(response.addServerHeader());

		// connection close
		const char *idents[] = {"close"};
		ED_RETURN_EXCEPTION(response.addIdentListHeader( HDR_CONNECTION, idents, 1 ));

		ED_RETURN_EXCEPTION(response.responseLine.setValue( HTTP_OK, respMajor, respMinor ));
		response.isRequest = false;
//	}
/*
	catch ( int errCode )
	{
		HttpServerException e;
		e.setErrorCode( errCode );
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
		throw e;
	}
*/

	DBG(
		UpnpPrintf( UPNP_INFO, MSERV, __FILE__, __LINE__,
			"ProcessRequest(): done\n"); )

	return ED_OK;
}

////////////////////////////////////////////
static void HandleError( IN HttpMessage& request, IN int errCode,
    IN int sockfd )
{
    HttpMessage response;
    int respMajor, respMinor;
    
    response.isRequest = false;

    // if request is invalid, don't use it to get version number
    if ( errCode != HTTP_HTTP_VERSION_NOT_SUPPORTED &&
         errCode != HTTP_BAD_REQUEST                &&
         errCode != HTTP_INTERNAL_SERVER_ERROR
       )
    {
        GetReplyVersion( request.requestLine, respMajor, respMinor );
    }
    else
    {
        respMajor = 1;
        respMinor = 1;
    }
    
    response.responseLine.setValue( errCode, respMajor, respMinor );
    if ( errCode == HTTP_HTTP_VERSION_NOT_SUPPORTED )
    {
        char *reason = "<html><body><h2>Supports HTTP versions 0.9, 1.0 and 1.1</h2></body></html>";
        response.entity.append( reason, strlen(reason) );
        response.entity.type = HttpEntity::TEXT;
        response.addNumTypeHeader( HDR_CONTENT_LENGTH, strlen(reason) );
        response.addContentTypeHeader( "text", "html" );
    }
    else
    {
        xstring htmlDoc;
        xstring numStr;

        IntToStr( response.responseLine.statusCode, numStr );

        htmlDoc = "<html><body><h1>";
        htmlDoc += numStr;
        htmlDoc += ' ';
        htmlDoc += response.responseLine.reason;
        htmlDoc += "</h1></body></html>";

        response.addNumTypeHeader( HDR_CONTENT_LENGTH, htmlDoc.length() );
        response.entity.type = HttpEntity::TEXT;
        response.entity.append( htmlDoc.c_str(), htmlDoc.length() );
    }

    response.addDateTypeHeader( HDR_DATE, time(NULL) );
    response.addServerHeader();

    http_SendMessage( sockfd, response );
}


////////////////////////////////////////////
int http_ServerCallback( IN HttpMessage& request, int sockfd )
{
	HttpMessage response;
	bool usingAlias = false;
	xstring alias;
	int code = 0;

//	try
//	{
		if ( gServerActive == false )
		{
//			HttpServerException e("HTTP GET Server Inactive");
//			e.setErrorCode( HTTP_INTERNAL_SERVER_ERROR );
//			diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//			throw e;
			DEBUG_THROW_HTTP_INTERNAL_SERVER_ERROR_EXCEPTION;
			code = EDERR_HTTP_INTERNAL_SERVER_ERROR_EXCEPTION;
			goto CatchError;
		}

		code = ProcessRequest( request, response, usingAlias, alias );
		if (ED_FAILED(code))
			goto CatchError;

		code = http_SendMessage( sockfd, response );
		if (ED_FAILED(code))
			goto CatchError;
//	}

CatchError:

	if ((code < -400) && (code > -499))
//		HandleError( request, e.getErrorCode(), sockfd );
		HandleError( request, code, sockfd );

	close( sockfd );
	free_alias( usingAlias, alias.c_str() );

	return 0;
}

////////////////////////////////////////////
void http_OldServerCallback( IN const char* msg, int sockfd )
{
    HttpMessage request;
    int code;
	char * endfirstline;
	char firstline[100];

	endfirstline = strstr(msg,"\n");
	strncpy(firstline,msg,endfirstline-msg+1);
	firstline[endfirstline-msg+1] = '\r';
	firstline[endfirstline-msg+2] = '\n';
	firstline[endfirstline-msg+3] = '\0';
    code = request.loadRequest( firstline );

	//de = request.loadRequest( msg


    if ( code < 0 )
    {
        if ( code == HTTP_E_BAD_MSG_FORMAT )
        {
            HandleError( request, HTTP_BAD_REQUEST, sockfd );
        }
        else if ( code == HTTP_E_OUT_OF_MEMORY )
        {
            HandleError( request, HTTP_INTERNAL_SERVER_ERROR, sockfd );
        }

        close( sockfd );
        return;
    }

    http_ServerCallback( request, sockfd );
}

////////////////////////////////////////////
// returns:
//    0
//    HTTP_E_OUT_OF_MEMORY
int http_AddAlias( IN const char* aliasRelURL, IN HttpEntity* entity,
    OUT xstring& actualAlias )
{
	int retCode;

	assert( aliasRelURL != NULL );
	assert( entity != NULL );

//	try
//	{
		if (ED_FAILED(gAliasList.addEntity( aliasRelURL, entity, actualAlias )))
			retCode = HTTP_E_OUT_OF_MEMORY;
		else
			retCode = 0;
//	}
//	catch ( OutOfMemoryException& /* e */ )
//	{
//		retCode = HTTP_E_OUT_OF_MEMORY;
//	}

	return retCode;
}

////////////////////////////////////////////
int http_RemoveAlias( IN const char* alias )
{
	int retCode;

//	try
//	{
		if (ED_FAILED(gAliasList.releaseEntity( alias, true )))
			return -1;		// not found
		else
			return 0;		// removed
//	}
//	catch ( AliasedListException& /* e */ )
//	{
//		retCode = -1;       // not found
//	}

//	return retCode;
}
#endif
#endif
