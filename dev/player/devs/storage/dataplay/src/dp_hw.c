// dp_hw.c: hw support routines for dataplay
// danc@iobjects.com 12/13/2000
// (c) Interactive Objects

#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/hal/hal_diag.h>
#include <cyg/hal/drv_api.h>

#include "dp_hw.h"

#define DEBUG(s...) diag_printf(##s)

// ******************* support routines *********************

static cyg_uint32 dp_interrupt_isr( cyg_vector_t, cyg_addrword_t );
static void dp_interrupt_dsr( cyg_vector_t, cyg_ucount32, cyg_addrword_t );
static void dp_wait_busy_clear( void );
static cyg_uint16 dp_wait_cds_int( void );


static cyg_sem_t dp_cds_sem, dp_att_sem;
static cyg_interrupt dp_int;
static cyg_handle_t dp_int_handle;
static bool dp_interrupt_initialized = false;

//bool dp_media_present = false;

// #define DO_HARD_RESET

int dp_hw_init( void ) {
  
  cyg_uint8 data, bc;
  // do our setup

  // memory config
  *(volatile cyg_uint32*) DP_MEMCFG_REG &= ~DP_MEMCFG_MASK;
  *(volatile cyg_uint32*) DP_MEMCFG_REG |= DP_MEMCFG_VAL;

#if defined(DO_HARD_RESET)
  *(volatile cyg_uint8 *)PBDDR |= 0x60;

  cyg_thread_delay( 20 );
  /* force reset */
  *(volatile cyg_uint8 *)PBDR |= 0x60;

  cyg_thread_delay( 20 );
  /* Power on interface */
  *(volatile cyg_uint8 *)PBDR &= ~0x20;

  cyg_thread_delay( 20 );
  /* Enable interface */
  *(volatile cyg_uint8 *)PBDR &= ~0x40;

  *(volatile cyg_uint8 *)PBDDR &= ~0x60;

  cyg_thread_delay( 50 );
#endif
  
  /* Write Control register with 'Enable Power-On Signature' function code. */

  DP_WRITE_CONTROL( DP_C_FC_POS_ENABLE );

  // verify drive status
  data = DP_READ_DATA();
  bc   = (cyg_uint8) DP_READ_BYTECOUNT();


  diag_printf("data = %x, bc = %x\n",data,bc);

  // wait for busy to clear
  dp_wait_busy_clear();

  // set up our interrupt code
  if( !dp_interrupt_initialized ) {
    // create our semaphores
    cyg_semaphore_init( &dp_cds_sem, 0 );
    cyg_semaphore_init( &dp_att_sem, 0 );

    // create our interrupt
    cyg_drv_interrupt_create( DP_INTERRUPT,
			      10,
			      0,
			      (cyg_ISR_t*) dp_interrupt_isr,
			      (cyg_DSR_t*) dp_interrupt_dsr,
			      &dp_int_handle,
			      &dp_int );

    cyg_drv_interrupt_attach( dp_int_handle );

    cyg_drv_interrupt_acknowledge( DP_INTERRUPT );

    dp_interrupt_initialized = true;
    cyg_drv_interrupt_unmask( DP_INTERRUPT );
  }

  DP_WRITE_CONTROL( DP_C_FC_ACK_ALL );  // since we just unmasked the interrupts, ack all
  
  // return 1 on success, 0 on failure
  return ( data == 0xAA && bc == 0x55 );
}


static cyg_uint32
dp_interrupt_isr( cyg_vector_t vector, cyg_addrword_t data ) {
  cyg_drv_interrupt_mask( DP_INTERRUPT );
  return (CYG_ISR_HANDLED | CYG_ISR_CALL_DSR);
}

static void
dp_interrupt_dsr( cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data ) {
  cyg_uint8 status = DP_READ_STATUS();

  if( status & DP_S_ATT_IRQ ) {
    // hack
    // dp_media_present = true;
    DP_WRITE_CONTROL( DP_C_FC_ACK_AI );
    cyg_semaphore_post( &dp_att_sem );
  }

  // determine the type of interrupt
  if( status & DP_S_CDS_IRQ ) {
    DP_WRITE_CONTROL( DP_C_FC_ACK_CDSI );
    cyg_semaphore_post( &dp_cds_sem );
  }

  // unmask the interrupt so we can receive more
  cyg_drv_interrupt_unmask( DP_INTERRUPT );
}


static void dp_wait_busy_clear( void ) {
  cyg_uint8 status;
  do {
	// FIXME: Wait timeout value?
    status = DP_READ_STATUS();
  } while( status & DP_S_BUSY );
}

static cyg_uint16 dp_wait_cds_int( void ) {
  cyg_uint16 bc;
  
  if( dp_interrupt_initialized ) {
    cyg_semaphore_wait( &dp_cds_sem );
  } else {
    cyg_uint8 status;
    do {
      status = DP_READ_STATUS();
    } while( !( status & DP_S_CDS_IRQ ));
    // ack the interrupt
    DP_WRITE_CONTROL(DP_C_FC_ACK_CDSI);
  }

  bc = dp_read_bytecount();

  return bc;
}

