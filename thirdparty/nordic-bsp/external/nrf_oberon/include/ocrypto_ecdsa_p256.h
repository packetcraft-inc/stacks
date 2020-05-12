/**
 * Copyright (c) 2019, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/**@file
 * @defgroup nrf_oberon_ecdsa ECDSA APIs
 * @ingroup nrf_oberon
 * @{
 * @brief Type declarations and APIs to do Elliptic Curve Digital Signature Algorith using the
 * NIST secp256r1 curve.
 */
#ifndef OCRYPTO_ECDSA_P256_H
#define OCRYPTO_ECDSA_P256_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

/**
 * ECDSA P-256 signature key pair generation.
 *
 * @param[out]  pk  Generated public key.
 * @param[out]  sk  Private key.
 *
 * @retval 0 If @p sk is a legal secret key.
 */
int ocrypto_ecdsa_p256_public_key(
    uint8_t pk[64],
    const uint8_t sk[32]);


/**
 * ECDSA P-256 signature generation.
 *
 * @param[out]  sig     Generated signature.
 * @param       m       Input message.
 * @param       mlen    Length of the message.
 * @param       sk      Secret key.
 * @param       ek      Session key.
 *
 * @retval 0 If @p ek is a valid session key.
 */
int ocrypto_ecdsa_p256_sign(
    uint8_t sig[64],
    const uint8_t *m, size_t mlen,
    const uint8_t sk[32],
    const uint8_t ek[32]);


/**
 * ECDSA P-256 signature generation from hash.
 *
 * @param[out]  sig     Generated signature.
 * @param       hash    SHA-256 hash of the input message.
 * @param       sk      Secret key.
 * @param       ek      Session key.
 *
 * @retval 0 If @p ek is a valid session key.
 */
int ocrypto_ecdsa_p256_sign_hash(uint8_t sig[64],
                                 const uint8_t hash[32],
                                 const uint8_t sk[32],
                                 const uint8_t ek[32]);


/**
 * ECDSA P-256 signature verification.
 *
 * @param       sig     Input signature.
 * @param       m       Input message.
 * @param       mlen    Input length.
 * @param       pk      Public key.
 *
 * @retval 0  If signature is valid.
 * @retval -1 Otherwise.
 */
int ocrypto_ecdsa_p256_verify(
    const uint8_t sig[64],
    const uint8_t *m, size_t mlen,
    const uint8_t pk[64]);


/**
 * ECDSA P-256 signature verification from hash.
 *
 * @param        sig     Input signature.
 * @param        hash    SHA-256 hash of the input message.
 * @param        pk      Public key.
 *
 * @retval 0  If signature is valid.
*  @retval -1 Otherwise.
 */
int ocrypto_ecdsa_p256_verify_hash(const uint8_t sig[64],
                                   const uint8_t hash[32],
                                   const uint8_t pk[64]);


#ifdef __cplusplus
}
#endif

#endif

/** @} */
