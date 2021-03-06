// -*- C++ -*-
// $Id: drand48.src,v 1.4 2002/07/24 16:47:52 mf Exp $
// ---------------------------------------------------------------------------
//
// This code is based on a code extracted from GNU C Library 2.1.3 with
// a purpose to provide drand48() on Windows NT.
//

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define	__LITTLE_ENDIAN	1234
#define	__BIG_ENDIAN	4321

#ifdef __BIG_ENDIAN__
#define __BYTE_ORDER __BIG_ENDIAN     /* powerpc-apple is big-endian. */
#else
#define __BYTE_ORDER __LITTLE_ENDIAN  /* i386 is little-endian.  */
#endif
#define __FLOAT_WORD_ORDER __BYTE_ORDER

#define IEEE754_DOUBLE_BIAS	0x3ff /* Added to exponent.  */

#ifdef WIN32
#include <wtypes.h>
typedef ULONGLONG u_int64_t;
#else
typedef unsigned long long int u_int64_t;
#endif

union ieee754_double {
  double d;

    /* This is the IEEE 754 double-precision format.  */
  struct {
#if __BYTE_ORDER == __BIG_ENDIAN
    unsigned int negative:1;
    unsigned int exponent:11;
    /* Together these comprise the mantissa.  */
    unsigned int mantissa0:20;
    unsigned int mantissa1:32;
#endif				/* Big endian.  */
#if __BYTE_ORDER == __LITTLE_ENDIAN
#if __FLOAT_WORD_ORDER == BIG_ENDIAN
    unsigned int mantissa0:20;
    unsigned int exponent:11;
    unsigned int negative:1;
    unsigned int mantissa1:32;
#else
    /* Together these comprise the mantissa.  */
    unsigned int mantissa1:32;
    unsigned int mantissa0:20;
    unsigned int exponent:11;
    unsigned int negative:1;
#endif
#endif				/* Little endian.  */
  } ieee;
};

/* Data structure for communication with thread safe versions.  */
typedef struct drand48_data_ {
  unsigned short int x[3];     /* Current state.  */
  unsigned short int a[3];     /* Factor in congruential formula.  */
  unsigned short int c;	       /* Additive const. in congruential formula.  */
  unsigned short int old_x[3]; /* Old state.  */
  int init;                    /* Flag for initializing.  */
} drand48_data;

double drand48 ();
int erand48_r (unsigned short int[3], drand48_data *, double *result);
int drand48_iterate (unsigned short int[3], drand48_data *);
void srand48 (long int);
int srand48_r (long int, drand48_data *);
unsigned short int * seed48 (unsigned short int[3]);
int seed48_r (unsigned short int[3], drand48_data *);

/* Global state for non-reentrant functions. */
static drand48_data libc_drand48_data;

// ---------------------------------------------------------------------------
double drand48 ()
{
  double result;
  (void) erand48_r (libc_drand48_data.x, &libc_drand48_data, &result);
  return result;
}

// ---------------------------------------------------------------------------
int erand48_r (unsigned short int xsubi[3],
	       drand48_data *buffer,
               double *result)
{
  union ieee754_double temp;

  /* Compute next state.  */
  if (drand48_iterate (xsubi, buffer) < 0) return -1;

  /* Construct a positive double with the 48 random bits distributed over
     its fractional part so the resulting FP number is [0.0,1.0).  */

#if USHRT_MAX == 65535
  temp.ieee.negative = 0;
  temp.ieee.exponent = IEEE754_DOUBLE_BIAS;
  temp.ieee.mantissa0 = (xsubi[2] << 4) | (xsubi[1] >> 12);
  temp.ieee.mantissa1 = ((xsubi[1] & 0xfff) << 20) | (xsubi[0] << 4);
#elif USHRT_MAX == 2147483647
  temp.ieee.negative = 0;
  temp.ieee.exponent = IEEE754_DOUBLE_BIAS;
  temp.ieee.mantissa0 = (xsubi[1] << 4) | (xsubi[0] >> 28);
  temp.ieee.mantissa1 = ((xsubi[0] & 0xfffffff) << 4);
#else
# error Unsupported size of short int
#endif

  /* Please note the lower 4 bits of mantissa1 are always 0.  */
  *result = temp.d - 1.0;

  return 0;
}

