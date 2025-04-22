#include "pal_uart.h"
#include "uart.h"
#include <string.h>

/*! \brief      Control block. */
typedef struct
{
  PalUartState_t state;
  PalUartConfig_t config;
  UART_Config_t mgConfig;
} palUartCtrlBlk_t;

static palUartCtrlBlk_t palUartCb[PAL_UART_ID_MAX];

static palUartCtrlBlk_t *palUartGetContext(PalUartId_t id)
{
  if (id < PAL_UART_ID_MAX) {
    return &palUartCb[id];
  }
  return NULL;
}

void PalUartInit(PalUartId_t id, const PalUartConfig_t *pCfg)
{
  palUartCtrlBlk_t *pCtx = palUartGetContext(id);
  if (!pCtx || !pCfg) return;

  pCtx->config = *pCfg;
  pCtx->mgConfig.uart_num = id; // assumes direct mapping, adjust if needed
  pCtx->mgConfig.baudrate = pCfg->baud;
  pCtx->mgConfig.stop_bits = 0; // 1 stop bit
  pCtx->mgConfig.parity = 0; // No parity
  pCtx->mgConfig.char_size = 8;
  pCtx->mgConfig.delay = 0;
  pCtx->mgConfig.transfer_mode=1;
	pCtx->mgConfig.receive_mode=1;
	pCtx->mgConfig.pullup=0;
  pCtx->mgConfig.rxthreshold = 12;

  UART_Init(&pCtx->mgConfig);
  UART_RX_Threshold(&pCtx->mgConfig, pCtx->mgConfig.rxthreshold);

  pCtx->state = PAL_UART_STATE_READY;
}

void PalUartDeInit(PalUartId_t id)
{
  palUartCtrlBlk_t *pCtx = palUartGetContext(id);
  if (!pCtx || pCtx->state != PAL_UART_STATE_READY) return;

  UART_Flush(&pCtx->mgConfig);
  pCtx->state = PAL_UART_STATE_UNINIT;
}

PalUartState_t PalUartGetState(PalUartId_t id)
{
  palUartCtrlBlk_t *pCtx = palUartGetContext(id);
  return (pCtx) ? pCtx->state : PAL_UART_STATE_ERROR;
}

void PalUartReadData(PalUartId_t id, uint8_t *pData, uint16_t len)
{
  palUartCtrlBlk_t *pCtx = palUartGetContext(id);
  if (!pCtx || pCtx->state == PAL_UART_STATE_UNINIT || !pData || len == 0) return;

  uint8_t temp=0;
  struct uart_buf rx = { .uart_data = &temp, .len = 0};

  for (uint16_t i = 0; i < len; i++) {
    if (UART_Read_Character(&pCtx->mgConfig, &rx) != 0) {
      break;
    }
  }

  if (pCtx->config.rdCback) {
    pCtx->config.rdCback();
  }
}

void PalUartWriteData(PalUartId_t id, const uint8_t *pData, uint16_t len)
{
  palUartCtrlBlk_t *pCtx = palUartGetContext(id);
  if (!pCtx || pCtx->state == PAL_UART_STATE_UNINIT || !pData || len == 0) return;

  pCtx->state = PAL_UART_STATE_BUSY;
  UART_Write(&pCtx->mgConfig, (uint8_t *)pData);
  UART_Write_Wait(&pCtx->mgConfig);
  pCtx->state = PAL_UART_STATE_READY;

  if (pCtx->config.wrCback) {
    pCtx->config.wrCback();
  }
}
