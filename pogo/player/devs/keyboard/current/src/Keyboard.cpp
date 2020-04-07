// Keyboard.cpp: keyboard (button pad) interface
// danc@iobjects.com 07/13/01
// (c) Interactive Objects


#include <devs/keyboard/Keyboard.h>
#include "KeyboardImp.h"

//
// CKeyboard implementation
//
static CKeyboard* s_pSingleton = 0;

CKeyboard* CKeyboard::GetInstance() 
{
    if( s_pSingleton == 0 ) {
        s_pSingleton = new CKeyboard;
    }
    return s_pSingleton;
}

void CKeyboard::Destroy()
{
    if( s_pSingleton ) delete s_pSingleton;
    s_pSingleton = NULL;
}

CKeyboard::CKeyboard() 
{
    m_pImp = new CKeyboardImp;
}

CKeyboard::~CKeyboard()
{
    delete m_pImp;
}

void CKeyboard::SetKeymap( const key_map_t* pKeymap ) 
{
    m_pImp->SetKeymap( pKeymap );
}

void CKeyboard::LockKeyboard()
{
    m_pImp->LockKeyboard( );
}
void CKeyboard::UnlockKeyboard()
{
    m_pImp->UnlockKeyboard( );
}