// ---------------------------------------------------------------------------
int drand48_iterate (unsigned short int xsubi[3], drand48_data *buffer)
{
  u_int64_t X, a, result;

  /* Initialize buffer, if not yet done.  */
  if (!buffer->init) {
#if (USHRT_MAX == 0xffffU)
      buffer->a[2] = 0x5;
      buffer->a[1] = 0xdeec;
      buffer->a[0] = 0xe66d;
#else
      buffer->a[2] = 0x5deecUL;
      buffer->a[1] = 0xe66d0000UL;
      buffer->a[0] = 0;
#endif
      buffer->c = 0xb;
      buffer->init = 1;
  }

  /* Do the real work.  We choose a data type which contains at least
     48 bits.  Because we compute the modulus it does not care how
     many bits really are computed.  */

  if (sizeof (unsigned short int) == 2) {
    X = (u_int64_t)xsubi[2] << 32 | (u_int64_t)xsubi[1] << 16 | xsubi[0];
    a = ((u_int64_t)buffer->a[2] << 32 | (u_int64_t)buffer->a[1] << 16
	 | buffer->a[0]);

    result = X * a + buffer->c;

    xsubi[0] = result & 0xffff;
    xsubi[1] = (result >> 16) & 0xffff;
    xsubi[2] = (result >> 32) & 0xffff;
  }else{
    X = (u_int64_t)xsubi[2] << 16 | xsubi[1] >> 16;
    a = (u_int64_t)buffer->a[2] << 16 | buffer->a[1] >> 16;

    result = X * a + buffer->c;

    xsubi[0] = result >> 16 & 0xffffffffl;
    xsubi[1] = result << 16 & 0xffff0000l;
  }

  return 0;
}

// ---------------------------------------------------------------------------
void srand48 (long int seedval)
{
  (void) srand48_r (seedval, &libc_drand48_data);
}

// ---------------------------------------------------------------------------
int srand48_r (long int seedval, drand48_data *buffer)
{
  /* The standards say we only have 32 bits.  */
  if (sizeof (long int) > 4)
    seedval &= 0xffffffffl;

#if USHRT_MAX == 0xffffU
  buffer->x[2] = seedval >> 16;
  buffer->x[1] = seedval & 0xffffl;
  buffer->x[0] = 0x330e;

  buffer->a[2] = 0x5;
  buffer->a[1] = 0xdeec;
  buffer->a[0] = 0xe66d;
#else
  buffer->x[2] = seedval;
  buffer->x[1] = 0x330e0000UL;
  buffer->x[0] = 0;

  buffer->a[2] = 0x5deecUL;
  buffer->a[1] = 0xe66d0000UL;
  buffer->a[0] = 0;
#endif
  buffer->c = 0xb;
  buffer->init = 1;

  return 0;
}

// ---------------------------------------------------------------------------
unsigned short int * seed48 (unsigned short int seed16v[3])
{
  (void) seed48_r (seed16v, &libc_drand48_data);
  return libc_drand48_data.old_x;
}

// ---------------------------------------------------------------------------
int seed48_r (unsigned short int seed16v[3], drand48_data *buffer)
{
  /* Save old value at a private place to be used as return value.  */
  memcpy (buffer->old_x, buffer->x, sizeof (buffer->x));

  /* Install new state.  */
#if USHRT_MAX == 0xffffU
  buffer->x[2] = seed16v[2];
  buffer->x[1] = seed16v[1];
  buffer->x[0] = seed16v[0];

  buffer->a[2] = 0x5;
  buffer->a[1] = 0xdeec;
  buffer->a[0] = 0xe66d;
#else
  buffer->x[2] = (seed16v[2] << 16) | seed16v[1];
  buffer->x[1] = seed16v[0] << 16;
  buffer->x[0] = 0;

  buffer->a[2] = 0x5deecUL;
  buffer->a[1] = 0xe66d0000UL;
  buffer->a[0] = 0;
#endif
  buffer->c = 0xb;
  buffer->init = 1;

  return 0;
}
