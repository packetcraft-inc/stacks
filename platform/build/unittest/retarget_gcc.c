/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Re-target system call stubs to satisfy linking to hosted system calls.
 *
 *  Copyright (c) 2013-2019 Arm Ltd. All Rights Reserved.
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

#include <sys/stat.h>

/**************************************************************************************************
  Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Open file.
 *
 *  \param      name        Filename.
 *  \param      flags       Flags.
 *  \param      mode        Open mode.
 *
 *  \return     File handle.
 */
/*************************************************************************************************/
int _open(const char *name, int flags, int mode)
{
  /* No implementation. */
  return -1;
}

/*************************************************************************************************/
/*!
 *  \brief      Close file.
 *
 *  \param      file        File handle.
 *
 *  \return     Result.
 */
/*************************************************************************************************/
int _close(int file)
{
  /* No implementation. */
  return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      File status.
 *
 *  \param      file        File handle.
 *  \param      *st         File status response.
 *
 *  \return     Result.
 */
/*************************************************************************************************/
int _fstat(int file, struct stat *st)
{
  /* No implementation. */
  return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      Get terminal type.
 *
 *  \param      file        File handle.
 *
 *  \return     Result.
 */
/*************************************************************************************************/
int _isatty(int file)
{
  /* No implementation. */
  return 1;
}

/*************************************************************************************************/
/*!
 *  \brief      File seek.
 *
 *  \param      file        File handle.
 *  \param      ptr         Offset.
 *  \param      dir         Direction of offset.
 *
 *  \return     Result.
 */
/*************************************************************************************************/
int _lseek(int file, int ptr, int dir)
{
  /* No implementation. */
  return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      Read from file.
 *
 *  \param      file        File handle.
 *  \param      ptr         Destination buffer.
 *  \param      len         Length to read.
 *
 *  \return     Result.
 */
/*************************************************************************************************/
int _read(int file, char *ptr, int len)
{
  /* No implementation. */
  return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      Write to file.
 *
 *  \param      file        File handle.
 *  \param      ptr         Source buffer.
 *  \param      len         Length to write.
 *
 *  \return     Result.
 */
/*************************************************************************************************/
int _write(int file, char *ptr, int len)
{
  /* No implementation. */
  return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      Change the program's break limit.
 *
 *  \param      incr        Size.
 *
 *  \return     Old break value.
 */
/*************************************************************************************************/
caddr_t _sbrk(int incr)
{
  /* No implementation. */
  return 0;
}
