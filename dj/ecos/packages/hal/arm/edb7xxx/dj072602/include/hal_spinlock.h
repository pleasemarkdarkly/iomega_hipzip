// hal_spinlock.h: hardware specific spinlock
// danc, 10/10/01
// (c) interactive objects

#ifndef __HAL_SPINLOCK_H__
#define __HAL_SPINLOCK_H__

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif
    
typedef struct hal_spinlock_s 
{
    int once;
    unsigned int lockval;
} hal_spinlock_t;

void hal_spinlock_init( hal_spinlock_t* );
void hal_spinlock_lock( hal_spinlock_t* );
void hal_spinlock_unlock( hal_spinlock_t* );

#ifdef __cplusplus
};
#endif

#endif  // __HAL_SPINLOCK_H__

