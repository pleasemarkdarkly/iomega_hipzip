#include <io/storage/blk_dev.h>
#include <io/storage/drives.h>
#include <devs/storage/ata/atadrv.h>
#include "cmds.h"


cyg_io_handle_t blk_devC,blk_devH;

static bool bInit = false;

int test_ata(char param_strs[][MAX_STRING_LEN],int* param_nums)
{  

	cyg_uint16 length;
	cyg_uint32 lba;
	unsigned int i;
	cyg_uint32 len;
	Cyg_ErrNo err;
	drive_geometry_t dg;

	DEBUG2("ATA init test\n");

	// intialize CD, get info
	if (cyg_io_lookup("/dev/cda/", &blk_devC) != ENOERR) 
	{
		DEBUG2("Could not get handle to dev");
		return TEST_ERR_FAIL;
	} 
	
	DEBUG3("got handle to cd\n");
	len = sizeof(len);

	while (cyg_io_set_config(blk_devC, IO_BLK_SET_CONFIG_POWER_UP, 0, &len) != ENOERR) 
	{
		DEBUG2("Could not power up device");
		return TEST_ERR_FAIL;
	}

	while (cyg_io_set_config(blk_devC, IO_ATAPI_SET_CONFIG_FEATURES, 0, &len) != ENOERR) 
	{
		DEBUG2("Could not power up device");
		return TEST_ERR_FAIL;
	}

	DEBUG3("powered up cd\n" );

	// intialize HD, get info
	if (cyg_io_lookup("/dev/hda/", &blk_devH) != ENOERR) 
	{
		DEBUG2("Could not get handle to dev");
		return TEST_ERR_FAIL;
	}

	DEBUG3("got handle to hd\n");
	len = sizeof(len);

	while (cyg_io_set_config(blk_devH, IO_BLK_SET_CONFIG_POWER_UP, 0, &len) != ENOERR) 
	{
		DEBUG2("Could not power up device");
		return TEST_ERR_FAIL;
	}

	while (cyg_io_set_config(blk_devH, IO_ATA_SET_CONFIG_FEATURES, 0, &len) != ENOERR) 
	{
		DEBUG2("Could not power up device");
		return TEST_ERR_FAIL;
	}

	DEBUG3("powered up hd\n" );


	len = sizeof(dg);
	
	if (cyg_io_get_config(blk_devH, IO_BLK_GET_CONFIG_GEOMETRY, &dg, &len) != ENOERR) 
	{
		DEBUG2("Could not get geometry");
		return TEST_ERR_FAIL;
	}

	DEBUG3("HD Info\n");
	DEBUG3("C/H/S: %d/%d/%d\n", dg.cyl, dg.hd, dg.sec);
	DEBUG3("Sector Size: %d\n", dg.bytes_p_sec);
	DEBUG3("Total Sectors: %d\n", dg.num_blks);
	dg.serial_num[40] = 0; dg.model_num[40] = 0;
	DEBUG3("SN: %s| MN: %s|\n", dg.serial_num, dg.model_num);

	bInit = true;

	return TEST_OK_PASS;

}


static int bClosed = 1;

int test_eject(char param_strs[][MAX_STRING_LEN],int* param_nums)
{
	
	if(!bInit)
	{
		DEBUG2("Initializing ATA\n");
		test_ata(NULL,NULL);
	}

	if(!bInit)
	{
		DEBUG2("Failed to init ATA\n");
		return TEST_ERR_FAIL;
	}

	DEBUG2("CD Eject\n");
	cyg_uint32 len = sizeof(len);
	cyg_uint8 len8 = sizeof(cyg_uint8);
	Cyg_ErrNo err = ENOERR;
	err = cyg_io_get_config(blk_devC, IO_BLK_GET_CONFIG_MEDIA_STATUS, &len8, &len8);
	if (err == -ENOMED && (bClosed == 0)) 
	{
		// tray open
		DEBUG3("closing cd\n");
		// eject CD tray
		if(cyg_io_set_config(blk_devC, IO_ATAPI_SET_CONFIG_TRAY_CLOSE, 0, &len) == ENOERR) 
		{
		  DEBUG3("tray close\n");
		  bClosed = 1;
		}

	
	}
	else 
	{
		if(cyg_io_set_config(blk_devC, IO_ATAPI_SET_CONFIG_TRAY_OPEN, 0, &len) == ENOERR) 
		{
		  DEBUG3("tray open\n");
		  bClosed = 0;
		}
	}

	return TEST_OK_PASS;
   
}
