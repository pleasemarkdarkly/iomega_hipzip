#include <cyg/hal/mmgpio.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/hal/hal_diag.h>
#include <cyg/hal/hal_intr.h>
#include <cyg/infra/diag.h>

#include <pkgconf/system.h>

#if __DHARMA==2

/* Shadow register since we cannot read the output */
static cyg_uint32 MMGPORegister = 0;

cyg_uint32
GetMMGPI(void) 
{
    cyg_uint32 old_memcfg1;
    cyg_uint32 memcfg1;
    cyg_uint32 mmgpi;
    CYG_INTERRUPT_STATE old;

    /* Need to lock nCS3 while we read the register.  This is the quickest way to do it. */
    HAL_DISABLE_INTERRUPTS(old);
    
    /* Set nCS3 to 32 bit 0 wait state access */
    old_memcfg1 = *(volatile cyg_uint32 *)MEMCFG1;
    memcfg1 = (old_memcfg1 & ~0xff000000) | 0x3C000000;
    *(volatile cyg_uint32 *)MEMCFG1 = memcfg1;

    /* Get the GPI settings */
    mmgpi = *(volatile cyg_uint32 *)MMGPI;

    /* Restore MEMCFG */
    *(volatile cyg_uint32 *)MEMCFG1 = old_memcfg1;

    HAL_RESTORE_INTERRUPTS(old);
    
    return mmgpi;
}
    
void
SetMMGPO(cyg_uint32 SetBits, cyg_uint32 ClearBits) 
{
    cyg_uint32 old_memcfg1;
    cyg_uint32 memcfg1;
    CYG_INTERRUPT_STATE old;
    
    /* Set GPO to desired state */
    MMGPORegister &= ~ClearBits;
    MMGPORegister |= SetBits;

    /* See note above. */
    HAL_DISABLE_INTERRUPTS(old);
    
    /* Set nCS3 to 32 bit 0 wait state access */
    old_memcfg1 = *(volatile cyg_uint32 *)MEMCFG1;
    memcfg1 = (old_memcfg1 & ~0xff000000) | 0x3C000000;
    *(volatile cyg_uint32 *)MEMCFG1 = memcfg1;

    /* Set the GPO */
    *(volatile cyg_uint32 *)MMGPO = MMGPORegister;

    /* Restore MEMCFG */
    *(volatile cyg_uint32 *)MEMCFG1 = old_memcfg1;

    HAL_RESTORE_INTERRUPTS(old);
}

#endif // __DHARMA==2