cyg_uint16 dp_read_bytecount( void )
{
	cyg_uint8 status,low,high;
	cyg_uint16 bytecount = 0;
	volatile cyg_uint8 counter = 1000;

	// write "Reset Byte Count Pointer" to control register
///	DP_WRITE_CONTROL(DP_C_FC_RESET_BC);

	// check that the DATA bit in the status register is set

while(1)
{	
	DP_WRITE_CONTROL(DP_C_FC_RESET_BC);
	low = DP_READ_REGISTER(DP_BYTECOUNT_REGISTER);			
	high = DP_READ_REGISTER(DP_BYTECOUNT_REGISTER);			
}
	status = DP_READ_STATUS();
	diag_printf("high = %x, low = %x\n",high,low);

	bytecount = ((cyg_uint16)high << 8) | (cyg_uint16)low;
		
	return bytecount;

}


static void dp_cmd_init( int cmdsize ) {
  // make sure busy is clear before sending a command
  dp_wait_busy_clear();

	// reset byte count
  DP_WRITE_CONTROL( DP_C_FC_RESET_BC );
  DP_WRITE_BYTECOUNT( cmdsize );
  DP_WRITE_CONTROL( DP_C_FC_STARTCMD );
}

static void dp_send_data_bulk( const char* data, int len ) {
  int i;
  for( i = 0; i < len; i++ ) {
    DP_WRITE_DATA( data[i] );
  }
}

static void dp_read_data_bulk( char* data, int len ) {
  int i;
  for( i = 0; i < len; i++ ) {
    data[i] = DP_READ_DATA();
  }
}

void dp_write_data( const char* data, int count ) {
  cyg_uint16 bc;

  bc = dp_wait_cds_int();

  // read how much data is going to be sent, verify it's the same
  if( count != bc ) {
    DEBUG("%s (%d): data mismatch (%d, %d)\n", __FUNCTION__, __LINE__, count, bc);
  }

  dp_send_data_bulk( data, count );
}

void dp_read_data( char* data, int len ) {
  cyg_uint16 bc;

  bc = dp_wait_cds_int();

  if( bc > len ) {
    DEBUG("%s (%d): buffer to small for read request\n",
	  __FUNCTION__, __LINE__ );
  }

  dp_read_data_bulk( data, bc );
}

void dp_send_cmd( const char* cmd, int cmdsize ) {
  cyg_uint16 bc;
  // reset the byte counter and send the start cmd code
  dp_cmd_init( cmdsize );

  // wait for the cmd interrupt to come in
  bc = dp_wait_cds_int();

  //  if( dp_verify_phase( DP_CMD_PHASE ) == 0 ) {
  //  	DEBUG("%s (%d): Warning: phase error (cmd bytes: %x %x, status %x)\n", 
  //	      __FUNCTION__, __LINE__, cmd[0], cmd[1], DP_READ_STATUS() );
  //  }

  // DP_S_DATA should be high at this point
  // write out all our data bytes to the data register

  dp_send_data_bulk( cmd, cmdsize );

  // note: according to the spec, after the data is done writing, DP_S_DATA will go low,
  // however this is a very brief period before it goes high again to indicate the device
  // has a response ready, so checking for DP_S_DATA here doesn't yield any conclusive results.
}

void dp_read_response( char* buf, int bufsize ) {
  //	int i;
  cyg_uint16 bc;

  // wait for the interrupt indicating that data is available
  bc = dp_wait_cds_int();

  // putting this print in here prevents the bottom warning
  // about DP_S_DATA from happening. beats me.
  //	DEBUG("%s (%d): Status: %x\n", __FUNCTION__, __LINE__, DP_READ_STATUS() );

	// figure out how much data we have to read
  //  bytecount = DP_READ_BYTECOUNT();
	
  DEBUG("bc = %d\n",bc);
  // sanity check
  if( bc <= 1 ) {
    DEBUG("%s (%d): Warning: bytecount came back as %d\n", __FUNCTION__, __LINE__, bc );
  }

  // sanity check: make sure we have enough space
  if( bufsize < bc ) {
    DEBUG("%s (%d): Warning: Insufficient buffer space (bufsize = 0x%x, bytecount = 0x%x )\n",
	  __FUNCTION__, __LINE__, bufsize, bc );
    // avoid memory overruns
    bc = bufsize;
  }

  DEBUG("bulk read\n");
  dp_read_data_bulk( buf, bc );

  // sanity check: make sure DP_S_DATA is low now
  //	if( DP_READ_STATUS() & DP_S_DATA ) {
  //		DEBUG("%s (%d): Warning: DP_S_DATA is still high after end of transfer\n", __FUNCTION__, __LINE__ );
  //	}

}

int dp_verify_phase( int phase ) {
  cyg_uint8 status = DP_READ_STATUS();
  status = (cyg_uint8)phase;
  return ( ( status & DP_PHASE_BITS ) == phase );
}


