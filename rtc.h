#ifndef RTC_H
#define RTC_H

#include "tests.h"
#include "lib.h"

//enable rtc interrupts
void enable_rtc();

//check for rtc interrupt
void rtc_interrupt();

//blocks until the next rtc interupt
int32_t read_rtc(int32_t fd, void* buf, int32_t nbytes);

//does nothing except change interupt rate to default (2)
int32_t open_rtc(const uint8_t* filename);

//does nothing
int32_t close_rtc(int32_t fd);

//changes the interupt rate of rtc as long as its a power of 2
int32_t write_rtc(int32_t fd, const void* buf, int32_t nbytes);



#endif
