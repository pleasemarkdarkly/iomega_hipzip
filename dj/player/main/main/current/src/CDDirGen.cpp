//
// CDDirGen.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include "CDDirGen.h"

#include <datasource/cddatasource/CDDataSource.h>
#include <datastream/fatfile/FileInputStream.h>
#include <datastream/fatfile/FileOutputStream.h>
#include <main/main/FatHelper.h>
#include <main/main/ProgressWatcher.h>
#include <fs/fat/sdapi.h>

#include <util/debug/debug.h>

#include <stdio.h>
#include <stdlib.h> /* atoi */

DEBUG_MODULE_S(CDDIRGEN, DBGLEV_DEFAULT);
DEBUG_USE_MODULE(CDDIRGEN);


#define CD_DB_FORMAT "CDs[TrackCount:I,Sum:I,TOC[Offset:I],DirName:S,DirID:I]"


CCDDirGen::CCDDirGen(const char* szDBFilename, const char* szDirNameBase)
    : m_mkCDs(szDBFilename, 1)
{
    DBASSERT(CDDIRGEN, strlen(szDirNameBase) + 8 + 1 < EMAXPATH, "Directory name string is too long\n");
    m_szDirNameBase = new char[strlen(szDirNameBase) + 1];
    DBASSERT(CDDIRGEN, m_szDirNameBase, "Unable to allocate directory name string\n");
    strcpy(m_szDirNameBase, szDirNameBase);

    m_vCDs = m_mkCDs.GetAs(CD_DB_FORMAT);

    // Search the database for the highest directory ID in use,
    // then initialize the starting ID one higher.
    m_uiNextDirID = 0;
    c4_IntProp pDirID("DirID");
    for (int i = 0; i < m_vCDs.GetSize(); ++i)
    {
        if (m_uiNextDirID < (unsigned int)pDirID(m_vCDs[i]))
            m_uiNextDirID = (unsigned int)pDirID(m_vCDs[i]);
    }
    DEBUGP(CDDIRGEN, DBGLEV_INFO, "cddir: Next directory ID to use: %d\n", m_uiNextDirID);
}

CCDDirGen::~CCDDirGen()
{
    delete [] m_szDirNameBase;
}

