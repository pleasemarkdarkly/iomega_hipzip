/*
	by Ezra Driebach, Mark Phillips, Todd Malsbary (notice the spelling)

	#define PROFILE 
	#define DEBUG_BAG
	#include "autoprofile.h"

	Used at the beginning of functions:
	-->FunctionProfiler fp("function1");	
	
	To print out profile data:
	#ifdef PROFILE 
	 sProfiler.dumpProfileData();
	#endif

        Changes:
        * PROFILE_ECOS is defined for outputting info via diag_printf.
        * There is no performance counter under eCos, only the system clock which is
          at 64Hz on the 7212.
*/
#ifndef AUTOPROFILE_H_
#define AUTOPROFILE_H_

//#define PROFILE

#ifdef PROFILE

#include <assert.h>

#define TIMESHIFT 0
#define MAXNMCHILDREN 16
#define MAXNMNODES 300

#ifdef DEBUG_FILE
#elif defined(DEBUG_BAG)
//#	include "apcdebug.h"
#endif  

#ifndef WCHAR
#define WCHAR char
#endif
#ifndef UINT
#define UINT cyg_uint32
#endif

typedef struct _node
{
  WCHAR* id;
  unsigned int totalTime,lastReportedTime;
//  unsigned int totalTime, lastReportedTimeUpper, lastReportedTimeLower;
  struct _node *child[MAXNMCHILDREN];
  struct _node *parent;
  int nmChildren;
} ProfileNode;

class FunctionProfiler;
class Profiler
{
  friend FunctionProfiler;
public:
  Profiler(WCHAR*);
  ~Profiler(void);
	
  unsigned int sumChildren(ProfileNode *node);
  void printTree(ProfileNode *tree,int level,unsigned int parentTotal,
                 unsigned int parentDifference);
  void dumpProfileData(void);
//  unsigned int getTimer(void);
  void getTimer(unsigned int& upper, unsigned int& lower);
  void print(WCHAR* fmt);

private:
  ProfileNode nodes[MAXNMNODES];
  int nmNodes;
  int level;
//  unsigned int lastTime;
  unsigned int lastTimeUpper, lastTimeLower;
	
#ifdef DEBUG_FILE
  HANDLE hProfile;
#endif
  UINT uiTimer;

  ProfileNode *currentNode;
};

extern Profiler sProfiler;

class FunctionProfiler
{
public:
  FunctionProfiler(WCHAR* wcsFunctionName);
  ~FunctionProfiler(void);
};

#else

/*
class FunctionProfiler 
{public:
 FunctionProfiler(WCHAR*){}; 
 void dumpProfileData(void){};
};
*/

#endif	// PROFILE
	
#endif	// AUTOPROFILE_H_
