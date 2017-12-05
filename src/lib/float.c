#include <inttypes.h>
#include <stdio.h>
#include "float.h"

struct float32 to_float (int x)
{
  struct float32 ret;
  ret.n = x * (1 << FLOAT_Q);
  return ret;
}

struct float32 add (struct float32 x, struct float32 y)
{
  struct float32 ret;
  ret.n = x.n + y.n;
  return ret;
}

struct float32 add_int (struct float32 x, int y)
{
  struct float32 ret;
  ret.n = x.n + y * (1 << FLOAT_Q);
  return ret;
}

struct float32 subtract (struct float32 x, struct float32 y)
{
  struct float32 ret;
  ret.n = x.n - y.n;
  return ret;
}

struct float32 subtract_int (struct float32 x, int y)
{
  struct float32 ret;
  ret.n = x.n - y * (1 << FLOAT_Q);
  return ret;
}

struct float32 multiply (struct float32 x, struct float32 y)
{
  struct float32 ret;
  ret.n = ((int64_t) x.n) * y.n / (1 << FLOAT_Q);
  return ret;
}

struct float32 multiply_int (struct float32 x, int y)
{
  struct float32 ret;
  ret.n = x.n * y;
  return ret;
}

struct float32 divide (struct float32 x, struct float32 y)
{
  struct float32 ret;
  ret.n = ((int64_t) x.n) * (1 << FLOAT_Q) / y.n;
  return ret;
}

struct float32 divide_int (struct float32 x, int y)
{
  struct float32 ret;
  ret.n = x.n / y;
  return ret;
}

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
