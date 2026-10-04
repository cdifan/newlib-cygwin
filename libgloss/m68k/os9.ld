/* os9.ld -- linker script for OS-9/68K programs, made into modules by
   elf2mod (link with -q to keep the relocations it needs; os9.specs does).

   Code, with its constants, is placed above the data area (elf2mod accepts
   any code address).  Data is addressed relative to a6 (-ma6rel), which
   OS-9 points 0x8000 bytes into the data area: .bss and .data from
   -0x8000, then remote data beyond the 64 KB that a6 reaches.  With .bss
   first, .data and .remote.data are adjacent, so elf2mod stores no zeros
   for .bss in the module.  With more than 32 KB of data, sections cross
   from 0xFFFFFFFF to 0: link with --no-check-sections (os9.specs does).

   The module header comes from the __os9_* symbols; a program can define
   its own (".set __os9_stack, 0x4000" with .globl in assembly) or pass
   options to elf2mod.  */

OUTPUT_FORMAT("elf32-m68k")
OUTPUT_ARCH(m68k)
ENTRY(_start)

/* A reentrant program module in object code, revision 1, edition 1, with
   8 KB of stack.  */
PROVIDE (__os9_tylan = 0x0101);
PROVIDE (__os9_attrev = 0x8001);
PROVIDE (__os9_edition = 1);
PROVIDE (__os9_stack = 0x2000);
PROVIDE (__os9_entry = _start);

/* The heap, for malloc: at least 16 KB, after the data; it extends to the
   stack, so the shell's #size modifier adds to it (os9-crt0.S).  */
PROVIDE (__os9_heap = 0x4000);

/* No jump table unless asked for (--defsym __jmptbl_size=N).  */
PROVIDE (__jmptbl_size = 0);

SECTIONS {
    . = 0x400000;
    .text : {
        /* One statement, so each object's constants follow its code:
           code reaches them PC-relative (+-32 KB), also in programs with
           more code than that (newlib's stdio).  */
        *(.text .text.* .rodata .rodata.*)
        . = ALIGN(2);
    }

    . = -0x8000;
    .bss (NOLOAD) : {
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(2);
    }
    .data : {
        *(.data .data.*)
        . = ALIGN(4);
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);
        PROVIDE_HIDDEN (__init_array_start = .);
        /* GCC puts constructors (with a priority: .ctors.N) in .ctors
           here; newlib runs them from __libc_init_array, in this order.  */
        KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
        KEEP (*(.init_array .ctors))
        PROVIDE_HIDDEN (__init_array_end = .);
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
        KEEP (*(.fini_array .dtors))
        PROVIDE_HIDDEN (__fini_array_end = .);
        . = ALIGN(2);
        /* elf2mod's jump table for far calls (programs with more than
           32 KB of code): --defsym __jmptbl_size=N */
        _jmptbl = .;
        . += __jmptbl_size;
        _ejmptbl = .;
    }
    .remote.data : {
        *(.remote.data .remote.data.*)
        . = ALIGN(2);
    }
    .remote.bss (NOLOAD) : {
        *(.remote.bss .remote.bss.*)
        . = ALIGN(4);
        __heap_start = .;
        . += __os9_heap;
    }
    end = .;
    _end = .;
}
