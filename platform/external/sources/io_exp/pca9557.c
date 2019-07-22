/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      I/O Expander for TI PCA9557.
 *
 *  Copyright (c) 2018-2019 Arm Ltd.
 *
 *  Copyright (c) 2019 Packetcraft, Inc.
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

#include "pal_twi.h"
#include "pal_io_exp.h"
#include "app_util_platform.h"
#include <string.h>

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/*! \brief      I/O Expander operations. */
enum
{
  IO_EXP_REG_READ,          /*!< I/O Expander read register. */
  IO_EXP_REG_WRITE          /*!< I/O Expander write register. */
};

/*! \brief      I/O Expander input register definition. */
enum
{
  IO_EXP_INPUT_PIN_LOW      = 0,          /*!< Voltage low. */
  IO_EXP_INPUT_PIN_HIGH     = 1,          /*!< Voltage High. */
};

/*! \brief      I/O Expander output register definition. */
enum
{
  IO_EXP_OUTPUT_PIN_LOW     = 0,          /*!< Voltage low. */
  IO_EXP_OUTPUT_PIN_HIGH    = 1,          /*!< Voltage High. */
};

/*! \brief      I/O Expander polarity register definition. */
enum
{
  IO_EXP_POLARITY_PIN_FALSE = 0,          /*!< Polarity of pin is not inverted. */
  IO_EXP_POLARITY_PIN_TRUE  = 1,          /*!< Polairty of pin is inverted. */
};
/*! \brief      I/O Expander Configuration register definition. */
enum
{
  IO_EXP_CONFIG_PIN_OUTPUT  = 0,          /*!< Output pin. */
  IO_EXP_CONFIG_PIN_INPUT   = 1,          /*!< Input pin. */
};

/*! \brief      I/O Expander TWI state. */
enum
{
  IO_EXP_READ_STATE,                      /*!< TWI Read state. */
  IO_EXP_WRITE_STATE,                     /*!< TWI Write state. */
};

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! \brief      Invalid device handle. */
#define IO_EXP_INVALID_DEVICE_HANDLE     0xFF

/*! \brief      Invalid device address. */
#define IO_EXP_INVALID_DEVICE_ADDR       0xFF

#ifdef DEBUG

/*! \brief      Parameter check. */
#define IO_EXP_PARAM_CHECK(expr)                   { if (!(expr)) { return; } }
#define IO_EXP_PARAM_CHECK_RETURN(expr)            { if (!(expr)) { return FALSE; } }
#define IO_EXP_PARAM_CHECK_RETURN_HANDLE(expr)     { if (!(expr)) { return IO_EXP_INVALID_DEVICE_HANDLE; } }
#define IO_EXP_PARAM_CHECK_RETURN_STATE(expr)      { if (!(expr)) { return PAL_IO_EXP_STATE_ERROR; } }

#else

/*! \brief      Parameter check (disabled). */
#define IO_EXP_PARAM_CHECK(expr)
#define IO_EXP_PARAM_CHECK_RETURN(expr)
#define IO_EXP_PARAM_CHECK_RETURN_HANDLE(expr)
#define IO_EXP_PARAM_CHECK_RETURN_STATE(expr)

#endif

/*! \brief      I/O Expander device group address. */
#define IO_EXP_GROUP_ADDR                 0x03

/*! \brief      I/O Expander device group address mask. */
#define IO_EXP_GROUP_MASK                 0x0F

/*! \brief      I/O Expander device sub address mask. */
#define IO_EXP_SUB_MASK                   0x07
/*! \brief      I/O Expander device address. */
#define IO_EXP_ADDR(group, sub)           ((uint8_t)(((group) & IO_EXP_GROUP_MASK) << 3) | ((sub) & IO_EXP_SUB_MASK))

