#include "cd9660_support.h"
#include <cyg/error/codes.h>

/* Find first set bit */
int
ffs(int value) 
{
    int retval = 1;
    while ((value & 0x1) == 0) {
	++retval;
	value >>= 1;
    }
    return retval;
}

int
copystr(const void *from,
	void *to,
	size_t maxlen,
	size_t *lencopied)
{
    int byte = 0;
    int len;
    
    len = 0;

    do {
	if (len == maxlen)
	    break;
	byte = *((char *)from)++;
	*((char *)to)++ = byte;
	++len;
    } while (byte != 0);

    if (lencopied)
	*lencopied = len;
     
    if (byte == 0)
	return(0);
    else
	return(ENAMETOOLONG);
}

int
cd9660_uiomove(char * cp, int n, cyg_uio *uio)
{
    cyg_iovec *iov;
    unsigned int cnt;
    int error = 0;

    while (n > 0 && uio->uio_resid) {
        iov = uio->uio_iov;
        cnt = iov->iov_len;
        if (cnt == 0) {
            uio->uio_iov++;
            uio->uio_iovcnt--;
            continue;
        }
        if (cnt > n)
            cnt = n;
        switch (uio->uio_segflg) {	    
            case UIO_USERSPACE:
            case UIO_SYSSPACE:
                if (uio->uio_rw == UIO_READ)
                    bcopy((char *)cp, iov->iov_base, cnt);
                else
                    bcopy(iov->iov_base, (char *)cp, cnt);
                break;
        }
        iov->iov_base += cnt;
        iov->iov_len -= cnt;
        uio->uio_resid -= cnt;
        uio->uio_offset += cnt;
        cp += cnt;
        n -= cnt;
    }
    return (error);
}
