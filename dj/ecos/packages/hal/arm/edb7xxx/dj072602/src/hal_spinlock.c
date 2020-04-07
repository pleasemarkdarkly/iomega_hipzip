#include <cyg/hal/hal_spinlock.h>

void hal_spinlock_init( hal_spinlock_t* var ) 
{
    if( !var->once ) {
        var->once = 1;
        var->lockval = 0;
    }
}

void hal_spinlock_lock( hal_spinlock_t* var ) 
{
    //
    // while( (r2 = var->lockval) != 0 ) ;
    //
    
    asm volatile (
        "       mov     r0, %0;"
        "       mov     r1, #1;"
        " 123:"
        "       swpb    r2, r1, [r0];"
        "       cmp     r2, #0;"
        "       bne     123b"
        : "=r"(var->lockval)
        :
        : "r0", "r1", "r2"
        );
}

void hal_spinlock_unlock( hal_spinlock_t* var ) 
{
    // we could check r2 to make sure the spinlock was actually locked here
    
    //
    // r2 = var->lockval
    // var->lockval = 0;
    //
    
    asm volatile (
        "       mov     r0, %0;"
        "       mov     r1, #0;"
        "       swpb    r2, r1, [r0];"
        : "=r"(var->lockval)
        :
        : "r0", "r1", "r2"
        );
}
