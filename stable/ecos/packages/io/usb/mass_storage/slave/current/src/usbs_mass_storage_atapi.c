//==========================================================================
//
//      usbs_mass_storage_atapi.c
//
//      Support for USB-mass storage ATAPI devices, slave-side.
//
//==========================================================================
//####COPYRIGHTBEGIN####
//                                                                          
// -------------------------------------------                              
// -------------------------------------------                              
//                                                                          
//####COPYRIGHTEND####
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    toddm@iobjects.com
// Contributors: toddm@iobjects.com
// Date:         2001-21-02
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <pkgconf/io_usb_slave_mass_storage.h>

#if defined(CYGPKG_IO_USB_SLAVE_MASS_STORAGE_DEVICE_ATAPI)

#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/hal/hal_diag.h>
#include <cyg/hal/drv_api.h>

#include <cyg/io/usb/usbs_mass_storage.h>
#include <cyg/io/usb/usbs_mass_storage_atapi.h>
#include "usbs_mass_storage_atapi_misc.h"

#if 1
# include <cyg/infra/diag.h>
# define DBG(a) diag_printf a
#else
# define DBG(a)
#endif

#if defined(USE_IRQ)
static cyg_interrupt ata_interrupt;
static cyg_handle_t ata_interrupt_handle;
cyg_sem_t ata_sem;
#endif /* USE_IRQ */

#if defined(USE_IRQ)
static cyg_uint32
ata_interrupt_isr(cyg_vector_t vector, cyg_addrword_t data)
{
    cyg_drv_interrupt_mask(ATA_IRQ);
    return (CYG_ISR_HANDLED | CYG_ISR_CALL_DSR);
}

static void
ata_interrupt_dsr(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data)
{
    unsigned char status;
    
    cyg_semaphore_post(&ata_sem);

    /* Clear the interrupt */
    /* The interrupt would get cleared fine in _ProcessInterrupt without the
     * following two lines, but then this code would have to be repeated in
     * quite a few places.  Short story, cleaner to put it here */
    _ReadStatus(status);
    cyg_drv_interrupt_unmask(ATA_IRQ);
}
#endif /* USE_IRQ */

static int
poll_status(unsigned char clear, unsigned char set, cyg_tick_count_t timeout)
{
    cyg_tick_count_t ticks;
    unsigned char status;
    unsigned int poll_status_count = 0;
    
    /* Do one read of status register very quickly to get out of the loop 
     * quicker if busy is already cleared */
    _ReadStatus(status);
    
    if ((status & clear) || ((status & set) != set)) {
    	timeout += cyg_current_time();
	do {
	    _ReadStatus(status);
	    ticks = cyg_current_time();
	    ++poll_status_count;
	} while(((status & clear) || ((status & set) != (set))) &&
		(ticks < timeout));
	
	if (ticks >= timeout) {
	    DBG(("%s %d exit -ETIME\n", __FUNCTION__, __LINE__));
	    return(-ETIME);
	}
    }
    if (poll_status_count > 0) {
	DBG(("PS Count %d\n", poll_status_count));
    }
    return(ENOERR);
}

static void
read_data_bulk(cyg_uint16 * buffer, int length)
{
    int i;

#if !defined(USE_MEMORY_MODULE)
    /* Re-configure bus for 16 bit data register */
    *(volatile cyg_uint32 *)MEMCFG2 &= ~ATA_MEMCFG_MASK;
    *(volatile cyg_uint32 *)MEMCFG2 |= ATA_16BIT_MEMCFG;
#endif /* USE_MEMORY_MODULE */
    
    for (i = 0; i < length; ++i) {
	buffer[i] = *((volatile cyg_uint16 *)ATA_DATA_REG);
    }

#if !defined(USE_MEMORY_MODULE)
    /* Re-configure bus for 8 bit task file registers */
    *(volatile cyg_uint32 *)MEMCFG2 &= ~ATA_MEMCFG_MASK;
    *(volatile cyg_uint32 *)MEMCFG2 |= ATA_8BIT_MEMCFG;
#endif /* USE_MEMORY_MODULE */
}

