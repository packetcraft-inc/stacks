/*
 * This Synopsys software and associated documentation (hereinafter the
 * "Software") is an unsupported proprietary work of Synopsys, Inc. unless
 * otherwise expressly agreed to in writing between Synopsys and you. The
 * Software IS NOT an item of Licensed Software or a Licensed Product under
 * any End User Software License Agreement or Agreement for Licensed Products
 * with Synopsys or any supplement thereto. Synopsys is a registered trademark
 * of Synopsys, Inc. Other names included in the SOFTWARE may be the
 * trademarks of their respective owners.
 *
 * The contents of this file are dual-licensed; you may select either version
 * 2 of the GNU General Public License ("GPL") or the BSD-3-Clause license
 * ("BSD-3-Clause"). The GPL is included in the COPYING file accompanying the
 * SOFTWARE. The BSD License is copied below.
 *
 * BSD-3-Clause License:
 * Copyright (c) 2015 Synopsys, Inc. and/or its affiliates.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer, without
 *    modification.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The names of the above-listed copyright holders may not be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SYNTRNG3_H_
#define SYNTRNG3_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "synversion.h"
#include "elppdu.h"

#define BASICTRNG_MAXLEN 32

struct basictrng_state {
   void     (*writereg)(void *base, unsigned offset, uint32_t val);
   uint32_t (*readreg) (void *base, unsigned offset);

   void *regbase;

   unsigned short epn, tts,stepping, output_len, diag_level;
   unsigned secure_reset:1,
            rings_avail:1;
};

void basictrng_setup(struct basictrng_state *basic_trng, uint32_t *regbase);

/*
 * Enable/disable the IRQ sources specified in irq_mask, which is the
 * bitwise-OR of one or more of the TRNG3_IRQ_xxx_MASK macros in
 * basictrng_hw.h.
 */
void basictrng_enable_irqs(struct basictrng_state *basic_trng, uint32_t irq_mask);
void basictrng_disable_irqs(struct basictrng_state *basic_trng, uint32_t irq_mask);

/*
 * If secure is non-zero, switch the TRNG into secure mode.  Otherwise,
 * switch the TRNG into promiscuous mode.
 */
void basictrng_set_secure(struct basictrng_state *basic_trng, int secure);

/*
 * Adjust the bit rejection tweak threshold.
 */
int basictrng_set_max_rejects(struct basictrng_state *basic_trng, unsigned rejects);

/*
 * Initiate a reseed of the engine.  If nonce is a null pointer, then a
 * random reseed operation is performed.  Otherwise, nonce should point to
 * a 32-byte buffer which will be used to perform a nonce reseed.
 */
int basictrng_reseed(struct basictrng_state *basic_trng, const void *nonce);

/*
 * Read the last-used seed into a buffer, which must be at least 32 bytes long.
 */
int basictrng_get_seed(struct basictrng_state *basic_trng, void *out);

/*
 * Configure the output length of the engine.  Possible values are 16 and 32
 * bytes.  32-byte mode requires appropriate H/W configuration.
 */
int basictrng_set_output_len(struct basictrng_state *basic_trng, unsigned outlen);

/*
 * Configure the request-based reseed reminder alarm to trigger after the
 * specified number of random numbers generated.
 */
int basictrng_set_request_reminder(struct basictrng_state *basic_trng,
                                  unsigned long requests);

/*
 * Configure the age-based reseed reminder alarm to trigger after the specified
 * number of clock cycles.
 */
int basictrng_set_age_reminder(struct basictrng_state *basic_trng,
                              unsigned long long cycles);

/*
 * Retrieve random data from the engine; out should point to a buffer
 * that is at least 32 bytes long to accomodate all possible output
 * sizes.  On success, returns the number of output bytes written to
 * the buffer (may be either 16 or 32).
 */
int basictrng_get_random(struct basictrng_state *basic_trng, void *out);

/* Helpers to access TRNG registers directly. */
static inline void
basictrng_writereg(struct basictrng_state *basic_trng, unsigned offset, uint32_t val)
{
   basic_trng->writereg(basic_trng->regbase, offset, val);
}

static inline uint32_t
basictrng_readreg(struct basictrng_state *basic_trng, unsigned offset)
{
   return basic_trng->readreg(basic_trng->regbase, offset);
}

#ifdef __cplusplus
}
#endif

#endif
