/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  PAL Flash driver.
 *
 *  Copyright (c) 2018-2019 Arm Ltd. All Rights Reserved.
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

#include <string.h>
#include "pal_flash.h"
#if defined(NRF52840_XXAA)
#include "nrfx_qspi.h"
#include "boards.h"
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Flash block size. */
#define PAL_FLASH_SECTOR4K                   1
#define PAL_FLASH_SECTOR64K                  16

#define PAL_FLASH_SECTOR4K_SIZE              0x1000
#define PAL_FLASH_SECTOR64K_SIZE             0x10000

/*! Flash Total size. */
#define PAL_FLASH_TOTAL_SIZE                 0x800000

/*! Flash internal cache buffer size. Note: should be at least 2. */
#define PAL_FLASH_CACHE_BUF_SIZE             11

/*! Flash word size. */
#define PAL_FLASH_WORD_SIZE                  4

/*! Aligns a value to word size. */
#define PAL_FLASH_WORD_ALIGN(value)          (((value) + (PAL_FLASH_WORD_SIZE - 1)) & \
                                                        ~(PAL_FLASH_WORD_SIZE - 1))
/*! Validates if a value is aligned to word. */
#define PAL_FLASH_IS_WORD_ALIGNED(value)     (((uint32_t)(value) & \
                                                         (PAL_FLASH_WORD_SIZE - 1)) == 0)

/*! QSPI flash commands. */
#define QSPI_STD_CMD_WRSR   0x01
#define QSPI_STD_CMD_RSTEN  0x66
#define QSPI_STD_CMD_RST    0x99

#ifdef DEBUG

/*! \brief      Parameter check. */
#define PAL_FLASH_PARAM_CHECK(expr)               { \
                                                  if (!(expr)) \
                                                  { \
                                                     palFlashCb.state = PAL_FLASH_STATE_ERROR; \
                                                     return; \
                                                  } \
                                                }

#else

/*! \brief      Parameter check (disabled). */
#define PAL_FLASH_PARAM_CHECK(expr)

#endif

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! flash cache buffer. */
static uint32_t palFlashCacheBuf[PAL_FLASH_CACHE_BUF_SIZE];

/*! \brief      Control block. */
struct
{
  PalFlashState_t  state;                                   /*!< State. */
  uint32_t       writeAddr;                                 /*!< Write address. */
} palFlashCb;

/**************************************************************************************************
  Global Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initialize flash.
 *
 *  \param[in] actCback     Callback function.
 */
/*************************************************************************************************/
void PalFlashInit(PalFlashCback_t actCback)
{
#if defined(NRF52840_XXAA)

  uint32_t status;
  uint8_t  temp = 0x40;

  (void)actCback;

  nrfx_qspi_config_t config =
  {                                                                       \
      .xip_offset  = NRFX_QSPI_CONFIG_XIP_OFFSET,                         \
      .pins = {                                                           \
         .sck_pin     = BSP_QSPI_SCK_PIN,                                 \
         .csn_pin     = BSP_QSPI_CSN_PIN,                                 \
         .io0_pin     = BSP_QSPI_IO0_PIN,                                 \
         .io1_pin     = BSP_QSPI_IO1_PIN,                                 \
         .io2_pin     = BSP_QSPI_IO2_PIN,                                 \
         .io3_pin     = BSP_QSPI_IO3_PIN,                                 \
      },                                                                  \
      .irq_priority   = (uint8_t)NRFX_QSPI_CONFIG_IRQ_PRIORITY,           \
      .prot_if = {                                                        \
          .readoc     = (nrf_qspi_readoc_t)NRFX_QSPI_CONFIG_READOC,       \
          .writeoc    = (nrf_qspi_writeoc_t)NRFX_QSPI_CONFIG_WRITEOC,     \
          .addrmode   = (nrf_qspi_addrmode_t)NRFX_QSPI_CONFIG_ADDRMODE,   \
          .dpmconfig  = false,                                            \
      },                                                                  \
      .phy_if = {                                                         \
          .sck_freq   = (nrf_qspi_frequency_t)NRFX_QSPI_CONFIG_FREQUENCY, \
          .sck_delay  = (uint8_t)NRFX_QSPI_CONFIG_SCK_DELAY,              \
          .spi_mode   = (nrf_qspi_spi_mode_t)NRFX_QSPI_CONFIG_MODE,       \
          .dpmen      = false                                             \
      },                                                                  \
  }
  ;

  /* Verify palFlashCacheBuf size is at least 2. */
  PAL_FLASH_PARAM_CHECK(PAL_FLASH_CACHE_BUF_SIZE >= 2);

  status = nrfx_qspi_init(&config, NULL, NULL);

  PAL_FLASH_PARAM_CHECK(status == NRFX_SUCCESS);

  nrf_qspi_cinstr_conf_t cinstr_cfg = {
      .opcode    = QSPI_STD_CMD_RSTEN,
      .length    = NRF_QSPI_CINSTR_LEN_1B,
      .io2_level = 1,
      .io3_level = 1,
      .wipwait   = 1,
      .wren      = 1
  };

  /* Send reset enable. */
  status = nrfx_qspi_cinstr_xfer(&cinstr_cfg, NULL, NULL);

  /* Send reset command */
  cinstr_cfg.opcode = QSPI_STD_CMD_RST;

  status = nrfx_qspi_cinstr_xfer(&cinstr_cfg, NULL, NULL);

  PAL_FLASH_PARAM_CHECK(status == NRFX_SUCCESS);

  /* Switch to qspi mode */
  cinstr_cfg.opcode = QSPI_STD_CMD_WRSR;
  cinstr_cfg.length = NRF_QSPI_CINSTR_LEN_2B;

  status = nrfx_qspi_cinstr_xfer(&cinstr_cfg, &temp, NULL);

  PAL_FLASH_PARAM_CHECK(status == NRFX_SUCCESS);

  memset(&palFlashCb, 0, sizeof(palFlashCb));

  palFlashCb.state = PAL_FLASH_STATE_READY;

  (void)status;
#else
  (void)palFlashCacheBuf;
#endif
}

