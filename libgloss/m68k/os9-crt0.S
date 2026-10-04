/* os9-crt0.S -- startup for OS-9/68K programs (modules made by elf2mod).

   OS-9 enters a program with the module at a3, the parameter string (the
   command line's arguments, ending with a carriage return) at a5 and its
   size in d5, a6 pointing 0x8000 bytes into the data area, which OS-9 has
   cleared and filled with the module's initialized data, and the stack at
   its top; memory added with the shell's #size modifier lies between the
   data and the stack.  The heap is the memory from __heap_start (os9.ld)
   to __os9_stack bytes below the top of the stack, contiguous, as newlib's
   malloc needs.  The rest is done in C (__os9_start in os9-start.c),
   called with the stack convention, as in either multilib.

   The module header comes from the __os9_* symbols, which os9.ld provides
   with defaults that a program can override.  */

	.text
	.globl	_start
	.type	_start, @function
_start:
	move.l	%sp,%d0
	sub.l	#__os9_stack,%d0
	move.l	%d0,-(%sp)		/* the end of the heap */
	move.l	#__heap_start,%d0
	add.l	%a6,%d0
	move.l	%d0,-(%sp)		/* the heap */
	move.l	%d5,-(%sp)		/* parameter size */
	move.l	%a5,-(%sp)		/* parameters */
	move.l	%a3,-(%sp)		/* module */
	sub.l	%a5,%a5			/* the end of the frame chain */
	bsr.w	__os9_start		/* doesn't return; bsr.w, which elf2mod's
				   jump table redirects when it's far */
	.size	_start, . - _start

/* newlib's __libc_init_array and __libc_fini_array call _init and _fini;
   os9.ld puts constructors in .init_array instead.  */
	.globl	_init, _fini
	.type	_init, @function
	.type	_fini, @function
_init:
_fini:
	rts
	.size	_init, . - _init