/*! \brief      Maximum number of of I/O expanders. */
#define IO_EXP_MAX_DEVICE                 4

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! \brief    Control block. */
static struct
{
  uint8_t subAddr;                             /*!< pca9557 subAddrress defined by the voltage on board. */
  PalIoExpState_t  state:8;                       /*!< pca9557 state. */
  uint8_t rdOrWrReg;                           /*!< pca9557 read or write register. */
  uint8_t twiHandle;                           /*!< Device handle from TWI interface. */
  uint8_t dataLen;                             /*!< Data length. */
  uint8_t data[2];                             /*!< Data buffer. */
  uint8_t rdOrWr;                              /*!< Read or write TWI operation. */
  PalIoExpRdRegCompCback_t rdCback;               /*!< I/O Expander read register complete callback. */
  PalIoExpWrRegCompCback_t wrCback;               /*!< I/O Expander write register complete callback. */
} ioExpCb[IO_EXP_MAX_DEVICE];

/*************************************************************************************************/
/*!
 *  \brief    Find device handle from twi handle.
 *
 *  \param    twiHandle       Twi handle.
 *
 *  \return   Device handle.
 */
/*************************************************************************************************/
static uint8_t ioExpFindDevHandle(uint8_t twiHandle)
{
  for (unsigned int devHandle = 0; devHandle < IO_EXP_MAX_DEVICE; devHandle++)
  {
    if (ioExpCb[devHandle].twiHandle == twiHandle)
    {
      return (uint8_t)devHandle;
    }
  }
  return IO_EXP_INVALID_DEVICE_HANDLE;
}

/*************************************************************************************************/
/*!
 *  \brief    Read register in I/O Expander.
 *
 *  \param    devHandle       Device handle.
 *  \param    regNum          Register to read.
 *
 *  \return   None.
 */
/*************************************************************************************************/
static void ioExpReadReg(uint8_t devHandle, uint8_t regNum)
{
  ioExpCb[devHandle].data[0]   = regNum;
  ioExpCb[devHandle].dataLen   = 1;

  ioExpCb[devHandle].rdOrWrReg = IO_EXP_REG_READ;
  ioExpCb[devHandle].rdOrWr    = IO_EXP_WRITE_STATE;

  PalTwiStartOperation(ioExpCb[devHandle].twiHandle);
}

/*************************************************************************************************/
/*!
 *  \brief    Write register in I/O Expander.
 *
 *  \param    devHandle       Device handle.
 *  \param    regNum          Register to write.
 *  \param    regValue        Register value.
 *
 *  \return   None.
 */
/*************************************************************************************************/
static void ioExpWriteReg(uint8_t devHandle, uint8_t regNum, uint8_t regValue)
{
  ioExpCb[devHandle].data[0]   = regNum;
  ioExpCb[devHandle].data[1]   = regValue;
  ioExpCb[devHandle].dataLen   = 2;

  ioExpCb[devHandle].rdOrWrReg = IO_EXP_REG_WRITE;
  ioExpCb[devHandle].rdOrWr    = IO_EXP_WRITE_STATE;

  PalTwiStartOperation(ioExpCb[devHandle].twiHandle);
}

/*************************************************************************************************/
/*!
 *  \brief      TWI operation ready callback.
 *
 *  \param      twiHandle     Twi handle value.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void ioExpReadyCback(uint8_t twiHandle)
{
  uint8_t devHandle = ioExpFindDevHandle(twiHandle);

  IO_EXP_PARAM_CHECK(devHandle < IO_EXP_MAX_DEVICE);

  if (ioExpCb[devHandle].rdOrWr == IO_EXP_WRITE_STATE)
  {
    PalTwiWriteData(twiHandle, ioExpCb[devHandle].data, ioExpCb[devHandle].dataLen);
  }
  else
  {
    PalTwiReadData(twiHandle, ioExpCb[devHandle].data,ioExpCb[devHandle].dataLen);
  }
}

/*************************************************************************************************/
/*!
 *  \brief      Write or read complete callback.
 *
 *  \param      twiHandle     Twi handle value.
 *  \param      result        Result.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void ioExpRdCompCback(uint8_t twiHandle, bool_t result)
{
  uint8_t devHandle = ioExpFindDevHandle(twiHandle);

  IO_EXP_PARAM_CHECK(devHandle < IO_EXP_MAX_DEVICE);

  PalTwiStopOperation(twiHandle);

  if (ioExpCb[devHandle].rdCback)
  {
    ioExpCb[devHandle].rdCback(result, ioExpCb[devHandle].data[0]);
  }

  ioExpCb[devHandle].state = result ?  PAL_IO_EXP_STATE_READY : PAL_IO_EXP_STATE_ERROR;
}

/*************************************************************************************************/
/*!
 *  \brief      Write or write complete callback.
 *
 *  \param      twiHandle     Twi handle value.
 *  \param      result        Result.
 *
 *  \return     None.
 */
