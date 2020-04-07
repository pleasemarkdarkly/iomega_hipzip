#include "i2c.h"
#include <cyg/hal/hal_edb7xxx.h>

#if defined(__EDB7212) && !defined(__EDB7312)
#define GPIO_DATA_DIRECTION PBDDR
#define GPIO_DATA PBDR
#define I2C_SCL 0x08
#define I2C_SDA 0x04
#define I2C_SDA_SHIFT 2
#elif defined(__EDB7312)
#define GPIO_DATA_DIRECTION PDDDR
#define GPIO_DATA PDDR
#define I2C_SCL 0x20
#define I2C_SDA 0x10
#define I2C_SDA_SHIFT 4
#elif defined(__IOMEGA_32)
#define GPIO_DATA_DIRECTION PBDDR
#define GPIO_DATA PBDR
#define I2C_SCL 0x08
#define I2C_SDA 0x04
#define I2C_SDA_SHIFT 2
#endif /* __IOMEGA_32 */

#define I2C_WRITE 0x00
#define I2C_READ  0x01
#define WAIT_1MS 0x4800
#define WAIT_1US 0x000b

static void _Wait(unsigned int Delay);
static void _WriteByte(cyg_uint8 Byte);
static cyg_uint8 _ReadByte(void);

void
I2CInit(void)
{
#if (GPIO_DATA_DIRECTION == PDDDR)
    *(volatile cyg_uint8 *)GPIO_DATA_DIRECTION &= ~(I2C_SCL | I2C_SDA);
#else /* GPIO_DATA_DIRECTION == PDDDR */
    *(volatile cyg_uint8 *)GPIO_DATA_DIRECTION |= (I2C_SCL | I2C_SDA);
#endif /* GPIO_DATA_DIRECTION == PDDDR */
    *(volatile cyg_uint8 *)GPIO_DATA &= ~(I2C_SCL | I2C_SDA);
    *(volatile cyg_uint8 *)GPIO_DATA |= (I2C_SCL | I2C_SDA);
    _Wait(WAIT_1MS);
}

void
I2CWrite(cyg_uint8 Address, cyg_uint8 Register, cyg_uint8 Value)
{
    /* Start */
    _Wait(WAIT_1MS);
    *(volatile cyg_uint8 *)GPIO_DATA &= ~I2C_SDA;
    _Wait(WAIT_1US * 16);
    
    /* Write address */
    _WriteByte(Address | I2C_WRITE);

    /* Write register */
    _WriteByte(Register);

    /* Write value */
    _WriteByte(Value);

    /* Clock high */
    *(volatile cyg_uint8 *)GPIO_DATA |= I2C_SCL;
    _Wait(WAIT_1US * 16);

    /* Stop */
    _Wait(WAIT_1US * 8);
    *(volatile cyg_uint8 *)GPIO_DATA |= I2C_SDA;
    _Wait(WAIT_1US * 16);
}

cyg_uint8
I2CRead(cyg_uint8 Address, cyg_uint8 Register)
{
    cyg_uint8 Value;

    /* Start */
    _Wait(WAIT_1MS);
    *(volatile cyg_uint8 *)GPIO_DATA &= ~I2C_SDA;
    _Wait(WAIT_1US * 16);
    
    /* Write address */
    _WriteByte(Address | I2C_WRITE);

    /* Write MAP register */
    _WriteByte(Register);

    _WriteByte(0x00);

    /* Clock high */
    *(volatile cyg_uint8 *)GPIO_DATA |= I2C_SCL;
    _Wait(WAIT_1US * 16);

    /* Stop */
    _Wait(WAIT_1US * 8);
    *(volatile cyg_uint8 *)GPIO_DATA |= I2C_SDA;
    _Wait(WAIT_1US * 16);




    /* Start */
    _Wait(WAIT_1MS);
    *(volatile cyg_uint8 *)GPIO_DATA &= ~I2C_SDA;
    _Wait(WAIT_1US * 16);


    /* Write address */
    _WriteByte(Address | I2C_READ);

    /* Read value */
    Value = _ReadByte();

    /* Clock high */
    *(volatile cyg_uint8 *)GPIO_DATA |= I2C_SCL;
    _Wait(WAIT_1US * 16);

    /* Stop */
    _Wait(WAIT_1US * 8);
    *(volatile cyg_uint8 *)GPIO_DATA |= I2C_SDA;
    _Wait(WAIT_1US * 16);


    return Value;
}

static void
_Wait(unsigned int Delay)
{
    volatile unsigned int Count = Delay;
    
    while(Count--)
        ;
}

