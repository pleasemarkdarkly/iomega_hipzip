#include <pkgconf/kernel.h>
#include <cyg/kernel/kapi.h>            /* All the kernel specific stuff */
#include <cyg/hal/hal_arch.h>           /* CYGNUM_HAL_STACK_SIZE_TYPICAL */
#include <cyg/infra/diag.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/hal/drv_api.h>

#include <datasource/common/DataSource.h>
#include <datasource/datasourcemanager/DataSourceManager.h>

/* DEFINES */

#define DEBUG(s...) diag_printf(##s)
//#define DEBUG(s...) /**/

#define NTHREADS 1
#define STACKSIZE (CYGNUM_HAL_STACK_SIZE_TYPICAL + (16 * 4096))

/* STATICS */

static cyg_handle_t _ThreadH[NTHREADS];
static cyg_thread _Thread[NTHREADS];
static char _ThreadStack[NTHREADS][STACKSIZE];

static void
_MainThread(CYG_ADDRESS Data)
{       
    DEBUG("+%s\n", __FUNCTION__);

    CDataSourceManager* pDSM = CDataSourceManager::GetInstance();

    int iDataSourceNum = pDSM->GetDataSourceCount();
    DEBUG("Data source count: %d\n", iDataSourceNum);
    for (int i = 0; i < iDataSourceNum; ++i)
        DEBUG("Data source %d ID: %d Type: %d\n", i, pDSM->GetDataSourceByIndex(i)->GetInstanceID(), pDSM->GetDataSourceByIndex(i)->GetClassID());

    CDataSourceManager::Destroy();

    DEBUG("-%s\n", __FUNCTION__);
}

extern "C" {

void
cyg_user_start(void)
{
    DEBUG("+%s\n", __FUNCTION__);
    
    cyg_thread_create(10, _MainThread, (cyg_addrword_t) 0, "MainThread",
		      (void *)_ThreadStack[0], STACKSIZE, &_ThreadH[0], &_Thread[0]);
    cyg_thread_resume(_ThreadH[0]);

    diag_printf("-%s\n", __FUNCTION__);
}

};
