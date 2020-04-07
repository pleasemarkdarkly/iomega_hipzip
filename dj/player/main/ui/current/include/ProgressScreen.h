//
// ProgressScreen.h: This class is derived from the CScreen class, and
//  contains the various widgets that make up a generic screen with 
// danb@iobjects.com 07/10/2002
// (c) Interactive Objects
//
#ifndef PROGRESSSCREEN_H_
#define PROGRESSSCREEN_H_

#include <main/ui/Screen.h>
#include <main/ui/SystemMessageString.h>

class CProgressScreen : public CScreen
{
public:
    CProgressScreen(CScreen* pParent);
    virtual ~CProgressScreen();

    virtual void Draw();

    virtual void SetTitleText(const TCHAR* szText);
    virtual void SetActionText(const TCHAR* szText);
    
    virtual void SetMessageText(const char* szText, CSystemMessageString::SysMsgType iMessageType = CSystemMessageString::STATUS);
    virtual void SetMessageText(const TCHAR* szText, CSystemMessageString::SysMsgType iMessageType = CSystemMessageString::STATUS);

    void ResetProgressBar(int iProgress = 0, int iTotal = 0);
    void UpdateProgressBar(int iProgress = 0);

protected:
    void DrawProgressBar();
    
    CSystemMessageString *m_pMessageTextString;
    
    PegString *m_pScreenTitle;
    PegString *m_pActionTextString;
    
    PegIcon *m_pTopScreenHorizontalDottedBarIcon;
    PegIcon *m_pBottomScreenHorizontalDottedBarIcon;
    
    PegRect m_ProgressBarRect;
    
    int     m_iProgressBarTotal;

private:
    void BuildScreen();

};

#endif  // PROGRESSSCREEN_H_
