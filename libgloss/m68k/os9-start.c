/* os9-start.c -- the C part of the OS-9/68K startup (see os9-crt0.S).

   Sets up the heap, splits the parameter string into arguments, runs the
   constructors and calls main; its result goes to exit.  argv[0] is the
   module's name.  Arguments are separated by spaces or tabs; single or
   double quotes group words with their spaces (from a program's F$Fork:
   the OS-9 shell takes double quotes off its command line itself, and
   rejects single ones).  The OS-9 shell expands no wildcards, and neither
   does this.  */

#include <stdlib.h>

extern int main (int, char **, char **);
extern void __libc_init_array (void);
extern void __libc_fini_array (void);

extern char **environ;		/* newlib's, empty */
extern char *__os9_brk, *__os9_brk_end;	/* os9-proc.c */

/* Splits the LENGTH characters at P in place, storing pointers to the
   arguments in ARGV (unless null); returns their number.  */
static int
split (char *p, long length, char **argv)
{
  char *end = p + length;
  int argc = 0;

  while (p < end)
    {
      char *out;

      while (p < end && (*p == ' ' || *p == '\t'))
	p++;
      if (p == end)
	break;
      out = p;
      if (argv)
	argv[argc] = out;
      argc++;
      while (p < end && *p != ' ' && *p != '\t')
	{
	  if (*p == '"' || *p == '\'')
	    {
	      char quote = *p++;
	      while (p < end && *p != quote)
		{
		  if (argv)
		    *out++ = *p;
		  p++;
		}
	      if (p < end)
		p++;
	    }
	  else if (argv)
	    *out++ = *p++;
	  else
	    p++;
	}
      if (argv)
	{
	  /* The terminator overwrites the separator, or the character
	     after the last argument; the string has one to spare.  */
	  if (p < end)
	    p++;
	  *out = '\0';
	}
    }
  return argc;
}

void __attribute__ ((stackcall, noreturn))
__os9_start (char *module, char *params, long size, char *heap,
	     char *heap_end)
{
  char **argv;
  int argc;

  __os9_brk = heap;
  __os9_brk_end = heap_end;

  /* The parameter string ends with a carriage return (and possibly a
     terminator after it): that ends it.  */
  {
    long n = 0;
    while (n < size && params[n] != '\r' && params[n] != '\0')
      n++;
    size = n;
  }
  argc = split (params, size, NULL) + 1;
  argv = __builtin_alloca ((argc + 1) * sizeof *argv);
  argv[0] = module + *(long *) (module + 12);	/* M$Name */
  split (params, size, argv + 1);
  argv[argc] = NULL;

  atexit (__libc_fini_array);
  __libc_init_array ();
  exit (main (argc, argv, environ));
}
