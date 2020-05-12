/*************************************************************************************************/
/*!
 *  \brief  Main entry.
 *
 *  Copyright (c) 2013-2019 Arm Ltd. All Rights Reserved.
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

#include "unity_fixture.h"

#include "pal_uart.h"

/*************************************************************************************************/
/*!
 *  \brief      Run all tests.
 */
/*************************************************************************************************/
static void RunAllTests(void)
{
  RUN_TEST_GROUP(Uart);
  RUN_TEST_GROUP(Twi);
  RUN_TEST_GROUP(Flash);
  RUN_TEST_GROUP(Button);
}

/*************************************************************************************************/
/*!
 *  \brief      Uart read completion callback.
 */
/*************************************************************************************************/
static void mainUartReadCompletion(void)
{
  uartRdCompleteFlag = 1;
}

/*************************************************************************************************/
/*!
 *  \brief      Uart write completion callback.
 */
/*************************************************************************************************/
static void mainUartWriteCompletion(void)
{
  uartWrCompleteFlag = 1;
}

/*************************************************************************************************/
/*!
 *  \brief      Main entry point.
 *
 *  \return     Number of errors.
 */
/*************************************************************************************************/
int main(void)
{
  char * unityArgv[] =
  {
      "unittest-host",
      "-v",                 /* verbose */
      /* "-g", "", */       /* group filter */
      /* "-n", "", */       /* name filter */
      /* "-r", "1" */       /* repeat count */
  };

  int unityArgc = sizeof(unityArgv) / sizeof(char *);

  PalUartConfig_t cfg;
  cfg.baud   = 1000000;
  cfg.hwFlow = TRUE;
  cfg.rdCback = mainUartReadCompletion;
  cfg.wrCback = mainUartWriteCompletion;

  PalUartInit(PAL_UART_ID_TERMINAL, &cfg);

#ifdef __IAR_SYSTEMS_ICC__
  __enable_interrupt();
#endif
#ifdef __GNUC__
  __asm volatile ("cpsie i");
#endif
#ifdef __CC_ARM
  __enable_irq();
#endif
  return UnityMain(unityArgc, unityArgv, RunAllTests);
}

/*************************************************************************************************/
/*!
 *  \brief      Write a character to the console transport interface.
 *
 *  \param      ch      Character to write to the console.
 *
 *  \return     Actual character written to the console.
 */
/*************************************************************************************************/
int UnityPutchar(int ch)
{
  uartWrCompleteFlag = 0;
  PalUartWriteData(PAL_UART_ID_TERMINAL, (const uint8_t *)&ch, 1);
  while (!uartWrCompleteFlag);

  return ch;
}

/*************************************************************************************************/
/*!
 *  \brief      Log an assert action.
 *
 *  \param      pFile   Name of file originating assert.
 *  \param      line    Line number of assert statement.
 */
/*************************************************************************************************/
void WsfAssert(const char *pFile, uint16_t line)
{
  UNITY_OUTPUT_CHAR('\n');
  UnityPrint("Assertion failed at: ");
  UnityPrint(pFile);
  UnityPrint(":");
  UnityPrintNumber(line);

  UNITY_TEST_FAIL(__LINE__, "Assertion failed");
}