//! Search the database for the given CD TOC.
//! If a match is found, return the corresponding directory name.
//! If no match is found, generate a new directory name.
const char*
CCDDirGen::GetDirectory(const cdda_toc_t* pTOC)
{
    c4_IntProp pTrackCount("TrackCount"), pSum("Sum");
    c4_StringProp pDirName("DirName");

    // Try to find a match in the database.
    int row = MatchTOC(pTOC);

    if (row != -1)
        return (const char*)pDirName(m_vCDs[row]);

    // No match was found in the database, so generate a new row and a new directory name.
    // Look for a directory name that isn't in use.
    char szDirName[EMAXPATH];
    strcpy(szDirName, m_szDirNameBase);
    unsigned int uiStartID = m_uiNextDirID;
    do
    {
        sprintf(szDirName + strlen(m_szDirNameBase), "%x", m_uiNextDirID++);
        if (!pc_isdir(szDirName))
            break;
    } while (uiStartID != m_uiNextDirID);

    // This should never, ever happen.
    DBASSERT(CDDIRGEN, uiStartID != m_uiNextDirID, "Every single directory name in use\n");

    DEBUGP(CDDIRGEN, DBGLEV_INFO, "cddir: New directory name: %s\n", szDirName);

    int sum = 0;
    for (int i = 0; i < pTOC->entries; ++i)
        sum += pTOC->entry_list[i].lba_startsector;

    // Create the new database entry.
    c4_IntProp pDirID("DirID");
    c4_IntProp pOffset("Offset");
    c4_ViewProp vpTOC("TOC");

    row = m_vCDs.Add(pTrackCount[pTOC->entries] + pSum[sum] + pDirID[m_uiNextDirID - 1] + pDirName[szDirName]);
    c4_View vTOC = vpTOC(m_vCDs[row]);

    char szTOC[600] = "";
    char szEntry[7];

    int i = 0;
    for (; i < pTOC->entries; ++i)
    {
        vTOC.Add(pOffset[pTOC->entry_list[i].lba_startsector]);
        sprintf(szEntry, "%d ", (int)pTOC->entry_list[i].lba_startsector);
        strcat(szTOC, szEntry);
    }
    vTOC.Add(pOffset[pTOC->entry_list[i - 1].lba_length]);
    sprintf(szEntry, "%d\n", (int)pTOC->entry_list[i - 1].lba_length);
    strcat(szTOC, szEntry);

    // Save the new database.
    CProgressWatcher::GetInstance()->SetTask(TASK_LOADING_CD_DIR_CACHE);
    Commit();

    // Create the new directory.
    if (VerifyOrCreateDirectory(szDirName))
    {
        // Write the TOC to file so the database can be reconstructed later.
        char szTOCFileName[EMAXPATH];
        strcpy(szTOCFileName, szDirName);
        strcat(szTOCFileName, "/toc");
        CFatFileOutputStream ffos;
        if (SUCCEEDED(ffos.Open(szTOCFileName)))
        {
            DEBUGP(CDDIRGEN, DBGLEV_INFO, "cddir: Saving TOC: %s\n", szTOC);
            ffos.Write((void*)szTOC, strlen(szTOC));
        }
        else
            DEBUG(CDDIRGEN, DBGLEV_ERROR, "Unable to create TOC file: %s\n", szTOCFileName);
    }
    else
        DEBUG(CDDIRGEN, DBGLEV_ERROR, "Unable to create directory: %s\n", szDirName);

    CProgressWatcher::GetInstance()->UnsetTask(TASK_LOADING_CD_DIR_CACHE);

    return (const char*)pDirName(m_vCDs[row]);
}


//! Saves the database to file.
bool
CCDDirGen::Commit()
{
    return m_mkCDs.Commit(true);
}

//! Removes all disk info from the cache.
void
CCDDirGen::Clear()
{
    m_vCDs.RemoveAll();
}

//! Rebuilds the database from TOC files on the hard drive.
void
CCDDirGen::Rebuild()
{
    c4_IntProp pTrackCount("TrackCount"), pSum("Sum");
    c4_StringProp pDirName("DirName");

    char szDirName[EMAXPATH];
    strcpy(szDirName, m_szDirNameBase);
    unsigned int iDirID = 0;
    do
    {
        // Build the directory name for the next ID.
        // If no matching directory is found, assume we've reached the end of the list.
        sprintf(szDirName + strlen(m_szDirNameBase), "%x", iDirID++);
        if (!pc_isdir(szDirName))
            break;

        // Open the TOC file for this CD.
        char szTOCFileName[EMAXPATH];
        strcpy(szTOCFileName, szDirName);
        strcat(szTOCFileName, "/toc");
        CFatFileInputStream ffis;
        if (SUCCEEDED(ffis.Open(szTOCFileName)))
        {
            char szTOC[600];
            ffis.Read((void*)szTOC, 600);
            DEBUGP(CDDIRGEN, DBGLEV_INFO, "cddir: Read TOC: %s\n", szTOC);

            // Build the TOC record.
            cdda_toc_t toc;
            cdda_toc_entry_t aryTOC[100];
            toc.entries = 0;
            toc.entry_list = aryTOC;
            char* offset = strtok(szTOC, " ");
            while (offset)
            {
                aryTOC[toc.entries].lba_startsector = atoi(offset);
                DEBUGP(CDDIRGEN, DBGLEV_TRACE, "cddir: Offset %d: %d\n", toc.entries, aryTOC[toc.entries].lba_startsector);
                ++toc.entries;
                offset = strtok(NULL, " ");
            }
            if (toc.entries > 1)
            {
                aryTOC[toc.entries - 2].lba_length = aryTOC[toc.entries - 1].lba_startsector;
                --toc.entries;
            }

            int row = MatchTOC(&toc);

            // If there's no matching TOC in the database, then add a new entry.
            if (row == -1)
            {
                unsigned int uiDirID = 0;
                sscanf(szDirName + strlen(m_szDirNameBase), "%x", &uiDirID);
                if (m_uiNextDirID <= uiDirID)
                    m_uiNextDirID = uiDirID + 1;

                // Create the new database entry.
                c4_IntProp pDirID("DirID");
                c4_IntProp pOffset("Offset");
                c4_ViewProp vpTOC("TOC");

                int sum = 0;
                for (int i = 0; i < toc.entries; ++i)
                    sum += toc.entry_list[i].lba_startsector;

                row = m_vCDs.Add(pTrackCount[toc.entries] + pSum[sum] + pDirID[uiDirID] + pDirName[szDirName]);
                c4_View vTOC = vpTOC(m_vCDs[row]);

                int i = 0;
                for (; i < toc.entries; ++i)
                {
                    vTOC.Add(pOffset[toc.entry_list[i].lba_startsector]);
                }
                vTOC.Add(pOffset[toc.entry_list[i - 1].lba_length]);
            }
        }
        else
            DEBUG(CDDIRGEN, DBGLEV_ERROR, "Unable to open TOC file: %s\n", szTOCFileName);

    } while (1);

    // Save the new database.
    Commit();
}

