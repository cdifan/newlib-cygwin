/* $NetBSD: cargl.c,v 1.1 2014/10/10 00:48:18 christos Exp $ */

/*
 * Public domain.
 */

#include <complex.h>
#include <math.h>
#include <newlib.h>

/* The long double functions this needs (cosl, expl, logl, ...) exist only
   where long double is double; math.h declares them only then.  */
#if defined (_LDBL_EQ_DBL) || defined (__CYGWIN__)

long double
cargl(long double complex z)
{     
       #ifdef _LDBL_EQ_DBL
         return carg (z);
       #else
         return atan2l (cimagl (z), creall (z));
       #endif
}

#endif /* _LDBL_EQ_DBL || __CYGWIN__ */
