// cmds.h
// temancl@fullplaymedia.com 03/06/02
// (c) fullplay media 
// 
// description:
// monitor command declarations

#ifndef _CMDS_H_
#define _CMDS_H_
#define MAX_CMDS 6
#include <stdio.h>
#include <cyg/infra/diag.h>
#include <cyg/kernel/kapi.h> 
#include "io.h"

#define MAX_STRING_LEN 32

// error return values
#define TEST_OK_PASS 0
#define TEST_ERR_FAIL -1
#define TEST_ERR_NOTFOUND -2
#define TEST_ERR_INTEGRITY -3
#define TEST_ERR_PARAMS -4

// memory commands
int test_readwords(char param_strs[][MAX_STRING_LEN],int* param_nums);
int test_writewords(char param_strs[][MAX_STRING_LEN],int* param_nums);
int test_readbytes(char param_strs[][MAX_STRING_LEN],int* param_nums);
int test_writebytes(char param_strs[][MAX_STRING_LEN],int* param_nums);
int test_readhalfwords(char param_strs[][MAX_STRING_LEN],int* param_nums);
int test_writehalfwords(char param_strs[][MAX_STRING_LEN],int* param_nums);
int test_reset(char param_strs[][MAX_STRING_LEN],int* param_nums);
int test_temp(char param_strs[][MAX_STRING_LEN],int* param_nums);

int memtest(unsigned long nBytes);

// hw shortcuts
int test_gpio(char param_strs[][MAX_STRING_LEN],int* param_nums);

// internal commands
int test_setverbosity(char param_strs[][MAX_STRING_LEN],int* param_nums);
int test_help(char param_strs[][MAX_STRING_LEN],int* param_nums);
int test_rem(char param_strs[][MAX_STRING_LEN],int* param_nums);
int test_echo(char param_strs[][MAX_STRING_LEN],int* param_nums);


extern cyg_uint8 verbosity;


// hardware module tests
int test_lcd(char param_strs[][MAX_STRING_LEN],int* param_nums);
int test_adc(char param_strs[][MAX_STRING_LEN],int* param_nums); // loopback test
int test_ata(char param_strs[][MAX_STRING_LEN],int* param_nums);
int test_eject(char param_strs[][MAX_STRING_LEN],int* param_nums);
int test_net(char param_strs[][MAX_STRING_LEN],int* param_nums);
int test_ir(char param_strs[][MAX_STRING_LEN],int* param_nums);
int test_key(char param_strs[][MAX_STRING_LEN],int* param_nums);
int test_dac(char param_strs[][MAX_STRING_LEN],int* param_nums);
int test_tone(char param_strs[][MAX_STRING_LEN],int* param_nums);

int test_bypass(char param_strs[][MAX_STRING_LEN],int* param_nums);
int test_gain(char param_strs[][MAX_STRING_LEN],int* param_nums);
int test_boost(char param_strs[][MAX_STRING_LEN],int* param_nums);
int test_adcbg(char param_strs[][MAX_STRING_LEN],int* param_nums);

int test_memtest(char param_strs[][MAX_STRING_LEN],int* param_nums);
int test_netmem(char param_strs[][MAX_STRING_LEN],int* param_nums);
int test_stress(char param_strs[][MAX_STRING_LEN],int* param_nums);
int test_net_slave(char param_strs[][MAX_STRING_LEN],int* param_nums);
int test_net_master(char param_strs[][MAX_STRING_LEN],int* param_nums);
int test_cdstress(char param_strs[][MAX_STRING_LEN],int* param_nums);
int test_hdstress(char param_strs[][MAX_STRING_LEN],int* param_nums);
int test_serial2(char param_strs[][MAX_STRING_LEN],int* param_nums);
int test_flash(char param_strs[][MAX_STRING_LEN],int* param_nums);

#endif // _CMDS_H_
