#ifndef _ASM_MEMORY_H
#define _ASM_MEMORY_H

extern inline void copy_page(unsigned long from, unsigned long to)
{
	unsigned long * s = (unsigned long *) from;
	unsigned long * d = (unsigned long *) to;
	int i;
	for (i = 0; i < 1024; i++)
		d[i] = s[i];
}

#ifndef memcpy
#define memcpy(dest,src,n) ({ \
	void * _res = (dest); \
	__builtin_memcpy(_res, (src), (n)); \
	_res; \
})
#endif

#endif
