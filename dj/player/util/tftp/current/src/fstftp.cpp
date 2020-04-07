#include <tftp_support.h>
#include <util/tftp/fstftp.h>
#include <stdlib.h>
#include <stdio.h>
#include <fs/fat/sdapi.h>


static int fs_open(const char *, int);
static int fs_close(int);
static int fs_write(int, const void *, int);
static int fs_read(int, void *, int);

struct tftpd_fileops fs_file_ops = {
    fs_open, fs_close, fs_write, fs_read

};

void fs_build_path(const char * fn)
{
	char *dirName=(char*)malloc(120);

	char *dir;
	bool res = true;

	dir = strstr(fn,"/");
	dir++;

	if (dir)
	{
		while (res && (dir = strstr(dir,"/")))
		{
			strncpy(dirName,fn,dir-fn);
			dirName[dir-fn]='\0';
			res = pc_mkdir(dirName);
			dir++;
		}
	}
	free(dirName);
}




static int
fs_open(const char *fn, int flags)
{

	int localFlags = 0, localMode = 0;

	int fd;

	// translate the flags to our filesystem

	if (flags == O_RDONLY)
	{
		localFlags = PO_RDONLY;
		localMode  = PS_IREAD;
	}

	if (flags == O_WRONLY)
	{
		localFlags = PO_WRONLY | PO_CREAT | PO_TRUNC | PO_NOSHAREANY;
		localMode  = PS_IWRITE;

		// now try to create the path to the file

		fd = po_open(fn,localFlags,localMode);

		if (fd>=0)
		{
			// seemed to work, so return the file descriptor
			return fd;
		}
		
		// the path doesn't exist, so lets build it

		fs_build_path(fn);
	}

	// now let the file system open the file

    fd = po_open(fn,localFlags,localMode);

    return fd;
}

static int
fs_close(int fd)
{
	return  po_close(fd);
}

static int 
fs_write(int fd, const void *buf, int len)
{
	return po_write(fd, (UCHAR*)buf,len);

}

static int
fs_read(int fd, void *buf, int len)
{
	return po_read(fd,(UCHAR*)buf,len);
}

static int iServerID = -1;

int StartTFTPService()
{
	iServerID = tftpd_start(TFTP_PORT,&fs_file_ops);
	return iServerID;
}

void StopTFTPService()
{
    if(iServerID >= 0) {
        tftpd_stop(iServerID);
        iServerID = -1;
    }
}
