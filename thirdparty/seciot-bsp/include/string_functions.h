#ifndef STRING_FUNTIONS_h
#define STRING_FUNTIONS_h
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <ctype.h>
#include "math_functions.h"
#include "log.h"

#define isalpha(c)    (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z'))
#define isupper(c)    ((c) >= 'A' && (c) <= 'Z')
#define islower(c)    ((c) >= 'a' && (c) <= 'z')
#define isdigit(c)    ((c) >= '0' && (c) <= '9')
#define isxdigit(c)   (isdigit(c) || ((c) >= 'a' && (c) <= 'f') || ((c) >= 'A' && (c) <= 'F'))
#define isspace(c)    ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\v' || (c) == '\f' || (c) == '\r')
#define ispunct(c)    (((c) >= 33 && (c) <= 47) || ((c) >= 58 && (c) <= 64) || \
                       ((c) >= 91 && (c) <= 96) || ((c) >= 123 && (c) <= 126))
#define isalnum(c)    (isalpha(c) || isdigit(c))

#ifdef __cplusplus
extern "C" {
#endif

/**
 * struct str_info - Input string parameters
 * @neg: negative number or not
 *	 0 - not negative
 *	 1 - negative
 * @any: set any if any `digits' consumed; make it negative to indicate
 *	 overflow
 * @acc: accumulated value
 */
struct str_info {
	int neg, any;
	unsigned int acc;
};

/**
 * @fn int atoi(const char *str)
 * @brief Converts a string to an integer.
 * @details This function takes a string representation of an integer and converts it to an
 * integer value. It handles optional leading whitespace, a leading sign (+ or -),
 * and ensures that the result does not overflow the range of a standard integer.
 * @param str A pointer to the null-terminated string to be converted.
 *            The string may contain leading whitespace and an optional
 *            sign. It should only contain digits after the optional sign.
 * @return The integer representation of the string. If the string is not a valid
 *         integer or exceeds the range of an integer, the function returns
 *         INT_MAX or INT_MIN.
 * @note The function does not handle non-numeric characters gracefully. If
 *       the first non-whitespace character is not a valid digit or sign, the
 *       function will return 0.
 * @warning This function does not perform error handling for invalid input
 *          beyond the overflow checks. It assumes the input will be a valid
 *          representation of an integer or a whitespace-padded string.
 * @see Str2Int, Str2Long
 */
int atoi(const char *str);

/**
 * @fn size_t StrLen(const char *str)
 * @brief The StrLen function calculates the length of a C-style string by iterating through the characters
 * until reaching the null terminator.
 * @param str The `str` parameter in the `StrLen` function is a pointer to a constant character
 * (string) that represents the starting address of the string whose length we want to calculate.
 * @return The function `StrLen` returns the length of the input string `str`, which is calculated by
 * subtracting the initial pointer `str` from the final pointer `s` after iterating through the string
 * to find the null terminator.
 */
size_t StrLen(const char *str);

/** @fn void ReverseStr(char *str, int length)
 * @brief Reverse a string and store in the same string pointer.
 * @param char *str
 * @param int length
 */
void ReverseStr(char *str, int len);

/** @fn int Int2Str(int number, char str[], int afterpoint)
 * @brief convert decimal numbers to string
 * @details Takes num as input and converts it to string.
 *	    The converted string is stored in str. The
 *          position of last character in the str is returned.
 *          This function is tailored to support ftoa.
 * @param int number
 * @param char str[]
 * @param int afterpoint
 * @return int
 */
int Int2Str(int number, char str[], unsigned int afterpoint);

/**
 * @fn void Float2Str(double f, int precision))
 * @brief Converts a floating-point number to a string and prints it to uart
 * @param f The floating-point number to be converted
 * @param precision The number of digits to be printed after the decimal point
 * @return None : The number is printed while in the function
 */
void Float2Str(double, int, char) ;

/**
 * @fn int atox(const char p[], int pos)
 * @brief The function "atox" converts a hexadecimal string to an integer value.
 * 
 * @param p The parameter `p` is a character array that represents a hexadecimal number.
 * @param pos The parameter "pos" represents the position of the character in the string "p" that we
 * want to convert to an integer.
 * 
 * @return The converted hexadecimal value of the input string.
 */
int Hex2Int(const char p[], int pos);

/**
 * @fn long Str2Long(const char *nptr, char **endptr, int base)
 * @brief Converts a string to a long integer
 * @param nptr A pointer to the string to be converted
 * @param endptr If not NULL, stores the address of the first invalid character in nptr
 * @param base The base of the number represented in nptr
 * @return The converted long int
 */
long Str2Long(const char *nptr, char **endptr, int base);

/**
 * @fn float Str2Float(const char* str, char** endptr)
 * @brief Converts a string to a float
 * @param nptr A pointer to the string to be converted
 * @param endptr If not NULL, stores the address of the first invalid character in nptr
 * @return The converted float
 */
float Str2Float(const char* str, char** endptr);

/**
 * Str2Int() - Write string data to structure
 * @nptr: pointer to string
 * @base: number's base
 * @unsign: describes what integer is expected
 *	    0 - not unsigned
 *	    1 - unsigned
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 *
 * Return: struct str_info *, which contains string data to future process
 */
struct str_info *Str2Int(const char **nptr, int base, unsigned int unsign);

/**
 * Str2QInt() - Convert a string to a quad integer
 * @nptr: pointer to string
 * @endptr: pointer to number's end in the string
 * @base: number's base
 *
 * Return: s64 quad integer number converted from input string
 */
signed int Str2QInt(const char *nptr, char **endptr, int base);

/**
 * Str2UQInt() - Convert a string to an unsigned quad integer
 * @nptr: pointer to string
 * @endptr: pointer to number's end in the string
 * @base: number's base
 *
 * Return: s64 unsigned quad integer number converted from
 *         input string
 */
unsigned int Str2UQInt(const char *nptr, char **endptr, int base);

#ifdef __cplusplus
}
#endif
#endif