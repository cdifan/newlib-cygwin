/* os9-proc.c -- newlib's process, memory and time system calls on
   OS-9/68K.  */

#include <errno.h>
#include <stddef.h>
#include <sys/time.h>
#include <sys/times.h>
#include <unistd.h>
#include "os9-sys.h"

void
_exit (int status)
{
  struct os9_regs r = { 0 };

  r.d1 = status & 0xFFFF;
  for (;;)
    OS9_CALL (F_EXIT, &r);
}

int
_getpid (void)
{
  struct os9_regs r = { 0 };

  OS9_CALL (F_ID, &r);
  return r.d0 & 0xFFFF;
}

/* Sends signal SIG to process PID.  Without a signal handler (newlib's
   signal and raise don't install one with OS-9), the process dies with
   the signal number as its exit status.  */
int
_kill (int pid, int sig)
{
  struct os9_regs r = { 0 };
  int error;

  r.d0 = pid;
  r.d1 = sig;
  error = OS9_CALL (F_SEND, &r);
  return error ? __os9_error (error) : 0;
}

/* The heap: the memory between the program's data and its stack (set by
   the startup), which newlib's malloc needs contiguous and growing
   upwards.  Not F$SRqMem: OS-9 hands out its blocks from the top of
   memory down.  The module reserves __os9_heap bytes (os9.ld); the
   shell's #size modifier adds more.  It doesn't shrink (malloc copes).  */
char *__os9_brk, *__os9_brk_end;

void *
_sbrk (ptrdiff_t increment)
{
  char *p = __os9_brk;

  if (increment < 0 || increment > __os9_brk_end - __os9_brk)
    {
      errno = ENOMEM;
      return (void *) -1;
    }
  __os9_brk += increment;
  return p;
}

/* Days from 1970-01-01 to the date Y-M-D (proleptic Gregorian).  */
static long
days_from_civil (int y, int m, int d)
{
  long era;
  int yoe, doy, doe;

  y -= m <= 2;
  era = (y >= 0 ? y : y - 399) / 400;
  yoe = y - era * 400;
  doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + doe - 719468;
}

/* The system clock is local time; OS-9 keeps no time zone, so it's
   returned as UTC.  To the second.  */
int
_gettimeofday (struct timeval *tv, void *tz)
{
  struct os9_regs r = { 0 };
  int error;
  unsigned long t, d;

  (void) tz;
  r.d0 = 0;			/* Gregorian: 00hhmmss, yyyymmdd */
  error = OS9_CALL (F_TIME, &r);
  if (error)
    return __os9_error (error);
  t = r.d0;
  d = r.d1;
  tv->tv_sec = days_from_civil (d >> 16, (d >> 8) & 0xFF, d & 0xFF) * 86400
	       + ((t >> 16) & 0xFF) * 3600 + ((t >> 8) & 0xFF) * 60
	       + (t & 0xFF);
  tv->tv_usec = 0;
  return 0;
}

/* Process times aren't available.  */
clock_t
_times (struct tms *buf)
{
  (void) buf;
  errno = ENOSYS;
  return (clock_t) -1;
}