/*************************************************************************************************/
/*!
 *  \brief  De-initialize flash.
 */
/*************************************************************************************************/
void PalFlashDeInit(void)
{
#if defined(NRF52840_XXAA)
  nrfx_qspi_uninit();
#else
#endif
}

/*************************************************************************************************/
/*!
 *  \brief     Reads data from flash storage.
 *
 *  \param[in] pBuf     Pointer to memory buffer where data will be stored.
 *  \param[in] size     Data size in bytes to be read.
 *  \param[in] srcAddr  Word aligned address from where data is read.
 */
/*************************************************************************************************/
void PalFlashRead(void *pBuf, uint32_t size, uint32_t srcAddr)
{
#if defined(NRF52840_XXAA)
  uint32_t readSize = PAL_FLASH_WORD_ALIGN(size);
  uint32_t actualSize = size;
  uint32_t status;
  uint16_t addrOffset = 0;

  PAL_FLASH_PARAM_CHECK(palFlashCb.state == PAL_FLASH_STATE_READY);
  PAL_FLASH_PARAM_CHECK(pBuf != NULL);
  PAL_FLASH_PARAM_CHECK(size != 0);
  PAL_FLASH_PARAM_CHECK(PAL_FLASH_IS_WORD_ALIGNED(srcAddr));

  do
  {
    if (readSize <= sizeof(palFlashCacheBuf))
    {
      /* Read data. */
      status = nrfx_qspi_read(palFlashCacheBuf, readSize, srcAddr + addrOffset);

      PAL_FLASH_PARAM_CHECK(status == NRFX_SUCCESS);

      memcpy((uint8_t*)pBuf + addrOffset, palFlashCacheBuf, actualSize);

      readSize = 0;
    }
    else
    {
      /* Read data. */
      status = nrfx_qspi_read(palFlashCacheBuf, sizeof(palFlashCacheBuf), srcAddr + addrOffset);

      PAL_FLASH_PARAM_CHECK (status == NRFX_SUCCESS);

      memcpy((uint8_t*)pBuf + addrOffset, palFlashCacheBuf, sizeof(palFlashCacheBuf));

      addrOffset += sizeof(palFlashCacheBuf);
      readSize -= sizeof(palFlashCacheBuf);
      actualSize -= sizeof(palFlashCacheBuf);
    }
  } while (readSize != 0);
  (void)status;
#else
  memset(pBuf, 0xFF, size);
#endif
}

