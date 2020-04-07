// PEGRestoreUserInterface.cpp:  interface for the PEG UI in Restore Mode
// dand@iobjects.com 05/01/02
// (c) Interactive Objects

#include <main/ui/PEGRestoreUserInterface.h>

// peg headers
//#include <gui/peg/peg.hpp>
#include <main/ui/Messages.h>
#include <main/ui/Keys.h>
#include <main/ui/RestoreScreen.h>

void CPEGRestoreUserInterface::NotifyCDTrayOpened() 
{
}

void CPEGRestoreUserInterface::NotifyCDTrayClosed() 
{
}

void CPEGRestoreUserInterface::NotifyMediaInserted( int MediaIndex ) 
{
    CRestoreScreen::GetInstance()->NotifyMediaInserted();
}

void CPEGRestoreUserInterface::NotifyMediaRemoved( int MediaIndex ) 
{
}

