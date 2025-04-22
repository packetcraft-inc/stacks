/**
 * Project                           : Secure IoT SoC
 * Name of the file                  : perf_monitors.c
 * Brief Description of file         : Driver to Standard perf_monitor driver
 * Name of Author                    : Shri Mahaalakshmi S J
 * Email ID                          : mahaalakshmi@mindgrovetech.in
 * 
 * @file perf_monitors.h
 * @author Shri Mahaalakshmi S J (mahaalakshmi@mindgrovetech.in)
 * @brief This is a Baremetal perf_monitor Driver's header file for Mindgrove Silicon's performance monitor.
 * @version 0.2
 * @date 2024-12-04
 * 
 * @copyright Copyright (c) Mindgrove Technologies Pvt. Ltd 2024. All rights reserved.
 * 
 */

#ifndef PERF_MONITOR_H
#define PERF_MONITOR_H

#include <stddef.h>
#include <stdint.h>
#include "encoding.h"
#include "secure_iot.h"
#include "io.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Cache_perf_t
{
    uint64_t Ifbhit;           // Instruction Cache Fill Buffer Hits
    uint64_t Ifbrelease;       // Instruction Cache Fill Buffer Releases
    uint64_t Imiss;            // Instruction Cache Misses
    uint64_t Inc_access;       // Non-cached Instruction Access
    uint64_t Iaccess;          // Instruction Cache Accesses

    uint64_t Dread_access;     // Data Cache Read Accesses
    uint64_t Dwrite_access;    // Data Cache Write Accesses
    uint64_t Datomic_access;   // Data Cache Atomic Accesses
    uint64_t Dnc_read_access;  // Non-cached Data Read Accesses
    uint64_t Dnc_write_access; // Non-cached Data Write Accesses

    uint64_t Dread_miss;       // Data Cache Read Misses
    uint64_t Dwrite_miss;      // Data Cache Write Misses
    uint64_t Datomic_miss;     // Data Cache Atomic Misses

    uint64_t Dread_fbhit;      // Data Cache Read Fill Buffer Hits
    uint64_t Dwrite_fbhit;     // Data Cache Write Fill Buffer Hits
    uint64_t Datomic_fbhit;    // Data Cache Atomic Fill Buffer Hits

    uint64_t Dfbrelease;       // Data Cache Fill Buffer Releases
    uint64_t Dline_evictions;  // Data Cache Line Evictions

    uint64_t Itlb_miss;        // Instruction TLB Misses
    uint64_t Dtlb_miss;        // Data TLB Misses
};

struct Stalls_perf_t
{
  uint64_t rawstalls; // RAW stalls
  uint64_t exestalls; // Execution stalls
};

struct Branch_perf_t 
{
  uint64_t misprediction; // Branch misprediction
  uint64_t jumps; // Number of jumps
  uint64_t branches; // Number of branches
};

struct Arith_perf_t 
{
  uint64_t floats; // Floating-point Operations
  uint64_t muldiv; // Multiply and Divide Operations
};

/** 
 * @fn void mcycle_init()
 * 
 * @brief The function 'mcycle_init' Clears the mcycle register
 * 
 * @return Void
 */
void mcycle_init();

/**
 * @fn uint64_t get_mcycle_start()
 * 
 * @brief The function 'get_mcycle_start' starts the mcycle value 
 * 
 * @return returns the mcycle count
 */
uint64_t get_mcycle_start();

/** 
 * @fn uint64_t get_mcycle_stop()
 * 
 * @brief The function 'get_mcycle_stop' Stops the mcycle value
 * 
 * @return returns the mcycle count
 */
uint64_t get_mcycle_stop();

/** 
 * @fn uint64_t millis(int total_cycles)
 * 
 * @brief The function 'millis' converts the total number of cycles taken to milliseconds
 * 
 * @return returns the real time in milliseconds
 */
uint64_t millis(uint64_t total_cycles);

/** 
 * @fn void PERF_cache_init()
 * 
 * @brief The function `PERF_cache_init` initializes performance monitoring counters and events related to
 * cache accesses.
 * 
 * @return Void
 */
void PERF_cache_init();

