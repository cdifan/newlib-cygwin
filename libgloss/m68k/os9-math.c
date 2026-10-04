/* os9-math.c -- floating point through OS-9's math module (libos9math,
   for -mos9math).

   GCC's soft-float entry points (__adddf3, __ltsf2, __floatsidf, ...),
   which libgcc otherwise provides, as calls of the math trap handler
   (trap #15, the "math" module a CD-i player has in ROM), as Microware
   C 3.2 compiles floating point with -X: smaller programs, and the
   results Microware C gets.  The handler takes a double in d0:d1 (a
   second one in d2:d3) or a float in d0 (a second one in d1) and returns
   the result in d0:d1 or d0, or the condition codes of a comparison.
   It doesn't keep libgcc's exception flags (_fpCCR).  A constructor links
   the handler (F$TLink), as Microware's cstart.r does on the first trap.

   Called the way GCC calls libgcc: with the stack convention, in either
   multilib.  Nothing here may use floating-point arithmetic itself.  */

#include "os9-sys.h"

#define F_TLINK		0x21
#define T_MATH		15

/* Trap #15 function codes.  */
#define T_FADD		6
#define T_FSUB		8
#define T_FMUL		10
#define T_FDIV		11
#define T_FCMP		12
#define T_DADD		14
#define T_DSUB		16
#define T_DMUL		18
#define T_DDIV		19
#define T_DCMP		20
#define T_LTOF		31
#define T_LTOD		32
#define T_UTOF		33
#define T_UTOD		34
#define T_FTOL		35
#define T_DTOL		36
#define T_FTOU		37
#define T_DTOU		38
#define T_FTOD		39
#define T_DTOF		40

#define STACKCALL __attribute__ ((stackcall))

union dbl { double d; unsigned long w[2]; };
union flt { float f; unsigned long w; };

/* Function CODE on the double in HI:LO, and on X:Y (the second operand,
   when there's one); the result (d0:d1) in HI:LO.  */
#define MATH_D(code, hi, lo, x, y)					\
  do {									\
    register unsigned long d0_ __asm__ ("d0") = (hi);			\
    register unsigned long d1_ __asm__ ("d1") = (lo);			\
    register unsigned long d2_ __asm__ ("d2") = (x);			\
    register unsigned long d3_ __asm__ ("d3") = (y);			\
    __asm__ volatile ("trap #15\n\t.word %c4"				\
		      : "+d" (d0_), "+d" (d1_), "+d" (d2_), "+d" (d3_)	\
		      : "i" (code) : "cc");				\
    (hi) = d0_;								\
    (lo) = d1_;								\
  } while (0)

/* Function CODE on d0 (and d1); the result, d0, in A.  */
#define MATH_S(code, a, b)						\
  do {									\
    register unsigned long d0_ __asm__ ("d0") = (a);			\
    register unsigned long d1_ __asm__ ("d1") = (b);			\
    __asm__ volatile ("trap #15\n\t.word %c2"				\
		      : "+d" (d0_), "+d" (d1_)				\
		      : "i" (code) : "cc");				\
    (a) = d0_;								\
  } while (0)

/* Compares, with function CODE, d0:d1 with d2:d3 (doubles) or d0 with d1
   (floats); -1, 0 or 1.  */
static int
compare (int code, unsigned long a0, unsigned long a1, unsigned long b0,
	 unsigned long b1)
{
  register unsigned long d0_ __asm__ ("d0") = a0;
  register unsigned long d1_ __asm__ ("d1") = a1;
  register unsigned long d2_ __asm__ ("d2") = b0;
  register unsigned long d3_ __asm__ ("d3") = b1;
  char lt, eq;

  if (code == T_DCMP)
    __asm__ volatile ("trap #15\n\t.word %c6\n\tslt %4\n\tseq %5"
		      : "+d" (d0_), "+d" (d1_), "+d" (d2_), "+d" (d3_),
			"=d" (lt), "=d" (eq)
		      : "i" (T_DCMP) : "cc");
  else
    __asm__ volatile ("trap #15\n\t.word %c6\n\tslt %4\n\tseq %5"
		      : "+d" (d0_), "+d" (d1_), "+d" (d2_), "+d" (d3_),
			"=d" (lt), "=d" (eq)
		      : "i" (T_FCMP) : "cc");
  return lt ? -1 : eq ? 0 : 1;
}

static int
dnan (union dbl x)
{
  return (x.w[0] & 0x7FF00000) == 0x7FF00000
	 && ((x.w[0] & 0xFFFFF) | x.w[1]);
}

static int
fnan (union flt x)
{
  return (x.w & 0x7F800000) == 0x7F800000 && (x.w & 0x7FFFFF);
}

/* Doubles.  */

#define DBINARY(name, code)						\
  double STACKCALL							\
  name (double a, double b)						\
  {									\
    union dbl x, y;							\
    x.d = a;								\
    y.d = b;								\
    MATH_D (code, x.w[0], x.w[1], y.w[0], y.w[1]);			\
    return x.d;								\
  }

DBINARY (__adddf3, T_DADD)
DBINARY (__subdf3, T_DSUB)
DBINARY (__muldf3, T_DMUL)
DBINARY (__divdf3, T_DDIV)

