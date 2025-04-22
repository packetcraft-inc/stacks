#include "pal_sys.h"
#include <string.h>

static uint32_t palSysAssertCount = 0;
static volatile bool_t PalSysAssertTrapEnable = FALSE;
static uint32_t palSysBusyCount = 0;

extern unsigned long _heap;
extern unsigned long _heap_end;

uint8_t *SystemHeapStart;
uint32_t SystemHeapSize;

void PalEnterCs(void)
{
  // Stub: insert platform-specific critical section entry
}

void PalExitCs(void)
{
  // Stub: insert platform-specific critical section exit
}

void PalSysInit(void)
{
  palSysAssertCount = 0;
  PalSysAssertTrapEnable = TRUE;
  palSysBusyCount = 0;
  SystemHeapStart = (uint8_t *)&_heap;
  SystemHeapSize = (uint32_t)((uintptr_t)&_heap_end - (uintptr_t)&_heap);
  printf("Heap Start: %x\nHeap Size: %d\n", SystemHeapStart, SystemHeapSize);

#ifdef DEBUG
  // Optionally clear heap or memory
#endif
}

void PalSysAssertTrap(void)
{
  palSysAssertCount++;
  while (PalSysAssertTrapEnable);
}

void PalSysSetTrap(bool_t enable)
{
  PalSysAssertTrapEnable = enable;
}

uint32_t PalSysGetAssertCount(void)
{
  return palSysAssertCount;
}

uint32_t PalSysGetStackUsage(void)
{
  // Stubbed: return 0 or dummy value if no stack analysis yet
  return 0;
}

void PalSysSleep(void)
{
  // Stub: insert wait-for-interrupt or platform idle loop
  __asm__ volatile("wfi");
}

bool_t PalSysIsBusy(void)
{
  bool_t busy;
  PalEnterCs();
  busy = (palSysBusyCount > 0);
  PalExitCs();
  return busy;
}

void PalSysSetBusy(void)
{
  PalEnterCs();
  palSysBusyCount++;
  PalExitCs();
}

void PalSysSetIdle(void)
{
  PalEnterCs();
  if (palSysBusyCount > 0)
  {
    palSysBusyCount--;
  }
  PalExitCs();
}