static void
write_data_bulk(cyg_uint16 * buffer, int length)
{
    int i;

    cyg_uint16 word;
    cyg_uint8 * byte_buffer = (cyg_uint8 *)buffer;
    
#if !defined(USE_MEMORY_MODULE)
    /* Re-configure bus for 16 bit data register */
    *(volatile cyg_uint32 *)MEMCFG2 &= ~ATA_MEMCFG_MASK;
    *(volatile cyg_uint32 *)MEMCFG2 |= ATA_16BIT_MEMCFG;
#endif /* USE_MEMORY_MODULE */

    for (i = 0; i < length * 2; i += 2) {
	/* This must be aligned */
	word = byte_buffer[i] | ((byte_buffer[i + 1]) << 8);
	*((volatile cyg_uint16 *)ATA_DATA_REG) = word;
    }

#if !defined(USE_MEMORY_MODULE)
    /* Re-configure bus for 8 bit task file registers */
    *(volatile cyg_uint32 *)MEMCFG2 &= ~ATA_MEMCFG_MASK;
    *(volatile cyg_uint32 *)MEMCFG2 |= ATA_8BIT_MEMCFG;
#endif /* USE_MEMORY_MODULE */
}

#if defined(USE_MEMORY_MODULE)
static int
hard_reset_bus(void)
{
    int status = ENOERR;
    
    /* Assert reset */
#if defined(USE_MEMORY_MODULE)
    *(volatile cyg_uint8 *)PBDDR |= 0x10;
    *(volatile cyg_uint8 *)PBDR &= ~0x10;
#endif /* USE_MEMORY_MODULE */
    
    /* Wait for at least 25 us */
    HAL_DELAY_US(25);
    
    /* Negate reset and wait */
#if defined(USE_MEMORY_MODULE)
    *(volatile cyg_uint8 *)PBDR |= 0x10;
#endif /* USE_MEMORY_MODULE */
    
    /* Wait for at least 2 ms */
    HAL_DELAY_US(2000);
    
    /* Check status */
    if (poll_status(STATUS_BSY, 0, TIMEOUT_5S)) {
	status = -ETIME;
    }

    return status;
}
#endif /* USE_MEMORY_MODULE */

static int
soft_reset_bus(void)
{
    int status = ENOERR;
#if defined(USE_CLIK)
    int reset_count = 0;
#endif /* USE_CLIK */
    
    /* Software reset protocol */

#if defined(USE_CLIK)
  sr_set_srst:
#endif /* USE_CLIK */
#if defined(USE_IRQ)
    _WriteDeviceControl((CONTROL_SRST|CONTROL_IRQ));
#else /* USE_IRQ */
    _WriteDeviceControl((CONTROL_SRST|CONTROL_POLLED));
#endif /* USE_IRQ */
    HAL_DELAY_US(5);
    goto sr_clear_wait;
    
  sr_clear_wait:
#if defined(USE_IRQ)
    _WriteDeviceControl(CONTROL_IRQ);
#else /* USE_IRQ */
    _WriteDeviceControl(CONTROL_POLLED);
#endif /* USE_IRQ */
    HAL_DELAY_US(2000);
    goto sr_check_status;

  sr_check_status:
#if defined(USE_CLIK)
    if (poll_status(STATUS_BSY, 0, TIMEOUT_1S)) {
	if (reset_count < 3) {
	    ++reset_count;
	    goto sr_set_srst;
	}
	status = -ETIME;
    }
#else /* USE_CLIK */
    if (poll_status(STATUS_BSY, 0, TIMEOUT_5S)) {
	status = -ETIME;
    }
#endif /* USE_CLIK */

    return status;
}

