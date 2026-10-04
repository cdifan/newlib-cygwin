/* os9-sys.h -- OS-9/68K system calls for the OS-9 runtime (libos9).

   A system call is trap #0 followed by its function code.  Arguments and
   results are in registers; an error sets the carry flag and puts its
   code in d1.w.  The other registers are preserved.  */

#ifndef OS9_SYS_H
#define OS9_SYS_H

/* Function codes.  */
#define F_EXIT		0x06
#define F_SEND		0x08
#define F_ID		0x0C
#define F_TIME		0x15
#define I_CREATE	0x83
#define I_OPEN		0x84
#define I_DELETE	0x87
#define I_SEEK		0x88
#define I_READ		0x89
#define I_WRITE		0x8A
#define I_READLN	0x8B
#define I_WRITLN	0x8C
#define I_GETSTT	0x8D
#define I_SETSTT	0x8E
#define I_CLOSE		0x8F

/* I$GetStt/I$SetStt codes.  */
#define SS_OPT		0x00	/* the path's options (a0: 128 bytes) */
#define SS_SIZE		0x02	/* file size (d2) */
#define SS_POS		0x05	/* file position (d2) */

/* Device types, the first byte of the path options.  */
#define DT_SCF		0	/* sequential character file: terminals */
#define DT_RBF		1	/* random block file: disks */

/* Error codes.  */
#define E_PERMIT	0xA4
#define E_BPNUM		0xC9
#define E_BMODE		0xCB
#define E_MEMFUL	0xCF
#define E_UNKSVC	0xD0
#define E_EOF		0xD3
#define E_FNA		0xD6
#define E_BPNAM		0xD7
#define E_PNNF		0xD8
#define E_CEF		0xDA
#define E_FULL		0xF8
#define E_SHARE		0xFD

/* The registers of a system call, in and out.  */
struct os9_regs
{
  unsigned long d0, d1, d2, d3;
  void *a0, *a2;
};

/* Makes system call CODE (a constant) with the registers in *R, puts the
   results back and evaluates to the error code, or 0.  */
#define OS9_CALL(code, r)						\
  ({									\
    register unsigned long d0_ __asm__ ("d0") = (r)->d0;		\
    register unsigned long d1_ __asm__ ("d1") = (r)->d1;		\
    register unsigned long d2_ __asm__ ("d2") = (r)->d2;		\
    register unsigned long d3_ __asm__ ("d3") = (r)->d3;		\
    register void *a0_ __asm__ ("a0") = (r)->a0;			\
    register void *a2_ __asm__ ("a2") = (r)->a2;			\
    char failed_;							\
    __asm__ volatile ("trap #0\n\t.word %c7\n\tscs %6"			\
		      : "+d" (d0_), "+d" (d1_), "+d" (d2_), "+d" (d3_),	\
			"+a" (a0_), "+a" (a2_), "=d" (failed_)		\
		      : "i" (code)					\
		      : "cc", "memory");				\
    (r)->d0 = d0_;							\
    (r)->d1 = d1_;							\
    (r)->d2 = d2_;							\
    (r)->d3 = d3_;							\
    (r)->a0 = a0_;							\
    (r)->a2 = a2_;							\
    failed_ ? (int) (d1_ & 0xFFFF) : 0;					\
  })

/* Sets errno from an OS-9 error code; returns -1.  */
extern int __os9_error (int);

/* The device type of path FD (DT_SCF, DT_RBF, ...), or -1 with errno set
   when FD isn't open.  Cached; __os9_forget clears it when FD closes.  */
extern int __os9_devtype (int);
extern void __os9_forget (int);

#endif
