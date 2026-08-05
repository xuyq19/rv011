#ifndef _STRING_H_
#define _STRING_H_

#ifndef NULL
#define NULL ((void *) 0)
#endif

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int size_t;
#endif

extern char * strerror(int errno);

/* Portable C string functions (RISC-V port) */

extern inline char * strcpy(char * dest,const char *src)
{
	char * d = dest;
	while ((*d++ = *src++) != 0) ;
	return dest;
}

extern inline char * strncpy(char * dest,const char *src,int count)
{
	char * d = dest;
	while (count-- > 0 && (*d++ = *src++) != 0) ;
	while (count-- > 0)
		*d++ = 0;
	return dest;
}

extern inline char * strcat(char * dest,const char * src)
{
	char * d = dest;
	while (*d) d++;
	while ((*d++ = *src++) != 0) ;
	return dest;
}

extern inline char * strncat(char * dest,const char * src,int count)
{
	char * d = dest;
	while (*d) d++;
	while (count-- > 0 && (*d = *src++) != 0)
		d++;
	*d = 0;
	return dest;
}

extern inline int strcmp(const char * cs,const char * ct)
{
	while (*cs == *ct && *cs) {
		cs++;
		ct++;
	}
	return (int)(unsigned char)*cs - (int)(unsigned char)*ct;
}

extern inline int strncmp(const char * cs,const char * ct,int count)
{
	while (count-- > 0 && *cs == *ct && *cs) {
		cs++;
		ct++;
	}
	if (count < 0)
		return 0;
	return (int)(unsigned char)*cs - (int)(unsigned char)*ct;
}

extern inline char * strchr(const char * s,char c)
{
	do {
		if (*s == c)
			return (char *) s;
	} while (*s++);
	return NULL;
}

extern inline char * strrchr(const char * s,char c)
{
	char * found = NULL;
	do {
		if (*s == c)
			found = (char *) s;
	} while (*s++);
	return found;
}

extern inline int strspn(const char * cs, const char * ct)
{
	const char * s;
	for (s = cs; *s; s++) {
		const char * t;
		for (t = ct; *t; t++)
			if (*t == *s)
				break;
		if (!*t)
			break;
	}
	return s - cs;
}

extern inline int strcspn(const char * cs, const char * ct)
{
	const char * s;
	for (s = cs; *s; s++) {
		const char * t;
		for (t = ct; *t; t++)
			if (*t == *s)
				break;
		if (*t)
			break;
	}
	return s - cs;
}

extern inline char * strpbrk(const char * cs,const char * ct)
{
	const char * s;
	for (s = cs; *s; s++) {
		const char * t;
		for (t = ct; *t; t++)
			if (*t == *s)
				return (char *) s;
	}
	return NULL;
}

extern inline int strlen(const char * s);

extern inline char * strstr(const char * cs,const char * ct)
{
	int n = strlen(ct);
	if (!n)
		return (char *) cs;
	while (*cs) {
		if (*cs == *ct && !strncmp(cs, ct, n))
			return (char *) cs;
		cs++;
	}
	return NULL;
}

extern inline int strlen(const char * s)
{
	const char * p = s;
	while (*p) p++;
	return p - s;
}

extern char * ___strtok;

extern inline char * strtok(char * s,const char *ct)
{
	char * tok;
	if (s)
		___strtok = s;
	if (!___strtok)
		return NULL;
	___strtok += strspn(___strtok, ct);
	if (!*___strtok) {
		___strtok = NULL;
		return NULL;
	}
	tok = ___strtok;
	___strtok += strcspn(___strtok, ct);
	if (*___strtok)
		*___strtok++ = 0;
	return tok;
}

extern inline void * memcpy(void * dest,const void * src, int n)
{
	char * d = (char *) dest;
	const char * s = (const char *) src;
	while (n-- > 0)
		*d++ = *s++;
	return dest;
}

extern inline void * memmove(void * dest,const void * src, int n)
{
	char * d = (char *) dest;
	const char * s = (const char *) src;
	if (d < s || d >= s + n)
		return memcpy(dest, src, n);
	d += n;
	s += n;
	while (n-- > 0)
		*--d = *--s;
	return dest;
}

extern inline int memcmp(const void * cs,const void * ct,int count)
{
	const char * a = (const char *) cs;
	const char * b = (const char *) ct;
	while (count-- > 0) {
		if (*a != *b)
			return (int)(unsigned char)*a - (int)(unsigned char)*b;
		a++;
		b++;
	}
	return 0;
}

extern inline void * memchr(const void * cs,char c,int count)
{
	const char * p = (const char *) cs;
	while (count-- > 0) {
		if (*p == c)
			return (void *) p;
		p++;
	}
	return NULL;
}

extern inline void * memset(void * s,char c,int count)
{
	char * p = (char *) s;
	while (count-- > 0)
		*p++ = c;
	return s;
}

#endif
