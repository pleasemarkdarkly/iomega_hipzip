#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_io.h>             // IO macros
#include <cyg/hal/hal_arch.h>           // Register state info
#include <cyg/hal/hal_intr.h>           // HAL interrupt macros
#include <cyg/hal/hal_if.h>
#include <cyg/hal/drv_api.h>
#include "cmds.h"

#define CS8900_BASE 0x20000000
#define ETHER_ADDR_LEN 6
#include "cs8900.h"

static unsigned char enaddr[ETHER_ADDR_LEN];


int test_netmem(char param_strs[][MAX_STRING_LEN],int* param_nums)
{

	unsigned short chip_type, chip_rev, chip_status;
    unsigned short reg, write_val, read_val;
    bool bmac = false;
    int i;

    
    cyg_drv_interrupt_acknowledge(CYGNUM_HAL_INTERRUPT_EINT3);
    
	put_reg(PP_BusCtl, PP_BusCtl_IOCH_RDYE);
    chip_type = get_reg(PP_ChipID);
    chip_rev = get_reg(PP_ChipRev);
	
 
	DEBUG2("CS8900A Ethernet Memory Test\n");

    DEBUG3("CS8900 - type: %x, rev: %x\n", chip_type, chip_rev);


	while(1)
	{
		
		// Verify memory on chip
		// Walk 1s on data bus
		for (write_val = 0x0001; write_val != 0; write_val <<= 1) {
			for (reg = 0x0150; reg <= 0x015d; reg += 2) {
				put_reg(reg, write_val);
				read_val = get_reg(reg);
				if (write_val != read_val) {
					DEBUG2("Memory Read Error\n");
					return TEST_ERR_FAIL;
				}
			}
		}

				// Incrementing value test
		for (reg = 0x0150, write_val = 0x0101; reg <= 0x015d; reg += 2, write_val += 0x0101) {
		put_reg(reg, write_val);
		}
		for (reg = 0x0150, write_val = 0x0101; reg <= 0x015d; reg += 2, write_val += 0x0101) {
		read_val = get_reg(reg);
		if (write_val != read_val) {
			DEBUG2("Memory Read Error\n");
			return TEST_ERR_FAIL;
		}
		} 

		DEBUG3("netmem loop pass\n");
	}

	return TEST_OK_PASS;
}

int test_net(char param_strs[][MAX_STRING_LEN],int* param_nums)
{ 

    unsigned short chip_type, chip_rev, chip_status;
    unsigned short reg, write_val, read_val;
    bool bmac = false;
    int i;

    
    cyg_drv_interrupt_acknowledge(CYGNUM_HAL_INTERRUPT_EINT3);
   
	put_reg(PP_BusCtl, PP_BusCtl_IOCH_RDYE);
 
    chip_type = get_reg(PP_ChipID);
    chip_rev = get_reg(PP_ChipRev);


	DEBUG2("CS8900A Ethernet Test\n");

    DEBUG3("CS8900 - type: %x, rev: %x\n", chip_type, chip_rev);


    // Verify memory on chip
    // Walk 1s on data bus
    for (write_val = 0x0001; write_val != 0; write_val <<= 1) {
	for (reg = 0x0150; reg <= 0x015d; reg += 2) {
	    put_reg(reg, write_val);
	    read_val = get_reg(reg);
	    if (write_val != read_val) {
		DEBUG2("Memory Read Error\n");
		return TEST_ERR_FAIL;
	    }
	}
    }
    

	
	// Incrementing value test
    for (reg = 0x0150, write_val = 0x0101; reg <= 0x015d; reg += 2, write_val += 0x0101) {
	put_reg(reg, write_val);
    }
    for (reg = 0x0150, write_val = 0x0101; reg <= 0x015d; reg += 2, write_val += 0x0101) {
	read_val = get_reg(reg);
	if (write_val != read_val) {
	    DEBUG2("Memory Read Error\n");
	    return TEST_ERR_FAIL;
	}
    } 

    DEBUG3("CS8900 - Memory OK\n");
    
    put_reg(PP_SelfCtl, PP_SelfCtl_Reset);  // Reset chip
    // TODO Cirrus driver reads uchar from PPtr, PPtr + 1, PPtr, PPtr + 1 here to transition SBHE from 8 to 16 bit
    // So what is SBHE? 
    while ((get_reg(PP_SelfStat) & PP_SelfStat_SIBSY) == 0) ; // Wait for EEPROM not busy
    while ((get_reg(PP_SelfStat) & PP_SelfStat_InitD) == 0) ; // Wait for initialization to be done

    chip_status = get_reg(PP_SelfStat);

    DEBUG3("CS8900 - status: %x (%sEEPROM present)\n", chip_status,
                chip_status&PP_SelfStat_EEPROM ? "" : "no ");
	
	put_reg(PP_BusCtl, PP_BusCtl_IOCH_RDYE);
 

    DEBUG3("CS8900 - ");
    for (i = 0;  i < ETHER_ADDR_LEN;  i += 2) {
        unsigned short esa_reg = get_reg(PP_IA+i);
        enaddr[i] = esa_reg & 0xFF; 
        enaddr[i+1] = esa_reg >> 8;
	DEBUG3("%02x %02x ", enaddr[i], enaddr[i+1]);
	
	// make sure eeprom is initialized with mac correctly

    }
    DEBUG3("\n");
    
	// check for fullplay ethernet address set (00:06:D4)
    if(enaddr[0] == 0x00 && enaddr[1] == 0x06 && enaddr[2] == 0xd4)
      return TEST_OK_PASS;
    else
      return TEST_ERR_FAIL;
}
