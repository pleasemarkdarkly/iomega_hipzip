//
// EditString.h: definition of CEditString class
// chuckf@fullplaymedia.com 12/05/01
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#ifndef __EDITSTRING_H__
#define __EDITSTRING_H__

#include <gui/peg/peg.hpp>

typedef struct Key_s
{
	unsigned int Length;
	char* KeyValues;
}Key_t;

class CKeyTable
{
public:
	CKeyTable();
	~CKeyTable() {}
	char GetKey(unsigned int KeyNum, unsigned int KeyRepeated, bool bCaps);
private:
	const Key_t* Keys[10];
};

class CEditString : public PegString
{
public:
	CEditString();
	CEditString(const PegRect &Position, PEGCHAR **pText, PegFont* pFont);
	virtual ~CEditString();
	virtual SIGNED Message(const PegMessage &Mesg);
	virtual void Draw();
protected:
	virtual BOOL InsertKey(SIGNED iKey);
	bool m_bCaps;
	bool m_bShowCursor;
	PEGCHAR** mp_ReturnText;
	CKeyTable m_KeyTable;
	unsigned int m_Repeat;
	unsigned int m_CurrentKey;
};

#endif // __EDITSTRING_H__
