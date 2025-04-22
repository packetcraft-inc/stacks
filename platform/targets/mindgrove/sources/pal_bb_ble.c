#include "pal_bb_ble.h"
#include <string.h>
#include <stdio.h>

static PalBbBleTxIsr_t txCback = NULL;
static PalBbBleRxIsr_t rxCback = NULL;

void PalBbBleInit(void) {
  // Initialize baseband BLE driver. Replace with Mindgrove-specific radio init later.
  printf("Stub: PalBbBleInit called.\n");
}

void PalBbBleEnable(void) {
  // Wake and enable BB hardware
  printf("Stub: PalBbBleEnable called.\n");
}

void PalBbBleDisable(void) {
  // Put BB hardware to sleep
  printf("Stub: PalBbBleDisable called.\n");
}

void PalBbBleSetChannelParam(PalBbBleChan_t *pChan) {
  (void)pChan;
  // Setup channel config
  printf("Stub: PalBbBleSetChannelParam called.\n");
}

void PalBbBleSetDataParams(const PalBbBleDataParam_t *pParam) {
  txCback = pParam->txCback;
  rxCback = pParam->rxCback;
  printf("Stub: PalBbBleSetDataParams called.\n");
}

void PalBbBleSetOpParams(const PalBbBleOpParam_t *pOpParam) {
  (void)pOpParam;
  printf("Stub: PalBbBleSetOpParams called.\n");
}

void PalBbBleTxData(PalBbBleTxBufDesc_t descs[], uint8_t cnt) {
  printf("Stub: PalBbBleTxData called. Pretending to transmit %d buffer(s).\n", cnt);
  if (txCback) {
    txCback(0); // 0 = success
  }
}

void PalBbBleTxTifsData(PalBbBleTxBufDesc_t descs[], uint8_t cnt) {
  printf("Stub: PalBbBleTxTifsData called. Pretending to TIFS-transmit %d buffer(s).\n", cnt);
  if (txCback) {
    txCback(0); // 0 = success
  }
}

void PalBbBleRxData(uint8_t *pBuf, uint16_t len) {
  printf("Stub: PalBbBleRxData called. Pretending to receive %u bytes.\n", len);
  memset(pBuf, 0xAB, len); // simulate dummy data
  if (rxCback) {
    rxCback(0, -42, 0xDEADBEEF, 12345678, 0); // dummy RSSI, CRC, timestamp, options
  }
}

void PalBbBleRxTifsData(uint8_t *pBuf, uint16_t len) {
  printf("Stub: PalBbBleRxTifsData called. Pretending to TIFS-receive %u bytes.\n", len);
  memset(pBuf, 0xCD, len);
  if (rxCback) {
    rxCback(0, -40, 0xBADBEEF, 87654321, 0);
  }
}

void PalBbBleCancelTifs(void) {
  printf("Stub: PalBbBleCancelTifs called.\n");
}

void PalBbBleCancelData(void) {
  printf("Stub: PalBbBleCancelData called.\n");
}

void PalBbBleEnableDataWhitening(bool_t enable) {
  printf("Stub: PalBbBleEnableDataWhitening called: %d\n", enable);
}

void PalBbBleEnablePrbs15(bool_t enable) {
  printf("Stub: PalBbBleEnablePrbs15 called: %d\n", enable);
}

void PalBbBleInlineEncryptDecryptSetDirection(uint8_t dir) {
  printf("Stub: PalBbBleInlineEncryptDecryptSetDirection called: dir=%d\n", dir);
}

void PalBbBleInlineEncryptSetPacketCount(uint64_t count) {
  printf("Stub: PalBbBleInlineEncryptSetPacketCount called: count=%llu\n", count);
}

void PalBbBleLowPower(void) {
  printf("Stub: PalBbBleLowPower called.\n");
}
