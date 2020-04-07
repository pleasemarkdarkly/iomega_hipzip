//
// CodecManagerImp.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef CODECMANAGERIMP_H_
#define CODECMANAGERIMP_H_

#include <util/registry/Registry.h>

typedef struct codec_table_s codec_table_t;

class ICodec;

class CCodecManagerImp
{
public:
    
    CCodecManagerImp();
    ~CCodecManagerImp();

    ICodec* FindCodec( unsigned int iCodecID, void* pAlloc, int iSize );
    ICodec* FindCodec( const char* szExtension, void* pAlloc, int iSize );
    RegKey  FindCodecID( const char* szExtension );

    // If you can't find a match based on known info,
    // try and "poll" for a codec by stepping through
    ICodec* TryCodec( int iCodecIndex, void* pAlloc, int iSize );
    
private:

    CRegistry* m_pRegistry;

    codec_table_t* m_pCodecTable;

    unsigned int m_iTableSize;

    unsigned int m_iMapSize;
};


#endif // CODECMANAGERIMP_H_
