// eth_test.c: test out ethernet w/ fat
// danc@iobjects.com

#include <cyg/kernel/kapi.h>
#include <malloc.h>
#include <stdio.h>

#include <pkgconf/system.h>

#ifndef CYGPKG_NET
#error "You must build against a network enabled kernel"
#else
#include <network.h>
#endif

#include <fs/fat/sdapi.h>
#include <util/debug/debug.h>

DEBUG_MODULE(ETEST);
DEBUG_USE_MODULE(ETEST);

#define STACKSIZE (8192 * 6)

typedef struct thread_data_s 
{
    cyg_handle_t threadh;
    cyg_thread   thread;
    char         tstack[STACKSIZE];
    void*        arg;
} thread_data_t;

static void create_thread( thread_data_t* , void* , int );
static void handle_client( cyg_uint32 data );

static thread_data_t* new_thread_data( void );
static void end_thread( thread_data_t* );
static void release_threads( void );


// Main routine
void thread_entry( cyg_uint32 data ) 
{
    thread_data_t* srv_thread;
    struct sockaddr_in saddr;
    int listenfd;

    init_all_network_interfaces();
    init_eth0();
    //    init_eth1();

    pc_system_init( 0 );
    
    listenfd = socket( AF_INET, SOCK_STREAM, 0 );

    memset( &saddr, 0, sizeof( struct sockaddr_in ) );
    saddr.sin_family      = AF_INET;
    saddr.sin_addr.s_addr = htonl( INADDR_ANY );
    saddr.sin_port        = htons( 80 );

    if( bind( listenfd, (struct sockaddr*) &saddr, sizeof( struct sockaddr_in ) ) == -1 ) {
        DEBUG(ETEST, DBGLEV_FATAL, "Couldn't bind server port\n");
        return ;
    }

    listen( listenfd, 10 );
    
    DEBUG(ETEST, DBGLEV_WARNING, "eth_test listening for connections on port 80\n");
    while( true ) {
        int connectfd;
        struct sockaddr_in caddr;
        socklen_t clen;
        int a,b,c,d;

        // kill all zombie threads
        release_threads();

        while( true ) {
            connectfd = accept( listenfd, (struct sockaddr*) &caddr, &clen );

            if( connectfd > 0 ) {
                break;
            }
        }
        
        // no doubt i am supposed to use htonl here first. :p
        a = (caddr.sin_addr.s_addr) & 0xff;
        b = (caddr.sin_addr.s_addr >>  8) & 0xff;
        c = (caddr.sin_addr.s_addr >> 16) & 0xff;
        d = (caddr.sin_addr.s_addr >> 24);
        
        DEBUG(ETEST, DBGLEV_WARNING, "Accepted connection from %d.%d.%d.%d\n", a,b,c,d );
        
        // spawn a thread to handle this connection
        srv_thread = new_thread_data();
        srv_thread->arg = (void*)connectfd;
        create_thread( srv_thread, handle_client, 10 );
    }
}

// support routines
static void transfer_file( int fd, char* rqst, char* data, int len );
static void list_directory( int fd, char* rqst, char* data, int len );

// cgi-type routines
static void delete_file( int fd, char* rqst, char* data, int len );
static void upload_file( int fd, char* rqst, char* data, int len );

// mini cgi type engine
typedef void (*p_cgi_fun)( int fd, char* rqst, char* data, int len );
typedef struct cgi_table_s 
{
    p_cgi_fun cgi_func;
    const char* cgi_name;
} cgi_table_t;

static const cgi_table_t cgi_table[] = 
{
    { delete_file,  "delete.cgi", },
    { upload_file,  "upload.cgi", },
    { 0,            0,            },
};
    

