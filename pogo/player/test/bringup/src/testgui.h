#include <pkgconf/kernel.h>
#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/infra/diag.h>
#include <stdio.h>

typedef int BOOL;
#define TRUE 1
#define FALSE 0


#ifdef __cplusplus
extern "C" {
#endif

static void _KeyboardTestThread(CYG_ADDRESS Data);


void InitGUI();
void GUIPrint(const char * szText);



void InitKeyboard();
void ResetKeys();
BOOL KeyPressed(int key);


#ifdef __cplusplus
};
#endif
