#ifndef CD9660_SUPPORT_H
#define CD9660_SUPPORT_H

#include <string.h>
#include <time.h>
#include <cyg/fileio/fileio.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* TODO Do this the correct way */
#define hostname "dadio"
#define hostnamelen 5

#define MAXSYMLINKS 8
    
#define min(a,b) (((a) < (b)) ? (a) : (b))

#ifndef bcopy
#define bcopy(src, dst, size) memcpy(dst, src, size)
#endif
#ifndef bzero
#define bzero(dst, len) memset(dst, 0, len)
#endif
#define bcmp memcmp

#define	DEV_BSHIFT	9		/* log2(DEV_BSIZE) */
#define	DEV_BSIZE	(1 << DEV_BSHIFT)

int ffs(int value);
int copystr(const void *from, void *to, size_t maxlen, size_t *lencopied);
int cd9660_uiomove(char * cp, int n, cyg_uio *uio);	/* ecm - name conflict */
    
#ifdef __cplusplus
};
#endif /* __cplusplus */
	
#endif /* CD9660_SUPPORT_H */