void
usbs_ms_protocol_init(void)
{
    /* Initialize bus */
#if defined(USE_MEMORY_MODULE)
    *(volatile cyg_uint32 *)MEMCFG1 &= ~ATA_MEMCFG_MASK;
    *(volatile cyg_uint32 *)MEMCFG1 |= ATA_16BIT_MEMCFG;
#else /* USE_MEMORY_MODULE */
    *(volatile cyg_uint32 *)MEMCFG2 &= ~ATA_MEMCFG_MASK;
    *(volatile cyg_uint32 *)MEMCFG2 |= ATA_8BIT_MEMCFG;
    *(volatile cyg_uint32 *)SYSCON1 |= SYSCON1_EXCKEN;
#endif /* USE_MEMORY_MODULE */
    
#if defined(USE_MEMORY_MODULE)
    /* TODO Power management */
    
    /* Power on interface */
    *(volatile cyg_uint8 *)PBDDR |= 0x20;
    *(volatile cyg_uint8 *)PBDR &= ~0x20;
    
    /* Enable interface */
    *(volatile cyg_uint8 *)PBDDR |= 0x40;
    *(volatile cyg_uint8 *)PBDR &= ~0x40;
    
    /* TODO This doesn't work with the USB module plugged in */
#if 0//defined(USE_CF)
    /* Configure card detect GPIO */
    *(volatile cyg_uint8 *)PDDDR |= 0x04;/* nCD1 */
    
    /* Configure card detect IRQ */
    /* nEINT1 : nCD2 */
    cyg_semaphore_init(&CFCardDetectSem, 0);
    cyg_drv_interrupt_create(CF_CARD_DETECT_IRQ,
			     10,
			     0,
			     (cyg_ISR_t *)_CFCardDetectInterrupt_isr,
			     (cyg_DSR_t *)_CFCardDetectInterruptDSR,
			     &_CFCardDetectInterrupt_handle,
			     &_CFCardDetectInterrupt);
    cyg_drv_interrupt_attach(_CFCardDetectInterrupt_handle);
    cyg_drv_interrupt_acknowledge(CF_CARD_DETECT_IRQ);
    cyg_drv_interrupt_unmask(CF_CARD_DETECT_IRQ);
#endif /* USE_CF */
#endif /* USE_MEMORY_MODULE */

#if defined(USE_IRQ)
    /* Set up interrupt handler */
    cyg_semaphore_init(&ata_sem, 0);
    cyg_drv_interrupt_create(ATA_IRQ,
			     10,
			     0,
			     (cyg_ISR_t *)ata_interrupt_isr,
			     (cyg_DSR_t *)ata_interrupt_dsr,
			     &ata_interrupt_handle,
			     &ata_interrupt);
    cyg_drv_interrupt_attach(ata_interrupt_handle);
    cyg_drv_interrupt_acknowledge(ATA_IRQ);
#endif /* USE_IRQ */

    /* Reset the bus */
#if defined(USE_MEMORY_MODULE)
    hard_reset_bus();
#else /* USE_MEMORY_MODULE */
    soft_reset_bus();
#endif /* USE_MEMORY_MODULE */
    
#if defined(USE_IRQ)
    /* Unmask the interrupt */
    cyg_drv_interrupt_unmask(ATA_IRQ);
#endif /* USE_IRQ */
}

