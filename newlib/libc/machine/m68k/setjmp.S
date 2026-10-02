#include "m68kasm.h"

/* Where the arguments are: on the stack, or with -mos9call (the OS-9
   calling convention of the m68k-elfos9 GCC) the first in d0 and the
   second in d1.  */
#ifdef __OS9CALL__
#define GET_ENV moveal d0,a0
#define GET_VAL movel d1,d0
/* setjmp preserves a0, as the convention requires (it returns d0).  */
#define SETJMP_ENV exg d0,a0
#define SETJMP_RET exg d0,a0; clrl d0
#else
#define GET_ENV moveal sp@(4),a0
#define GET_VAL movel sp@(8),d0
#define SETJMP_ENV moveal sp@(4),a0
#define SETJMP_RET clrl d0
#endif

.global SYM (setjmp)
.global SYM (longjmp)

SYM (setjmp):
	SETJMP_ENV
	movel sp@(0),a0@(12)
	movel sp,a0@(8)
	moveml d2-d7/a2-a6,a0@(20)
	SETJMP_RET
	rts

SYM (longjmp):
	GET_ENV
	GET_VAL
	bne 1f
	movel &1,d0
1:
	moveml a0@(20),d2-d7/a2-a6
	moveal a0@(8),sp
	movel a0@(12),sp@
	rts

#ifdef M68881
.global SYM (setjmp_68881)
.global SYM (longjmp_68881)

SYM (setjmp_68881):
	SETJMP_ENV
	movel sp@(0),a0@(12)
	movel sp,a0@(8)
	moveml d2-d7/a2-a6,a0@(20)
	fmovemx fp2-fp7,a0@(64)
	SETJMP_RET
	rts

SYM (longjmp_68881):
	GET_ENV
	fmovemx a0@(64),fp2-fp7
	GET_VAL
	bne 1f
	movel &1,d0
1:
	moveml a0@(20),d2-d7/a2-a6
	moveal a0@(8),sp
	movel a0@(12),sp@
	rts
#endif
