/*
	by Ezra Driebach, Mark Phillips, Todd Malsbary (notice the spelling)

	#define PROFILE 
	#include <util/diag/AutoProfile.h>

	Used at the beginning of functions:
	-->FunctionProfiler fp("function1");	
	
	To Print out profile data:
	#ifdef PROFILE 
	 sProfiler.DumpProfileData();
	#endif 
*/

#include <util/diag/AutoProfile.h>
#include <util/debug/debug.h>

DEBUG_MODULE_S(AUTOPROFILE, DBGLEV_DEFAULT | DBGLEV_INFO);
DEBUG_USE_MODULE(AUTOPROFILE);


#define TIMESHIFT 0

#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/hal_intr.h>
#include <stdio.h>

Profiler::Profiler(void)
{
    int i;
    level=0;
    
    for (i=0;i<MAXNMNODES;i++)
    {
        nodes[i].nmChildren = 0;
        nodes[i].totalTime = 0;
        nodes[i].parent = NULL;
        nodes[i].lastReportedTime = 0;
    }
    
    nmNodes = 1;
    currentNode = nodes;
    currentNode->id = "root";
    GetTimer(lastTimeUpper, lastTimeLower);
}

Profiler::~Profiler(void)
{
}

unsigned int Profiler::SumChildren(Profiler::profile_node_t *node)
{
    int i;
    unsigned int tot;
    tot = node->totalTime;
    for (i = 0; i < node->nmChildren; i++)
        tot += SumChildren(node->child[i]);
    return tot;
}

void Profiler::PrintTree(Profiler::profile_node_t *tree, int level,
                         unsigned int parentTotal, unsigned int parentDifference)
{
    int i;
    char buff[80];
    int difPercent, totPercent;
    int sum;
    for (i = 0; i < level; i++)
        Print("   ");
    sum=SumChildren(tree) >> 10;
    if (parentTotal < 1)
        totPercent = 0;
    else
        totPercent = (1000 * sum) / parentTotal;
    
    if (parentDifference < 1)
        difPercent=0;
    else
        difPercent=(1000 * (sum - tree->lastReportedTime)) / parentDifference;
    
    sprintf(buff,"%.1f (%.1f) (%d): %s\n",difPercent*.1f,totPercent*.1f,sum,tree->id);
    Print(buff);
    for (i=0; i < tree->nmChildren; i++)
        PrintTree(tree->child[i], level+1, level == 0 ? sum : parentTotal, sum - tree->lastReportedTime);
    
    tree->lastReportedTime = sum;
}

void Profiler::DumpProfileData(void)
{
    unsigned int sum;
    Print("\n\n");
    sum=SumChildren(nodes) >> 10;
    PrintTree(nodes, 0, sum, sum-nodes->lastReportedTime);
}

void Profiler::Print(char* szMsg)
{
    DEBUGP(AUTOPROFILE, DBGLEV_INFO, szMsg);
}

void Profiler::GetTimer(unsigned int& upper, unsigned int& lower)
{
    HAL_CLOCK_READ(&lower);
    upper = cyg_current_time();
}

Profiler sProfiler;

FunctionProfiler::FunctionProfiler(char* id) 
{
    unsigned int timeUpper, timeLower;
    int i;
    for (i=0; i < sProfiler.currentNode->nmChildren; i++)
        if (sProfiler.currentNode->child[i]->id == id)
            break;
        
        sProfiler.GetTimer(timeUpper, timeLower);
        int timeDiff = (timeUpper - sProfiler.lastTimeUpper) * CYGNUM_KERNEL_COUNTERS_RTC_PERIOD + timeLower - sProfiler.lastTimeLower;
        if (timeDiff < 0)
        {
            timeDiff += CYGNUM_KERNEL_COUNTERS_RTC_PERIOD;
            if (timeDiff < 0)
            {
                DEBUGP(AUTOPROFILE, DBGLEV_INFO, "*****************************************\n");
                DEBUGP(AUTOPROFILE, DBGLEV_INFO, "time: %d .", timeUpper);
                DEBUGP(AUTOPROFILE, DBGLEV_INFO, "%d last: ", timeLower);
                DEBUGP(AUTOPROFILE, DBGLEV_INFO, "%d . ", sProfiler.lastTimeUpper);
                DEBUGP(AUTOPROFILE, DBGLEV_INFO, "%d\n", sProfiler.lastTimeLower);
                DEBUGP(AUTOPROFILE, DBGLEV_INFO, "*****************************************\n");
            }
        }
        sProfiler.currentNode->totalTime += timeDiff >> TIMESHIFT;
        
        sProfiler.lastTimeUpper = timeUpper;
        sProfiler.lastTimeLower = timeLower;
        
        if (i==sProfiler.currentNode->nmChildren)
        {
            DBASSERT(AUTOPROFILE, sProfiler.nmNodes < MAXNMNODES, "Too many nodes");
            DBASSERT(AUTOPROFILE, sProfiler.currentNode->nmChildren < MAXNMCHILDREN, "Too many children");
            sProfiler.currentNode->child[sProfiler.currentNode->nmChildren++] = sProfiler.nodes + sProfiler.nmNodes;
            sProfiler.nodes[sProfiler.nmNodes].parent = sProfiler.currentNode;
            sProfiler.nodes[sProfiler.nmNodes].id = id;
            sProfiler.currentNode = sProfiler.nodes + sProfiler.nmNodes; 
            sProfiler.nmNodes++;
        }
        else
            sProfiler.currentNode=sProfiler.currentNode->child[i];
        
}

FunctionProfiler::~FunctionProfiler(void)
{
    unsigned int timeUpper, timeLower;
    sProfiler.GetTimer(timeUpper, timeLower);
    sProfiler.currentNode->totalTime += ((timeUpper - sProfiler.lastTimeUpper) * CYGNUM_KERNEL_COUNTERS_RTC_PERIOD + timeLower - sProfiler.lastTimeLower) >> TIMESHIFT;
    sProfiler.lastTimeUpper = timeUpper;
    sProfiler.lastTimeLower = timeLower;
    sProfiler.currentNode = sProfiler.currentNode->parent; 
    DBASSERT(AUTOPROFILE, sProfiler.currentNode, "Missing node");
}