static void
_WriteByte(cyg_uint8 Byte)
{
    unsigned int Bit;
    cyg_uint8 BitMask;

    BitMask = 0x80;        /* Start with MSB */
    for (Bit = 0; Bit < 8; ++Bit) {
    
        /* Clock low */
        *(volatile cyg_uint8 *)GPIO_DATA &= ~I2C_SCL;
        _Wait(WAIT_1US * 8);

        /* Set data */
        if(Byte & BitMask) {
            *(volatile cyg_uint8 *)GPIO_DATA |= I2C_SDA;
        }
        else {
            *(volatile cyg_uint8 *)GPIO_DATA &= ~I2C_SDA;
        }
        _Wait(WAIT_1US * 16);

        /* Clock high */
        *(volatile cyg_uint8 *)GPIO_DATA |= I2C_SCL;
        _Wait(WAIT_1US * 16);

        /* Shift bit */
        BitMask = BitMask >> 1;    
    }
    
    /* Clock low */
    *(volatile cyg_uint8 *)GPIO_DATA &= ~I2C_SCL;
    _Wait(WAIT_1US * 8);

    /* Data low */
    *(volatile cyg_uint8 *)GPIO_DATA &= ~I2C_SDA;
    _Wait(WAIT_1US * 4);

    /* Turn SDA into input for ACK */
#if (GPIO_DATA_DIRECTION == PDDDR)
    *(volatile cyg_uint8 *)GPIO_DATA_DIRECTION |= I2C_SDA;
#else /* GPIO_DATA_DIRECTION == PDDDR */
    *(volatile cyg_uint8 *)GPIO_DATA_DIRECTION &= ~I2C_SDA;
#endif /* GPIO_DATA_DIRECTION == PDDDR */
    _Wait(WAIT_1US * 4);

    /* Clock high for ACK */
    *(volatile cyg_uint8 *)GPIO_DATA |= I2C_SCL;
    _Wait(WAIT_1US * 16);

    /* Clock low */
    *(volatile cyg_uint8 *)GPIO_DATA &= ~I2C_SCL;
    _Wait(WAIT_1US * 4);

    /* Turn SDA back to output */
#if (GPIO_DATA_DIRECTION == PDDDR)
    *(volatile cyg_uint8 *)GPIO_DATA_DIRECTION &= ~I2C_SDA;
#else /* GPIO_DATA_DIRECTION == PDDDR */
    *(volatile cyg_uint8 *)GPIO_DATA_DIRECTION |= I2C_SDA;
#endif /* GPIO_DATA_DIRECTION == PDDDR */
    _Wait(WAIT_1US * 4);
}

static cyg_uint8
_ReadByte(void)
{
    int Bit;
    cyg_uint8 BitMask;
    cyg_uint8 Value = 0;


    _Wait(WAIT_1US * 10 );
    *(volatile cyg_uint8 *)GPIO_DATA |= I2C_SDA;
    
    /* Turn SDA into input */
#if (GPIO_DATA_DIRECTION == PDDDR)
    *(volatile cyg_uint8 *)GPIO_DATA_DIRECTION |= I2C_SDA;
#else /* GPIO_DATA_DIRECTION == PDDDR */
    *(volatile cyg_uint8 *)GPIO_DATA_DIRECTION &= ~I2C_SDA;
#endif /* GPIO_DATA_DIRECTION == PDDDR */

    BitMask = 0x80;        /* Start with MSB */
    for (Bit = 7; Bit >= 0; --Bit) {
        cyg_uint8 Byte;
    
        /* Clock low */
        _Wait(WAIT_1US * 10 );
        *(volatile cyg_uint8 *)GPIO_DATA &= ~I2C_SCL;
        _Wait(WAIT_1US * 10 );

        /* Clock high */
        _Wait(WAIT_1US * 10);
        *(volatile cyg_uint8 *)GPIO_DATA |= I2C_SCL;
        _Wait(WAIT_1US * 10 );

        /* Get data */
        if( (*(volatile cyg_uint8 *)GPIO_DATA) & I2C_SDA) {
            Value |= (1 << Bit);
        }  
    }

    /* Clock low */
    *(volatile cyg_uint8 *)GPIO_DATA &= ~I2C_SCL;
    _Wait(WAIT_1US * 10 );
    
    /* Turn SDA back to output */
#if (GPIO_DATA_DIRECTION == PDDDR)
    *(volatile cyg_uint8 *)GPIO_DATA_DIRECTION &= ~I2C_SDA;
#else /* GPIO_DATA_DIRECTION == PDDDR */
    *(volatile cyg_uint8 *)GPIO_DATA_DIRECTION |= I2C_SDA;
#endif /* GPIO_DATA_DIRECTION == PDDDR */

    /* write ack bit after reading register - timings are guesstimated -dc */
    
    /* Write ack */
    *(volatile cyg_uint8 *)GPIO_DATA |= I2C_SDA;
    _Wait(WAIT_1US * 10);

    /* Clock high */
    *(volatile cyg_uint8 *)GPIO_DATA |= I2C_SCL;
    _Wait(WAIT_1US * 10);

    /* Clock low */
    *(volatile cyg_uint8 *)GPIO_DATA &= ~(I2C_SCL);
    _Wait(WAIT_1US * 10);
    
    return Value;
}
