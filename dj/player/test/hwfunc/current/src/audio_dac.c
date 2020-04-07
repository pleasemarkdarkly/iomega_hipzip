#include <devs/audio/dai.h>
#include <devs/audio/cs4343.h>
#include <math.h>
#include "cmds.h"
#include "parser.h"

/* FUNCTIONS */

static int _GetNextBuffer(void);
static int bDac = 0;

static void
dac_on(void)
{
    DAIInit();
	DAIEnable();
	DACSetVolume(0);
	DACSetBypass(DAC_BYPASS_72);
	bDac = 1;
}

static void
dac_off(void)
{
    
	DAIDisable();
	bDac = 0;
}


static int
StartPlayback(void)
{
    DAIResetPlayback();
    _GetNextBuffer();
    
    return 0;
}

static short* _Buffer;
static short* _BufferBegin;
static short* _BufferEnd;

static int
_GetNextBuffer(void)
{
    unsigned int NumSamples;
    unsigned int NumSamplesLeft;
    for (;;) {
        //	NumSamplesLeft = DAIGetNextBuffer(&_Left, &_Right, &NumSamples);
        NumSamplesLeft = DAIGetNextBuffer( &_Buffer, &NumSamples );
	if (NumSamplesLeft <= 0) {
	    break;
	}
	unsigned int Delay = NumSamplesLeft / DAISamplesPerTick();
	cyg_thread_delay(Delay);
    }
    if (NumSamplesLeft == 0) {
	_BufferBegin = &_Buffer[0];
	_BufferEnd = &_Buffer[NumSamples];
	return 0;
    }
    else {
	return NumSamplesLeft;
    }
}

static void
_WriteFullBuffer(void)
{
    //    unsigned int BufferSize = _Left >= _LeftEnd ? _LeftEnd - _LeftBegin : _Left - _LeftBegin;
    unsigned int BufferSize = (_Buffer >= _BufferEnd) ? _BufferEnd - _BufferBegin : _Buffer - _BufferBegin;

    DAIWrite(BufferSize);
    _GetNextBuffer();
}

static int
Write(short * Left, short * Right, unsigned long SamplesPerChannel)
    //Write(short * Buffer, unsigned long SamplesPerChannel)
{
	unsigned long Sample;
    for (Sample = 0; Sample < SamplesPerChannel; ++Sample) {
        *_Buffer++ = *Left++;
        *_Buffer++ = *Right++;
	if (_Buffer >= _BufferEnd) {
	    _WriteFullBuffer();
	}
    }
    return 0;
}

static int
EndPlayback()
{
    _BufferEnd = _Buffer;
    _WriteFullBuffer();    
    return 0;
}

// square wave now
void makesample(int frequency, int samplerate, unsigned short* buffer, unsigned int length /* == buffersize */)
{
	int i;
	int tocopy; 
//	double red = 2.0*3.1415927*(double)frequency;

	// get one period of the sample rate
	unsigned int period = samplerate / frequency;
//	diag_printf("period = %d\n",period);
	
	for(i = 0; i < (period / 2); i++)
	{
	
		//buffer[i] = (unsigned short)(32768.0 + 8192.0*sin((double)(((double)(i))*red/(double)samplerate)));			
		buffer[i] = 0x7FFF;
		//diag_printf("%d %d\n",i,buffer[i]);
	}

	for(i = (period / 2); i < period; i++)
	{
		//buffer[i] = (unsigned short)(32768.0 + 8192.0*sin((double)(((double)(i))*red/(double)samplerate)));	
		buffer[i] = 0x8000;
		//diag_printf("%d %d\n",i,buffer[i]);

	}
	// stamp out the rest of the buffer, now that complete period has been calculated
	for(i = period; i < length;)
	{
		if((length - i) < period)		
			tocopy = length - i;
		else
			tocopy = period;
		
		memcpy(buffer + i,buffer,tocopy*2);

		i += tocopy;		
	}

}

// extra bit of slack for any possible alingment magic
static unsigned short sample_data[100000];

int test_bypass(char param_strs[][MAX_STRING_LEN],int* param_nums)
{
	if(bDac == 0)
		dac_on();

	DACSetBypass(param_nums[0]);

	return TEST_OK_PASS;


}

//tone <frequency> <samplerate> <length (sec)>
int test_tone(char param_strs[][MAX_STRING_LEN],int* param_nums)
{

	
	if(param_nums[0] && param_nums[1] && param_nums[2])
	{
		if(bDac == 0)
			dac_on();

		generate_tone(param_nums[0],param_nums[1],param_nums[2]);

	}
	else
	{
		DEBUG1("invalid parameters\n");
		return TEST_ERR_PARAMS;
	}

	return TEST_OK_PASS;


}




int
generate_tone(int freq, int rate, int len)
{
	// validate 3 parameters
	int i,sper,slen,splay,stoplay;


	DEBUG3("Setting sampling rate to %d\n", rate);
	if(DAISetSampleFrequency(rate) == 0)
	{
		// generate 1 second tone
		DEBUG3("Generating %dhz tone for %d second(s)\n",freq,len);

		// make length of generated tone evenly divisibly by frequency period (in samples)
		sper = rate / freq;

		// period-align buffer
		slen = rate - (rate % sper);

		// make a period-aligned sample (slen samples)
		makesample(freq, rate, sample_data, slen);

		StartPlayback();


		// samples to play
		stoplay = len * rate;

		// play exactly that number of samples
		for(i = 0; i < stoplay; i+=splay)
		{
			// end case
			if((stoplay - i) < slen)
				splay = (stoplay - i);
			else
				splay = slen;

			Write((short *)sample_data, (short *)sample_data, splay);
		}



		EndPlayback();
	}
	else
	{
		DEBUG2("Could not set sample rate to %d\n",rate);
		return TEST_ERR_PARAMS;
	}


	return TEST_OK_PASS;

}

// enable dac, disable dac, generate tone at playback rate
// dac <mode (optional)>
int
test_dac(char param_strs[][MAX_STRING_LEN],int* param_nums)
{

	unsigned int FrequencyList[] = {44100,22050,48000,32000};
	unsigned int Frequency = 0;

	if(param_strs[0][0] == '\0')
	{
		DEBUG2("Full DAC Test\n");
		dac_on();

		for (Frequency = 0; Frequency < (sizeof(FrequencyList) / sizeof(FrequencyList[0])); ++Frequency) 
		{
			generate_tone(1000, FrequencyList[Frequency], 2);
			cyg_thread_delay(100);
		}

		dac_off();

	}
	else
	{
		if(strncmpci(param_strs[0],"ON",MAX_STRING_LEN) == 0)
		{
			DEBUG2("DAC On\n");
			dac_on();
		}
		else if(strncmpci(param_strs[0],"OFF",MAX_STRING_LEN) == 0)
		{
			DEBUG2("DAC Off\n");
			dac_off();
		}
		else if(strncmpci(param_strs[0],"TEST",MAX_STRING_LEN) == 0)
		{
			DEBUG2("DAC Tone Test\n");
			for (Frequency = 0; Frequency < (sizeof(FrequencyList) / sizeof(FrequencyList[0])); ++Frequency) 
			{
				generate_tone(1000, FrequencyList[Frequency], 2);
				cyg_thread_delay(100);
			}
		}
		else
		{
			DEBUG1("invalid parameters\n");
			return TEST_ERR_PARAMS;
		}

	}

	return TEST_OK_PASS;

        
}
