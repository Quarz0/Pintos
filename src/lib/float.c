#include <inttypes.h>
#include <stdio.h>
#include "float.h"

/* This file contains the basic arithmetic operations on the floating point struct
   type defined in the header file. */

/* Initializes a new float type from an int. */
struct float32 to_float (int x)
{
  struct float32 ret;
  ret.n = x * (1 << FLOAT_Q);
  return ret;
}

/* Adds two floating point numbers. */
struct float32 add (struct float32 x, struct float32 y)
{
  struct float32 ret;
  ret.n = x.n + y.n;
  return ret;
}

/* Adds a floating point number and an int */
struct float32 add_int (struct float32 x, int y)
{
  struct float32 ret;
  ret.n = x.n + y * (1 << FLOAT_Q);
  return ret;
}

/* Subtracts two floating point numbers. */
struct float32 subtract (struct float32 x, struct float32 y)
{
  struct float32 ret;
  ret.n = x.n - y.n;
  return ret;
}

/* Subtracts a floating point number and an int */
struct float32 subtract_int (struct float32 x, int y)
{
  struct float32 ret;
  ret.n = x.n - y * (1 << FLOAT_Q);
  return ret;
}

/* Multiplies two floating point numbers. */
struct float32 multiply (struct float32 x, struct float32 y)
{
  struct float32 ret;
  ret.n = ((int64_t) x.n) * y.n / (1 << FLOAT_Q);
  return ret;
}

/* Multiplies a floating point number and an int */
struct float32 multiply_int (struct float32 x, int y)
{
  struct float32 ret;
  ret.n = x.n * y;
  return ret;
}

/* Divides two floating point numbers. */
struct float32 divide (struct float32 x, struct float32 y)
{
  struct float32 ret;
  ret.n = ((int64_t) x.n) * (1 << FLOAT_Q) / y.n;
  return ret;
}

/* Divides a floating point number and an int */
struct float32 divide_int (struct float32 x, int y)
{
  struct float32 ret;
  ret.n = x.n / y;
  return ret;
}

/* Converts a float to an int. If round set to true,
   then the value is rounded to the nearest integer,
   else the part after the decimal point is truncated. */
int to_int (struct float32 x, bool round)
{
  if (round)
  {
    if (x.n >= 0)
    {
      return (x.n + (1 << FLOAT_Q) / 2) / (1 << FLOAT_Q);
    }
    else
    {
      return (x.n - (1 << FLOAT_Q) / 2) / (1 << FLOAT_Q);
    }
  }
  else
  {
    return x.n / (1 << FLOAT_Q);
  }
}