/*************************************************************************************************/
static void ioExpWrCompCback(uint8_t twiHandle, bool_t result)
{
  uint8_t devHandle = ioExpFindDevHandle(twiHandle);

  IO_EXP_PARAM_CHECK(devHandle < IO_EXP_MAX_DEVICE);

  if (ioExpCb[devHandle].rdOrWrReg == IO_EXP_REG_READ)
  {
    if (result)
    {
      ioExpCb[devHandle].dataLen = 1;
      ioExpCb[devHandle].rdOrWr  = IO_EXP_READ_STATE;
      ioExpReadyCback(twiHandle);
    }
    else
    {
      PalTwiStopOperation(twiHandle);
      if (ioExpCb[devHandle].rdCback)
      {
        ioExpCb[devHandle].rdCback(FALSE, 0);
      }
      ioExpCb[devHandle].state = PAL_IO_EXP_STATE_ERROR;
    }
  }
  else
  {
    PalTwiStopOperation(twiHandle);

    if (ioExpCb[devHandle].wrCback)
    {
      ioExpCb[devHandle].wrCback(result);
    }

    ioExpCb[devHandle].state = PAL_IO_EXP_STATE_READY;
  }
}

/*************************************************************************************************/
/*!
 *  \brief    Initialize I/O Expander.
 *
 *  \return   None.
 */
/*************************************************************************************************/
void PalIoExpInit(void)
{
  for (unsigned int devHandle = 0; devHandle < IO_EXP_MAX_DEVICE; devHandle++)
  {
    memset(&ioExpCb[devHandle], 0, sizeof(ioExpCb[0]));
    ioExpCb[devHandle].state     = PAL_IO_EXP_STATE_INIT;
    ioExpCb[devHandle].twiHandle = PAL_TWI_INVALID_ID;
    ioExpCb[devHandle].subAddr    = IO_EXP_INVALID_DEVICE_ADDR;
  }

  PalTwiInit();
}

/*************************************************************************************************/
/*!
 *  \brief    De-Initialize I/O Expander.
 *
 *  \return   None.
 */
/*************************************************************************************************/
void PalIoExpDeInit(void)
{
  for (unsigned int devHandle = 0; devHandle < IO_EXP_MAX_DEVICE; devHandle++)
  {
    ioExpCb[devHandle].state = PAL_IO_EXP_STATE_UNINIT;
  }
}

/*************************************************************************************************/
/*!
 *  \brief    Register I/O Expander device.
 *
 *  \param    subAddr     Sub address defined by the voltages on board.
 *
 *  \return   Device handle.
 */