/** 
 * @fn void PERF_Print_Cache(int iterations)
 * 
 * @brief The function `PERF_Print_Cache` prints various performance metrics related to instruction cache
 * (ICache) and data cache (DCache) based on the given number of iterations.
 * 
 * @param iterations The `iterations` parameter is used to calculate the average value of each metric over 
 * a certain number of iterations. By dividing the value read from the performance counters by the `iterations`
 * 
 * @return Void
 */
void PERF_Print_Cache(int iterations);

/** 
 * @fn void PERF_Print_Cache_MissPercentage()
 * 
 * @brief The function calculates and prints the cache miss percentages for instruction cache (ICache) and
 * data cache (DCache) reads and writes.
 * 
 * @return Void
 */
void PERF_Print_Cache_MissPercentage();

/** 
 * @fn void PERF_Stalls_Init()
 * 
 * @brief The function `PERF_Stalls_Init` initializes performance monitoring counters for raw and execution
 * stalls.
 * 
 * @return Void
 */
void PERF_Stalls_Init();

/** 
 * @fn void PERF_Print_Stalls(int iterations)
 * 
 * @brief The function `PERF_Print_Stalls` prints the number of raw stalls and execution stalls per iteration.
 * 
 * @param iterations The `iterations` parameter is used to calculate the average value of each metric over 
 * a certain number of iterations. By dividing the value read from the performance counters by the `iterations`
 * 
 * @return Void
 */
void PERF_Print_Stalls(int iterations);

/** 
 * @fn void PERF_Branches_Init()
 * 
 * @brief The function `PERF_Branches_Init` initializes performance monitoring counters for branch
 * instructions in a C program.
 * 
 * @return Void
 */
void PERF_Branches_Init();

/**
 * @fn void PERF_Print_Branches(int iterations)
 * 
 * @brief The function `PERF_Print_Branches` calculates and prints the number of branch mispredictions, jumps,
 * branches, and the percentage of branch mispredictions based on the given number of iterations.
 * 
 * @param iterations The `iterations` parameter is used to calculate the average value of each metric over 
 * a certain number of iterations. By dividing the value read from the performance counters by the `iterations`
 * 
 * @return Void
 */
void PERF_Print_Branches(int iterations);

/** 
 * @fn void PERF_Arithops_Init()
 * 
 * @brief The function `PERF_Arithops_Init` initializes performance monitoring counters for floating-point
 * operations and multiplication/division events.
 * 
 * @return Void
 */
void PERF_Arithops_Init();

/** 
 * @fn void PERF_Print_Arithops(int iterations)
 * 
 * @brief The function `PERF_Print_Arithops` prints the average number of floating-point operations and
 * multiplication/division operations per iteration.
 * 
 * @param iterations The `iterations` parameter is used to calculate the average value of each metric over 
 * a certain number of iterations. By dividing the value read from the performance counters by the `iterations`
 * 
 * @return Void
 */
void PERF_Print_Arithops(int iterations);

/** 
 * @fn void PERF_Set_Event(int counter, int event)
 * 
 * @brief The function `PERF_Set_Event` sets a specific event for a performance counter in a system.
 * 
 * @param counter The `counter` parameter sets the event for the specified counter based on the value 
 * of the `counter` parameter.
 * 
 * @param event The `event` parameter sets the specific event to set for a performance counter.
 * 
 * @return Void
 */
void PERF_Set_Event(int counter, int event);

/** 
 * @fn void PERF_Disable(int counter)
 * 
 * @brief The function `PERF_Disable` disables a specific performance counter based on the 
 * input parameter `counter`.
 * 
 * @param counter The `counter` parameter is used in a switch-case statement to determine which performance 
 * counter to disable by writing specific values to control and event registers (`mhpmcounterX` and `mhpmeventX`).
 * 
 * @return Void
 */
void PERF_Disable(int counter);

/** 
 * @fn void PERF_Disable_All()
 * 
 * @brief The function `PERF_Disable_All` disables all performance monitoring event registers.
 * 
 * @return Void
 */
void PERF_Disable_All();

/** 
 * @fn void PERF_Clear_All()
 * 
 * @brief The function `PERF_Clear_All` clears all performance monitor counter values.
 * 
 * @return Void
 */
void PERF_Clear_All();

#ifdef __cplusplus
}
#endif

#endif