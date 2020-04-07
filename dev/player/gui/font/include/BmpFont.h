#include <gui/common/UITypes.h>

class CDrawRegion;

class CBmpFont {
public:
	CBmpFont(short h, UChar* data, short* offsets, UChar* widths, short bytes_per_line);
	~CBmpFont() {}
	short DrawChar(char c, CDrawRegion dr, guiRect clip, guiPoint start, guiColor color);
private:
	short m_nHeight;
	UChar* m_aData;
	short* m_aOffsets;
	UChar* m_aWidths;
	short m_cBytesPerLine;
};