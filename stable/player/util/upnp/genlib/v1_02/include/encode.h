#ifndef __ENCODE_H
#define __ENCODE_H

#define USE_DECODE

typedef unsigned char BYTE;
typedef unsigned long DWORD;

//HRESULT HrBlobTo64Sz(BYTE bIn[], DWORD cbIn, LPSTR *cOut);
//HRESULT Hr64SzToBlob(LPCSTR cIn, BYTE **pbOut, DWORD *pcbOut);
bool HrBlobTo64Sz(BYTE bIn[], DWORD cbIn, char **cOut);
#ifdef USE_DECODE
bool Hr64SzToBlob(char* cIn, BYTE **pbOut, DWORD *pcbOut);
#endif	// USE_DECODE

#endif // __ENCODE_H