int
usbs_ms_protocol_do_command(usbs_mass_storage * ms, int * bytes_processed)
{
    unsigned char buf[64];	/* Must be a multiple of 64 bytes */
    int buf_valid = 0;
    int buf_i = 0;
    int status;
    unsigned char interrupt_reason;
    unsigned char byte_count_low, byte_count_high;
    unsigned short byte_count;
#if defined(USE_CLIK)
    int num_phase_errors = 0;
#endif /* USE_CLIK */
    
    /* Send command stage */
    if (poll_status(STATUS_BSY | STATUS_DRQ, 0, TIMEOUT_5S)) {
	return -ETIME;
    }

#if defined(USE_CLIK)
    _WriteDeviceHead(SELECT_MASTER);
#else /* USE_CLIK */
    // TODO Multiple IDE devices
    _WriteDeviceHead(SELECT_MASTER);
#endif /* USE_CLIK */
    if (poll_status(STATUS_BSY | STATUS_DRQ, 0, TIMEOUT_5S)) {
	return -ETIME;
    }
    _WriteFeatures(0);
#if defined(USE_CLIK)
    _WriteCylinderLow(0x00);
    _WriteCylinderHigh(0x02);
#else
    // TODO This assumes a CD drive
    _WriteCylinderLow(0x00);
    _WriteCylinderHigh(0x08);
#endif
    _WriteCommand(ATA_PACKET);    

    /* Process interrupt stage */
    
  p_check_status_a:
    /* Wait for BSY to clear */
#if defined(USE_CLIK)  || defined(DVD1640PRO)
    /* Clik may clear BSY before setting DRQ, so wait for DRQ also */
    if (poll_status(STATUS_BSY, STATUS_DRQ, TIMEOUT_5S)) {
	return -ETIME;
    }
#else /* USE_CLIK */
    if (poll_status(STATUS_BSY, 0, TIMEOUT_5S)) {
	return -ETIME;
    }
#endif /* USE_CLIK */

    /* Check for error */
    _ReadStatus(status);
    if (status & STATUS_ERR) {
	return -EIO;
    }
	    
    /* Determine next state to go to */
    if (status & STATUS_DRQ) {

	/* This may be more than just a Clik issue */
#if defined(USE_IRQ) && defined(USE_CLIK)
	cyg_semaphore_trywait(&ata_sem);
#endif /* USE_IRQ && USE_CLIK */
	
	/* Send packet */
	goto p_send_packet;
    }
    
    _ReadSectorCount(interrupt_reason);
    if (!(status & STATUS_BSY) && !(status & STATUS_DRQ) &&
	!(interrupt_reason & INTERRUPT_REL) && !(status & STATUS_SERV)) {
	
	/* Command complete */
	if (status & STATUS_ERR) {
	    
	    /* Error occured */
	    return -EIO;
	}
	else {
	    return ENOERR;
	}
    }

    /* No transition defined for this state now so stay in this state */
    DBG(("%s %d No transition defined, staying in state\n", __FUNCTION__, __LINE__));
    goto p_check_status_a;

    /* Awaiting command */
  p_send_packet:
	    
    /* Check that device is in correct state */
    _ReadSectorCount(interrupt_reason);
    if ((interrupt_reason & INTERRUPT_IO) || !(interrupt_reason & INTERRUPT_COMMAND)) {
	DBG(("%s %d Send packet out of phase %x\n", __FUNCTION__, __LINE__, interrupt_reason));
	return -EPHASE;
    }

    /* Write command packet */
    // TODO This could be 8 also, although have never seen it
    write_data_bulk((cyg_uint16 *)ms->cbw.CBWCB, 6);

#if defined(USE_IRQ)
    goto p_intrq_wait;
#else /* USE_IRQ */
    /* Wait one PIO transfer cycle time before transitioning */
    _ReadAlternateStatus(status);
    goto p_check_status_b;
#endif /* USE_IRQ */
    
  p_check_status_b:
    
    if (poll_status(STATUS_BSY, 0, TIMEOUT_5S)) {
	return -ETIME;
    }
    
    /* Check for error */
    _ReadStatus(status);
    if (status & STATUS_ERR) {
	return -EIO;
    }
    
    /* (Ready to transfer data or command complete) & nIEN = 0
     * This transition is moved to end of p_transfer_data */
	    
#if defined(USE_CLIK)
    /* It looks like Clik doesn't set DRQ in this state */
    goto p_transfer_data;
#else /* USE_CLIK */
    
    if (status & STATUS_DRQ) {
	goto p_transfer_data;
    }

    if (!(status & STATUS_BSY) && !(status & STATUS_DRQ)) {
	if (!(interrupt_reason & INTERRUPT_REL) && !(status & STATUS_SERV)) {
	    
	    /* Command complete if device queue is empty (command queuing not currently supported) */
	    if (status & STATUS_ERR) {
		
		/* Error occured */
		return -EIO;
	    }
	    else {
		return ENOERR;
	    }
	}
	else if ((interrupt_reason & INTERRUPT_REL) && !(status & STATUS_SERV)) {
	    
	    /* The bus has been released */
#if defined(USE_IRQ)
	    /* TODO Transition to HIO0:INTRQ_wait_A */
	    DBG(("%s %d Unsupported transition\n", __FUNCTION__, __LINE__));
#else /* USE_IRQ */
	    /* TODO Transition to HIO3:Check_status_A */
	    DBG(("%s %d Unsupported transition\n", __FUNCTION__, __LINE__));
#endif /* USE_IRQ */
	}
	else if (status & STATUS_SERV) {
	    
	    /* The command is completed or the bus has been released */
	    /* TODO Assume command complete for now.  Spec says to transition to HIO5:Write_SERVICE,
	     * but since overlap commands are not supported in this driver, don't do that. */
	    if (status & STATUS_ERR) {
		
		/* Error occured */
		return -EIO;
	    }
	    else {
		return ENOERR;
	    }
	}
    }
    
    DBG(("%s %d No transition specified, staying in this state\n", __FUNCTION__, __LINE__));
    goto p_check_status_b;
#endif /* USE_CLIK */
    
#if defined(USE_IRQ)
  p_intrq_wait:
    if (!cyg_semaphore_timed_wait(&ata_sem, cyg_current_time() + TIMEOUT_5S)) {
	return -ETIME;
    }
    goto p_check_status_b;
#endif /* USE_IRQ */
    
  p_transfer_data:
    _ReadStatus(status);
    _ReadSectorCount(interrupt_reason);
    switch ((status & STATUS_DRQ) | (interrupt_reason & (INTERRUPT_IO | INTERRUPT_COMMAND))) {
	case STATUS_DRQ: /* DRQ, !COMMAND, !IO = Data from host phase */ 
	{
	    _ReadCylinderLow(byte_count_low);
	    _ReadCylinderHigh(byte_count_high);
	    byte_count = (byte_count_high << 8) | byte_count_low;
	    
	    if ((ms->cbw.bmCBWFlags & CBW_DIRECTION) == 0) {
		while (byte_count) {
		    if (*bytes_processed >= ms->cbw.dCBWDataTransferLength) {
			return -EINVAL;
		    }

		    // If we alread received the data then write it to the device
		    if (byte_count <= (buf_valid - buf_i)) {
			write_data_bulk((cyg_uint16 *)&buf[buf_i], (byte_count / 2));
			*bytes_processed += byte_count;
			buf_i += byte_count;
			byte_count = 0;
		    }
		    else {
			int bytes_to_rx;

			// Write the remaining data in the buffer
			write_data_bulk((cyg_uint16 *)&buf[buf_i], (buf_valid - buf_i) / 2);
			byte_count -= (buf_valid - buf_i);
			*bytes_processed += (buf_valid - buf_i);

			// Get more data
			bytes_to_rx = byte_count < sizeof(buf) ? byte_count : sizeof(buf);
			usbs_start_rx_buffer(ms->rx_endpoint, buf, bytes_to_rx,
					     ms->rx_completion_fn, ms);
			cyg_semaphore_wait(&ms->rx_completion_wait);
			buf_valid = ms->rx_result;
			if (buf_valid < 0) {
			    return buf_valid;
			}
			buf_i = 0;
		    }
		}
	    }
	    else {
		
		/* Wrong direction. */
		return -EPHASE;
	    }
	    break;
	}

	case (STATUS_DRQ | INTERRUPT_IO): /* DRQ, !COMMAND, IO = Data to host phase */ 
	{
	    _ReadCylinderLow(byte_count_low);
	    _ReadCylinderHigh(byte_count_high);
	    byte_count = (byte_count_high << 8) | byte_count_low;

	    if (ms->cbw.bmCBWFlags & CBW_DIRECTION) {
		status = 0;	/* TODO This is a fix for usbs_start_tx_buffer returning without
				   setting status.  I think the semaphore might be getting set
				   one time too many somewhere. */
		while (byte_count) {
		    int bytes_to_tx;
		    if (*bytes_processed == ms->cbw.dCBWDataTransferLength) {
			return -EINVAL;
		    }
		    bytes_to_tx = byte_count < sizeof(buf) ? byte_count : sizeof(buf);
		    read_data_bulk((cyg_uint16 *)buf, (bytes_to_tx / 2));
		    usbs_start_tx_buffer(ms->tx_endpoint, buf, bytes_to_tx,
					 ms->tx_completion_fn, ms);
		    cyg_semaphore_wait(&ms->tx_completion_wait);
		    status = ms->tx_result;
		    if (status >= 0) {
			*bytes_processed += status;
			byte_count -= status;
		    }
		    else {
			return status;
		    }
		}
	    }
	    else {
		
		/* Wrong direction. */
		return -EPHASE;
	    }
	    break;
	}
	
	case (INTERRUPT_COMMAND | INTERRUPT_IO): /* !DRQ, COMMAND, IO = Status phase */
	{
	    if (status & STATUS_ERR) {
		return -EIO;
	    }
	    else {
		return ENOERR;
	    }
	    break;
	}
	
	default: 
	{
	    /* Phase error */
	    DBG(("%s %d Phase error\n", __FUNCTION__, __LINE__));
#if defined(USE_CLIK)
	    ++num_phase_errors;
	    if (num_phase_errors > 5) {
		return -EPHASE;
	    }
	    else {
#if defined(USE_IRQ)
		/* Fake an interrupt so that we may retry */
		cyg_semaphore_post(&ata_sem);
#endif /* USE_IRQ */
	    }
#else /* USE_CLIK */
	    return -EPHASE;
#endif /* USE_CLIK */
	    break;
	}
    }
#if defined(USE_IRQ)
    goto p_intrq_wait;
#else /* USE_IRQ */
    
    /* Wait one PIO transfer cycle time before transitioning */
    _ReadAlternateStatus(status);
    goto p_check_status_b;
#endif /* USE_IRQ */
}

int
usbs_ms_protocol_check_cbw(cbw_t * cbw)
{
    int status = false;

    // Can't check bCBWCBLength without deciphering the protocol
#if defined(USE_CLIK)
    if (0 == cbw->bCBWLUN) {
	status = true;
    } 
#else /* USE_CLIK */
    // TODO This depends on the IDE devices attached
    if (0 == cbw->bCBWLUN) {
	status = true;
    }
#endif /* USE_CLIK */
    return status;
}

void
usbs_ms_protocol_reset_bus(void)
{
#if defined(USE_CLIK)
    hard_reset_bus();
#else /* USE_CLIK */
    soft_reset_bus();
#endif /* USE_CLIK */
}

int
usbs_ms_protocol_get_max_lun(void)
{
#if defined(USE_CLIK)
    return 0;
#else /* USE_CLIK */
    // TODO This depends on the IDE devices attached
    return 0;
#endif /* USE_CLIK */
}

#endif /* CYGPKG_IO_USB_SLAVE_MASS_STORAGE_DEVICE_ATAPI */