static void list_directory( int fd, char* rqst, char* data, int len ) 
{
    // request for a directory listing
    DSTAT statobj;
    char path[64];
    char* p;
    char* resp;
    
    sprintf( path, "A:%s\\*.*", rqst );
    for( p = path; *p; p++ ) {
        if( *p == '/' ) *p = '\\';
    }
    if( !pc_gfirst( &statobj, path ) ) {
        resp = "HTTP/1.1 404 Not found\r\nContent-type: text/html\r\nConnection: close\r\n\r\n";
        write( fd, resp, strlen( resp ) );
        resp = "<html><head><title>Not Found</title></head><body>HTTP 404: The file you requested was not found</body></html>";
        write( fd, resp, strlen( resp ) );
        return ;
    }
    resp = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";
    write( fd, resp, strlen( resp ) );
    resp =
        "<html><head><title>Directory listing</title></head><body>" \
        "<h1>Index of ";
    write( fd, resp, strlen( resp ) );
    write( fd, rqst, strlen( rqst ) );
    resp = "</h1><br><br><table cellpadding=0 cellspacing=0>";
    write( fd, resp, strlen( resp ) );
    
    do {
        char link[256];
        char* dirchar = "";
        int i, is_diritem = 0;
        if( statobj.fattribute & ADIRENT ) {
            dirchar = "/";
            is_diritem = 1;
        }
        // fix the statobj. sandisk puts trailing spaces in :<
        for( i = 2; i >= 0; i-- ) {
            if( statobj.fext[i] == ' ' ) statobj.fext[i] = 0;
            else break;
        }
        for( i = 7; i >= 0; i-- ) {
            if( statobj.fname[i] == ' ' ) statobj.fname[i] = 0;
            else break;
        }
        
        if( statobj.fext && statobj.fext[0] ) {
            sprintf( link, "<tr><td><pre><a href=\"%s%s.%s%s\">%s%s.%s%s</a></pre></td>",
                     rqst, statobj.fname, statobj.fext, dirchar,
                     rqst, statobj.fname, statobj.fext, dirchar );
        }
        else {
            sprintf( link, "<tr><td><pre><a href=\"%s%s%s\">%s%s%s</a></pre></td>",
                     rqst, statobj.fname, dirchar,
                     rqst, statobj.fname, dirchar );
        }
        write( fd, link, strlen( link ) );
        
        if( !is_diritem ) {
            // write out a delete link and a file size
            if( statobj.fext && statobj.fext[0] ) {
                sprintf( link,
                         "<td><pre><a href=\"/delete.cgi?%s%s.%s\">[delete]</a></pre></td>"
                         "<td><pre>%ld bytes</pre></td></tr>",
                         rqst, statobj.fname, statobj.fext,
                         statobj.fsize );
            }
            else {
                sprintf( link, "<td><pre><a href=\"/delete.cgi?%s%s\">[delete]</a></pre></td></tr>",
                         rqst, statobj.fname );
            }
        } else {
            // write out a /tr
            sprintf( link, "</tr>" );
        }
        write( fd, link, strlen( link ) );
    } while( pc_gnext( &statobj ) );
    pc_gdone( &statobj );
    
    resp = "</table><br><br>End of listing<br><br>";
    write( fd, resp, strlen( resp ) );

    resp = "Upload a file...<form method=post action=/upload.cgi><input type=file name=fileitem><input type=submit value=\"Upload\"></form>";
    write( fd, resp, strlen( resp ) );
    
    resp = "</body></html>";
    write( fd, resp, strlen( resp ) );
}
static void transfer_file( int fd, char* rqst, char* data, int len )
{
    // request for a specific file
    PCFD file;
    char b[128];
    char* p, *ctype, *resp;
    int length;
    char buf[16384];
    
    sprintf( b, "A:%s", rqst );
    for( p = b; *p; p++ ) {
        if( *p == '/' ) *p = '\\';
    }
    
    file = po_open( b, PO_RDONLY, PS_IREAD );
    if( file < 0 ) {
        // failed to open file
        resp = "HTTP/1.1 404 Not Found\r\nContent-type: text/html\r\nConnection: close\r\n\r\n";
        write( fd, resp, strlen( resp ) );
        resp = "<html><head><title>Not Found</title></head><body>HTTP 404: The file you requested was not found</body></html>";
        write( fd, resp, strlen( resp ) );
        return;
    }
    resp = "HTTP/1.1 200 OK\r\n";
    write( fd, resp, strlen( resp ) );
    // Try and be good about content type, length, etc
    {
        // find content length
        short ignored;
        length = po_lseek( file, 0, PSEEK_END, &ignored );
        po_lseek( file, 0, PSEEK_SET, &ignored );
    }
    // find content type
    p -= 3;
    if( strnicmp( p, "wma", 3 ) == 0 ) {
        ctype = "audio/x-ms-wma";
    }
    else if( strnicmp( p, "mp3", 3 ) == 0 ) {
        ctype = "audio/mpeg";
    }
    else if( strnicmp( p, "wav", 3 ) == 0 ) {
        ctype = "audio/wav";
    }
    else {
        ctype = "text/plain";
    }
    
    sprintf( b, "Content-Type: %s\r\nContent-Length: %d\r\n\r\n", ctype, length );
    
    write( fd, b, strlen( b ) );
    while( length > 0 ) {
        int c = po_read( file, buf, 16384 );
        length -= c;
        write( fd, buf, c );
    }
}
//
// psuedo-cgi
//
static void delete_file( int fd, char* rqst, char* data, int len ) 
{
    // delete request
    // get a ptr to the filename
    char* p = rqst;
    char path[128];
    int i;
    
    sprintf( path, "A:" );
    strcat( path, p );
    for( i = 0; i < strlen( path ); i++ ) {
        if( path[i] == '/' ) {
            path[i] = '\\';
        }
    }

    if( !pc_unlink( path ) ) {
        diag_printf( " unlink failed on %s\n", path );
    }
    
    // figure out the directory and list it;
    for( i = strlen( path ); i >= 0; i-- ) {
        if( path[i] == '\\' ) break;
    }
    
    if( i < 0 ) {
        // fuck!
        diag_printf(" couldn't determine current directory\n");
        return ;
    }
    path[i+1] = 0;
    for( i = 0; i < strlen( path ); i++ ) {
        if( path[i] == '\\' ) {
            path[i] = '/';
        }
    }
    diag_printf(" %s\n", path+2 );
    list_directory( fd, path+2, data, len );
}

