/* os9-io.c -- newlib's I/O system calls on OS-9/68K.

   Paths are OS-9 path numbers; 0, 1 and 2 are the standard paths.  Line
   ends: OS-9 ends a line with a carriage return, newlib with a line feed.
   On terminals (SCF paths) a line feed written becomes a carriage return,
   written with I$WritLn, so the terminal driver ends the line (and adds
   its line feed); reading uses I$ReadLn, with the line editing that brings,
   and the carriage return ending a line comes back as a line feed.  Files
   (RBF paths and others) are read and written unchanged.  */

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "os9-sys.h"

int _close (int);
off_t _lseek (int, off_t, int);

int
__os9_error (int code)
{
  switch (code)
    {
    case E_PNNF:	errno = ENOENT; break;
    case E_FNA:
    case E_PERMIT:	errno = EACCES; break;
    case E_CEF:		errno = EEXIST; break;
    case E_BPNUM:	errno = EBADF; break;
    case E_BMODE:	errno = EINVAL; break;
    case E_BPNAM:	errno = ENAMETOOLONG; break;
    case E_MEMFUL:	errno = ENOMEM; break;
    case E_FULL:	errno = ENOSPC; break;
    case E_SHARE:	errno = EBUSY; break;
    case E_UNKSVC:	errno = ENOSYS; break;
    default:		errno = EIO; break;
    }
  return -1;
}

/* Device types of the first paths, plus one: 0 is unknown.  */
#define CACHED 32
static unsigned char devtypes[CACHED];

int
__os9_devtype (int fd)
{
  unsigned char options[128];
  struct os9_regs r = { 0 };
  int error;

  if (fd >= 0 && fd < CACHED && devtypes[fd])
    return devtypes[fd] - 1;
  r.d0 = fd;
  r.d1 = SS_OPT;
  r.a0 = options;
  error = OS9_CALL (I_GETSTT, &r);
  if (error)
    return __os9_error (error);
  if (fd >= 0 && fd < CACHED)
    devtypes[fd] = options[0] + 1;
  return options[0];
}

void
__os9_forget (int fd)
{
  if (fd >= 0 && fd < CACHED)
    devtypes[fd] = 0;
}

/* Writes N bytes at BUF to FD with system call CODE; returns the number
   written, or -1.  */
static int
write_raw (int fd, const char *buf, int n, int code)
{
  struct os9_regs r = { 0 };
  int error;

  r.d0 = fd;
  r.d1 = n;
  r.a0 = (void *) buf;
  if (code == I_WRITLN)
    error = OS9_CALL (I_WRITLN, &r);
  else
    error = OS9_CALL (I_WRITE, &r);
  if (error)
    return __os9_error (error);
  return r.d1;
}

_READ_WRITE_RETURN_TYPE
_write (int fd, const void *data, size_t n)
{
  const char *buf = data;
  size_t done = 0;
  int type = __os9_devtype (fd);

  if (type < 0)
    return -1;
  if (type != DT_SCF)
    return write_raw (fd, buf, n, I_WRITE);

  /* A terminal: line by line, each line feed as a carriage return.  */
  while (done < n)
    {
      char line[128];
      size_t length = 0;
      int written, end = 0;

      while (done + length < n && length < sizeof line)
	{
	  char c = buf[done + length];
	  if (c == '\n')
	    c = '\r';
	  line[length++] = c;
	  if (c == '\r')
	    {
	      end = 1;
	      break;
	    }
	}
      written = write_raw (fd, line, length, end ? I_WRITLN : I_WRITE);
      if (written < 0)
	return done ? (int) done : -1;
      done += written;
      if ((size_t) written < length)
	break;
    }
  return done;
}

_READ_WRITE_RETURN_TYPE
_read (int fd, void *data, size_t n)
{
  struct os9_regs r = { 0 };
  int type = __os9_devtype (fd);
  int error;

  if (type < 0)
    return -1;
  r.d0 = fd;
  r.d1 = n;
  r.a0 = data;
  if (type == DT_SCF)
    error = OS9_CALL (I_READLN, &r);
  else
    error = OS9_CALL (I_READ, &r);
  if (error == E_EOF)
    return 0;
  if (error)
    return __os9_error (error);
  if (type == DT_SCF && r.d1 > 0 && ((char *) data)[r.d1 - 1] == '\r')
    ((char *) data)[r.d1 - 1] = '\n';
  return r.d1;
}

/* OS-9 file attributes from Unix permissions: owner read, write, execute
   (bits 0-2), public read, write, execute (bits 3-5).  */
