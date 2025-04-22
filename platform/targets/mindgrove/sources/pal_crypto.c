#include "pal_crypto.h"
#include <string.h>

void PalCryptoInit(void) {}
void PalCryptoDeInit(void) {}

void PalCryptoGenerateRandomNumber(uint8_t *pBuf, uint8_t len)
{
  // Fill with a dummy pattern for testing
  memset(pBuf, 0xA5, len);
}

void PalCryptoGenerateP256KeyPair(const uint8_t *pPrivKey, uint8_t *pPubKey)
{
  memset(pPubKey, 0x01, 64); // Fake public key
}

void PalCryptoGenerateDhKey(const uint8_t *pPubKey, const uint8_t *pPrivKey, uint8_t *pDhKey)
{
  memset(pDhKey, 0x02, 32); // Fake shared secret
}

bool_t PalCryptoValidatePublicKey(const uint8_t *pPubKey, bool_t generateKey)
{
  return TRUE; // Always accept
}

void PalCryptoAesEcb(const uint8_t *pKey, uint8_t *pOut, const uint8_t *pIn)
{
  memcpy(pOut, pIn, 16); // No-op encryption
}

void PalCryptoAesCmac(const uint8_t *pKey, uint8_t *pOut, const uint8_t *pIn, uint16_t len)
{
  memset(pOut, 0x5C, 16); // Dummy CMAC
}

void PalCryptoAesEnable(PalCryptoEnc_t *pEnc, uint8_t id, uint8_t localDir)
{
  // Just set context pointers to dummy internal struct
  static int dummy;
  pEnc->pEncryptCtx = &dummy;
  pEnc->pDecryptCtx = &dummy;
}

bool_t PalCryptoAesCcmEncrypt(PalCryptoEnc_t *pEnc, uint8_t *pHdr, uint8_t *pBuf, uint8_t *pMic)
{
  if (pMic) memset(pMic, 0x3C, PAL_CRYPTO_LL_DATA_MIC_LEN);
  return TRUE;
}

bool_t PalCryptoAesCcmDecrypt(PalCryptoEnc_t *pEnc, uint8_t *pBuf)
{
  return TRUE; // Always succeed
}

uint32_t PalCryptoCcmDec(const uint8_t *pKey, uint8_t *pNonce, uint8_t *pCypherText, uint16_t textLen,
                         uint8_t *pClear, uint16_t clearLen, uint8_t *pMic, uint8_t micLen,
                         uint8_t *pResult, uint8_t handlerId, uint16_t param, uint8_t event)
{
  memcpy(pResult, pCypherText, textLen);
  return 0; // Success
}

void PalCryptoCcmEnc(const uint8_t *pKey, uint8_t *pNonce, uint8_t *pPlainText, uint16_t textLen,
                     uint8_t *pClear, uint16_t clearLen, uint8_t micLen, uint8_t *pResult,
                     uint8_t handlerId, uint16_t param, uint8_t event)
{
  memcpy(pResult + clearLen, pPlainText, textLen);
  memset(pResult + clearLen + textLen, 0xAB, micLen); // Dummy MIC
}

#if (BB_ENABLE_INLINE_ENC_TX)
void PalCryptoSetEncryptPacketCount(PalCryptoEnc_t *pEnc, uint64_t pktCnt) {}
#endif

#if (BB_ENABLE_INLINE_DEC_RX)
void PalCryptoSetDecryptPacketCount(PalCryptoEnc_t *pEnc, uint64_t pktCnt) {}
#endif
