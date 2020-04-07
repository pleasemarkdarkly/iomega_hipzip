// eresult.h: definitions for the ERESULT error system
// danc@iobjects.com 07/12/01
// (c) Interactive Objects

#ifndef __ERESULT_H__
#define __ERESULT_H__

//
// Every distinct module should define its own zone for eresults,
// and a set of codes within that zone. The code by itself is not
// unique, in that every zone will likely define error codes that
// start at 0 and increment. the combination of zone+code, however,
// is certainly unique
//

typedef int ERESULT;

#define MAKE_ERESULT( severity, zone, code )        \
        (ERESULT)(((unsigned int)(severity)<<24) |  \
                  ((unsigned int)(zone)<<16)     |  \
                  ((unsigned int)(code)))

#define SUCCEEDED( res ) ((ERESULT)(res) >= 0)
#define FAILED( res )    ((ERESULT)(res) <  0)

#define SEVERITY( res )  (ERESULT)((unsigned int)(res) >> 24)
#define ZONE( res )      (ERESULT)(((unsigned int)(res) >> 16) & 0xff)
#define CODE( res )      (ERESULT)((unsigned int)(res) & 0xffff)


#define SEVERITY_SUCCESS  0x00
#define SEVERITY_FAILED   0x80

#endif // __ERESULT_H__
