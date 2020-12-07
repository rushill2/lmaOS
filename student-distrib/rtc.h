#ifndef _RTC_H_
#define _RTC_H_

#include "lib.h"
#include "i8259.h"
#include "types.h"
#ifndef ASM

#define RTC_PORT 0x70
#define RTC_DATA 0x71
#define RTC_A   0x8A
#define RTC_B   0x8B
#define RTC_C   0x0C
#define ENABLE  0x40
#define FREQ    0x0F
#define W_MASK  0xF0
#define IRQ2    0x02
#define IRQ8    0x08
#define DIVIDER_VALUE 0x06

extern void rtc_init ();
extern void rtc_intr ();

int32_t rtc_read (int32_t fd, void* buf, int32_t nbytes);
int32_t rtc_write (int32_t fd, const void* buf, int32_t nbytes);
int32_t rtc_open (const uint8_t* filename);
int32_t rtc_close (int32_t fd);

int32_t set_frequency (int32_t target_frequency);

#endif
#endif