static void upload_file( int fd, char* rqst, char* data, int len ) 
{
    char buf[256];
    int i, count;
    while( (count = read( fd, buf, 256 )) > 0 ) {
        for( i = 0; i < count-7; i += 8 ) {
            if( (i % 8) == 0 ) {
                diag_printf("\n");
            }
            diag_printf("%06x\t%02x %02x %02x %02x  %02x %02x %02x %02x",
                        i, buf[i],buf[i+1],buf[i+2],buf[i+3],
                        buf[i+4],buf[i+5],buf[i+6],buf[i+7] );
            diag_printf("\t%c%c%c%c%c%c%c%c\n",
                        buf[i  ],buf[i+1],buf[i+2],buf[i+3],
                        buf[i+4],buf[i+5],buf[i+6],buf[i+7] );
        }
    }
}

#define REQ_TYPE_GET   0x01
#define REQ_TYPE_POST  0x02
static void handle_client( cyg_uint32 data ) 
{
    thread_data_t* pThreadData = (thread_data_t*) data;
    int fd = (int) pThreadData->arg;
    int keepalive = 0;
    char buf[512];
    
    do {
        char* line, *nextline, *rqst;
        int req_type;
        int count = read( fd, buf, 512 );
        DEBUG(ETEST, DBGLEV_WARNING, "%s started, listenfd = %d, request size = %d\n", __FUNCTION__, fd, count );

        if( count > 0 ) {
            line = buf;
        } else {
            break;
        }

        req_type = 0;
        // step through the request
        while( line ) {
            nextline = strchr( line, '\r' );
            if( nextline ) {
                *nextline = 0;
                // HTTP standard = "\r\n"
                nextline += 2;
            }
            if( strncmp( line, "GET", 3 ) == 0 ) {
                // request for a resource
                // move past the "GET "
                req_type = REQ_TYPE_GET;
                rqst = line + 4;
            }
            else if( strncmp( line, "POST", 4 ) == 0 ) {
                // data post
                // move past the "POST "
                req_type = REQ_TYPE_POST;
                rqst = line + 5;
            }
            else if( strncmp( line, "Connection: Keep-Alive", 22 ) == 0 ) {
                keepalive = 1;
            } else if( strncmp( line, "\r\n", 2 ) == 0 ) {
                break;
            }
            line = nextline;
        }

        if( req_type && rqst ) {
            char* spc;
            int len, z;
            DEBUG(ETEST, DBGLEV_WARNING, "HTTP Request: %s\n", rqst);
            spc = strchr( rqst, ' ');
            if( spc ) *spc = 0;
            len = strlen( rqst );

            // try and process the request - look through CGI support
            for( z = 0; cgi_table[z].cgi_name; z++ ) {
                if( strncmp( rqst+1, cgi_table[z].cgi_name, strlen( cgi_table[z].cgi_name ) ) == 0 ) {
                    cgi_table[z].cgi_func( fd, rqst + (strlen(cgi_table[z].cgi_name) + 2 ), nextline, 0 );
                    break;
                }
            }

            if( !cgi_table[z].cgi_name ) {
                // if cgi failed, look for other stuff here
                if( rqst[len-1] == '/' ) {
                    // request for a directory listing
                    list_directory( fd, rqst, nextline, 0 );
                } else {
                    transfer_file( fd, rqst, nextline, 0 );
                }
            }
        }
    } while( keepalive );
    
    close( fd );
    end_thread( pThreadData );
}

// Support routines/data

static thread_data_t main_thread;
static cyg_mbox release_queue;
static cyg_handle_t release_mbox;

static thread_data_t* new_thread_data( void ) 
{
    return (thread_data_t*) malloc( sizeof( thread_data_t ) );
}

static void end_thread( thread_data_t* pThreadData ) 
{
    cyg_mbox_put( release_mbox, (void*) pThreadData );
}
static void release_threads( void ) 
{
    thread_data_t* pThread;
    
    while( ( pThread = cyg_mbox_tryget( release_mbox )) ) {
        while( !cyg_thread_delete( pThread->threadh ) ) {
            cyg_thread_delay( 1 );
        }
        free( (void*) pThread );
    }
}

static void create_thread( thread_data_t* pThreadData, void* pFun, int priority ) 
{
    cyg_thread_create(priority, pFun, (cyg_uint32)pThreadData, "some thread",
                      (void*)(pThreadData->tstack), STACKSIZE,
                      &(pThreadData->threadh), &(pThreadData->thread) );
    cyg_thread_resume( pThreadData->threadh );
}

void cyg_user_start(void) 
{
    cyg_mbox_create( &release_mbox, &release_queue );
    
    main_thread.arg = NULL;
    create_thread( &main_thread, thread_entry, 10 );
}



