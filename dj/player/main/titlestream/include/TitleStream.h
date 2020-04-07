#ifndef TITLE_STREAM_H
#define TITLE_STREAM_H

#include <cyg/kernel/kapi.h>
#include "EventQueue.h"
#include "ddo_system.h"

extern "C" {
#include <network.h>
};

class CTitleStream
{
  public:
    CTitleStream();
    ~CTitleStream();
    
    int Initialize(int & Port);
    int Run(in_addr & InAddr, int Port);

    void Stats(void);
    
  private:
    int _ServerSocket;
    bool _Running;
    CEventQueue * _EventQueue;
    
    char _TitleStreamThreadStack[TITLE_STREAM_THREAD_STACK_SIZE];
    cyg_handle_t _TitleStreamThreadHandle;
    cyg_thread _TitleStreamThread;
    cyg_sem_t _TitleStreamThreadExitSem;
    static void _StartTitleStreamThread(cyg_addrword_t Data);
    void _TitleStreamThreadEntry(void);
};

#endif /* TITLE_STREAM_H */
