/*
    by Ezra Driebach, Mark Phillips, Todd Malsbary (notice the spelling)

    #include <util/diag/AutoProfile.h>

    Used at the beginning of functions:
    -->FunctionProfiler fp("function1");    
    
    To Print out profile data:
     sProfiler.DumpProfileData();

        Changes:
        * PROFILE_ECOS is defined for outputting info via diag_printf.
        * There is no performance counter under eCos, only the system clock which is
          at 64Hz on the 7212.
*/

#ifndef AUTOPROFILE_H_
#define AUTOPROFILE_H_

#define MAXNMCHILDREN 8
#define MAXNMNODES 60

class Profiler
{
friend class FunctionProfiler;

typedef struct profile_node_s
{
    char* id;
    unsigned int totalTime, lastReportedTime;
    struct profile_node_s *child[MAXNMCHILDREN];
    struct profile_node_s *parent;
    int nmChildren;
} profile_node_t;

public:
    Profiler(void);
    ~Profiler(void);
    
    unsigned int SumChildren(profile_node_t *node);
    void PrintTree(profile_node_t *tree, int level, unsigned int parentTotal,
        unsigned int parentDifference);
    void DumpProfileData(void);
    //  unsigned int GetTimer(void);
    void GetTimer(unsigned int& upper, unsigned int& lower);
    void Print(char* fmt);
    
private:
    profile_node_t nodes[MAXNMNODES];
    int nmNodes;
    int level;
    //  unsigned int lastTime;
    unsigned int lastTimeUpper, lastTimeLower;
    
    unsigned int uiTimer;
    
    profile_node_t *currentNode;
};

extern Profiler sProfiler;

class FunctionProfiler
{
public:
    FunctionProfiler(char* szFunctionName);
    ~FunctionProfiler(void);
};


#endif	// AUTOPROFILE_H_
