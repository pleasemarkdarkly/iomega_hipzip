#ifndef __BATTERY_H__
#define __BATTERY_H__

#ifdef __cplusplus
extern "C" {
#endif
    
    int BatteryGetLevel(void);
    int BatteryGetChargeStatus(void);

#ifdef __cplusplus
};
#endif

#endif /* __BATTERY_H__ */