/*************************************************************************************************/
/*!
 *  \brief     Writes data to flash storage.
 *
 *  \param[in] pBuf     Pointer to memory buffer from where data will be written.
 *  \param[in] size     Data size in bytes to be written.
 *  \param[in] dstAddr  Word aligned address to write data.
 */
/*************************************************************************************************/
void PalFlashWrite(void *pBuf, uint32_t size, uint32_t dstAddr)
{
#if defined(NRF52840_XXAA)
  uint32_t writeSize = PAL_FLASH_WORD_ALIGN(size);
  uint32_t actualSize = size;
  uint32_t status;
  uint16_t addrOffset = 0;

  PAL_FLASH_PARAM_CHECK(palFlashCb.state == PAL_FLASH_STATE_READY);
  PAL_FLASH_PARAM_CHECK(pBuf != NULL);
  PAL_FLASH_PARAM_CHECK(size != 0);
  PAL_FLASH_PARAM_CHECK(PAL_FLASH_IS_WORD_ALIGNED(dstAddr));

  do
  {
    if (writeSize <= sizeof(palFlashCacheBuf))
    {
      memcpy(palFlashCacheBuf, (uint8_t*)pBuf + addrOffset, actualSize);
      memset((uint8_t*)palFlashCacheBuf + actualSize, 0xFF, sizeof(palFlashCacheBuf) - actualSize);

      /* Write data. */
      status = nrfx_qspi_write(palFlashCacheBuf, writeSize, dstAddr + addrOffset);

      PAL_FLASH_PARAM_CHECK(status == NRFX_SUCCESS);

      writeSize = 0;
    }
    else
    {
      memcpy(palFlashCacheBuf, (uint8_t*)pBuf + addrOffset, sizeof(palFlashCacheBuf));

      /* Write data. */
      status = nrfx_qspi_write(palFlashCacheBuf, sizeof(palFlashCacheBuf), dstAddr + addrOffset);

      PAL_FLASH_PARAM_CHECK(status == NRFX_SUCCESS);

      addrOffset += sizeof(palFlashCacheBuf);
      writeSize -= sizeof(palFlashCacheBuf);
      actualSize -= sizeof(palFlashCacheBuf);
    }
  } while (writeSize != 0);
  (void)status;
#else
#endif
}

/*************************************************************************************************/
/*!
 *  \brief  Erase sector.
 *
 *  \param[in] numOfSectors       Number of sectors to be erased.
 *  \param[in] startAddr          Word aligned address.
 */
/*************************************************************************************************/
void PalFlashEraseSector(uint32_t numOfSectors, uint32_t startAddr)
{
#if defined(NRF52840_XXAA)
  PAL_FLASH_PARAM_CHECK(PAL_FLASH_IS_WORD_ALIGNED(startAddr));
  nrf_qspi_erase_len_t eSize = QSPI_ERASE_LEN_LEN_4KB;

  for (uint32_t i = 0; i < numOfSectors; i++)
  {
    nrfx_qspi_erase(eSize, startAddr + PAL_FLASH_SECTOR4K_SIZE * i);
  }
#endif
}

/*************************************************************************************************/
/*!
 *  \brief  Erase chip. It is not recommended to use since it takes up to 240s.
 */
/*************************************************************************************************/
void PalFlashEraseChip(void)
{
#if defined(NRF52840_XXAA)
  nrfx_qspi_chip_erase();
#endif
}

/*************************************************************************************************/
/*!
 *  \brief  Get total size of NVM storage.
 *
 *  \return Total size.
 */
/*************************************************************************************************/
uint32_t PalNvmGetTotalSize(void)
{
#if defined(NRF52840_XXAA)
  return PAL_FLASH_TOTAL_SIZE;
#else
  return 0;
#endif
}

/*************************************************************************************************/
/*!
 *  \brief  Get sector size of NVM storage.
 *
 *  \return Sector size.
 */
/*************************************************************************************************/
uint32_t PalNvmGetSectorSize(void)
{
#if defined(NRF52840_XXAA)
  return PAL_FLASH_SECTOR4K_SIZE;
#else
  return 0;
#endif
}

/*************************************************************************************************/
/*!
 *  \brief     Get flash state.
 *
 *  \return    flash state.
 */
/*************************************************************************************************/
PalFlashState_t PalFlashGetState(void)
{
  return palFlashCb.state;
}

