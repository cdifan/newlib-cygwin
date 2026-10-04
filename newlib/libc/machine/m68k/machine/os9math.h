/* machine/os9math.h -- with m68k-elfos9-gcc -mos9math (__OS9MATH__),
   the math functions OS-9's math module has, inline: a trap #15 each, as
   for the arithmetic, which libos9math routes to the same module.  The
   argument and result are in d0:d1, the precision in d2:d3 (pow: the
   exponent in d2:d3, the precision in d4:d5; OS-9 Technical Manual,
   chapter 6).  The precision is 1E-14, the finest the manual allows.

   As in Microware C, errors (sqrt(-1), log(0), exp(1000), ...) raise
   TRAPV, which ends the program with error 107 unless it handles that
   exception (F$STrap); errno isn't set.  The results are the math
   module's, which may differ from libm's in the last digits.  libm's
   versions stay: for their addresses, and for code built without
   -mos9math.  -mos9math also links libos9math's __os9_math_link, which
   links the math module before main runs.

   The manual says nothing about other registers; on a CD-i 605 tan and
   acos change d2 and d3, so the precision registers count as
   destroyed.  */

#ifndef _MACHINE_OS9MATH_H_
#define _MACHINE_OS9MATH_H_

#define __OS9MATH_INLINE \
  extern __inline__ __attribute__ ((__gnu_inline__, __always_inline__))

/* 1E-14, as the manual gives it: 3d06849b 86a12b9c.  */
#define __OS9MATH_PRECISION 1E-14

#define __OS9MATH_1(__name, __code)					\
  __OS9MATH_INLINE double						\
  __name (double __x)							\
  {									\
    register double __r __asm__ ("d0") = __x;				\
    register double __p __asm__ ("d2") = __OS9MATH_PRECISION;		\
    __asm__ ("trap #15\n\t.word " #__code				\
	     : "+d" (__r), "+d" (__p) : : "cc");			\
    return __r;								\
  }

__OS9MATH_1 (sin, 46)
__OS9MATH_1 (cos, 47)
__OS9MATH_1 (tan, 48)
__OS9MATH_1 (asin, 49)
__OS9MATH_1 (acos, 50)
__OS9MATH_1 (atan, 51)
__OS9MATH_1 (log, 52)
__OS9MATH_1 (log10, 53)
__OS9MATH_1 (sqrt, 54)
__OS9MATH_1 (exp, 55)

__OS9MATH_INLINE double
pow (double __x, double __y)
{
  register double __r __asm__ ("d0") = __x;
  register double __e __asm__ ("d2") = __y;
  register double __p __asm__ ("d4") = __OS9MATH_PRECISION;
  __asm__ ("trap #15\n\t.word 56"
	   : "+d" (__r), "+d" (__e), "+d" (__p) : : "cc");
  return __r;
}

#undef __OS9MATH_1
#undef __OS9MATH_INLINE
#undef __OS9MATH_PRECISION

#endif /* _MACHINE_OS9MATH_H_ */
