#ifndef IO_H
#define IO_H
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include "uart.h"
#include "math_functions.h"
#include "string_functions.h"
#include "memory_functions.h"
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>

#define	EOF	(-1)

#ifdef __cplusplus
extern "C" {
#endif

/** @fn void _print_(const char *fmt, va_list ap)
 * @brief Handles the input stream of characters to printf on screen
 * @details Identifies the type of format string, number of arguments and prints
 * the right characer on screen
 * @param const char * fmt - formatting strings
 * @param const va_list ap - arg list
 */
void _print_(const char *fmt, va_list ap);

/** @fn int printf(const char* fmt, ...)
 * @brief function to printf characters on file
 * @details prints the characters on terminal
 * @param const char*
 * @return int
 */
int printf(const char *fmt, ...);

/**
* @fn int scanf(const char* format, ...)
* @brief reads formatted input from the standard input stream
* @details the format parameter specifies the format of the input, consisting of conversion specifications and ordinary characters.
* @param const char* format - string that specifies the format of the input
* @return int - number of input items successfully read and assigned. If an error occurs, or if the end of the input is reached before the first conversion, EOF is returned.
**/
int scanf(const char* format, ...);

/**
 * sscanf - Unformat a buffer into a list of arguments
 * @buf:	input buffer
 * @fmt:	formatting of buffer
 * @...:	resulting arguments
 */
int sscanf(const char *buf, const char *fmt, ...);

// void sprintf(char *buffer, const char *format, ...) ;

#ifdef __cplusplus
}
#endif
#endif