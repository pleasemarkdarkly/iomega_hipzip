// dp_hw.h: dharma-specific hardware definitions for the dataplay drive
// danc@iobjects.com 12/13/2000

#ifndef __DP_HW_H__
#define __DP_HW_H__

#include <devs/storage/dataplay/_dp_hw.h>

#define DP_DATA_REGISTER       ( DP_BASE_ADDRESS + 0x00 )
#define DP_CONTROL_REGISTER    ( DP_BASE_ADDRESS + 0x02 )
#define DP_STATUS_REGISTER     ( DP_BASE_ADDRESS + 0x02 )
#define DP_BYTECOUNT_REGISTER  ( DP_BASE_ADDRESS + 0x20 )   // a5 on the processor -> a0 on dev

// register bits

#define DP_C_CMD_IRQ_ENABLE       ( 0x08 )     // enable command interrupt
#define DP_C_DATA_IRQ_ENABLE      ( 0x04 )     // enable data interrupt
#define DP_C_STATUS_IRQ_ENABLE    ( 0x02 )     // enable status interrupt
#define DP_C_ATTENTION_IRQ_ENABLE ( 0x01 )     // enable attention interrupt

// interrupts to enable
#define DP_C_INTERRUPTS  (DP_C_CMD_IRQ_ENABLE | DP_C_DATA_IRQ_ENABLE | \
                          DP_C_STATUS_IRQ_ENABLE )

// control register
#define DP_C_FUNCTION_CODE( x ) (( (x) << 4 ) | DP_C_INTERRUPTS )

#define DP_C_FC_NONE            DP_C_FUNCTION_CODE( 0x00 )   // no command
#define DP_C_FC_STARTCMD        DP_C_FUNCTION_CODE( 0x01 )   // start command
#define DP_C_FC_ABORT           DP_C_FUNCTION_CODE( 0x02 )   // abort command
#define DP_C_FC_RESET_BC        DP_C_FUNCTION_CODE( 0x03 )   // reset byte counter
#define DP_C_FC_POS_ENABLE      DP_C_FUNCTION_CODE( 0x04 )   // enable power on signature
#define DP_C_FC_ACK_AI          DP_C_FUNCTION_CODE( 0x05 )   // acknowledge attention interrupt
#define DP_C_FC_ACK_CDSI        DP_C_FUNCTION_CODE( 0x06 )   // acknowledge cmd/data/status interrupt
#define DP_C_FC_ACK_ALL         DP_C_FUNCTION_CODE( 0x07 )   // acknowledge all interrupts

// status register
#define DP_S_BUSY    ( 0x80 )     // device is busy
#define DP_S_DATA    ( 0x40 )     // data is available
#define DP_S_CDS_IRQ ( 0x20 )     // command/data/status interrupt
#define DP_S_ATT_IRQ ( 0x10 )     // attention interrupt
#define DP_S_RD_WR   ( 0x02 )     // bit is 1 when reading from drive, 0 when writing
#define DP_S_CD      ( 0x01 )     // phase sync on the drive:

// phase bits, these are DP_S_RD_WR | DP_S_CD
#define DP_WRITE_PHASE    0x00
#define DP_READ_PHASE     0x01
#define DP_CMD_PHASE      0x02
#define DP_STATUS_PHASE   0x03

#define DP_PHASE_BITS     0x03


// notes on the status register:
//  if( status == 0x03 ) : status bytes are to be read
//  if( status == 0x02 ) : command packets are to be written
//  if( status == 0x01 ) : data is to be read
//  if( status == 0x00 ) : data is to be written
// these bits are only valid if status & DP_S_DATA == 1


// register operations
#define DP_READ_REGISTER( x )      ( *(volatile cyg_uint8*) (x) )

#define DP_READ_STATUS()           DP_READ_REGISTER( DP_STATUS_REGISTER )


#define DP_READ_BYTECOUNT()        ( (cyg_uint16) DP_READ_REGISTER( DP_BYTECOUNT_REGISTER ) | \
	                               ( (cyg_uint16) DP_READ_REGISTER( DP_BYTECOUNT_REGISTER ) << 8 ) )

#define DP_READ_DATA()             DP_READ_REGISTER( DP_DATA_REGISTER )

#define DP_WRITE_REGISTER( x, y )  ( *(volatile cyg_uint8*) (x) = (cyg_uint8) (y) )

#define DP_WRITE_CONTROL( y )      DP_WRITE_REGISTER( DP_CONTROL_REGISTER, y )
#define DP_WRITE_BYTECOUNT( y )    {                                                         \
                                      cyg_uint16 z = (cyg_uint16)y;                          \
                                      DP_WRITE_REGISTER( DP_BYTECOUNT_REGISTER, z & 0xff );  \
                                      DP_WRITE_REGISTER( DP_BYTECOUNT_REGISTER, z >> 8 );    \
                                   }
#define DP_WRITE_DATA( y )         DP_WRITE_REGISTER( DP_DATA_REGISTER, y )


#ifdef __cplusplus
extern "C" {
#endif

// functions for hardware
int  dp_hw_init( void );

void dp_write_data( const char* data, int len );
void dp_read_data( char* data, int len );
cyg_uint16 dp_read_bytecount( void );
void dp_send_cmd( const char* cmd, int cmdsize );
void dp_read_response( char* buf, int bufsize );
int  dp_verify_phase( int phase );

#ifdef __cplusplus
};
#endif

#endif // __DP_HW_H__
