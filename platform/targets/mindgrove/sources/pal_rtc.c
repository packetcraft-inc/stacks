#include "pal_rtc.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define RTC_TIMER_TOTAL_CHANNEL 4

#define CLINT_BASE_ADDR     0x02000000UL
#define CLINT_MTIME_ADDR    (CLINT_BASE_ADDR + 0xBFF8UL)

// volatile ensures we actually read from hardware
#define CLINT_MTIME         (*(volatile uint64_t *)(CLINT_MTIME_ADDR))

static palRtcIrqCback_t palRtcTimerCback[RTC_TIMER_TOTAL_CHANNEL] = {0};

void PalRtcInit(void)
{
  // Stub init - assume RTC setup success
  printf("[RTC Stub] PalRtcInit called.\n");
}

void PalRtcEnableCompareIrq(uint8_t channelId)
{
  if (channelId < RTC_TIMER_TOTAL_CHANNEL)
  {
    printf("[RTC Stub] Enable Compare IRQ on channel %d\n", channelId);
  }
}

void PalRtcDisableCompareIrq(uint8_t channelId)
{
  if (channelId < RTC_TIMER_TOTAL_CHANNEL)
  {
    printf("[RTC Stub] Disable Compare IRQ on channel %d\n", channelId);
  }
}

uint32_t PalRtcCounterGet(void)
{
  // static uint32_t fake_counter = 0;
  // fake_counter += 100; // increment to simulate time passage
  // return fake_counter;
  uint32_t time_read = (uint32_t)(CLINT_MTIME & 0xFFFFFFFF);
  printf("Timer : %d\n", time_read);
  return time_read;
}

void PalRtcCompareSet(uint8_t channelId, uint32_t value)
{
  if (channelId < RTC_TIMER_TOTAL_CHANNEL)
  {
    printf("[RTC Stub] Set Compare[%d] = %u\n", channelId, value);
  }
}

uint32_t PalRtcCompareGet(uint8_t channelId)
{
  if (channelId < RTC_TIMER_TOTAL_CHANNEL)
  {
    printf("[RTC Stub] Get Compare[%d]\n", channelId);
    return 123456; // stub value
  }
  return 0;
}

void PalRtcIrqRegister(uint8_t channelId, palRtcIrqCback_t cback)
{
  if (channelId < RTC_TIMER_TOTAL_CHANNEL)
  {
    palRtcTimerCback[channelId] = cback;
    printf("[RTC Stub] IRQ callback registered for channel %d\n", channelId);
  }
}

void PalRtcClearCompareEvents(uint8_t channelId)
{
  if (channelId < RTC_TIMER_TOTAL_CHANNEL)
  {
    printf("[RTC Stub] Clear Compare Event for channel %d\n", channelId);
  }
}
