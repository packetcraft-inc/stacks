#ifndef MATH_FUNCTIONS_H
#define MATH_FUNCTIONS_H

#include "stddef.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @fn float pow_10(unsigned int y)
 * @brief generate different powers of 10
 * @param unsigned int y
 * @return return result in float 
 */

double exponents(double x);
double logrithm(double x);
double modfloat(double value, double *intpart);
double powers(double base, double exponent);
double squareroot(double value);
double cuberoot(double value);

#ifdef __cplusplus
}
#endif
#endif