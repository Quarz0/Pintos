#ifndef __LIB_FLOAT_H
#define __LIB_FLOAT_H

#define FLOAT_P 17
#define FLOAT_Q 14

struct float32
{
  int n; /* First FLOAT_P bits represent the part before decimal point
            and the remaining bits represent the part after the decimal point. */
};

struct float32 to_float (int x);
struct float32 add (struct float32 x, struct float32 y);
struct float32 add_int (struct float32 x, int y);
struct float32 subtract (struct float32 x, struct float32 y);
struct float32 subtract_int (struct float32 x, int y);
struct float32 multiply (struct float32 x, struct float32 y);
struct float32 multiply_int (struct float32 x, int y);
struct float32 divide (struct float32 x, struct float32 y);
struct float32 divide_int (struct float32 x, int y);
int to_int (struct float32 x, bool round);

#endif /* lib/float.h */
