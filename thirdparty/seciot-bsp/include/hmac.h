#ifndef HMAC_H
#define HMAC_H
#include"io.h"
#include"sha256.h"
void HMAC_SHA256(const uint8_t *key, size_t key_len, const uint8_t *message, size_t message_len, uint8_t *hmac_result);
#endif