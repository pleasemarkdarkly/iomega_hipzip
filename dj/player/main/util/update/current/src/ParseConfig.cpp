//
// ParseConfig.cpp
// parse restore config file, extract useful information
// done with string pointer manipulation and linked lists
//
// Teman Clark-Lindh 
// temancl@fullplaymedia.com
// (c) 2002 Fullplay Media
//
    

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <main/util/update/ParseConfig.h>

#include <util/debug/debug.h>          // debugging hooks
DEBUG_MODULE_S( DBG_PARSE_CFG, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_PARSE_CFG );

CParseConfig::CParseConfig()
{

    // nothing to do here

}


CParseConfig::~CParseConfig()
{

    // only you can prevent leaks
    FreeBuffers();

}

void CParseConfig::FreeBuffers()
{
    // destroy listGroups
    while (!m_listGroups.IsEmpty())
    {
        delete m_listGroups.PopFront();
    }
}


// internal parser functions for extracting and tracking
// current variables
bool CParseConfig::AddGroup(char* line)
{


    int i;

    ParseGroup* group;

    group = new ParseGroup();


    // look for a ']'
    for(i = 1; line[i] != '\0' && line[i] != ']'; i++);

    if(line[i] != ']')
    {
        return false;
    }
    
    
    line[i] = '\0';

    
    
    strncpy(group->name,line + 1,MAX_GROUP_LEN);
    
    // dupe check
    if(FindGroup(group->name) != NULL)
        return false;

    strncpy(m_CurrentGroup,line + 1,MAX_GROUP_LEN);

    m_listGroups.PushFront(group);

    return true;
    
}

bool CParseConfig::AddVariable(char* line)
{

    // find group
    ParseGroup* group;

    if((group = FindGroup(m_CurrentGroup)) == NULL)
        return false;

    char* variable;
    char* value;
    int i;

    for(i = 1; line[i] != '\0' && line[i] != '='; i++);

    if(line[i] != '=')
    {
        return false;
    }

    line[i] = '\0';

    // skip *
    variable = line + 1;

    // after null/=, before end of line
    value = &line[i] + 1;



    VarPair varp;

    strncpy(varp.var,variable,MAX_VAR_LEN);
    strncpy(varp.val,value,MAX_VAL_LEN);

    group->listVariables.PushFront(varp);


    return true;

}


bool CParseConfig::AddFile(char* line)
{

    // find current group
    ParseGroup* group;

    if((group = FindGroup(m_CurrentGroup)) == NULL)
        return false;


    FileInfo file;

    strncpy(file.path,line,MAX_PATH_LEN);

    group->listFiles.PushFront(file);


    return true;
}

// destructive parse
bool CParseConfig::ParseBuffer(char* dataBuffer, int iSize)
{

    char* line;
    char* szBuffer;
    int len;
    bool ret = true;

    if(!dataBuffer || !iSize)
        return false;

    // copy the buffer into our own so we know it's null terminated and has a newline
    szBuffer = new char[iSize + 2];
    memcpy(szBuffer,dataBuffer,iSize);
    szBuffer[iSize]   = '\n';
    szBuffer[iSize+1] = '\0';

    line = strtok(szBuffer,"\n\0");
    
    while(line != NULL)
    {

        // strip leading
        while(*line == ' ' && *line != '\0')
            line++;


        // strip trailing spaces and linefeeds
        len = strlen(line);

        len--;
        while(line[len] == ' ' || line[len] == '\r')
            len--;
        len++;

        line[len] = '\0';

        // trailing, and return


        // is there a line left?
        if(*line != '\0')
        {


            // what type of line is this?

            switch(*line)
            {
                case '[':
                    // group
                    if(!AddGroup(line))
                    {
                        DEBUGP( DBG_PARSE_CFG, DBGLEV_INFO, "parse error on %s",line);
                        ret = false;
                        goto parseerror;
                    }
                    break;


                case '*':
                    // variable                 
                    if(!AddVariable(line))
                    {
                        DEBUGP( DBG_PARSE_CFG, DBGLEV_INFO, "parse error on %s",line);
                        ret = false;
                        goto parseerror;
                    }
                    break;

                default:

                    // all others are files
                    if(!AddFile(line))
                    {
                        DEBUGP( DBG_PARSE_CFG, DBGLEV_INFO, "parse error on %s",line);
                        ret = false;
                        goto parseerror;
                    }
                    break;
            }
            
        }


        // try and get next line
        line = strtok(NULL ,"\n\0");

    }

parseerror:

    delete []szBuffer;

    // did this work?
    return ret;
}


ParseGroup* CParseConfig::FindGroup(const char* group)
{

    for (SimpleListIterator<ParseGroup*> itGroup = m_listGroups.GetHead(); 
        itGroup != m_listGroups.GetEnd(); ++itGroup)                
    {
            if(strncmp((*itGroup)->name,group,MAX_GROUP_LEN) == 0)
                return (*itGroup);
    }   

    return NULL;

}

const char* CParseConfig::FindVariable(const char* szGroup, const char* szVarname)
{

    ParseGroup* group;

    if((group = FindGroup(szGroup)) == NULL)
        return NULL;

    for (SimpleListIterator<VarPair> itVars = group->listVariables.GetHead(); 
        itVars != group->listVariables.GetEnd(); ++itVars)              
    {
            if(strncmp((*itVars).var,szVarname,MAX_VAR_LEN) == 0)
                return (*itVars).val;
    }   
    
            
    return NULL;


} 


// get a list of all files for a group
bool CParseConfig::GetFileList(const char* szGroup, SimpleList<FileInfo> &list)
{
    ParseGroup* group;

    if((group = FindGroup(szGroup)) == NULL)
        return false;

    list = group->listFiles;

    return true;

}