/*************************************************************************************************/
uint8_t PalIoExpRegisterDevice(uint8_t subAddr)
{
  /* Make sure sub address is 3-bit only. */
  IO_EXP_PARAM_CHECK_RETURN_HANDLE((subAddr & ~IO_EXP_SUB_MASK) == 0);

  PalTwiDevConfig_t devCfgPca9557 =
  {
    .devAddr = IO_EXP_ADDR(IO_EXP_GROUP_ADDR, subAddr),
    .opReadyCback = ioExpReadyCback,
    .wrCback = ioExpWrCompCback,
    .rdCback = ioExpRdCompCback
  };

  for (unsigned int devHandle = 0; devHandle < IO_EXP_MAX_DEVICE; devHandle++)
  {
    /* Make sure there's no duplicated address registered for I/O Expander. */
    IO_EXP_PARAM_CHECK_RETURN_HANDLE(ioExpCb[devHandle].subAddr != subAddr);

    if (ioExpCb[devHandle].state == PAL_IO_EXP_STATE_INIT)
    {
      ioExpCb[devHandle].twiHandle = PalTwiRegisterDevice(&devCfgPca9557);
      ioExpCb[devHandle].state     = (ioExpCb[devHandle].twiHandle != PAL_TWI_INVALID_ID) ?
                                       PAL_IO_EXP_STATE_READY : PAL_IO_EXP_STATE_ERROR;
      ioExpCb[devHandle].subAddr    = subAddr;

      return (uint8_t)devHandle;
    }
  }

  return IO_EXP_INVALID_DEVICE_HANDLE;
}

/*************************************************************************************************/
/*!
 *  \brief    Register I/O Expander callback functions.
 *
 *  \param    devHandle   Device handle.
 *  \param    rdCback     Read register completion callback.
 *  \param    wrCback     Write register completion callback.
 *
 *  \return   None
 *
 *  \note     Callback functions is allowed to be changed multiple times by this API.
 */
/*************************************************************************************************/
void PalIoExpRegisterCback(uint8_t devHandle, PalIoExpRdRegCompCback_t rdCback, PalIoExpWrRegCompCback_t wrCback)
{
  IO_EXP_PARAM_CHECK(devHandle < IO_EXP_MAX_DEVICE);
  IO_EXP_PARAM_CHECK(ioExpCb[devHandle].state == PAL_IO_EXP_STATE_READY);

  ioExpCb[devHandle].rdCback = rdCback;
  ioExpCb[devHandle].wrCback = wrCback;
}
/*************************************************************************************************/
/*!
 *  \brief      Get the current state.
 *
 *  \param      devHandle     Device handle.
 *  \return     Current state.
 *
 *  Return the current state.
 */
/*************************************************************************************************/
PalIoExpState_t PalIoExpGetState(uint8_t devHandle)
{
  IO_EXP_PARAM_CHECK_RETURN_STATE(devHandle < IO_EXP_MAX_DEVICE);

  return ioExpCb[devHandle].state;
}

/*************************************************************************************************/
/*!
 *  \brief    Read port status.
 *
 *  \param    devHandle   Device handle.
 *  \param    op          Operation.
 *
 *  \return   None.
 *
 *  \note     This is a non-blocking API. Value will be updated in rdCback.
 */
/*************************************************************************************************/
void PalIoExpRead(uint8_t devHandle, PalIoOp_t op)
{
  IO_EXP_PARAM_CHECK(devHandle < IO_EXP_MAX_DEVICE);
  IO_EXP_PARAM_CHECK(ioExpCb[devHandle].state == PAL_IO_EXP_STATE_READY);

  ioExpCb[devHandle].state = PAL_IO_EXP_STATE_BUSY;
  ioExpReadReg(devHandle, op);
}

/*************************************************************************************************/
/*!
 *  \brief    Write port value.
 *
 *  \param    devHandle   Device handle.
 *  \param    op          Operation.
 *  \param    value       New port value.
 *
 *  \return   None.
 *
 *  \note     This is a non-blocking API. Value will be updated in wrCback.
 */
/*************************************************************************************************/
void PalIoExpWrite(uint8_t devHandle, PalIoOp_t op, uint8_t value)
{
  IO_EXP_PARAM_CHECK(devHandle < IO_EXP_MAX_DEVICE);
  IO_EXP_PARAM_CHECK(ioExpCb[devHandle].state == PAL_IO_EXP_STATE_READY);

  ioExpCb[devHandle].state = PAL_IO_EXP_STATE_BUSY;
  ioExpWriteReg(devHandle, op, value);
}
