#include <gui/peg/peg.hpp>

extern PegBitmap gbZipDiskBitmap;
extern PegFont VerdanaFont16;

// (epg,9/4/2001): some gay class

class CDAReceiverMaster : public PegWindow
{
public:
  CDAReceiverMaster(const PegRect& rect, WORD wStyle = FF_NONE);
  virtual ~CDAReceiverMaster();

private:

};

// (epg,9/4/2001): sample peg screen class decl

class CInsertDiskScreen : public PegDecoratedWindow
{
public:

	CInsertDiskScreen(CDAReceiverMaster* pMaster);
	virtual ~CInsertDiskScreen();
	SIGNED Message(const PegMessage &Mesg);

	// Hides any visible menu screens.
	void HideMenu();
	void Draw();

private:

	void BuildScreen();
	int m_iDiskMBFree;
	TCHAR	m_szDiskLabel[128];

	CDAReceiverMaster* m_pMaster;
	PegFont *p_Verdana;
	PegString *p_icon_text;
	PegString *p_icon_text2;
	PegString *p_icon_Disk_Title;
	
	PegIcon *p_icon_full_screen;

};

bool PEG_ProcessEvent();