// Searches the database for a matching TOC.
// If found, then the row index of the match is returned.
// If not found, then -1 is returned.
int
CCDDirGen::MatchTOC(const cdda_toc_t* pTOC)
{
    c4_IntProp pTrackCount("TrackCount"), pSum("Sum");
    c4_StringProp pDirName("DirName");

    int sum = 0;
    for (int i = 0; i < pTOC->entries; ++i)
        sum += pTOC->entry_list[i].lba_startsector;
    DEBUGP(CDDIRGEN, DBGLEV_INFO, "cddir: Checking database for CD with %d tracks, sum %d\n", pTOC->entries, sum);

    // Find a quick match by comparing just the track count and the sum of the track offsets.
    int row = m_vCDs.Find(pTrackCount[pTOC->entries] + pSum[sum]);
    while (row != -1)
    {
        // Match based on lba compares.
        c4_IntProp pOffset("Offset");
        c4_ViewProp vpTOC("TOC");
        c4_View vTOC = vpTOC(m_vCDs[row]);

        if (vTOC.GetSize() != pTrackCount(m_vCDs[row]) + 1)
        {
            DEBUGP(CDDIRGEN, DBGLEV_ERROR, "Corrupt cache entry in row %d: Offset count: %d vs %d\n", row, vTOC.GetSize(), (int)pTrackCount(m_vCDs[row]) + 1);
            m_vCDs.RemoveAt(row);
            row = m_vCDs.Find(pTrackCount[pTOC->entries] + pSum[sum], row);
        }
        else
        {
            int i = 0;
            // Check each offset in this row.
            for (; i < pTOC->entries; ++i)
            {
                if ((int)pOffset(vTOC[i]) != pTOC->entry_list[i].lba_startsector)
                    break;
            }
            if (i == pTOC->entries)
            {
                // All the offsets matched, so check the last lba length.
                if ((int)pOffset(vTOC[i]) == pTOC->entry_list[i - 1].lba_length)
                {
                    // Everything matched, so return the directory name.
                    DEBUGP(CDDIRGEN, DBGLEV_INFO, "cddir: Matching TOC found in row %d: Dir %s\n", row, (const char*)pDirName(m_vCDs[row]));
                    return row;
                }
            }

            // This row didn't match, so check again.
            row = m_vCDs.Find(pTrackCount[pTOC->entries] + pSum[sum], row + 1);
        }
    }

    // No match was found in the database, so generate a new row and a new directory name.
    DEBUGP(CDDIRGEN, DBGLEV_INFO, "cddir: No matching TOC found\n");

    return -1;
}


#undef CD_DB_FORMAT
