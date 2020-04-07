//
// ParseConfig.h
// parse restore config file, extract useful information
// create an instance of this class for each file you parse
//
// Teman Clark-Lindh 
// temancl@fullplaymedia.com
// (c) 2002 Fullplay Media
//

#ifndef PARSECONFIG_H_
#define PARSECONFIG_H_

#define MAX_VAR_LEN 64
#define MAX_VAL_LEN 64
#define MAX_GROUP_LEN 64
#define MAX_PATH_LEN 256
#include <util/datastructures/SimpleList.h>

typedef struct FileInfo_t
{
	char path[MAX_PATH_LEN];
	// flags here?
} FileInfo;

typedef struct VarPair_t
{
	char var[MAX_VAR_LEN];
	char val[MAX_VAL_LEN]; // I feel bad about the var/val thing
} VarPair;


typedef struct ParseGroup_t
{

	SimpleList<VarPair> listVariables;
	SimpleList<FileInfo> listFiles;
	char name[MAX_GROUP_LEN];
} ParseGroup;
	

class CParseConfig
{

	public:
		// standard class stuff
		CParseConfig();
		~CParseConfig();

		// parse a buffer, delete and new appropriate values
		bool ParseBuffer(char* dataBuffer, int iSize);
		ParseGroup* FindGroup(const char* group);
		const char* FindVariable(const char* szGroup, const char* szVarname);
		bool GetFileList(const char* szGroup, SimpleList<FileInfo> &list);
	private:

		
		void FreeBuffers();
		bool AddGroup(char* line);
		bool AddVariable(char* line);
		bool AddFile(char* line);


		SimpleList<ParseGroup*> m_listGroups;
		char m_CurrentGroup[MAX_GROUP_LEN];

};

#endif  // PARSECONFIG_H_