/* Comparisons: libgcc's results, with what an unordered pair gives.  */
#define DCOMPARE(name, unordered)					\
  int STACKCALL								\
  name (double a, double b)						\
  {									\
    union dbl x, y;							\
    x.d = a;								\
    y.d = b;								\
    if (dnan (x) || dnan (y))						\
      return unordered;							\
    return compare (T_DCMP, x.w[0], x.w[1], y.w[0], y.w[1]);		\
  }

DCOMPARE (__cmpdf2, 1)
DCOMPARE (__eqdf2, 1)
DCOMPARE (__nedf2, 1)
DCOMPARE (__ltdf2, 1)
DCOMPARE (__ledf2, 1)
DCOMPARE (__gtdf2, -1)
DCOMPARE (__gedf2, -1)

int STACKCALL
__unorddf2 (double a, double b)
{
  union dbl x, y;

  x.d = a;
  y.d = b;
  return dnan (x) || dnan (y);
}

double STACKCALL
__floatsidf (long l)
{
  union dbl x;

  x.w[0] = l;
  x.w[1] = 0;
  MATH_D (T_LTOD, x.w[0], x.w[1], 0, 0);
  return x.d;
}

double STACKCALL
__floatunsidf (unsigned long u)
{
  union dbl x;

  x.w[0] = u;
  x.w[1] = 0;
  MATH_D (T_UTOD, x.w[0], x.w[1], 0, 0);
  return x.d;
}

long STACKCALL
__fixdfsi (double a)
{
  union dbl x;

  x.d = a;
  MATH_D (T_DTOL, x.w[0], x.w[1], 0, 0);
  return x.w[0];
}

unsigned long STACKCALL
__fixunsdfsi (double a)
{
  union dbl x;

  x.d = a;
  MATH_D (T_DTOU, x.w[0], x.w[1], 0, 0);
  return x.w[0];
}

float STACKCALL
__truncdfsf2 (double a)
{
  union dbl x;
  union flt r;

  x.d = a;
  MATH_D (T_DTOF, x.w[0], x.w[1], 0, 0);
  r.w = x.w[0];
  return r.f;
}

/* Floats.  */

#define FBINARY(name, code)						\
  float STACKCALL							\
  name (float a, float b)						\
  {									\
    union flt x, y;							\
    x.f = a;								\
    y.f = b;								\
    MATH_S (code, x.w, y.w);						\
    return x.f;								\
  }

FBINARY (__addsf3, T_FADD)
FBINARY (__subsf3, T_FSUB)
FBINARY (__mulsf3, T_FMUL)
FBINARY (__divsf3, T_FDIV)

#define FCOMPARE(name, unordered)					\
  int STACKCALL								\
  name (float a, float b)						\
  {									\
    union flt x, y;							\
    x.f = a;								\
    y.f = b;								\
    if (fnan (x) || fnan (y))						\
      return unordered;							\
    return compare (T_FCMP, x.w, y.w, 0, 0);				\
  }

FCOMPARE (__cmpsf2, 1)
FCOMPARE (__eqsf2, 1)
FCOMPARE (__nesf2, 1)
FCOMPARE (__ltsf2, 1)
FCOMPARE (__lesf2, 1)
FCOMPARE (__gtsf2, -1)
FCOMPARE (__gesf2, -1)

int STACKCALL
__unordsf2 (float a, float b)
{
  union flt x, y;

  x.f = a;
  y.f = b;
  return fnan (x) || fnan (y);
}

#define FUNARY(name, type, code, ret)					\
  ret STACKCALL								\
  name (type a)								\
  {									\
    union { type v; unsigned long w; } x;				\
    union { ret v; unsigned long w; } r;				\
    x.v = a;								\
    MATH_S (code, x.w, 0);						\
    r.w = x.w;								\
    return r.v;								\
  }

FUNARY (__floatsisf, long, T_LTOF, float)
FUNARY (__floatunsisf, unsigned long, T_UTOF, float)
FUNARY (__fixsfsi, float, T_FTOL, long)
FUNARY (__fixunssfsi, float, T_FTOU, unsigned long)

double STACKCALL
__extendsfdf2 (float a)
{
  union flt x;
  union dbl r;

  x.f = a;
  r.w[0] = x.w;
  r.w[1] = 0;
  MATH_D (T_FTOD, r.w[0], r.w[1], 0, 0);
  return r.d;
}

/* Links the math trap handler before main (constructors run first); a
   program that can't have it stops, as Microware's cstart.r does.  Global:
   -mos9math links it (-u) also into programs that use only the inline
   functions of machine/os9math.h, which call nothing here.  */
void __attribute__ ((constructor (101)))
__os9_math_link (void)
{
  static const char name[] = "math";
  static const char message[] = "**** Can't install math trap handler ****\r";
  register unsigned long d0_ __asm__ ("d0") = T_MATH;
  register unsigned long d1_ __asm__ ("d1") = 0;
  register const char *a0_ __asm__ ("a0") = name;
  char failed;

  __asm__ volatile ("trap #0\n\t.word %c4\n\tscs %3"
		    : "+d" (d0_), "+d" (d1_), "+a" (a0_), "=d" (failed)
		    : "i" (F_TLINK)
		    : "a1", "a2", "cc", "memory");
  if (failed)
    {
      struct os9_regs r = { 0 };
      unsigned long error = d1_ & 0xFFFF;

      r.d0 = 2;
      r.d1 = sizeof message - 1;
      r.a0 = (void *) message;
      OS9_CALL (I_WRITLN, &r);
      r.d1 = error;
      OS9_CALL (F_EXIT, &r);
    }
}
