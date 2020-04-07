//
// TinyProfile.h
//

#ifndef __TINYPROFILE_H__
#define __TINYPROFILE_H__


#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/hal_arch.h>

typedef struct profile_data {
	cyg_uint32 start_tick, start_jiffy;
} profile_data;

static inline void profile_start(profile_data *dat) {
	do {
		HAL_CLOCK_READ(&dat->start_jiffy);
	} while (dat->start_jiffy > 5);
	dat->start_tick = cyg_current_time();
	HAL_CLOCK_READ(&dat->start_jiffy);
}

static inline cyg_uint32 profile_end(profile_data *dat) {
	cyg_uint32 end_tick, end_jiffy1, end_jiffy2;
	HAL_CLOCK_READ(&end_jiffy1);
	end_tick = cyg_current_time();
	HAL_CLOCK_READ(&end_jiffy2);
	if (end_jiffy1 > end_jiffy2) {
		end_tick = cyg_current_time() - 1;
	}
	return (end_tick-dat->start_tick)*CYGNUM_KERNEL_COUNTERS_RTC_PERIOD
		+ (end_jiffy1 - dat->start_jiffy);
}

#ifdef __cplusplus

class TinyProfile {
public:
	TinyProfile(const char *in_str) : str(in_str) {
		profile_start(&dat);
	}

	~TinyProfile() {
		cyg_uint32 elapsed = profile_end(&dat);
		diag_printf("Profile: %s %ld jiffies\n", str, elapsed);
	}
	profile_data dat;
	const char *str;
};

#endif // __cplusplus

#endif // __TINYPROFILE_H__