static int
attributes (mode_t mode)
{
  return ((mode & S_IRUSR) ? 0x01 : 0) | ((mode & S_IWUSR) ? 0x02 : 0)
	 | ((mode & S_IXUSR) ? 0x04 : 0) | ((mode & S_IROTH) ? 0x08 : 0)
	 | ((mode & S_IWOTH) ? 0x10 : 0) | ((mode & S_IXOTH) ? 0x20 : 0);
}

static int
set_size (int fd, unsigned long size)
{
  struct os9_regs r = { 0 };
  int error;

  r.d0 = fd;
  r.d1 = SS_SIZE;
  r.d2 = size;
  error = OS9_CALL (I_SETSTT, &r);
  return error ? __os9_error (error) : 0;
}

int
_open (const char *name, int flags, ...)
{
  struct os9_regs r = { 0 };
  int access = (flags & O_ACCMODE) == O_RDONLY ? 1
	       : (flags & O_ACCMODE) == O_WRONLY ? 2 : 3;
  int error = E_PNNF, fd;

  if (flags & O_CREAT)
    {
      mode_t mode;
      __builtin_va_list ap;

      __builtin_va_start (ap, flags);
      mode = __builtin_va_arg (ap, int);
      __builtin_va_end (ap);
      r.d0 = access;
      r.d1 = attributes (mode);
      r.a0 = (void *) name;
      error = OS9_CALL (I_CREATE, &r);
      if (error && (error != E_CEF || (flags & O_EXCL)))
	return __os9_error (error);
    }
  if (error)
    {
      memset (&r, 0, sizeof r);
      r.d0 = access;
      r.a0 = (void *) name;
      error = OS9_CALL (I_OPEN, &r);
      if (error)
	return __os9_error (error);
      fd = r.d0 & 0xFFFF;
      __os9_forget (fd);
      if ((flags & O_TRUNC) && __os9_devtype (fd) == DT_RBF
	  && set_size (fd, 0) < 0)
	{
	  _close (fd);
	  return -1;
	}
    }
  else
    {
      fd = r.d0 & 0xFFFF;
      __os9_forget (fd);
    }
  if ((flags & O_APPEND) && __os9_devtype (fd) == DT_RBF)
    _lseek (fd, 0, SEEK_END);
  return fd;
}

int
_close (int fd)
{
  struct os9_regs r = { 0 };
  int error;

  __os9_forget (fd);
  r.d0 = fd;
  error = OS9_CALL (I_CLOSE, &r);
  return error ? __os9_error (error) : 0;
}

/* The status call CODE (SS_SIZE or SS_POS) on FD: its result, or -1.  */
static long
get_status (int fd, int code)
{
  struct os9_regs r = { 0 };
  int error;

  r.d0 = fd;
  r.d1 = code;
  error = OS9_CALL (I_GETSTT, &r);
  return error ? __os9_error (error) : (long) r.d2;
}

off_t
_lseek (int fd, off_t offset, int whence)
{
  struct os9_regs r = { 0 };
  long base = 0;
  int error;

  if (whence == SEEK_CUR)
    base = get_status (fd, SS_POS);
  else if (whence == SEEK_END)
    base = get_status (fd, SS_SIZE);
  else if (whence != SEEK_SET)
    {
      errno = EINVAL;
      return -1;
    }
  if (base < 0)
    return -1;
  r.d0 = fd;
  r.d1 = base + offset;
  error = OS9_CALL (I_SEEK, &r);
  return error ? __os9_error (error) : (off_t) (base + offset);
}

int
_fstat (int fd, struct stat *st)
{
  int type = __os9_devtype (fd);

  if (type < 0)
    return -1;
  memset (st, 0, sizeof *st);
  if (type == DT_RBF)
    {
      long size = get_status (fd, SS_SIZE);
      st->st_mode = S_IFREG | 0666;
      st->st_size = size < 0 ? 0 : size;
    }
  else
    st->st_mode = S_IFCHR | 0666;
  st->st_blksize = type == DT_RBF ? 256 : 0;
  return 0;
}

int
_stat (const char *name, struct stat *st)
{
  int fd = _open (name, O_RDONLY), result;

  if (fd < 0)
    return -1;
  result = _fstat (fd, st);
  _close (fd);
  return result;
}

int
_isatty (int fd)
{
  int type = __os9_devtype (fd);

  if (type == DT_SCF)
    return 1;
  if (type >= 0)
    errno = ENOTTY;
  return 0;
}

int
_unlink (const char *name)
{
  struct os9_regs r = { 0 };
  int error;

  r.d0 = 2;			/* write access */
  r.a0 = (void *) name;
  error = OS9_CALL (I_DELETE, &r);
  return error ? __os9_error (error) : 0;
}
