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
*/

#include "autoprofile.h"

#ifdef PROFILE

#define PROFILE_ECOS

#ifdef PROFILE_ECOS
#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/hal_intr.h>
#include <stdio.h>
#define swprintf sprintf
#endif

Profiler::Profiler(WCHAR* wcsProfilerName)
{
  int i;
  level=0;

  for (i=0;i<MAXNMNODES;i++)
    {
      nodes[i].nmChildren=0;
      nodes[i].totalTime=0;
      nodes[i].parent=NULL;
      nodes[i].lastReportedTime=0;
//      nodes[i].lastReportedTimeUpper=0;
//      nodes[i].lastReportedTimeLower=0;
    }

  nmNodes=1;
  currentNode=nodes;
  currentNode->id="root";
#ifdef DEBUG_FILE
  hProfile = CreateFileW(wcsProfilerName, GENERIC_WRITE, FILE_SHARE_WRITE, 
                         NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
#elif defined (DEBUG_BAG)
  DEBUGMSG(ZONE_VERBOSE,("\nDebuging session: %s.", wcsProfilerName));	
#elif defined (PROFILE_NODEBUG)
  NKDbgPrintfW("\nDebuging session: %s.\n", wcsProfilerName);
#elif defined (PROFILE_ECOS)
  diag_printf("\nDebugging session: %s.\n", wcsProfilerName);
#endif
  //  uiTimer = SetTimer(NULL, NULL, 60000, NULL);
//  lastTime=getTimer();
  getTimer(lastTimeUpper, lastTimeLower);
}

Profiler::~Profiler(void)
{
#ifdef DEBUG_FILE
  CloseHandle(hProfile);
#endif
  //  KillTimer(NULL, uiTimer);
}

unsigned int Profiler::sumChildren(ProfileNode *node)
{
  int i;
  unsigned int tot;
  tot=node->totalTime;
  for (i=0;i<node->nmChildren;i++)
    tot+=sumChildren(node->child[i]);
  return tot;
}

void Profiler::printTree(ProfileNode *tree,int level,
                         unsigned int parentTotal,unsigned int parentDifference)
{
  int i;
  WCHAR buff[80];
//  unsigned int difPercent,totPercent;
//  unsigned int sum;
  int difPercent,totPercent;
  int sum;
  for (i=0;i<level;i++)
    print("   ");
  sum=sumChildren(tree)>>10;
  if (parentTotal<1)
    totPercent=0;
  else
    totPercent=(1000*sum)/parentTotal;

  if (parentDifference<1)
    difPercent=0;
  else
    difPercent=(1000*(sum-tree->lastReportedTime))/parentDifference;

  // swprintf(buff,L"%.1f (%.1f): %s\n",difPercent*.1f,totPercent*.1f,tree->id);
  swprintf(buff,"%.1f (%.1f) (%d): %s\n",difPercent*.1f,totPercent*.1f,sum,tree->id);
//  swprintf(buff,"%d (%d) (%d): %s\n",difPercent,totPercent,sum,tree->id);
  print(buff);
  /*
    for (i=0;i<tree->nmChildren;i++)
    printTree(tree->child[i],level+1,sum,sum-tree->lastReportedTime);
  */
  for (i=0;i<tree->nmChildren;i++)
    printTree(tree->child[i],level+1,level == 0 ? sum : parentTotal,sum-tree->lastReportedTime);

  tree->lastReportedTime=sum;
}

void Profiler::dumpProfileData(void)
{
  unsigned int sum;
  print("\n\n");
  sum=sumChildren(nodes)>>10;
  printTree(nodes,0,sum,sum-nodes->lastReportedTime);
}

void Profiler::print(WCHAR* szMsg)
{
#ifdef DEBUG_FILE
  DWORD dwBytesWritten;		
  if (hProfile)
    {
      WriteFile(hProfile, (void*) szMsg, sizeof(WCHAR) * strlen(szMsg), &dwBytesWritten, NULL);
    }		
#elif defined (DEBUG_BAG) 
  DEBUGMSG(ZONE_VERBOSE,(szMsg));
#elif defined (PROFILE_NODEBUG)
  NKDbgPrintfW(szMsg);
#elif defined (PROFILE_ECOS)
  diag_printf(szMsg);
#endif
}

void Profiler::getTimer(unsigned int& upper, unsigned int& lower)
{
  HAL_CLOCK_READ(&lower);
  upper = cyg_current_time();
//  diag_printf("clock: %d.%d\n", cyg_current_time(), val);
//  diag_printf("%d\n", CYGNUM_KERNEL_COUNTERS_RTC_RESOLUTION);
//  diag_printf("%d\n", CYGNUM_KERNEL_COUNTERS_RTC_PERIOD);
//	cyg_tick_count_t val = cyg_current_time();
//  diag_printf("clock: %d . ", upper);
//  diag_printf("%d\n", lower);
}

Profiler sProfiler("profiling.txt");

FunctionProfiler::FunctionProfiler(WCHAR* id) 
{
  unsigned int timeUpper, timeLower;
  int i;
  for (i=0;i<sProfiler.currentNode->nmChildren;i++)
    if (sProfiler.currentNode->child[i]->id==id)
      break;

  sProfiler.getTimer(timeUpper, timeLower);
  int timeDiff = (timeUpper-sProfiler.lastTimeUpper) * CYGNUM_KERNEL_COUNTERS_RTC_PERIOD + timeLower-sProfiler.lastTimeLower;
  if (timeDiff < 0)
  {
	timeDiff += CYGNUM_KERNEL_COUNTERS_RTC_PERIOD;
	if (timeDiff < 0)
	{
		diag_printf("*****************************************\n");
		diag_printf("time: %d .", timeUpper);
		diag_printf("%d last: ", timeLower);
		diag_printf("%d . ", sProfiler.lastTimeUpper);
		diag_printf("%d\n", sProfiler.lastTimeLower);
		diag_printf("*****************************************\n");
	}
  }
  sProfiler.currentNode->totalTime += timeDiff >> TIMESHIFT;

//  sProfiler.currentNode->totalTime+=((timeUpper-sProfiler.lastTimeUpper) * CYGNUM_KERNEL_COUNTERS_RTC_PERIOD + timeLower-sProfiler.lastTimeLower) >>TIMESHIFT;
/*
  time=sProfiler.getTimer();
  sProfiler.currentNode->totalTime+=(time-sProfiler.lastTime)>>TIMESHIFT;
*/
/*
if (time < sProfiler.lastTime)
diag_printf("*****************************************\n");
diag_printf("time: %d last: %d\n", time, sProfiler.lastTime);
if (time < sProfiler.lastTime)
diag_printf("*****************************************\n");
*/
//  sProfiler.lastTime=time;
  sProfiler.lastTimeUpper=timeUpper;
  sProfiler.lastTimeLower=timeLower;

  if (i==sProfiler.currentNode->nmChildren)
    {
      assert(sProfiler.nmNodes<MAXNMNODES);
      assert(sProfiler.currentNode->nmChildren<MAXNMCHILDREN);
      sProfiler.currentNode->child[sProfiler.currentNode->nmChildren++]=sProfiler.nodes+sProfiler.nmNodes;
      sProfiler.nodes[sProfiler.nmNodes].parent=sProfiler.currentNode;
      sProfiler.nodes[sProfiler.nmNodes].id=id;
      sProfiler.currentNode=sProfiler.nodes+sProfiler.nmNodes; 
      sProfiler.nmNodes++;
    }
  else
    sProfiler.currentNode=sProfiler.currentNode->child[i];

}

FunctionProfiler::~FunctionProfiler(void)
{
/*
  unsigned int time=sProfiler.getTimer();
  sProfiler.currentNode->totalTime+=(time-sProfiler.lastTime)>>TIMESHIFT;
  sProfiler.lastTime=time;
*/
  unsigned int timeUpper, timeLower;
  sProfiler.getTimer(timeUpper, timeLower);
  sProfiler.currentNode->totalTime+=((timeUpper-sProfiler.lastTimeUpper) * CYGNUM_KERNEL_COUNTERS_RTC_PERIOD + timeLower-sProfiler.lastTimeLower) >>TIMESHIFT;
  sProfiler.lastTimeUpper=timeUpper;
  sProfiler.lastTimeLower=timeLower;
  sProfiler.currentNode=sProfiler.currentNode->parent; 
  assert(sProfiler.currentNode);
}

#endif // PROFILE
