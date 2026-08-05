#ifndef _STDARG_H
#define _STDARG_H

/* RISC-V port: use the compiler's native varargs (register passing) */

typedef __builtin_va_list va_list;

#define va_start(ap, last) __builtin_va_start((ap), (last))
#define va_arg(ap, type)   __builtin_va_arg((ap), type)
#define va_end(ap)         __builtin_va_end((ap))
#define va_copy(d, s)      __builtin_va_copy((d), (s))

#endif /* _STDARG_H */
