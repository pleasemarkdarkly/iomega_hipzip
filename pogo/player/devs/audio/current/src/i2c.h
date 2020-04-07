#ifndef I2C_H
#define I2C_H

#include <cyg/kernel/kapi.h>

void I2CInit(void);
void I2CWrite(cyg_uint8 Address, cyg_uint8 Register, cyg_uint8 Value);
cyg_uint8 I2CRead(cyg_uint8 Address, cyg_uint8 Register);

#endif /* I2C_H */
