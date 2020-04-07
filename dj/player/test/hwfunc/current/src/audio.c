// DJ lcd test
// adc test, dac tests
//

#include <devs/audio/dai.h>
#include <devs/audio/cs5332.h>
#include "cmds.h"
#include "parser.h"

static int s_gain = 0;

static void
_Constructor(void)
{
    DAIInit();
}

int
Start(void)
{
    DACSetVolume(0);
    
    DAIEnable();
	DAIWrite(0);
	DACSetBypass(DAC_BYPASS_72);
	ADCSetGain(s_gain,s_gain);
    
    return 0;
}

int
Stop(void)
{
    DAIDisable();
    
    return 0;
}

int
StartRecord(void)
{
	DAISetLoopbackFIQ();    
    return 0;
}

int
StopRecord(void)
{
    DAISetNormalFIQ();
    return 0;
}

int test_gain(char param_strs[][MAX_STRING_LEN],int* param_nums)
{

	DEBUG2("Setting gain to %d\n",param_nums[0]);
	ADCSetGain(param_nums[0],param_nums[0]);
	s_gain = param_nums[0];

	return TEST_OK_PASS;
}


int test_boost(char param_strs[][MAX_STRING_LEN],int* param_nums)
{

	if(param_nums[0] > 0)
		ADCEnableMicBoost();
	else
		ADCDisableMicBoost();

	return TEST_OK_PASS;

}

int test_adcbg(char param_strs[][MAX_STRING_LEN],int* param_nums)
{

	unsigned int FrequencyList[] = {48000, 44100, 32000, 24000, 22050, 16000, 11025, 8000};
	unsigned int Frequency = 0;
	int sec = 30;
	cyg_int8 Volume;
	
	if(param_strs[0][0] == '\0')
	{
		return TEST_ERR_PARAMS;
	}

	if(strncmpci(param_strs[0],"ON",MAX_STRING_LEN) == 0)
	{
		DEBUG2("Background ADC Enable\n");

		_Constructor();

		Start();
		StartRecord();
	}
	else if(strncmpci(param_strs[0],"OFF",MAX_STRING_LEN) == 0)
	{
		DEBUG2("Background ADC Disable\n");
		StopRecord();
		Stop();
	}

	return TEST_OK_PASS;
}


int test_adc(char param_strs[][MAX_STRING_LEN],int* param_nums)
{       
  unsigned int FrequencyList[] = {48000, 44100, 32000, 24000, 22050, 16000, 11025, 8000};
  unsigned int Frequency = 0;
  int time;
  int sec = 30;
  cyg_uint8 clip;
  cyg_int8 Volume;

  if(param_nums[0] > 0)
	  sec = param_nums[0];

  DEBUG2("ADC Test for %d seconds\n",sec);

    _Constructor();

    Start();
	StartRecord();
    
	time = cyg_current_time();

	while(cyg_current_time() < (time + sec*100))
	{

		
		// delay a half second
		cyg_thread_delay(100);

		clip = ADCGetClip();

		DEBUG3("Clip %d Peak %d\n",ADCGetClip(),DAIGetPeak());	

		// read clip bits
	}

	StopRecord();
    Stop();

	return TEST_OK_PASS;
} 
