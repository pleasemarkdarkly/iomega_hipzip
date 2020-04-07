//
// CDDirGen.h
//
// Copyright (c) 1998 - 2002 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef CDDIRGEN_H_
#define CDDIRGEN_H_

#include <util/metakit/mk4.h>
#include <util/metakit/mk4str.h>

// fdecl
typedef struct cdda_toc_s cdda_toc_t;

//! Class used for caching disk information for fast lookup of CD metadata.
class CCDDirGen
{
public:

    //! \param szDBFilename The metadata database file.
    //! \param szDirNameBase The base string to use for directory name generation.
    CCDDirGen(const char* szDBFilename, const char* szDirNameBase);
    ~CCDDirGen();

    //! Search the database for the given CD TOC.
    //! If a match is found, return the corresponding directory name.
    //! If no match is found, generate a new directory name.
    const char* GetDirectory(const cdda_toc_t* pTOC);

    //! Saves the database to file.
    bool Commit();

    //! Removes all disk info from the cache.
    void Clear();

    //! Rebuilds the database from TOC files on the hard drive.
    void Rebuild();

private:

    c4_Storage  m_mkCDs;
    c4_View     m_vCDs;

    char*           m_szDirNameBase;
    unsigned int    m_uiNextDirID;

    // Searches the database for a matching TOC.
    // If found, then the row index of the match is returned.
    // If not found, then -1 is returned.
    int MatchTOC(const cdda_toc_t* pTOC);

};

#endif // CDDIRGEN_H_
