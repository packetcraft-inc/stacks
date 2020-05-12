/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      NRFX glue definitions.
 *
 *  Copyright (c) 2019-2020 Packetcraft, Inc.
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*************************************************************************************************/

#ifndef NRFX_GLUE_H
#define NRFX_GLUE_H

/* Use MDK interrupt handler bindings. */
#include <soc/nrfx_irqs.h>

/*************************************************************************************************/

#define NRFX_ASSERT(expression)
#define NRFX_STATIC_ASSERT(expression)

/*************************************************************************************************/

#ifdef NRF51
#define INTERRUPT_PRIORITY_IS_VALID(pri) ((pri) < 4)
#else
#define INTERRUPT_PRIORITY_IS_VALID(pri) ((pri) < 8)
#endif

static inline void NRFX_IRQ_PRIORITY_SET(IRQn_Type irq_number,
                                          uint8_t   priority)
{
    NVIC_SetPriority(irq_number, priority);
}

static inline void NRFX_IRQ_ENABLE(IRQn_Type irq_number)
{
    NVIC_EnableIRQ(irq_number);
}

static inline bool NRFX_IRQ_IS_ENABLED(IRQn_Type irq_number)
{
    return 0 != (NVIC->ISER[irq_number / 32] & (1UL << (irq_number % 32)));
}

static inline void NRFX_IRQ_DISABLE(IRQn_Type irq_number)
{
    NVIC_DisableIRQ(irq_number);
}

static inline void NRFX_IRQ_PENDING_SET(IRQn_Type irq_number)
{
    NVIC_SetPendingIRQ(irq_number);
}

static inline void NRFX_IRQ_PENDING_CLEAR(IRQn_Type irq_number)
{
    NVIC_ClearPendingIRQ(irq_number);
}

static inline bool NRFX_IRQ_IS_PENDING(IRQn_Type irq_number)
{
    return (NVIC_GetPendingIRQ(irq_number) == 1);
}

/*************************************************************************************************/

#define NRFX_DELAY_DWT_BASED 0

#include <soc/nrfx_coredep.h>

#define NRFX_DELAY_US(us_time) nrfx_coredep_delay_us(us_time)

/*************************************************************************************************/

#define NRFX_CRITICAL_SECTION_ENTER()
#define NRFX_CRITICAL_SECTION_EXIT()

/*************************************************************************************************/

#endif /* NRFX_GLUE_H */
