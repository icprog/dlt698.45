/* stb.h - v2.30

 Documentation: http://nothings.org/stb/stb_h.html
 Unit tests:    http://nothings.org/stb/stb.c


 ============================================================================
 You MUST

 #define STB_DEFINE

 in EXACTLY _one_ C or C++ file that includes this header, BEFORE the
 include, like this:

 #define STB_DEFINE
 #include "stb.h"

 All other files should just #include "stb.h" without the #define.
 ============================================================================
 */

#include <stdarg.h>

#ifndef STB__INCLUDE_STB_H
#define STB__INCLUDE_STB_H

#define STB_VERSION  1

#ifdef STB_INTROSPECT
#define STB_DEFINE
#endif

#ifdef STB_DEFINE_THREADS
#ifndef STB_DEFINE
#define STB_DEFINE
#endif
#ifndef STB_THREADS
#define STB_THREADS
#endif
#endif

#if defined(_WIN32) && !defined(__MINGW32__)
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE
#endif
#ifndef _CRT_NON_CONFORMING_SWPRINTFS
#define _CRT_NON_CONFORMING_SWPRINTFS
#endif
#if !defined(_MSC_VER) || _MSC_VER > 1700
#include <intrin.h> // _BitScanReverse
#endif
#endif

#include <stdlib.h>     // stdlib could have min/max
#include <stdio.h>      // need FILE
#include <string.h>     // stb_define_hash needs memcpy/memset
#include <time.h>       // stb_dirtree
#ifdef __MINGW32__
#include <fcntl.h>   // O_RDWR
#endif

#ifdef STB_PERSONAL
typedef int Bool;
#define False 0
#define True 1
#endif

#ifdef STB_MALLOC_WRAPPER_PAGED
#define STB_MALLOC_WRAPPER_DEBUG
#endif
#ifdef STB_MALLOC_WRAPPER_DEBUG
#define STB_MALLOC_WRAPPER
#endif
#ifdef STB_MALLOC_WRAPPER_FASTMALLOC
#define STB_FASTMALLOC
#define STB_MALLOC_WRAPPER
#endif

#ifdef STB_FASTMALLOC
#ifndef _WIN32
#undef STB_FASTMALLOC
#endif
#endif

#ifdef STB_DEFINE
#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <ctype.h>
#include <math.h>
#ifndef _WIN32
#include <unistd.h>
#else
#include <io.h>      // _mktemp
#include <direct.h>  // _rmdir
#endif
#include <sys/types.h> // stat()/_stat()
#include <sys/stat.h>  // stat()/_stat()
#endif

#define stb_min(a,b)   ((a) < (b) ? (a) : (b))
#define stb_max(a,b)   ((a) > (b) ? (a) : (b))

#ifndef STB_ONLY
#if !defined(__cplusplus) && !defined(min) && !defined(max)
#define min(x,y) stb_min(x,y)
#define max(x,y) stb_max(x,y)
#endif

#ifndef M_PI
#define M_PI  3.14159265358979323846f
#endif

#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

#ifndef deg2rad
#define deg2rad(a)  ((a)*(M_PI/180))
#endif
#ifndef rad2deg
#define rad2deg(a)  ((a)*(180/M_PI))
#endif

#ifndef swap
#ifndef __cplusplus
#define swap(TYPE,a,b)  \
               do { TYPE stb__t; stb__t = (a); (a) = (b); (b) = stb__t; } while (0)
#endif
#endif

typedef unsigned char uint8;
typedef signed char int8;
typedef unsigned short uint16;
typedef signed short int16;
#if defined(STB_USE_LONG_FOR_32_BIT_INT) || defined(STB_LONG32)
typedef unsigned long uint32;
typedef signed long int32;
#else
typedef unsigned int uint32;
typedef signed int int32;
#endif

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

// produce compile errors if the sizes aren't right
typedef char stb__testsize16[sizeof(int16) == 2];
typedef char stb__testsize32[sizeof(int32) == 4];
#endif

#ifndef STB_TRUE
#define STB_TRUE 1
#define STB_FALSE 0
#endif

// if we're STB_ONLY, can't rely on uint32 or even uint, so all the
// variables we'll use herein need typenames prefixed with 'stb':
typedef unsigned char stb_uchar;
typedef unsigned char stb_uint8;
typedef unsigned int stb_uint;
typedef unsigned short stb_uint16;
typedef short stb_int16;
typedef signed char stb_int8;
#if defined(STB_USE_LONG_FOR_32_BIT_INT) || defined(STB_LONG32)
typedef unsigned long stb_uint32;
typedef long stb_int32;
#else
typedef unsigned int stb_uint32;
typedef int stb_int32;
#endif
typedef char stb__testsize2_16[sizeof(stb_uint16) == 2 ? 1 : -1];
typedef char stb__testsize2_32[sizeof(stb_uint32) == 4 ? 1 : -1];

#ifdef _MSC_VER
typedef unsigned __int64 stb_uint64;
typedef __int64 stb_int64;
#define STB_IMM_UINT64(literalui64) (literalui64##ui64)
#define STB_IMM_INT64(literali64) (literali64##i64)
#else
// ??
typedef unsigned long long stb_uint64;
typedef long long stb_int64;
#define STB_IMM_UINT64(literalui64) (literalui64##ULL)
#define STB_IMM_INT64(literali64) (literali64##LL)
#endif
typedef char stb__testsize2_64[sizeof(stb_uint64) == 8 ? 1 : -1];

// add platform-specific ways of checking for sizeof(char*) == 8,
// and make those define STB_PTR64
#if defined(_WIN64) || defined(__x86_64__) || defined(__ia64__) || defined(__LP64__)
#define STB_PTR64
#endif

#ifdef STB_PTR64
typedef char stb__testsize2_ptr[sizeof(char *) == 8];
typedef stb_uint64 stb_uinta;
typedef stb_int64 stb_inta;
#else
typedef char stb__testsize2_ptr[sizeof(char *) == 4];
typedef stb_uint32 stb_uinta;
typedef stb_int32 stb_inta;
#endif
typedef char stb__testsize2_uinta[sizeof(stb_uinta) == sizeof(char*) ? 1 : -1];

// if so, we should define an int type that is the pointer size. until then,
// we'll have to make do with this (which is not the same at all!)

typedef union {
	unsigned int i;
	void * p;
} stb_uintptr;

#ifdef __cplusplus
#define STB_EXTERN   extern "C"
#else
#define STB_EXTERN   extern
#endif

// check for well-known debug defines
#if defined(DEBUG) || defined(_DEBUG) || defined(DBG)
#ifndef NDEBUG
#define STB_DEBUG
#endif
#endif

#ifdef STB_DEBUG
#include <assert.h>
#endif

STB_EXTERN void stb_wrapper_malloc(void *newp, int sz, char *file, int line);
STB_EXTERN void stb_wrapper_free(void *oldp, char *file, int line);
STB_EXTERN void stb_wrapper_realloc(void *oldp, void *newp, int sz, char *file,
		int line);
STB_EXTERN void stb_wrapper_calloc(size_t num, size_t sz, char *file, int line);
STB_EXTERN void stb_wrapper_listall(
		void (*func)(void *ptr, int sz, char *file, int line));
STB_EXTERN void stb_wrapper_dump(char *filename);
STB_EXTERN int stb_wrapper_allocsize(void *oldp);
STB_EXTERN void stb_wrapper_check(void *oldp);

#ifdef STB_DEFINE
// this is a special function used inside malloc wrapper
// to do allocations that aren't tracked (to avoid
// reentrancy). Of course if someone _else_ wraps realloc,
// this breaks, but if they're doing that AND the malloc
// wrapper they need to explicitly check for reentrancy.
//
// only define realloc_raw() and we do realloc(NULL,sz)
// for malloc() and realloc(p,0) for free().
static void * stb__realloc_raw(void *p, int sz)
{
	if (p == NULL) return malloc(sz);
	if (sz == 0) {free(p); return NULL;}
	return realloc(p,sz);
}
#endif

#ifdef _WIN32
STB_EXTERN void * stb_smalloc(size_t sz);
STB_EXTERN void stb_sfree(void *p);
STB_EXTERN void * stb_srealloc(void *p, size_t sz);
STB_EXTERN void * stb_scalloc(size_t n, size_t sz);
STB_EXTERN char * stb_sstrdup(char *s);
#endif

#ifdef STB_FASTMALLOC
#define malloc  stb_smalloc
#define free    stb_sfree
#define realloc stb_srealloc
#define strdup  stb_sstrdup
#define calloc  stb_scalloc
#endif

#ifndef STB_MALLOC_ALLCHECK
#define stb__check(p)  1
#else
#ifndef STB_MALLOC_WRAPPER
#error STB_MALLOC_ALLCHECK requires STB_MALLOC_WRAPPER
#else
#define stb__check(p) stb_mcheck(p)
#endif
#endif

#ifdef STB_MALLOC_WRAPPER
STB_EXTERN void * stb__malloc(int, char *, int);
STB_EXTERN void * stb__realloc(void *, int, char *, int);
STB_EXTERN void * stb__calloc(size_t n, size_t s, char *, int);
STB_EXTERN void stb__free(void *, char *file, int);
STB_EXTERN char * stb__strdup(char *s, char *file, int);
STB_EXTERN void stb_malloc_checkall(void);
STB_EXTERN void stb_malloc_check_counter(int init_delay, int rep_delay);
#ifndef STB_MALLOC_WRAPPER_DEBUG
#define stb_mcheck(p) 1
#else
STB_EXTERN int stb_mcheck(void *);
#endif

#ifdef STB_DEFINE

#ifdef STB_MALLOC_WRAPPER_DEBUG
#define STB__PAD   32
#define STB__BIAS  16
#define STB__SIG   0x51b01234
#define STB__FIXSIZE(sz)  (((sz+3) & ~3) + STB__PAD)
#define STB__ptr(x,y)   ((char *) (x) + (y))
#else
#define STB__ptr(x,y)   (x)
#define STB__FIXSIZE(sz)  (sz)
#endif

#ifdef STB_MALLOC_WRAPPER_DEBUG
int stb_mcheck(void *p)
{
	unsigned int sz;
	if (p == NULL) return 1;
	p = ((char *) p) - STB__BIAS;
	sz = * (unsigned int *) p;
	assert(* (unsigned int *) STB__ptr(p,4) == STB__SIG);
	assert(* (unsigned int *) STB__ptr(p,8) == STB__SIG);
	assert(* (unsigned int *) STB__ptr(p,12) == STB__SIG);
	assert(* (unsigned int *) STB__ptr(p,sz-4) == STB__SIG+1);
	assert(* (unsigned int *) STB__ptr(p,sz-8) == STB__SIG+1);
	assert(* (unsigned int *) STB__ptr(p,sz-12) == STB__SIG+1);
	assert(* (unsigned int *) STB__ptr(p,sz-16) == STB__SIG+1);
	stb_wrapper_check(STB__ptr(p, STB__BIAS));
	return 1;
}

static void stb__check2(void *p, int sz, char *file, int line)
{
	stb_mcheck(p);
}

void stb_malloc_checkall(void)
{
	stb_wrapper_listall(stb__check2);
}
#else
void stb_malloc_checkall(void) {}
#endif

static int stb__malloc_wait=(1 << 30), stb__malloc_next_wait = (1 << 30), stb__malloc_iter;
void stb_malloc_check_counter(int init_delay, int rep_delay)
{
	stb__malloc_wait = init_delay;
	stb__malloc_next_wait = rep_delay;
}

void stb_mcheck_all(void)
{
#ifdef STB_MALLOC_WRAPPER_DEBUG
	++stb__malloc_iter;
	if (--stb__malloc_wait <= 0) {
		stb_malloc_checkall();
		stb__malloc_wait = stb__malloc_next_wait;
	}
#endif
}

#ifdef STB_MALLOC_WRAPPER_PAGED
#define STB__WINDOWS_PAGE (1 << 12)
#ifndef _WINDOWS_
STB_EXTERN __declspec(dllimport) void * __stdcall VirtualAlloc(void *p, unsigned long size, unsigned long type, unsigned long protect);
STB_EXTERN __declspec(dllimport) int __stdcall VirtualFree(void *p, unsigned long size, unsigned long freetype);
#endif
#endif

static void *stb__malloc_final(int sz)
{
#ifdef STB_MALLOC_WRAPPER_PAGED
	int aligned = (sz + STB__WINDOWS_PAGE - 1) & ~(STB__WINDOWS_PAGE-1);
	char *p = VirtualAlloc(NULL, aligned + STB__WINDOWS_PAGE, 0x2000, 0x04); // RESERVE, READWRITE
	if (p == NULL) return p;
	VirtualAlloc(p, aligned, 0x1000, 0x04);// COMMIT, READWRITE
	return p;
#else
	return malloc(sz);
#endif
}

static void stb__free_final(void *p)
{
#ifdef STB_MALLOC_WRAPPER_PAGED
	VirtualFree(p, 0, 0x8000); // RELEASE
#else
	free(p);
#endif
}

int stb__malloc_failure;
static void *stb__realloc_final(void *p, int sz, int old_sz)
{
#ifdef STB_MALLOC_WRAPPER_PAGED
	void *q = stb__malloc_final(sz);
	if (q == NULL)
	return ++stb__malloc_failure, q;
	// @TODO: deal with p being smaller!
	memcpy(q, p, sz < old_sz ? sz : old_sz);
	stb__free_final(p);
	return q;
#else
	return realloc(p,sz);
#endif
}

void stb__free(void *p, char *file, int line)
{
	stb_mcheck_all();
	if (!p) return;
#ifdef STB_MALLOC_WRAPPER_DEBUG
	stb_mcheck(p);
#endif
	stb_wrapper_free(p,file,line);
#ifdef STB_MALLOC_WRAPPER_DEBUG
	p = STB__ptr(p,-STB__BIAS);
	* (unsigned int *) STB__ptr(p,0) = 0xdeadbeef;
	* (unsigned int *) STB__ptr(p,4) = 0xdeadbeef;
	* (unsigned int *) STB__ptr(p,8) = 0xdeadbeef;
	* (unsigned int *) STB__ptr(p,12) = 0xdeadbeef;
#endif
	stb__free_final(p);
}

void * stb__malloc(int sz, char *file, int line)
{
	void *p;
	stb_mcheck_all();
	if (sz == 0) return NULL;
	p = stb__malloc_final(STB__FIXSIZE(sz));
	if (p == NULL) p = stb__malloc_final(STB__FIXSIZE(sz));
	if (p == NULL) p = stb__malloc_final(STB__FIXSIZE(sz));
	if (p == NULL) {
		++stb__malloc_failure;
#ifdef STB_MALLOC_WRAPPER_DEBUG
		stb_malloc_checkall();
#endif
		return p;
	}
#ifdef STB_MALLOC_WRAPPER_DEBUG
	* (int *) STB__ptr(p,0) = STB__FIXSIZE(sz);
	* (unsigned int *) STB__ptr(p,4) = STB__SIG;
	* (unsigned int *) STB__ptr(p,8) = STB__SIG;
	* (unsigned int *) STB__ptr(p,12) = STB__SIG;
	* (unsigned int *) STB__ptr(p,STB__FIXSIZE(sz)-4) = STB__SIG+1;
	* (unsigned int *) STB__ptr(p,STB__FIXSIZE(sz)-8) = STB__SIG+1;
	* (unsigned int *) STB__ptr(p,STB__FIXSIZE(sz)-12) = STB__SIG+1;
	* (unsigned int *) STB__ptr(p,STB__FIXSIZE(sz)-16) = STB__SIG+1;
	p = STB__ptr(p, STB__BIAS);
#endif
	stb_wrapper_malloc(p,sz,file,line);
	return p;
}

void * stb__realloc(void *p, int sz, char *file, int line)
{
	void *q;

	stb_mcheck_all();
	if (p == NULL) return stb__malloc(sz,file,line);
	if (sz == 0 ) {stb__free(p,file,line); return NULL;}

#ifdef STB_MALLOC_WRAPPER_DEBUG
	stb_mcheck(p);
	p = STB__ptr(p,-STB__BIAS);
#endif
#ifdef STB_MALLOC_WRAPPER_PAGED
	{
		int n = stb_wrapper_allocsize(STB__ptr(p,STB__BIAS));
		if (!n)
		stb_wrapper_check(STB__ptr(p,STB__BIAS));
		q = stb__realloc_final(p, STB__FIXSIZE(sz), STB__FIXSIZE(n));
	}
#else
	q = realloc(p, STB__FIXSIZE(sz));
#endif
	if (q == NULL)
	return ++stb__malloc_failure, q;
#ifdef STB_MALLOC_WRAPPER_DEBUG
	* (int *) STB__ptr(q,0) = STB__FIXSIZE(sz);
	* (unsigned int *) STB__ptr(q,4) = STB__SIG;
	* (unsigned int *) STB__ptr(q,8) = STB__SIG;
	* (unsigned int *) STB__ptr(q,12) = STB__SIG;
	* (unsigned int *) STB__ptr(q,STB__FIXSIZE(sz)-4) = STB__SIG+1;
	* (unsigned int *) STB__ptr(q,STB__FIXSIZE(sz)-8) = STB__SIG+1;
	* (unsigned int *) STB__ptr(q,STB__FIXSIZE(sz)-12) = STB__SIG+1;
	* (unsigned int *) STB__ptr(q,STB__FIXSIZE(sz)-16) = STB__SIG+1;

	q = STB__ptr(q, STB__BIAS);
	p = STB__ptr(p, STB__BIAS);
#endif
	stb_wrapper_realloc(p,q,sz,file,line);
	return q;
}

STB_EXTERN int stb_log2_ceil(unsigned int);
static void *stb__calloc(size_t n, size_t sz, char *file, int line)
{
	void *q;
	stb_mcheck_all();
	if (n == 0 || sz == 0) return NULL;
	if (stb_log2_ceil(n) + stb_log2_ceil(sz) >= 32) return NULL;
	q = stb__malloc(n*sz, file, line);
	if (q) memset(q, 0, n*sz);
	return q;
}

char * stb__strdup(char *s, char *file, int line)
{
	char *p;
	stb_mcheck_all();
	p = stb__malloc(strlen(s)+1, file, line);
	if (!p) return p;
	strcpy(p, s);
	return p;
}
#endif // STB_DEFINE

#ifdef STB_FASTMALLOC
#undef malloc
#undef realloc
#undef free
#undef strdup
#undef calloc
#endif

// include everything that might define these, BEFORE making macros
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#define malloc(s)      stb__malloc (  s, __FILE__, __LINE__)
#define realloc(p,s)   stb__realloc(p,s, __FILE__, __LINE__)
#define calloc(n,s)    stb__calloc (n,s, __FILE__, __LINE__)
#define free(p)        stb__free   (p,   __FILE__, __LINE__)
#define strdup(p)      stb__strdup (p,   __FILE__, __LINE__)

#endif

//////////////////////////////////////////////////////////////////////////////
//
//                         Windows pretty display
//

STB_EXTERN void stbprint(const char *fmt, ...);
STB_EXTERN char *stb_sprintf(const char *fmt, ...);
STB_EXTERN char *stb_mprintf(const char *fmt, ...);
STB_EXTERN int stb_snprintf(char *s, size_t n, const char *fmt, ...);
STB_EXTERN int stb_vsnprintf(char *s, size_t n, const char *fmt, va_list v);

#ifdef STB_DEFINE

int stb_vsnprintf(char *s, size_t n, const char *fmt, va_list v)
{
	int res;
#ifdef _WIN32
	// Could use "_vsnprintf_s(s, n, _TRUNCATE, fmt, v)" ?
	res = _vsnprintf(s,n,fmt,v);
#else
	res = vsnprintf(s,n,fmt,v);
#endif
	if (n) s[n-1] = 0;
	// Unix returns length output would require, Windows returns negative when truncated.
	return (res >= (int) n || res < 0) ? -1 : res;
}

int stb_snprintf(char *s, size_t n, const char *fmt, ...)
{
	int res;
	va_list v;
	va_start(v,fmt);
	res = stb_vsnprintf(s, n, fmt, v);
	va_end(v);
	return res;
}

char *stb_sprintf(const char *fmt, ...)
{
	static char buffer[1024];
	va_list v;
	va_start(v,fmt);
	stb_vsnprintf(buffer,1024,fmt,v);
	va_end(v);
	return buffer;
}

char *stb_mprintf(const char *fmt, ...)
{
	static char buffer[1024];
	va_list v;
	va_start(v,fmt);
	stb_vsnprintf(buffer,1024,fmt,v);
	va_end(v);
	return strdup(buffer);
}

#ifdef _WIN32

#ifndef _WINDOWS_
STB_EXTERN __declspec(dllimport) int __stdcall WriteConsoleA(void *, const void *, unsigned int, unsigned int *, void *);
STB_EXTERN __declspec(dllimport) void * __stdcall GetStdHandle(unsigned int);
STB_EXTERN __declspec(dllimport) int __stdcall SetConsoleTextAttribute(void *, unsigned short);
#endif

static void stb__print_one(void *handle, char *s, int len)
{
	if (len)
	if (WriteConsoleA(handle, s, len, NULL,NULL))
	fwrite(s, 1, len, stdout); // if it fails, maybe redirected, so do normal
}

static void stb__print(char *s)
{
	void *handle = GetStdHandle((unsigned int) -11); // STD_OUTPUT_HANDLE
	int pad=0;// number of padding characters to add

	char *t = s;
	while (*s) {
		int lpad;
		while (*s && *s != '{') {
			if (pad) {
				if (*s == '\r' || *s == '\n')
				pad = 0;
				else if (s[0] == ' ' && s[1] == ' ') {
					stb__print_one(handle, t, s-t);
					t = s;
					while (pad) {
						stb__print_one(handle, t, 1);
						--pad;
					}
				}
			}
			++s;
		}
		if (!*s) break;
		stb__print_one(handle, t, s-t);
		if (s[1] == '{') {
			++s;
			continue;
		}

		if (s[1] == '#') {
			t = s+3;
			if (isxdigit(s[2]))
			if (isdigit(s[2]))
			SetConsoleTextAttribute(handle, s[2] - '0');
			else
			SetConsoleTextAttribute(handle, tolower(s[2]) - 'a' + 10);
			else {
				SetConsoleTextAttribute(handle, 0x0f);
				t=s+2;
			}
		} else if (s[1] == '!') {
			SetConsoleTextAttribute(handle, 0x0c);
			t = s+2;
		} else if (s[1] == '@') {
			SetConsoleTextAttribute(handle, 0x09);
			t = s+2;
		} else if (s[1] == '$') {
			SetConsoleTextAttribute(handle, 0x0a);
			t = s+2;
		} else {
			SetConsoleTextAttribute(handle, 0x08); // 0,7,8,15 => shades of grey
			t = s+1;
		}

		lpad = (t-s);
		s = t;
		while (*s && *s != '}') ++s;
		if (!*s) break;
		stb__print_one(handle, t, s-t);
		if (s[1] == '}') {
			t = s+2;
		} else {
			pad += 1+lpad;
			t = s+1;
		}
		s=t;
		SetConsoleTextAttribute(handle, 0x07);
	}
	stb__print_one(handle, t, s-t);
	SetConsoleTextAttribute(handle, 0x07);
}

void stbprint(const char *fmt, ...)
{
	int res;
	char buffer[1024];
	char *tbuf = buffer;
	va_list v;

	va_start(v,fmt);
	res = stb_vsnprintf(buffer, sizeof(buffer), fmt, v);
	va_end(v);

	if (res < 0) {
		tbuf = (char *) malloc(16384);
		va_start(v,fmt);
		res = _vsnprintf(tbuf,16384, fmt, v);
		va_end(v);
		tbuf[16383] = 0;
	}

	stb__print(tbuf);

	if (tbuf != buffer)
	free(tbuf);
}

#else  // _WIN32
void stbprint(const char *fmt, ...)
{
	va_list v;
	va_start(v,fmt);
	vprintf(fmt,v);
	va_end(v);
}
#endif // _WIN32
#endif // STB_DEFINE

//////////////////////////////////////////////////////////////////////////////
//
//                         Windows UTF8 filename handling
//
// Windows stupidly treats 8-bit filenames as some dopey code page,
// rather than utf-8. If we want to use utf8 filenames, we have to
// convert them to WCHAR explicitly and call WCHAR versions of the
// file functions. So, ok, we do.

#ifdef _WIN32
#define stb__fopen(x,y)    _wfopen((const wchar_t *)stb__from_utf8(x), (const wchar_t *)stb__from_utf8_alt(y))
#define stb__windows(x,y)  x
#else
#define stb__fopen(x,y)    fopen(x,y)
#define stb__windows(x,y)  y
#endif

typedef unsigned short stb__wchar;

STB_EXTERN stb__wchar * stb_from_utf8(stb__wchar *buffer, char *str, int n);
STB_EXTERN char * stb_to_utf8(char *buffer, stb__wchar *str, int n);

STB_EXTERN stb__wchar *stb__from_utf8(char *str);
STB_EXTERN stb__wchar *stb__from_utf8_alt(char *str);
STB_EXTERN char *stb__to_utf8(stb__wchar *str);

#ifdef STB_DEFINE
stb__wchar * stb_from_utf8(stb__wchar *buffer, char *ostr, int n)
{
	unsigned char *str = (unsigned char *) ostr;
	stb_uint32 c;
	int i=0;
	--n;
	while (*str) {
		if (i >= n)
		return NULL;
		if (!(*str & 0x80))
		buffer[i++] = *str++;
		else if ((*str & 0xe0) == 0xc0) {
			if (*str < 0xc2) return NULL;
			c = (*str++ & 0x1f) << 6;
			if ((*str & 0xc0) != 0x80) return NULL;
			buffer[i++] = c + (*str++ & 0x3f);
		} else if ((*str & 0xf0) == 0xe0) {
			if (*str == 0xe0 && (str[1] < 0xa0 || str[1] > 0xbf)) return NULL;
			if (*str == 0xed && str[1] > 0x9f) return NULL; // str[1] < 0x80 is checked below
			c = (*str++ & 0x0f) << 12;
			if ((*str & 0xc0) != 0x80) return NULL;
			c += (*str++ & 0x3f) << 6;
			if ((*str & 0xc0) != 0x80) return NULL;
			buffer[i++] = c + (*str++ & 0x3f);
		} else if ((*str & 0xf8) == 0xf0) {
			if (*str > 0xf4) return NULL;
			if (*str == 0xf0 && (str[1] < 0x90 || str[1] > 0xbf)) return NULL;
			if (*str == 0xf4 && str[1] > 0x8f) return NULL; // str[1] < 0x80 is checked below
			c = (*str++ & 0x07) << 18;
			if ((*str & 0xc0) != 0x80) return NULL;
			c += (*str++ & 0x3f) << 12;
			if ((*str & 0xc0) != 0x80) return NULL;
			c += (*str++ & 0x3f) << 6;
			if ((*str & 0xc0) != 0x80) return NULL;
			c += (*str++ & 0x3f);
			// utf-8 encodings of values used in surrogate pairs are invalid
			if ((c & 0xFFFFF800) == 0xD800) return NULL;
			if (c >= 0x10000) {
				c -= 0x10000;
				if (i + 2 > n) return NULL;
				buffer[i++] = 0xD800 | (0x3ff & (c >> 10));
				buffer[i++] = 0xDC00 | (0x3ff & (c ));
			}
		} else
		return NULL;
	}
	buffer[i] = 0;
	return buffer;
}

char * stb_to_utf8(char *buffer, stb__wchar *str, int n)
{
	int i=0;
	--n;
	while (*str) {
		if (*str < 0x80) {
			if (i+1 > n) return NULL;
			buffer[i++] = (char) *str++;
		} else if (*str < 0x800) {
			if (i+2 > n) return NULL;
			buffer[i++] = 0xc0 + (*str >> 6);
			buffer[i++] = 0x80 + (*str & 0x3f);
			str += 1;
		} else if (*str >= 0xd800 && *str < 0xdc00) {
			stb_uint32 c;
			if (i+4 > n) return NULL;
			c = ((str[0] - 0xd800) << 10) + ((str[1]) - 0xdc00) + 0x10000;
			buffer[i++] = 0xf0 + (c >> 18);
			buffer[i++] = 0x80 + ((c >> 12) & 0x3f);
			buffer[i++] = 0x80 + ((c >> 6) & 0x3f);
			buffer[i++] = 0x80 + ((c ) & 0x3f);
			str += 2;
		} else if (*str >= 0xdc00 && *str < 0xe000) {
			return NULL;
		} else {
			if (i+3 > n) return NULL;
			buffer[i++] = 0xe0 + (*str >> 12);
			buffer[i++] = 0x80 + ((*str >> 6) & 0x3f);
			buffer[i++] = 0x80 + ((*str ) & 0x3f);
			str += 1;
		}
	}
	buffer[i] = 0;
	return buffer;
}

stb__wchar *stb__from_utf8(char *str)
{
	static stb__wchar buffer[4096];
	return stb_from_utf8(buffer, str, 4096);
}

stb__wchar *stb__from_utf8_alt(char *str)
{
	static stb__wchar buffer[64];
	return stb_from_utf8(buffer, str, 64);
}

char *stb__to_utf8(stb__wchar *str)
{
	static char buffer[4096];
	return stb_to_utf8(buffer, str, 4096);
}

#endif

//////////////////////////////////////////////////////////////////////////////
//
//                         Miscellany
//

STB_EXTERN void stb_fatal(char *fmt, ...);
STB_EXTERN void stb_(char *fmt, ...);
STB_EXTERN void stb_append_to_file(char *file, char *fmt, ...);
STB_EXTERN void stb_log(int active);
STB_EXTERN void stb_log_fileline(int active);
STB_EXTERN void stb_log_name(char *filename);

STB_EXTERN void stb_swap(void *p, void *q, size_t sz);
STB_EXTERN void *stb_copy(void *p, size_t sz);
STB_EXTERN void stb_pointer_array_free(void *p, int len);
STB_EXTERN void **stb_array_block_alloc(int count, int blocksize);

#define stb_arrcount(x)   (sizeof(x)/sizeof((x)[0]))

STB_EXTERN int stb__record_fileline(char *f, int n);

#ifdef STB_DEFINE

static char *stb__file;
static int stb__line;

int stb__record_fileline(char *f, int n)
{
	stb__file = f;
	stb__line = n;
	return 0;
}

void stb_fatal(char *s, ...)
{
	va_list a;
	if (stb__file)
	fprintf(stderr, "[%s:%d] ", stb__file, stb__line);
	va_start(a,s);
	fputs("Fatal error: ", stderr);
	vfprintf(stderr, s, a);
	va_end(a);
	fputs("\n", stderr);
#ifdef STB_DEBUG
#ifdef _MSC_VER
#ifndef STB_PTR64
	__asm int 3;   // trap to debugger!
#else
	__debugbreak();
#endif
#else
	__builtin_trap();
#endif
#endif
	exit(1);
}

static int stb__log_active=1, stb__log_fileline=1;

void stb_log(int active)
{
	stb__log_active = active;
}

void stb_log_fileline(int active)
{
	stb__log_fileline = active;
}

#ifdef STB_NO_STB_STRINGS
char *stb__log_filename = "temp.log";
#else
char *stb__log_filename = "stb.log";
#endif

void stb_log_name(char *s)
{
	stb__log_filename = s;
}

void stb_(char *s, ...)
{
	if (stb__log_active) {
		FILE *f = fopen(stb__log_filename, "a");
		if (f) {
			va_list a;
			if (stb__log_fileline && stb__file)
			fprintf(f, "[%s:%4d] ", stb__file, stb__line);
			va_start(a,s);
			vfprintf(f, s, a);
			va_end(a);
			fputs("\n", f);
			fclose(f);
		}
	}
}

void stb_append_to_file(char *filename, char *s, ...)
{
	FILE *f = fopen(filename, "a");
	if (f) {
		va_list a;
		va_start(a,s);
		vfprintf(f, s, a);
		va_end(a);
		fputs("\n", f);
		fclose(f);
	}
}

typedef struct {char d[4];}stb__4;
typedef struct {char d[8];}stb__8;

// optimize the small cases, though you shouldn't be calling this for those!
void stb_swap(void *p, void *q, size_t sz)
{
	char buffer[256];
	if (p == q) return;
	if (sz == 4) {
		stb__4 temp = * ( stb__4 *) p;
		* (stb__4 *) p = * ( stb__4 *) q;
		* (stb__4 *) q = temp;
		return;
	} else if (sz == 8) {
		stb__8 temp = * ( stb__8 *) p;
		* (stb__8 *) p = * ( stb__8 *) q;
		* (stb__8 *) q = temp;
		return;
	}

	while (sz > sizeof(buffer)) {
		stb_swap(p, q, sizeof(buffer));
		p = (char *) p + sizeof(buffer);
		q = (char *) q + sizeof(buffer);
		sz -= sizeof(buffer);
	}

	memcpy(buffer, p , sz);
	memcpy(p , q , sz);
	memcpy(q , buffer, sz);
}

void *stb_copy(void *p, size_t sz)
{
	void *q = malloc(sz);
	memcpy(q, p, sz);
	return q;
}

void stb_pointer_array_free(void *q, int len)
{
	void **p = (void **) q;
	int i;
	for (i=0; i < len; ++i)
	free(p[i]);
}

void **stb_array_block_alloc(int count, int blocksize)
{
	int i;
	char *p = (char *) malloc(sizeof(void *) * count + count * blocksize);
	void **q;
	if (p == NULL) return NULL;
	q = (void **) p;
	p += sizeof(void *) * count;
	for (i=0; i < count; ++i)
	q[i] = p + i * blocksize;
	return q;
}
#endif

#ifdef STB_DEBUG
// tricky hack to allow recording FILE,LINE even in varargs functions
#define STB__RECORD_FILE(x)  (stb__record_fileline(__FILE__, __LINE__),(x))
#define stb_log              STB__RECORD_FILE(stb_log)
#define stb_                 STB__RECORD_FILE(stb_)
#ifndef STB_FATAL_CLEAN
#define stb_fatal            STB__RECORD_FILE(stb_fatal)
#endif
#define STB__DEBUG(x)        x
#else
#define STB__DEBUG(x)
#endif

//////////////////////////////////////////////////////////////////////////////
//
//                         stb_temp
//

#define stb_temp(block, sz)     stb__temp(block, sizeof(block), (sz))

STB_EXTERN void * stb__temp(void *b, int b_sz, int want_sz);
STB_EXTERN void stb_tempfree(void *block, void *ptr);

#ifdef STB_DEFINE

void * stb__temp(void *b, int b_sz, int want_sz)
{
	if (b_sz >= want_sz)
	return b;
	else
	return malloc(want_sz);
}

void stb_tempfree(void *b, void *p)
{
	if (p != b)
	free(p);
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//                      math/sampling operations
//

#define stb_lerp(t,a,b)               ( (a) + (t) * (float) ((b)-(a)) )
#define stb_unlerp(t,a,b)             ( ((t) - (a)) / (float) ((b) - (a)) )

#define stb_clamp(x,xmin,xmax)  ((x) < (xmin) ? (xmin) : (x) > (xmax) ? (xmax) : (x))

STB_EXTERN void stb_newell_normal(float *normal, int num_vert, float **vert,
		int normalize);
STB_EXTERN int stb_box_face_vertex_axis_side(int face_number, int vertex_number,
		int axis);
STB_EXTERN void stb_linear_controller(float *curpos, float target_pos,
		float acc, float deacc, float dt);

STB_EXTERN int stb_float_eq(float x, float y, float delta, int max_ulps);
STB_EXTERN int stb_is_prime(unsigned int m);
STB_EXTERN unsigned int stb_power_of_two_nearest_prime(int n);

STB_EXTERN float stb_smoothstep(float t);
STB_EXTERN float stb_cubic_bezier_1d(float t, float p0, float p1, float p2,
		float p3);

STB_EXTERN double stb_linear_remap(double x, double a, double b, double c,
		double d);

#ifdef STB_DEFINE
float stb_smoothstep(float t)
{
	return (3 - 2*t)*(t*t);
}

float stb_cubic_bezier_1d(float t, float p0, float p1, float p2, float p3)
{
	float it = 1-t;
	return it*it*it*p0 + 3*it*it*t*p1 + 3*it*t*t*p2 + t*t*t*p3;
}

void stb_newell_normal(float *normal, int num_vert, float **vert, int normalize)
{
	int i,j;
	float p;
	normal[0] = normal[1] = normal[2] = 0;
	for (i=num_vert-1,j=0; j < num_vert; i=j++) {
		float *u = vert[i];
		float *v = vert[j];
		normal[0] += (u[1] - v[1]) * (u[2] + v[2]);
		normal[1] += (u[2] - v[2]) * (u[0] + v[0]);
		normal[2] += (u[0] - v[0]) * (u[1] + v[1]);
	}
	if (normalize) {
		p = normal[0]*normal[0] + normal[1]*normal[1] + normal[2]*normal[2];
		p = (float) (1.0 / sqrt(p));
		normal[0] *= p;
		normal[1] *= p;
		normal[2] *= p;
	}
}

int stb_box_face_vertex_axis_side(int face_number, int vertex_number, int axis)
{
	static int box_vertices[6][4][3] =
	{
		{	{	1,1,1}, {1,0,1}, {1,0,0}, {1,1,0}},
		{	{	0,0,0}, {0,0,1}, {0,1,1}, {0,1,0}},
		{	{	0,0,0}, {0,1,0}, {1,1,0}, {1,0,0}},
		{	{	0,0,0}, {1,0,0}, {1,0,1}, {0,0,1}},
		{	{	1,1,1}, {0,1,1}, {0,0,1}, {1,0,1}},
		{	{	1,1,1}, {1,1,0}, {0,1,0}, {0,1,1}},
	};
	assert(face_number >= 0 && face_number < 6);
	assert(vertex_number >= 0 && vertex_number < 4);
	assert(axis >= 0 && axis < 3);
	return box_vertices[face_number][vertex_number][axis];
}

void stb_linear_controller(float *curpos, float target_pos, float acc, float deacc, float dt)
{
	float sign = 1, p, cp = *curpos;
	if (cp == target_pos) return;
	if (target_pos < cp) {
		target_pos = -target_pos;
		cp = -cp;
		sign = -1;
	}
	// first decelerate
	if (cp < 0) {
		p = cp + deacc * dt;
		if (p > 0) {
			p = 0;
			dt = dt - cp / deacc;
			if (dt < 0) dt = 0;
		} else {
			dt = 0;
		}
		cp = p;
	}
	// now accelerate
	p = cp + acc*dt;
	if (p > target_pos) p = target_pos;
	*curpos = p * sign;
	// @TODO: testing
}

float stb_quadratic_controller(float target_pos, float curpos, float maxvel, float maxacc, float dt, float *curvel)
{
	return 0; // @TODO
}

int stb_float_eq(float x, float y, float delta, int max_ulps)
{
	if (fabs(x-y) <= delta) return 1;
	if (abs(*(int *)&x - *(int *)&y) <= max_ulps) return 1;
	return 0;
}

int stb_is_prime(unsigned int m)
{
	unsigned int i,j;
	if (m < 2) return 0;
	if (m == 2) return 1;
	if (!(m & 1)) return 0;
	if (m % 3 == 0) return (m == 3);
	for (i=5; (j=i*i), j <= m && j > i; i += 6) {
		if (m % i == 0) return 0;
		if (m % (i+2) == 0) return 0;
	}
	return 1;
}

unsigned int stb_power_of_two_nearest_prime(int n)
{
	static signed char tab[32] = {0,0,0,0,1,0,-1,0,1,-1,-1,3,-1,0,-1,2,1,
		0,2,0,-1,-4,-1,5,-1,18,-2,15,2,-1,2,0};
	if (!tab[0]) {
		int i;
		for (i=0; i < 32; ++i)
		tab[i] = (1 << i) + 2*tab[i] - 1;
		tab[1] = 2;
		tab[0] = 1;
	}
	if (n >= 32) return 0xfffffffb;
	return tab[n];
}

double stb_linear_remap(double x, double x_min, double x_max,
		double out_min, double out_max)
{
	return stb_lerp(stb_unlerp(x,x_min,x_max),out_min,out_max);
}
#endif

// create a macro so it's faster, but you can get at the function pointer
#define stb_linear_remap(t,a,b,c,d)   stb_lerp(stb_unlerp(t,a,b),c,d)

//////////////////////////////////////////////////////////////////////////////
//
//                         bit operations
//

#define stb_big32(c)    (((c)[0]<<24) + (c)[1]*65536 + (c)[2]*256 + (c)[3])
#define stb_little32(c) (((c)[3]<<24) + (c)[2]*65536 + (c)[1]*256 + (c)[0])
#define stb_big16(c)    ((c)[0]*256 + (c)[1])
#define stb_little16(c) ((c)[1]*256 + (c)[0])

STB_EXTERN int stb_bitcount(unsigned int a);
STB_EXTERN unsigned int stb_bitreverse8(unsigned char n);
STB_EXTERN unsigned int stb_bitreverse(unsigned int n);

STB_EXTERN int stb_is_pow2(unsigned int n);
STB_EXTERN int stb_log2_ceil(unsigned int n);
STB_EXTERN int stb_log2_floor(unsigned int n);

STB_EXTERN int stb_lowbit8(unsigned int n);
STB_EXTERN int stb_highbit8(unsigned int n);

#ifdef STB_DEFINE
int stb_bitcount(unsigned int a)
{
	a = (a & 0x55555555) + ((a >> 1) & 0x55555555); // max 2
	a = (a & 0x33333333) + ((a >> 2) & 0x33333333);// max 4
	a = (a + (a >> 4)) & 0x0f0f0f0f;// max 8 per 4, now 8 bits
	a = (a + (a >> 8));// max 16 per 8 bits
	a = (a + (a >> 16));// max 32 per 8 bits
	return a & 0xff;
}

unsigned int stb_bitreverse8(unsigned char n)
{
	n = ((n & 0xAA) >> 1) + ((n & 0x55) << 1);
	n = ((n & 0xCC) >> 2) + ((n & 0x33) << 2);
	return (unsigned char) ((n >> 4) + (n << 4));
}

unsigned int stb_bitreverse(unsigned int n)
{
	n = ((n & 0xAAAAAAAA) >> 1) | ((n & 0x55555555) << 1);
	n = ((n & 0xCCCCCCCC) >> 2) | ((n & 0x33333333) << 2);
	n = ((n & 0xF0F0F0F0) >> 4) | ((n & 0x0F0F0F0F) << 4);
	n = ((n & 0xFF00FF00) >> 8) | ((n & 0x00FF00FF) << 8);
	return (n >> 16) | (n << 16);
}

int stb_is_pow2(unsigned int n)
{
	return (n & (n-1)) == 0;
}

// tricky use of 4-bit table to identify 5 bit positions (note the '-1')
// 3-bit table would require another tree level; 5-bit table wouldn't save one
#if defined(_WIN32) && !defined(__MINGW32__)
#pragma warning(push)
#pragma warning(disable: 4035)  // disable warning about no return value
int stb_log2_floor(unsigned int n)
{
#if _MSC_VER > 1700
	unsigned long i;
	_BitScanReverse(&i, n);
	return i != 0 ? i : -1;
#else
	__asm {
		bsr eax,n
		jnz done
		mov eax,-1
	}
	done:;
#endif
}
#pragma warning(pop)
#else
int stb_log2_floor(unsigned int n)
{
	static signed char log2_4[16] = {-1,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3};

	// 2 compares if n < 16, 3 compares otherwise
	if (n < (1U << 14))
	if (n < (1U << 4)) return 0 + log2_4[n ];
	else if (n < (1U << 9)) return 5 + log2_4[n >> 5];
	else return 10 + log2_4[n >> 10];
	else if (n < (1U << 24))
	if (n < (1U << 19)) return 15 + log2_4[n >> 15];
	else return 20 + log2_4[n >> 20];
	else if (n < (1U << 29)) return 25 + log2_4[n >> 25];
	else return 30 + log2_4[n >> 30];
}
#endif

// define ceil from floor
int stb_log2_ceil(unsigned int n)
{
	if (stb_is_pow2(n)) return stb_log2_floor(n);
	else return 1 + stb_log2_floor(n);
}

int stb_highbit8(unsigned int n)
{
	return stb_log2_ceil(n&255);
}

int stb_lowbit8(unsigned int n)
{
	static signed char lowbit4[16] = {-1,0,1,0, 2,0,1,0, 3,0,1,0, 2,0,1,0};
	int k = lowbit4[n & 15];
	if (k >= 0) return k;
	k = lowbit4[(n >> 4) & 15];
	if (k >= 0) return k+4;
	return k;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//                            qsort Compare Routines
//

#ifdef _WIN32
#define stb_stricmp(a,b) stricmp(a,b)
#define stb_strnicmp(a,b,n) strnicmp(a,b,n)
#else
#define stb_stricmp(a,b) strcasecmp(a,b)
#define stb_strnicmp(a,b,n) strncasecmp(a,b,n)
#endif

STB_EXTERN int (*stb_intcmp(int offset))(const void *a, const void *b);
STB_EXTERN int (*stb_qsort_strcmp(int offset))(const void *a, const void *b);
STB_EXTERN int (*stb_qsort_stricmp(int offset))(const void *a, const void *b);
STB_EXTERN int (*stb_floatcmp(int offset))(const void *a, const void *b);
STB_EXTERN int (*stb_doublecmp(int offset))(const void *a, const void *b);
STB_EXTERN int (*stb_charcmp(int offset))(const void *a, const void *b);

#ifdef STB_DEFINE
static int stb__intcmpoffset, stb__charcmpoffset, stb__strcmpoffset;
static int stb__floatcmpoffset, stb__doublecmpoffset;

int stb__intcmp(const void *a, const void *b)
{
	const int p = *(const int *) ((const char *) a + stb__intcmpoffset);
	const int q = *(const int *) ((const char *) b + stb__intcmpoffset);
	return p < q ? -1 : p > q;
}

int stb__charcmp(const void *a, const void *b)
{
	const int p = *(const unsigned char *) ((const char *) a + stb__charcmpoffset);
	const int q = *(const unsigned char *) ((const char *) b + stb__charcmpoffset);
	return p < q ? -1 : p > q;
}

int stb__floatcmp(const void *a, const void *b)
{
	const float p = *(const float *) ((const char *) a + stb__floatcmpoffset);
	const float q = *(const float *) ((const char *) b + stb__floatcmpoffset);
	return p < q ? -1 : p > q;
}

int stb__doublecmp(const void *a, const void *b)
{
	const double p = *(const double *) ((const char *) a + stb__doublecmpoffset);
	const double q = *(const double *) ((const char *) b + stb__doublecmpoffset);
	return p < q ? -1 : p > q;
}

int stb__qsort_strcmp(const void *a, const void *b)
{
	const char *p = *(const char **) ((const char *) a + stb__strcmpoffset);
	const char *q = *(const char **) ((const char *) b + stb__strcmpoffset);
	return strcmp(p,q);
}

int stb__qsort_stricmp(const void *a, const void *b)
{
	const char *p = *(const char **) ((const char *) a + stb__strcmpoffset);
	const char *q = *(const char **) ((const char *) b + stb__strcmpoffset);
	return stb_stricmp(p,q);
}

int (*stb_intcmp(int offset))(const void *, const void *)
{
	stb__intcmpoffset = offset;
	return &stb__intcmp;
}

int (*stb_charcmp(int offset))(const void *, const void *)
{
	stb__charcmpoffset = offset;
	return &stb__charcmp;
}

int (*stb_qsort_strcmp(int offset))(const void *, const void *)
{
	stb__strcmpoffset = offset;
	return &stb__qsort_strcmp;
}

int (*stb_qsort_stricmp(int offset))(const void *, const void *)
{
	stb__strcmpoffset = offset;
	return &stb__qsort_stricmp;
}

int (*stb_floatcmp(int offset))(const void *, const void *)
{
	stb__floatcmpoffset = offset;
	return &stb__floatcmp;
}

int (*stb_doublecmp(int offset))(const void *, const void *)
{
	stb__doublecmpoffset = offset;
	return &stb__doublecmp;
}

#endif

//////////////////////////////////////////////////////////////////////////////
//
//                           Binary Search Toolkit
//

typedef struct {
	int minval, maxval, guess;
	int mode, step;
} stb_search;

STB_EXTERN int stb_search_binary(stb_search *s, int minv, int maxv,
		int find_smallest);
STB_EXTERN int stb_search_open(stb_search *s, int minv, int find_smallest);
STB_EXTERN int stb_probe(stb_search *s, int compare, int *result); // return 0 when done

#ifdef STB_DEFINE
enum
{
	STB_probe_binary_smallest,
	STB_probe_binary_largest,
	STB_probe_open_smallest,
	STB_probe_open_largest,
};

static int stb_probe_guess(stb_search *s, int *result)
{
	switch(s->mode) {
		case STB_probe_binary_largest:
		if (s->minval == s->maxval) {
			*result = s->minval;
			return 0;
		}
		assert(s->minval < s->maxval);
		// if a < b, then a < p <= b
		s->guess = s->minval + (((unsigned) s->maxval - s->minval + 1) >> 1);
		break;

		case STB_probe_binary_smallest:
		if (s->minval == s->maxval) {
			*result = s->minval;
			return 0;
		}
		assert(s->minval < s->maxval);
		// if a < b, then a <= p < b
		s->guess = s->minval + (((unsigned) s->maxval - s->minval) >> 1);
		break;
		case STB_probe_open_smallest:
		case STB_probe_open_largest:
		s->guess = s->maxval;// guess the current maxval
		break;
	}
	*result = s->guess;
	return 1;
}

int stb_probe(stb_search *s, int compare, int *result)
{
	switch(s->mode) {
		case STB_probe_open_smallest:
		case STB_probe_open_largest: {
			if (compare <= 0) {
				// then it lies within minval & maxval
				if (s->mode == STB_probe_open_smallest)
				s->mode = STB_probe_binary_smallest;
				else
				s->mode = STB_probe_binary_largest;
			} else {
				// otherwise, we need to probe larger
				s->minval = s->maxval + 1;
				s->maxval = s->minval + s->step;
				s->step += s->step;
			}
			break;
		}
		case STB_probe_binary_smallest: {
			// if compare < 0, then s->minval <= a <  p
			// if compare = 0, then s->minval <= a <= p
			// if compare > 0, then         p <  a <= s->maxval
			if (compare <= 0)
			s->maxval = s->guess;
			else
			s->minval = s->guess+1;
			break;
		}
		case STB_probe_binary_largest: {
			// if compare < 0, then s->minval <= a < p
			// if compare = 0, then         p <= a <= s->maxval
			// if compare > 0, then         p <  a <= s->maxval
			if (compare < 0)
			s->maxval = s->guess-1;
			else
			s->minval = s->guess;
			break;
		}
	}
	return stb_probe_guess(s, result);
}

int stb_search_binary(stb_search *s, int minv, int maxv, int find_smallest)
{
	int r;
	if (maxv < minv) return minv-1;
	s->minval = minv;
	s->maxval = maxv;
	s->mode = find_smallest ? STB_probe_binary_smallest : STB_probe_binary_largest;
	stb_probe_guess(s, &r);
	return r;
}

int stb_search_open(stb_search *s, int minv, int find_smallest)
{
	int r;
	s->step = 4;
	s->minval = minv;
	s->maxval = minv+s->step;
	s->mode = find_smallest ? STB_probe_open_smallest : STB_probe_open_largest;
	stb_probe_guess(s, &r);
	return r;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//                           String Processing
//

#define stb_prefixi(s,t)  (0==stb_strnicmp((s),(t),strlen(t)))

enum stb_splitpath_flag {
	STB_PATH = 1, STB_FILE = 2, STB_EXT = 4, STB_PATH_FILE = STB_PATH
			+ STB_FILE, STB_FILE_EXT = STB_FILE + STB_EXT, STB_EXT_NO_PERIOD = 8,
};

STB_EXTERN char * stb_skipwhite(char *s);
STB_EXTERN char * stb_trimwhite(char *s);
STB_EXTERN char * stb_skipnewline(char *s);
STB_EXTERN char * stb_strncpy(char *s, char *t, int n);
STB_EXTERN char * stb_substr(char *t, int n);
STB_EXTERN char * stb_duplower(char *s);
STB_EXTERN void stb_tolower(char *s);
STB_EXTERN char * stb_strchr2(char *s, char p1, char p2);
STB_EXTERN char * stb_strrchr2(char *s, char p1, char p2);
STB_EXTERN char * stb_strtok(char *output, char *src, char *delimit);
STB_EXTERN char * stb_strtok_keep(char *output, char *src, char *delimit);
STB_EXTERN char * stb_strtok_invert(char *output, char *src, char *allowed);
STB_EXTERN char * stb_dupreplace(char *s, char *find, char *replace);
STB_EXTERN void stb_replaceinplace(char *s, char *find, char *replace);
STB_EXTERN char * stb_splitpath(char *output, char *src, int flag);
STB_EXTERN char * stb_splitpathdup(char *src, int flag);
STB_EXTERN char * stb_replacedir(char *output, char *src, char *dir);
STB_EXTERN char * stb_replaceext(char *output, char *src, char *ext);
STB_EXTERN void stb_fixpath(char *path);
STB_EXTERN char * stb_shorten_path_readable(char *path, int max_len);
STB_EXTERN int stb_suffix(char *s, char *t);
STB_EXTERN int stb_suffixi(char *s, char *t);
STB_EXTERN int stb_prefix(char *s, char *t);
STB_EXTERN char * stb_strichr(char *s, char t);
STB_EXTERN char * stb_stristr(char *s, char *t);
STB_EXTERN int stb_prefix_count(char *s, char *t);
STB_EXTERN char * stb_plural(int n);  // "s" or ""
STB_EXTERN size_t stb_strscpy(char *d, const char *s, size_t n);

STB_EXTERN char **stb_tokens(char *src, char *delimit, int *count);
STB_EXTERN char **stb_tokens_nested(char *src, char *delimit, int *count,
		char *nest_in, char *nest_out);
STB_EXTERN char **stb_tokens_nested_empty(char *src, char *delimit, int *count,
		char *nest_in, char *nest_out);
STB_EXTERN char **stb_tokens_allowempty(char *src, char *delimit, int *count);
STB_EXTERN char **stb_tokens_stripwhite(char *src, char *delimit, int *count);
STB_EXTERN char **stb_tokens_withdelim(char *src, char *delimit, int *count);
STB_EXTERN char **stb_tokens_quoted(char *src, char *delimit, int *count);
// with 'quoted', allow delimiters to appear inside quotation marks, and don't
// strip whitespace inside them (and we delete the quotation marks unless they
// appear back to back, in which case they're considered escaped)

#ifdef STB_DEFINE

size_t stb_strscpy(char *d, const char *s, size_t n)
{
	size_t len = strlen(s);
	if (len >= n) {
		if (n) d[0] = 0;
		return 0;
	}
	strcpy(d,s);
	return len + 1;
}

char *stb_plural(int n)
{
	return n == 1 ? "" : "s";
}

int stb_prefix(char *s, char *t)
{
	while (*t)
	if (*s++ != *t++)
	return STB_FALSE;
	return STB_TRUE;
}

int stb_prefix_count(char *s, char *t)
{
	int c=0;
	while (*t) {
		if (*s++ != *t++)
		break;
		++c;
	}
	return c;
}

int stb_suffix(char *s, char *t)
{
	size_t n = strlen(s);
	size_t m = strlen(t);
	if (m <= n)
	return 0 == strcmp(s+n-m, t);
	else
	return 0;
}

int stb_suffixi(char *s, char *t)
{
	size_t n = strlen(s);
	size_t m = strlen(t);
	if (m <= n)
	return 0 == stb_stricmp(s+n-m, t);
	else
	return 0;
}

// originally I was using this table so that I could create known sentinel
// values--e.g. change whitetable[0] to be true if I was scanning for whitespace,
// and false if I was scanning for nonwhite. I don't appear to be using that
// functionality anymore (I do for tokentable, though), so just replace it
// with isspace()
char *stb_skipwhite(char *s)
{
	while (isspace((unsigned char) *s)) ++s;
	return s;
}

char *stb_skipnewline(char *s)
{
	if (s[0] == '\r' || s[0] == '\n') {
		if (s[0]+s[1] == '\r' + '\n') ++s;
		++s;
	}
	return s;
}

char *stb_trimwhite(char *s)
{
	int i,n;
	s = stb_skipwhite(s);
	n = (int) strlen(s);
	for (i=n-1; i >= 0; --i)
	if (!isspace(s[i]))
	break;
	s[i+1] = 0;
	return s;
}

char *stb_strncpy(char *s, char *t, int n)
{
	strncpy(s,t,n);
	s[n-1] = 0;
	return s;
}

char *stb_substr(char *t, int n)
{
	char *a;
	int z = (int) strlen(t);
	if (z < n) n = z;
	a = (char *) malloc(n+1);
	strncpy(a,t,n);
	a[n] = 0;
	return a;
}

char *stb_duplower(char *s)
{
	char *p = strdup(s), *q = p;
	while (*q) {
		*q = tolower(*q);
		++q;
	}
	return p;
}

void stb_tolower(char *s)
{
	while (*s) {
		*s = tolower(*s);
		++s;
	}
}

char *stb_strchr2(char *s, char x, char y)
{
	for(; *s; ++s)
	if (*s == x || *s == y)
	return s;
	return NULL;
}

char *stb_strrchr2(char *s, char x, char y)
{
	char *r = NULL;
	for(; *s; ++s)
	if (*s == x || *s == y)
	r = s;
	return r;
}

char *stb_strichr(char *s, char t)
{
	if (tolower(t) == toupper(t))
	return strchr(s,t);
	return stb_strchr2(s, (char) tolower(t), (char) toupper(t));
}

char *stb_stristr(char *s, char *t)
{
	size_t n = strlen(t);
	char *z;
	if (n==0) return s;
	while ((z = stb_strichr(s, *t)) != NULL) {
		if (0==stb_strnicmp(z, t, n))
		return z;
		s = z+1;
	}
	return NULL;
}

static char *stb_strtok_raw(char *output, char *src, char *delimit, int keep, int invert)
{
	if (invert) {
		while (*src && strchr(delimit, *src) != NULL) {
			*output++ = *src++;
		}
	} else {
		while (*src && strchr(delimit, *src) == NULL) {
			*output++ = *src++;
		}
	}
	*output = 0;
	if (keep)
	return src;
	else
	return *src ? src+1 : src;
}

char *stb_strtok(char *output, char *src, char *delimit)
{
	return stb_strtok_raw(output, src, delimit, 0, 0);
}

char *stb_strtok_keep(char *output, char *src, char *delimit)
{
	return stb_strtok_raw(output, src, delimit, 1, 0);
}

char *stb_strtok_invert(char *output, char *src, char *delimit)
{
	return stb_strtok_raw(output, src, delimit, 1,1);
}

static char **stb_tokens_raw(char *src_, char *delimit, int *count,
		int stripwhite, int allow_empty, char *start, char *end)
{
	int nested = 0;
	unsigned char *src = (unsigned char *) src_;
	static char stb_tokentable[256]; // rely on static initializion to 0
	static char stable[256],etable[256];
	char *out;
	char **result;
	int num=0;
	unsigned char *s;

	s = (unsigned char *) delimit; while (*s) stb_tokentable[*s++] = 1;
	if (start) {
		s = (unsigned char *) start; while (*s) stable[*s++] = 1;
		s = (unsigned char *) end; if (s) while (*s) stable[*s++] = 1;
		s = (unsigned char *) end; if (s) while (*s) etable[*s++] = 1;
	}
	stable[0] = 1;

	// two passes through: the first time, counting how many
	s = (unsigned char *) src;
	while (*s) {
		// state: just found delimiter
		// skip further delimiters
		if (!allow_empty) {
			stb_tokentable[0] = 0;
			while (stb_tokentable[*s])
			++s;
			if (!*s) break;
		}
		++num;
		// skip further non-delimiters
		stb_tokentable[0] = 1;
		if (stripwhite == 2) { // quoted strings
			while (!stb_tokentable[*s]) {
				if (*s != '"')
				++s;
				else {
					++s;
					if (*s == '"')
					++s;   // "" -> ", not start a string
					else {
						// begin a string
						while (*s) {
							if (s[0] == '"') {
								if (s[1] == '"') s += 2; // "" -> "
								else {++s; break;} // terminating "
							} else
							++s;
						}
					}
				}
			}
		} else
		while (nested || !stb_tokentable[*s]) {
			if (stable[*s]) {
				if (!*s) break;
				if (end ? etable[*s] : nested)
				--nested;
				else
				++nested;
			}
			++s;
		}
		if (allow_empty) {
			if (*s) ++s;
		}
	}
	// now num has the actual count... malloc our output structure
	// need space for all the strings: strings won't be any longer than
	// original input, since for every '\0' there's at least one delimiter
	result = (char **) malloc(sizeof(*result) * (num+1) + (s-src+1));
	if (result == NULL) return result;
	out = (char *) (result + (num+1));
	// second pass: copy out the data
	s = (unsigned char *) src;
	num = 0;
	nested = 0;
	while (*s) {
		char *last_nonwhite;
		// state: just found delimiter
		// skip further delimiters
		if (!allow_empty) {
			stb_tokentable[0] = 0;
			if (stripwhite)
			while (stb_tokentable[*s] || isspace(*s))
			++s;
			else
			while (stb_tokentable[*s])
			++s;
		} else if (stripwhite) {
			while (isspace(*s)) ++s;
		}
		if (!*s) break;
		// we're past any leading delimiters and whitespace
		result[num] = out;
		++num;
		// copy non-delimiters
		stb_tokentable[0] = 1;
		last_nonwhite = out-1;
		if (stripwhite == 2) {
			while (!stb_tokentable[*s]) {
				if (*s != '"') {
					if (!isspace(*s)) last_nonwhite = out;
					*out++ = *s++;
				} else {
					++s;
					if (*s == '"') {
						if (!isspace(*s)) last_nonwhite = out;
						*out++ = *s++; // "" -> ", not start string
					} else {
						// begin a quoted string
						while (*s) {
							if (s[0] == '"') {
								if (s[1] == '"') {*out++ = *s; s += 2;}
								else {++s; break;} // terminating "
							} else
							*out++ = *s++;
						}
						last_nonwhite = out-1; // all in quotes counts as non-white
					}
				}
			}
		} else {
			while (nested || !stb_tokentable[*s]) {
				if (!isspace(*s)) last_nonwhite = out;
				if (stable[*s]) {
					if (!*s) break;
					if (end ? etable[*s] : nested)
					--nested;
					else
					++nested;
				}
				*out++ = *s++;
			}
		}

		if (stripwhite) // rewind to last non-whitespace char
		out = last_nonwhite+1;
		*out++ = '\0';

		if (*s) ++s;// skip delimiter
	}
	s = (unsigned char *) delimit; while (*s) stb_tokentable[*s++] = 0;
	if (start) {
		s = (unsigned char *) start; while (*s) stable[*s++] = 1;
		s = (unsigned char *) end; if (s) while (*s) stable[*s++] = 1;
		s = (unsigned char *) end; if (s) while (*s) etable[*s++] = 1;
	}
	if (count != NULL) *count = num;
	result[num] = 0;
	return result;
}

char **stb_tokens(char *src, char *delimit, int *count)
{
	return stb_tokens_raw(src,delimit,count,0,0,0,0);
}

char **stb_tokens_nested(char *src, char *delimit, int *count, char *nest_in, char *nest_out)
{
	return stb_tokens_raw(src,delimit,count,0,0,nest_in,nest_out);
}

char **stb_tokens_nested_empty(char *src, char *delimit, int *count, char *nest_in, char *nest_out)
{
	return stb_tokens_raw(src,delimit,count,0,1,nest_in,nest_out);
}

char **stb_tokens_allowempty(char *src, char *delimit, int *count)
{
	return stb_tokens_raw(src,delimit,count,0,1,0,0);
}

char **stb_tokens_stripwhite(char *src, char *delimit, int *count)
{
	return stb_tokens_raw(src,delimit,count,1,1,0,0);
}

char **stb_tokens_quoted(char *src, char *delimit, int *count)
{
	return stb_tokens_raw(src,delimit,count,2,1,0,0);
}

char *stb_dupreplace(char *src, char *find, char *replace)
{
	size_t len_find = strlen(find);
	size_t len_replace = strlen(replace);
	int count = 0;

	char *s,*p,*q;

	s = strstr(src, find);
	if (s == NULL) return strdup(src);
	do {
		++count;
		s = strstr(s + len_find, find);
	}while (s != NULL);

	p = (char *) malloc(strlen(src) + count * (len_replace - len_find) + 1);
	if (p == NULL) return p;
	q = p;
	s = src;
	for (;;) {
		char *t = strstr(s, find);
		if (t == NULL) {
			strcpy(q,s);
			assert(strlen(p) == strlen(src) + count*(len_replace-len_find));
			return p;
		}
		memcpy(q, s, t-s);
		q += t-s;
		memcpy(q, replace, len_replace);
		q += len_replace;
		s = t + len_find;
	}
}

void stb_replaceinplace(char *src, char *find, char *replace)
{
	size_t len_find = strlen(find);
	size_t len_replace = strlen(replace);
	int delta;

	char *s,*p,*q;

	delta = len_replace - len_find;
	assert(delta <= 0);
	if (delta > 0) return;

	p = strstr(src, find);
	if (p == NULL) return;

	s = q = p;
	while (*s) {
		memcpy(q, replace, len_replace);
		p += len_find;
		q += len_replace;
		s = strstr(p, find);
		if (s == NULL) s = p + strlen(p);
		memmove(q, p, s-p);
		q += s-p;
		p = s;
	}
	*q = 0;
}

void stb_fixpath(char *path)
{
	for(; *path; ++path)
	if (*path == '\\')
	*path = '/';
}

void stb__add_section(char *buffer, char *data, int curlen, int newlen)
{
	if (newlen < curlen) {
		int z1 = newlen >> 1, z2 = newlen-z1;
		memcpy(buffer, data, z1-1);
		buffer[z1-1] = '.';
		buffer[z1-0] = '.';
		memcpy(buffer+z1+1, data+curlen-z2+1, z2-1);
	} else
	memcpy(buffer, data, curlen);
}

char * stb_shorten_path_readable(char *path, int len)
{
	static char buffer[1024];
	int n = strlen(path),n1,n2,r1,r2;
	char *s;
	if (n <= len) return path;
	if (len > 1024) return path;
	s = stb_strrchr2(path, '/', '\\');
	if (s) {
		n1 = s - path + 1;
		n2 = n - n1;
		++s;
	} else {
		n1 = 0;
		n2 = n;
		s = path;
	}
	// now we need to reduce r1 and r2 so that they fit in len
	if (n1 < len>>1) {
		r1 = n1;
		r2 = len - r1;
	} else if (n2 < len >> 1) {
		r2 = n2;
		r1 = len - r2;
	} else {
		r1 = n1 * len / n;
		r2 = n2 * len / n;
		if (r1 < len>>2) r1 = len>>2, r2 = len-r1;
		if (r2 < len>>2) r2 = len>>2, r1 = len-r2;
	}
	assert(r1 <= n1 && r2 <= n2);
	if (n1)
	stb__add_section(buffer, path, n1, r1);
	stb__add_section(buffer+r1, s, n2, r2);
	buffer[len] = 0;
	return buffer;
}

static char *stb__splitpath_raw(char *buffer, char *path, int flag)
{
	int len=0,x,y, n = (int) strlen(path), f1,f2;
	char *s = stb_strrchr2(path, '/', '\\');
	char *t = strrchr(path, '.');
	if (s && t && t < s) t = NULL;
	if (s) ++s;

	if (flag == STB_EXT_NO_PERIOD)
	flag |= STB_EXT;

	if (!(flag & (STB_PATH | STB_FILE | STB_EXT))) return NULL;

	f1 = s == NULL ? 0 : s-path; // start of filename
	f2 = t == NULL ? n : t-path;// just past end of filename

	if (flag & STB_PATH) {
		x = 0; if (f1 == 0 && flag == STB_PATH) len=2;
	} else if (flag & STB_FILE) {
		x = f1;
	} else {
		x = f2;
		if (flag & STB_EXT_NO_PERIOD)
		if (buffer[x] == '.')
		++x;
	}

	if (flag & STB_EXT)
	y = n;
	else if (flag & STB_FILE)
	y = f2;
	else
	y = f1;

	if (buffer == NULL) {
		buffer = (char *) malloc(y-x + len + 1);
		if (!buffer) return NULL;
	}

	if (len) {strcpy(buffer, "./"); return buffer;}
	strncpy(buffer, path+x, y-x);
	buffer[y-x] = 0;
	return buffer;
}

char *stb_splitpath(char *output, char *src, int flag)
{
	return stb__splitpath_raw(output, src, flag);
}

char *stb_splitpathdup(char *src, int flag)
{
	return stb__splitpath_raw(NULL, src, flag);
}

char *stb_replacedir(char *output, char *src, char *dir)
{
	char buffer[4096];
	stb_splitpath(buffer, src, STB_FILE | STB_EXT);
	if (dir)
	sprintf(output, "%s/%s", dir, buffer);
	else
	strcpy(output, buffer);
	return output;
}

char *stb_replaceext(char *output, char *src, char *ext)
{
	char buffer[4096];
	stb_splitpath(buffer, src, STB_PATH | STB_FILE);
	if (ext)
	sprintf(output, "%s.%s", buffer, ext[0] == '.' ? ext+1 : ext);
	else
	strcpy(output, buffer);
	return output;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//                   stb_alloc - hierarchical allocator
//
//                                     inspired by http://swapped.cc/halloc
//
//
// When you alloc a given block through stb_alloc, you have these choices:
//
//       1. does it have a parent?
//       2. can it have children?
//       3. can it be freed directly?
//       4. is it transferrable?
//       5. what is its alignment?
//
// Here are interesting combinations of those:
//
//                              children   free    transfer     alignment
//  arena                          Y         Y         N           n/a
//  no-overhead, chunked           N         N         N         normal
//  string pool alloc              N         N         N            1
//  parent-ptr, chunked            Y         N         N         normal
//  low-overhead, unchunked        N         Y         Y         normal
//  general purpose alloc          Y         Y         Y         normal
//
// Unchunked allocations will probably return 16-aligned pointers. If
// we 16-align the results, we have room for 4 pointers. For smaller
// allocations that allow finer alignment, we can reduce the pointers.
//
// The strategy is that given a pointer, assuming it has a header (only
// the no-overhead allocations have no header), we can determine the
// type of the header fields, and the number of them, by stepping backwards
// through memory and looking at the tags in the bottom bits.
//
// Implementation strategy:
//     chunked allocations come from the middle of chunks, and can't
//     be freed. thefore they do not need to be on a sibling chain.
//     they may need child pointers if they have children.
//
// chunked, with-children
//     void *parent;
//
// unchunked, no-children -- reduced storage
//     void *next_sibling;
//     void *prev_sibling_nextp;
//
// unchunked, general
//     void *first_child;
//     void *next_sibling;
//     void *prev_sibling_nextp;
//     void *chunks;
//
// so, if we code each of these fields with different bit patterns
// (actually same one for next/prev/child), then we can identify which
// each one is from the last field.

STB_EXTERN void stb_free(void *p);
STB_EXTERN void *stb_malloc_global(size_t size);
STB_EXTERN void *stb_malloc(void *context, size_t size);
STB_EXTERN void *stb_malloc_nofree(void *context, size_t size);
STB_EXTERN void *stb_malloc_leaf(void *context, size_t size);
STB_EXTERN void *stb_malloc_raw(void *context, size_t size);
STB_EXTERN void *stb_realloc(void *ptr, size_t newsize);

STB_EXTERN void stb_reassign(void *new_context, void *ptr);
STB_EXTERN void stb_malloc_validate(void *p, void *parent);

extern int stb_alloc_chunk_size;
extern int stb_alloc_count_free;
extern int stb_alloc_count_alloc;
extern int stb_alloc_alignment;

#ifdef STB_DEFINE

int stb_alloc_chunk_size = 65536;
int stb_alloc_count_free = 0;
int stb_alloc_count_alloc = 0;
int stb_alloc_alignment = -16;

typedef struct stb__chunk
{
	struct stb__chunk *next;
	int data_left;
	int alloc;
}stb__chunk;

typedef struct
{
	void * next;
	void ** prevn;
}stb__nochildren;

typedef struct
{
	void ** prevn;
	void * child;
	void * next;
	stb__chunk *chunks;
}stb__alloc;

typedef struct
{
	stb__alloc *parent;
}stb__chunked;

#define STB__PARENT          1
#define STB__CHUNKS          2

typedef enum
{
	STB__nochildren = 0,
	STB__chunked = STB__PARENT,
	STB__alloc = STB__CHUNKS,

	STB__chunk_raw = 4,
}stb__alloc_type;

// these functions set the bottom bits of a pointer efficiently
#define STB__DECODE(x,v)  ((void *) ((char *) (x) - (v)))
#define STB__ENCODE(x,v)  ((void *) ((char *) (x) + (v)))

#define stb__parent(z)       (stb__alloc *) STB__DECODE((z)->parent, STB__PARENT)
#define stb__chunks(z)       (stb__chunk *) STB__DECODE((z)->chunks, STB__CHUNKS)

#define stb__setparent(z,p)  (z)->parent = (stb__alloc *) STB__ENCODE((p), STB__PARENT)
#define stb__setchunks(z,c)  (z)->chunks = (stb__chunk *) STB__ENCODE((c), STB__CHUNKS)

static stb__alloc stb__alloc_global =
{
	NULL,
	NULL,
	NULL,
	(stb__chunk *) STB__ENCODE(NULL, STB__CHUNKS)
};

static stb__alloc_type stb__identify(void *p)
{
	void **q = (void **) p;
	return (stb__alloc_type) ((stb_uinta) q[-1] & 3);
}

static void *** stb__prevn(void *p)
{
	if (stb__identify(p) == STB__alloc) {
		stb__alloc *s = (stb__alloc *) p - 1;
		return &s->prevn;
	} else {
		stb__nochildren *s = (stb__nochildren *) p - 1;
		return &s->prevn;
	}
}

void stb_free(void *p)
{
	if (p == NULL) return;

	// count frees so that unit tests can see what's happening
	++stb_alloc_count_free;

	switch(stb__identify(p)) {
		case STB__chunked:
		// freeing a chunked-block with children does nothing;
		// they only get freed when the parent does
		// surely this is wrong, and it should free them immediately?
		// otherwise how are they getting put on the right chain?
		return;
		case STB__nochildren: {
			stb__nochildren *s = (stb__nochildren *) p - 1;
			// unlink from sibling chain
			*(s->prevn) = s->next;
			if (s->next)
			*stb__prevn(s->next) = s->prevn;
			free(s);
			return;
		}
		case STB__alloc: {
			stb__alloc *s = (stb__alloc *) p - 1;
			stb__chunk *c, *n;
			void *q;

			// unlink from sibling chain, if any
			*(s->prevn) = s->next;
			if (s->next)
			*stb__prevn(s->next) = s->prevn;

			// first free chunks
			c = (stb__chunk *) stb__chunks(s);
			while (c != NULL) {
				n = c->next;
				stb_alloc_count_free += c->alloc;
				free(c);
				c = n;
			}

			// validating
			stb__setchunks(s,NULL);
			s->prevn = NULL;
			s->next = NULL;

			// now free children
			while ((q = s->child) != NULL) {
				stb_free(q);
			}

			// now free self
			free(s);
			return;
		}
		default:
		assert(0); /* NOTREACHED */
	}
}

void stb_malloc_validate(void *p, void *parent)
{
	if (p == NULL) return;

	switch(stb__identify(p)) {
		case STB__chunked:
		return;
		case STB__nochildren: {
			stb__nochildren *n = (stb__nochildren *) p - 1;
			if (n->prevn)
			assert(*n->prevn == p);
			if (n->next) {
				assert(*stb__prevn(n->next) == &n->next);
				stb_malloc_validate(n, parent);
			}
			return;
		}
		case STB__alloc: {
			stb__alloc *s = (stb__alloc *) p - 1;

			if (s->prevn)
			assert(*s->prevn == p);

			if (s->child) {
				assert(*stb__prevn(s->child) == &s->child);
				stb_malloc_validate(s->child, p);
			}

			if (s->next) {
				assert(*stb__prevn(s->next) == &s->next);
				stb_malloc_validate(s->next, parent);
			}
			return;
		}
		default:
		assert(0); /* NOTREACHED */
	}
}

static void * stb__try_chunk(stb__chunk *c, int size, int align, int pre_align)
{
	char *memblock = (char *) (c+1), *q;
	stb_inta iq;
	int start_offset;

	// we going to allocate at the end of the chunk, not the start. confusing,
	// but it means we don't need both a 'limit' and a 'cur', just a 'cur'.
	// the block ends at: p + c->data_left
	//   then we move back by size
	start_offset = c->data_left - size;

	// now we need to check the alignment of that
	q = memblock + start_offset;
	iq = (stb_inta) q;
	assert(sizeof(q) == sizeof(iq));

	// suppose align = 2
	// then we need to retreat iq far enough that (iq & (2-1)) == 0
	// to get (iq & (align-1)) = 0 requires subtracting (iq & (align-1))

	start_offset -= iq & (align-1);
	assert(((stb_uinta) (memblock+start_offset) & (align-1)) == 0);

	// now, if that + pre_align works, go for it!
	start_offset -= pre_align;

	if (start_offset >= 0) {
		c->data_left = start_offset;
		return memblock + start_offset;
	}

	return NULL;
}

static void stb__sort_chunks(stb__alloc *src)
{
	// of the first two chunks, put the chunk with more data left in it first
	stb__chunk *c = stb__chunks(src), *d;
	if (c == NULL) return;
	d = c->next;
	if (d == NULL) return;
	if (c->data_left > d->data_left) return;

	c->next = d->next;
	d->next = c;
	stb__setchunks(src, d);
}

static void * stb__alloc_chunk(stb__alloc *src, int size, int align, int pre_align)
{
	void *p;
	stb__chunk *c = stb__chunks(src);

	if (c && size <= stb_alloc_chunk_size) {

		p = stb__try_chunk(c, size, align, pre_align);
		if (p) {++c->alloc; return p;}

		// try a second chunk to reduce wastage
		if (c->next) {
			p = stb__try_chunk(c->next, size, align, pre_align);
			if (p) {++c->alloc; return p;}

			// put the bigger chunk first, since the second will get buried
			// the upshot of this is that, until it gets allocated from, chunk #2
			// is always the largest remaining chunk. (could formalize
			// this with a heap!)
			stb__sort_chunks(src);
			c = stb__chunks(src);
		}
	}

	// allocate a new chunk
	{
		stb__chunk *n;

		int chunk_size = stb_alloc_chunk_size;
		// we're going to allocate a new chunk to put this in
		if (size > chunk_size)
		chunk_size = size;

		assert(sizeof(*n) + pre_align <= 16);

		// loop trying to allocate a large enough chunk
		// the loop is because the alignment may cause problems if it's big...
		// and we don't know what our chunk alignment is going to be
		while (1) {
			n = (stb__chunk *) malloc(16 + chunk_size);
			if (n == NULL) return NULL;

			n->data_left = chunk_size - sizeof(*n);

			p = stb__try_chunk(n, size, align, pre_align);
			if (p != NULL) {
				n->next = c;
				stb__setchunks(src, n);

				// if we just used up the whole block immediately,
				// move the following chunk up
				n->alloc = 1;
				if (size == chunk_size)
				stb__sort_chunks(src);

				return p;
			}

			free(n);
			chunk_size += 16+align;
		}
	}
}

static stb__alloc * stb__get_context(void *context)
{
	if (context == NULL) {
		return &stb__alloc_global;
	} else {
		int u = stb__identify(context);
		// if context is chunked, grab parent
		if (u == STB__chunked) {
			stb__chunked *s = (stb__chunked *) context - 1;
			return stb__parent(s);
		} else {
			return (stb__alloc *) context - 1;
		}
	}
}

static void stb__insert_alloc(stb__alloc *src, stb__alloc *s)
{
	s->prevn = &src->child;
	s->next = src->child;
	src->child = s+1;
	if (s->next)
	*stb__prevn(s->next) = &s->next;
}

static void stb__insert_nochild(stb__alloc *src, stb__nochildren *s)
{
	s->prevn = &src->child;
	s->next = src->child;
	src->child = s+1;
	if (s->next)
	*stb__prevn(s->next) = &s->next;
}

static void * malloc_base(void *context, size_t size, stb__alloc_type t, int align)
{
	void *p;

	stb__alloc *src = stb__get_context(context);

	if (align <= 0) {
		// compute worst-case C packed alignment
		// e.g. a 24-byte struct is 8-aligned
		int align_proposed = 1 << stb_lowbit8(size);

		if (align_proposed < 0)
		align_proposed = 4;

		if (align_proposed == 0) {
			if (size == 0)
			align_proposed = 1;
			else
			align_proposed = 256;
		}

		// a negative alignment means 'don't align any larger
		// than this'; so -16 means we align 1,2,4,8, or 16

		if (align < 0) {
			if (align_proposed > -align)
			align_proposed = -align;
		}

		align = align_proposed;
	}

	assert(stb_is_pow2(align));

	// don't cause misalignment when allocating nochildren
	if (t == STB__nochildren && align > 8)
	t = STB__alloc;

	switch (t) {
		case STB__alloc: {
			stb__alloc *s = (stb__alloc *) malloc(size + sizeof(*s));
			if (s == NULL) return NULL;
			p = s+1;
			s->child = NULL;
			stb__insert_alloc(src, s);

			stb__setchunks(s,NULL);
			break;
		}

		case STB__nochildren: {
			stb__nochildren *s = (stb__nochildren *) malloc(size + sizeof(*s));
			if (s == NULL) return NULL;
			p = s+1;
			stb__insert_nochild(src, s);
			break;
		}

		case STB__chunk_raw: {
			p = stb__alloc_chunk(src, size, align, 0);
			if (p == NULL) return NULL;
			break;
		}

		case STB__chunked: {
			stb__chunked *s;
			if (align < sizeof(stb_uintptr)) align = sizeof(stb_uintptr);
			s = (stb__chunked *) stb__alloc_chunk(src, size, align, sizeof(*s));
			if (s == NULL) return NULL;
			stb__setparent(s, src);
			p = s+1;
			break;
		}

		default: p = NULL; assert(0); /* NOTREACHED */
	}

	++stb_alloc_count_alloc;
	return p;
}

void *stb_malloc_global(size_t size)
{
	return malloc_base(NULL, size, STB__alloc, stb_alloc_alignment);
}

void *stb_malloc(void *context, size_t size)
{
	return malloc_base(context, size, STB__alloc, stb_alloc_alignment);
}

void *stb_malloc_nofree(void *context, size_t size)
{
	return malloc_base(context, size, STB__chunked, stb_alloc_alignment);
}

void *stb_malloc_leaf(void *context, size_t size)
{
	return malloc_base(context, size, STB__nochildren, stb_alloc_alignment);
}

void *stb_malloc_raw(void *context, size_t size)
{
	return malloc_base(context, size, STB__chunk_raw, stb_alloc_alignment);
}

char *stb_malloc_string(void *context, size_t size)
{
	return (char *) malloc_base(context, size, STB__chunk_raw, 1);
}

void *stb_realloc(void *ptr, size_t newsize)
{
	stb__alloc_type t;

	if (ptr == NULL) return stb_malloc(NULL, newsize);
	if (newsize == 0) {stb_free(ptr); return NULL;}

	t = stb__identify(ptr);
	assert(t == STB__alloc || t == STB__nochildren);

	if (t == STB__alloc) {
		stb__alloc *s = (stb__alloc *) ptr - 1;

		s = (stb__alloc *) realloc(s, newsize + sizeof(*s));
		if (s == NULL) return NULL;

		ptr = s+1;

		// update pointers
		(*s->prevn) = ptr;
		if (s->next)
		*stb__prevn(s->next) = &s->next;

		if (s->child)
		*stb__prevn(s->child) = &s->child;

		return ptr;
	} else {
		stb__nochildren *s = (stb__nochildren *) ptr - 1;

		s = (stb__nochildren *) realloc(ptr, newsize + sizeof(s));
		if (s == NULL) return NULL;

		// update pointers
		(*s->prevn) = s+1;
		if (s->next)
		*stb__prevn(s->next) = &s->next;

		return s+1;
	}
}

void *stb_realloc_c(void *context, void *ptr, size_t newsize)
{
	if (ptr == NULL) return stb_malloc(context, newsize);
	if (newsize == 0) {stb_free(ptr); return NULL;}
	// @TODO: verify you haven't changed contexts
	return stb_realloc(ptr, newsize);
}

void stb_reassign(void *new_context, void *ptr)
{
	stb__alloc *src = stb__get_context(new_context);

	stb__alloc_type t = stb__identify(ptr);
	assert(t == STB__alloc || t == STB__nochildren);

	if (t == STB__alloc) {
		stb__alloc *s = (stb__alloc *) ptr - 1;

		// unlink from old
		*(s->prevn) = s->next;
		if (s->next)
		*stb__prevn(s->next) = s->prevn;

		stb__insert_alloc(src, s);
	} else {
		stb__nochildren *s = (stb__nochildren *) ptr - 1;

		// unlink from old
		*(s->prevn) = s->next;
		if (s->next)
		*stb__prevn(s->next) = s->prevn;

		stb__insert_nochild(src, s);
	}
}

#endif

//////////////////////////////////////////////////////////////////////////////
//
//                                stb_arr
//
//  An stb_arr is directly useable as a pointer (use the actual type in your
//  definition), but when it resizes, it returns a new pointer and you can't
//  use the old one, so you have to be careful to copy-in-out as necessary.
//
//  Use a NULL pointer as a 0-length array.
//
//     float *my_array = NULL, *temp;
//
//     // add elements on the end one at a time
//     stb_arr_push(my_array, 0.0f);
//     stb_arr_push(my_array, 1.0f);
//     stb_arr_push(my_array, 2.0f);
//
//     assert(my_array[1] == 2.0f);
//
//     // add an uninitialized element at the end, then assign it
//     *stb_arr_add(my_array) = 3.0f;
//
//     // add three uninitialized elements at the end
//     temp = stb_arr_addn(my_array,3);
//     temp[0] = 4.0f;
//     temp[1] = 5.0f;
//     temp[2] = 6.0f;
//
//     assert(my_array[5] == 5.0f);
//
//     // remove the last one
//     stb_arr_pop(my_array);
//
//     assert(stb_arr_len(my_array) == 6);

#ifdef STB_MALLOC_WRAPPER
#define STB__PARAMS    , char *file, int line
#define STB__ARGS      ,       file,     line
#else
#define STB__PARAMS
#define STB__ARGS
#endif

// calling this function allocates an empty stb_arr attached to p
// (whereas NULL isn't attached to anything)
STB_EXTERN void stb_arr_malloc(void **target, void *context);

// call this function with a non-NULL value to have all successive
// stbs that are created be attached to the associated parent. Note
// that once a given stb_arr is non-empty, it stays attached to its
// current parent, even if you call this function again.
// it turns the previous value, so you can restore it
STB_EXTERN void* stb_arr_malloc_parent(void *p);

// simple functions written on top of other functions
#define stb_arr_empty(a)       (  stb_arr_len(a) == 0 )
#define stb_arr_add(a)         (  stb_arr_addn((a),1) )
#define stb_arr_push(a,v)      ( *stb_arr_add(a)=(v)  )

typedef struct {
	int len, limit;
	int stb_malloc;
	unsigned int signature;
} stb__arr;

#define stb_arr_signature      0x51bada7b  // ends with 0123 in decimal

// access the header block stored before the data
#define stb_arrhead(a)         /*lint --e(826)*/ (((stb__arr *) (a)) - 1)
#define stb_arrhead2(a)        /*lint --e(826)*/ (((stb__arr *) (a)) - 1)

#ifdef STB_DEBUG
#define stb_arr_check(a)       assert(!a || stb_arrhead(a)->signature == stb_arr_signature)
#define stb_arr_check2(a)      assert(!a || stb_arrhead2(a)->signature == stb_arr_signature)
#else
#define stb_arr_check(a)       ((void) 0)
#define stb_arr_check2(a)      ((void) 0)
#endif

// ARRAY LENGTH

// get the array length; special case if pointer is NULL
#define stb_arr_len(a)         (a ? stb_arrhead(a)->len : 0)
#define stb_arr_len2(a)        ((stb__arr *) (a) ? stb_arrhead2(a)->len : 0)
#define stb_arr_lastn(a)       (stb_arr_len(a)-1)

// check whether a given index is valid -- tests 0 <= i < stb_arr_len(a)
#define stb_arr_valid(a,i)     (a ? (int) (i) < stb_arrhead(a)->len : 0)

// change the array length so is is exactly N entries long, creating
// uninitialized entries as needed
#define stb_arr_setlen(a,n)  \
            (stb__arr_setlen((void **) &(a), sizeof(a[0]), (n)))

// change the array length so that N is a valid index (that is, so
// it is at least N entries long), creating uninitialized entries as needed
#define stb_arr_makevalid(a,n)  \
            (stb_arr_len(a) < (n)+1 ? stb_arr_setlen(a,(n)+1),(a) : (a))

// remove the last element of the array, returning it
#define stb_arr_pop(a)         ((stb_arr_check(a), (a))[--stb_arrhead(a)->len])

// access the last element in the array
#define stb_arr_last(a)        ((stb_arr_check(a), (a))[stb_arr_len(a)-1])

// is iterator at end of list?
#define stb_arr_end(a,i)       ((i) >= &(a)[stb_arr_len(a)])

// (internal) change the allocated length of the array
#define stb_arr__grow(a,n)     (stb_arr_check(a), stb_arrhead(a)->len += (n))

// add N new unitialized elements to the end of the array
#define stb_arr__addn(a,n)     /*lint --e(826)*/ \
                               ((stb_arr_len(a)+(n) > stb_arrcurmax(a))      \
                                 ? (stb__arr_addlen((void **) &(a),sizeof(*a),(n)),0) \
                                 : ((stb_arr__grow(a,n), 0)))

// add N new unitialized elements to the end of the array, and return
// a pointer to the first new one
#define stb_arr_addn(a,n)      (stb_arr__addn((a),n),(a)+stb_arr_len(a)-(n))

// add N new uninitialized elements starting at index 'i'
#define stb_arr_insertn(a,i,n) (stb__arr_insertn((void **) &(a), sizeof(*a), i, n))

// insert an element at i
#define stb_arr_insert(a,i,v)  (stb__arr_insertn((void **) &(a), sizeof(*a), i, 1), ((a)[i] = v))

// delete N elements from the middle starting at index 'i'
#define stb_arr_deleten(a,i,n) (stb__arr_deleten((void **) &(a), sizeof(*a), i, n))

// delete the i'th element
#define stb_arr_delete(a,i)   stb_arr_deleten(a,i,1)

// delete the i'th element, swapping down from the end
#define stb_arr_fastdelete(a,i)  \
   (stb_swap(&a[i], &a[stb_arrhead(a)->len-1], sizeof(*a)), stb_arr_pop(a))

// ARRAY STORAGE

// get the array maximum storage; special case if NULL
#define stb_arrcurmax(a)       (a ? stb_arrhead(a)->limit : 0)
#define stb_arrcurmax2(a)      (a ? stb_arrhead2(a)->limit : 0)

// set the maxlength of the array to n in anticipation of further growth
#define stb_arr_setsize(a,n)   (stb_arr_check(a), stb__arr_setsize((void **) &(a),sizeof((a)[0]),n))

// make sure maxlength is large enough for at least N new allocations
#define stb_arr_atleast(a,n)   (stb_arr_len(a)+(n) > stb_arrcurmax(a)      \
                                 ? stb_arr_setsize((a), (n)) : 0)

// make a copy of a given array (copies contents via 'memcpy'!)
#define stb_arr_copy(a)        stb__arr_copy(a, sizeof((a)[0]))

// compute the storage needed to store all the elements of the array
#define stb_arr_storage(a)     (stb_arr_len(a) * sizeof((a)[0]))

#define stb_arr_for(v,arr)     for((v)=(arr); (v) < (arr)+stb_arr_len(arr); ++(v))

// IMPLEMENTATION

STB_EXTERN void stb_arr_free_(void **p);
STB_EXTERN void *stb__arr_copy_(void *p, int elem_size);
STB_EXTERN void stb__arr_setsize_(void **p, int size, int limit STB__PARAMS);
STB_EXTERN void stb__arr_setlen_(void **p, int size, int newlen STB__PARAMS);
STB_EXTERN void stb__arr_addlen_(void **p, int size, int addlen STB__PARAMS);
STB_EXTERN void stb__arr_deleten_(void **p, int size, int loc,
		int n STB__PARAMS);
STB_EXTERN void stb__arr_insertn_(void **p, int size, int loc,
		int n STB__PARAMS);

#define stb_arr_free(p)            stb_arr_free_((void **) &(p))
#define stb__arr_copy              stb__arr_copy_

#ifndef STB_MALLOC_WRAPPER
#define stb__arr_setsize         stb__arr_setsize_
#define stb__arr_setlen          stb__arr_setlen_
#define stb__arr_addlen          stb__arr_addlen_
#define stb__arr_deleten         stb__arr_deleten_
#define stb__arr_insertn         stb__arr_insertn_
#else
#define stb__arr_addlen(p,s,n)    stb__arr_addlen_(p,s,n,__FILE__,__LINE__)
#define stb__arr_setlen(p,s,n)    stb__arr_setlen_(p,s,n,__FILE__,__LINE__)
#define stb__arr_setsize(p,s,n)   stb__arr_setsize_(p,s,n,__FILE__,__LINE__)
#define stb__arr_deleten(p,s,i,n) stb__arr_deleten_(p,s,i,n,__FILE__,__LINE__)
#define stb__arr_insertn(p,s,i,n) stb__arr_insertn_(p,s,i,n,__FILE__,__LINE__)
#endif

#ifdef STB_DEFINE
static void *stb__arr_context;

void *stb_arr_malloc_parent(void *p)
{
	void *q = stb__arr_context;
	stb__arr_context = p;
	return q;
}

void stb_arr_malloc(void **target, void *context)
{
	stb__arr *q = (stb__arr *) stb_malloc(context, sizeof(*q));
	q->len = q->limit = 0;
	q->stb_malloc = 1;
	q->signature = stb_arr_signature;
	*target = (void *) (q+1);
}

static void * stb__arr_malloc(int size)
{
	if (stb__arr_context)
	return stb_malloc(stb__arr_context, size);
	return malloc(size);
}

void * stb__arr_copy_(void *p, int elem_size)
{
	stb__arr *q;
	if (p == NULL) return p;
	q = (stb__arr *) stb__arr_malloc(sizeof(*q) + elem_size * stb_arrhead2(p)->limit);
	stb_arr_check2(p);
	memcpy(q, stb_arrhead2(p), sizeof(*q) + elem_size * stb_arrhead2(p)->len);
	q->stb_malloc = !!stb__arr_context;
	return q+1;
}

void stb_arr_free_(void **pp)
{
	void *p = *pp;
	stb_arr_check2(p);
	if (p) {
		stb__arr *q = stb_arrhead2(p);
		if (q->stb_malloc)
		stb_free(q);
		else
		free(q);
	}
	*pp = NULL;
}

static void stb__arrsize_(void **pp, int size, int limit, int len STB__PARAMS)
{
	void *p = *pp;
	stb__arr *a;
	stb_arr_check2(p);
	if (p == NULL) {
		if (len == 0 && size == 0) return;
		a = (stb__arr *) stb__arr_malloc(sizeof(*a) + size*limit);
		a->limit = limit;
		a->len = len;
		a->stb_malloc = !!stb__arr_context;
		a->signature = stb_arr_signature;
	} else {
		a = stb_arrhead2(p);
		a->len = len;
		if (a->limit < limit) {
			void *p;
			if (a->limit >= 4 && limit < a->limit * 2)
			limit = a->limit * 2;
			if (a->stb_malloc)
			p = stb_realloc(a, sizeof(*a) + limit*size);
			else
#ifdef STB_MALLOC_WRAPPER
			p = stb__realloc(a, sizeof(*a) + limit*size, file, line);
#else
			p = realloc(a, sizeof(*a) + limit*size);
#endif
			if (p) {
				a = (stb__arr *) p;
				a->limit = limit;
			} else {
				// throw an error!
			}
		}
	}
	a->len = stb_min(a->len, a->limit);
	*pp = a+1;
}

void stb__arr_setsize_(void **pp, int size, int limit STB__PARAMS)
{
	void *p = *pp;
	stb_arr_check2(p);
	stb__arrsize_(pp, size, limit, stb_arr_len2(p) STB__ARGS);
}

void stb__arr_setlen_(void **pp, int size, int newlen STB__PARAMS)
{
	void *p = *pp;
	stb_arr_check2(p);
	if (stb_arrcurmax2(p) < newlen || p == NULL) {
		stb__arrsize_(pp, size, newlen, newlen STB__ARGS);
	} else {
		stb_arrhead2(p)->len = newlen;
	}
}

void stb__arr_addlen_(void **p, int size, int addlen STB__PARAMS)
{
	stb__arr_setlen_(p, size, stb_arr_len2(*p) + addlen STB__ARGS);
}

void stb__arr_insertn_(void **pp, int size, int i, int n STB__PARAMS)
{
	void *p = *pp;
	if (n) {
		int z;

		if (p == NULL) {
			stb__arr_addlen_(pp, size, n STB__ARGS);
			return;
		}

		z = stb_arr_len2(p);
		stb__arr_addlen_(&p, size, n STB__ARGS);
		memmove((char *) p + (i+n)*size, (char *) p + i*size, size * (z-i));
	}
	*pp = p;
}

void stb__arr_deleten_(void **pp, int size, int i, int n STB__PARAMS)
{
	void *p = *pp;
	if (n) {
		memmove((char *) p + i*size, (char *) p + (i+n)*size, size * (stb_arr_len2(p)-(i+n)));
		stb_arrhead2(p)->len -= n;
	}
	*pp = p;
}

#endif

//////////////////////////////////////////////////////////////////////////////
//
//                               Hashing
//
//      typical use for this is to make a power-of-two hash table.
//
//      let N = size of table (2^n)
//      let H = stb_hash(str)
//      let S = stb_rehash(H) | 1
//
//      then hash probe sequence P(i) for i=0..N-1
//         P(i) = (H + S*i) & (N-1)
//
//      the idea is that H has 32 bits of hash information, but the
//      table has only, say, 2^20 entries so only uses 20 of the bits.
//      then by rehashing the original H we get 2^12 different probe
//      sequences for a given initial probe location. (So it's optimal
//      for 64K tables and its optimality decreases past that.)
//
//      ok, so I've added something that generates _two separate_
//      32-bit hashes simultaneously which should scale better to
//      very large tables.

STB_EXTERN unsigned int stb_hash(char *str);
STB_EXTERN unsigned int stb_hashptr(void *p);
STB_EXTERN unsigned int stb_hashlen(char *str, int len);
STB_EXTERN unsigned int stb_rehash_improved(unsigned int v);
STB_EXTERN unsigned int stb_hash_fast(void *p, int len);
STB_EXTERN unsigned int stb_hash2(char *str, unsigned int *hash2_ptr);
STB_EXTERN unsigned int stb_hash_number(unsigned int hash);

#define stb_rehash(x)  ((x) + ((x) >> 6) + ((x) >> 19))

#ifdef STB_DEFINE
unsigned int stb_hash(char *str)
{
	unsigned int hash = 0;
	while (*str)
	hash = (hash << 7) + (hash >> 25) + *str++;
	return hash + (hash >> 16);
}

unsigned int stb_hashlen(char *str, int len)
{
	unsigned int hash = 0;
	while (len-- > 0 && *str)
	hash = (hash << 7) + (hash >> 25) + *str++;
	return hash + (hash >> 16);
}

unsigned int stb_hashptr(void *p)
{
	unsigned int x = (unsigned int) p;

	// typically lacking in low bits and high bits
	x = stb_rehash(x);
	x += x << 16;

	// pearson's shuffle
	x ^= x << 3;
	x += x >> 5;
	x ^= x << 2;
	x += x >> 15;
	x ^= x << 10;
	return stb_rehash(x);
}

unsigned int stb_rehash_improved(unsigned int v)
{
	return stb_hashptr((void *)(size_t) v);
}

unsigned int stb_hash2(char *str, unsigned int *hash2_ptr)
{
	unsigned int hash1 = 0x3141592c;
	unsigned int hash2 = 0x77f044ed;
	while (*str) {
		hash1 = (hash1 << 7) + (hash1 >> 25) + *str;
		hash2 = (hash2 << 11) + (hash2 >> 21) + *str;
		++str;
	}
	*hash2_ptr = hash2 + (hash1 >> 16);
	return hash1 + (hash2 >> 16);
}

// Paul Hsieh hash
#define stb__get16_slow(p) ((p)[0] + ((p)[1] << 8))
#if defined(_MSC_VER)
#define stb__get16(p) (*((unsigned short *) (p)))
#else
#define stb__get16(p) stb__get16_slow(p)
#endif

unsigned int stb_hash_fast(void *p, int len)
{
	unsigned char *q = (unsigned char *) p;
	unsigned int hash = len;

	if (len <= 0 || q == NULL) return 0;

	/* Main loop */
	if (((int) q & 1) == 0) {
		for (;len > 3; len -= 4) {
			unsigned int val;
			hash += stb__get16(q);
			val = (stb__get16(q+2) << 11);
			hash = (hash << 16) ^ hash ^ val;
			q += 4;
			hash += hash >> 11;
		}
	} else {
		for (;len > 3; len -= 4) {
			unsigned int val;
			hash += stb__get16_slow(q);
			val = (stb__get16_slow(q+2) << 11);
			hash = (hash << 16) ^ hash ^ val;
			q += 4;
			hash += hash >> 11;
		}
	}

	/* Handle end cases */
	switch (len) {
		case 3: hash += stb__get16_slow(q);
		hash ^= hash << 16;
		hash ^= q[2] << 18;
		hash += hash >> 11;
		break;
		case 2: hash += stb__get16_slow(q);
		hash ^= hash << 11;
		hash += hash >> 17;
		break;
		case 1: hash += q[0];
		hash ^= hash << 10;
		hash += hash >> 1;
		break;
		case 0: break;
	}

	/* Force "avalanching" of final 127 bits */
	hash ^= hash << 3;
	hash += hash >> 5;
	hash ^= hash << 4;
	hash += hash >> 17;
	hash ^= hash << 25;
	hash += hash >> 6;

	return hash;
}

unsigned int stb_hash_number(unsigned int hash)
{
	hash ^= hash << 3;
	hash += hash >> 5;
	hash ^= hash << 4;
	hash += hash >> 17;
	hash ^= hash << 25;
	hash += hash >> 6;
	return hash;
}

#endif

//////////////////////////////////////////////////////////////////////////////
//
//                     Perfect hashing for ints/pointers
//
//   This is mainly useful for making faster pointer-indexed tables
//   that don't change frequently. E.g. for stb_ischar().
//

typedef struct {
	stb_uint32 addend;
	stb_uint multiplicand;
	stb_uint b_mask;
	stb_uint8 small_bmap[16];
	stb_uint16 *large_bmap;

	stb_uint table_mask;
	stb_uint32 *table;
} stb_perfect;

STB_EXTERN int stb_perfect_create(stb_perfect *, unsigned int*, int n);
STB_EXTERN void stb_perfect_destroy(stb_perfect *);
STB_EXTERN int stb_perfect_hash(stb_perfect *, unsigned int x);
extern int stb_perfect_hash_max_failures;

#ifdef STB_DEFINE

int stb_perfect_hash_max_failures;

int stb_perfect_hash(stb_perfect *p, unsigned int x)
{
	stb_uint m = x * p->multiplicand;
	stb_uint y = x >> 16;
	stb_uint bv = (m >> 24) + y;
	stb_uint av = (m + y) >> 12;
	if (p->table == NULL) return -1;  // uninitialized table fails
	bv &= p->b_mask;
	av &= p->table_mask;
	if (p->large_bmap)
	av ^= p->large_bmap[bv];
	else
	av ^= p->small_bmap[bv];
	return p->table[av] == x ? av : -1;
}

static void stb__perfect_prehash(stb_perfect *p, stb_uint x, stb_uint16 *a, stb_uint16 *b)
{
	stb_uint m = x * p->multiplicand;
	stb_uint y = x >> 16;
	stb_uint bv = (m >> 24) + y;
	stb_uint av = (m + y) >> 12;
	bv &= p->b_mask;
	av &= p->table_mask;
	*b = bv;
	*a = av;
}

static unsigned long stb__perfect_rand(void)
{
	static unsigned long stb__rand;
	stb__rand = stb__rand * 2147001325 + 715136305;
	return 0x31415926 ^ ((stb__rand >> 16) + (stb__rand << 16));
}

typedef struct {
	unsigned short count;
	unsigned short b;
	unsigned short map;
	unsigned short *entries;
}stb__slot;

static int stb__slot_compare(const void *p, const void *q)
{
	stb__slot *a = (stb__slot *) p;
	stb__slot *b = (stb__slot *) q;
	return a->count > b->count ? -1 : a->count < b->count; // sort large to small
}

int stb_perfect_create(stb_perfect *p, unsigned int *v, int n)
{
	unsigned int buffer1[64], buffer2[64], buffer3[64], buffer4[64], buffer5[32];
	unsigned short *as = (unsigned short *) stb_temp(buffer1, sizeof(*v)*n);
	unsigned short *bs = (unsigned short *) stb_temp(buffer2, sizeof(*v)*n);
	unsigned short *entries = (unsigned short *) stb_temp(buffer4, sizeof(*entries) * n);
	int size = 1 << stb_log2_ceil(n), bsize=8;
	int failure = 0,i,j,k;

	assert(n <= 32768);
	p->large_bmap = NULL;

	for(;;) {
		stb__slot *bcount = (stb__slot *) stb_temp(buffer3, sizeof(*bcount) * bsize);
		unsigned short *bloc = (unsigned short *) stb_temp(buffer5, sizeof(*bloc) * bsize);
		unsigned short *e;
		int bad=0;

		p->addend = stb__perfect_rand();
		p->multiplicand = stb__perfect_rand() | 1;
		p->table_mask = size-1;
		p->b_mask = bsize-1;
		p->table = (stb_uint32 *) malloc(size * sizeof(*p->table));

		for (i=0; i < bsize; ++i) {
			bcount[i].b = i;
			bcount[i].count = 0;
			bcount[i].map = 0;
		}
		for (i=0; i < n; ++i) {
			stb__perfect_prehash(p, v[i], as+i, bs+i);
			++bcount[bs[i]].count;
		}
		qsort(bcount, bsize, sizeof(*bcount), stb__slot_compare);
		e = entries; // now setup up their entries index
		for (i=0; i < bsize; ++i) {
			bcount[i].entries = e;
			e += bcount[i].count;
			bcount[i].count = 0;
			bloc[bcount[i].b] = i;
		}
		// now fill them out
		for (i=0; i < n; ++i) {
			int b = bs[i];
			int w = bloc[b];
			bcount[w].entries[bcount[w].count++] = i;
		}
		stb_tempfree(buffer5,bloc);
		// verify
		for (i=0; i < bsize; ++i)
		for (j=0; j < bcount[i].count; ++j)
		assert(bs[bcount[i].entries[j]] == bcount[i].b);
		memset(p->table, 0, size*sizeof(*p->table));

		// check if any b has duplicate a
		for (i=0; i < bsize; ++i) {
			if (bcount[i].count > 1) {
				for (j=0; j < bcount[i].count; ++j) {
					if (p->table[as[bcount[i].entries[j]]])
					bad = 1;
					p->table[as[bcount[i].entries[j]]] = 1;
				}
				for (j=0; j < bcount[i].count; ++j) {
					p->table[as[bcount[i].entries[j]]] = 0;
				}
				if (bad) break;
			}
		}

		if (!bad) {
			// go through the bs and populate the table, first fit
			for (i=0; i < bsize; ++i) {
				if (bcount[i].count) {
					// go through the candidate table[b] values
					for (j=0; j < size; ++j) {
						// go through the a values and see if they fit
						for (k=0; k < bcount[i].count; ++k) {
							int a = as[bcount[i].entries[k]];
							if (p->table[(a^j)&p->table_mask]) {
								break; // fails
							}
						}
						// if succeeded, accept
						if (k == bcount[i].count) {
							bcount[i].map = j;
							for (k=0; k < bcount[i].count; ++k) {
								int a = as[bcount[i].entries[k]];
								p->table[(a^j)&p->table_mask] = 1;
							}
							break;
						}
					}
					if (j == size)
					break; // no match for i'th entry, so break out in failure
				}
			}
			if (i == bsize) {
				// success... fill out map
				if (bsize <= 16 && size <= 256) {
					p->large_bmap = NULL;
					for (i=0; i < bsize; ++i)
					p->small_bmap[bcount[i].b] = (stb_uint8) bcount[i].map;
				} else {
					p->large_bmap = (unsigned short *) malloc(sizeof(*p->large_bmap) * bsize);
					for (i=0; i < bsize; ++i)
					p->large_bmap[bcount[i].b] = bcount[i].map;
				}

				// initialize table to v[0], so empty slots will fail
				for (i=0; i < size; ++i)
				p->table[i] = v[0];

				for (i=0; i < n; ++i)
				if (p->large_bmap)
				p->table[as[i] ^ p->large_bmap[bs[i]]] = v[i];
				else
				p->table[as[i] ^ p->small_bmap[bs[i]]] = v[i];

				// and now validate that none of them collided
				for (i=0; i < n; ++i)
				assert(stb_perfect_hash(p, v[i]) >= 0);

				stb_tempfree(buffer3, bcount);
				break;
			}
		}
		free(p->table);
		p->table = NULL;
		stb_tempfree(buffer3, bcount);

		++failure;
		if (failure >= 4 && bsize < size) bsize *= 2;
		if (failure >= 8 && (failure & 3) == 0 && size < 4*n) {
			size *= 2;
			bsize *= 2;
		}
		if (failure == 6) {
			// make sure the input data is unique, so we don't infinite loop
			unsigned int *data = (unsigned int *) stb_temp(buffer3, n * sizeof(*data));
			memcpy(data, v, sizeof(*data) * n);
			qsort(data, n, sizeof(*data), stb_intcmp(0));
			for (i=1; i < n; ++i) {
				if (data[i] == data[i-1])
				size = 0; // size is return value, so 0 it
			}
			stb_tempfree(buffer3, data);
			if (!size) break;
		}
	}

	if (failure > stb_perfect_hash_max_failures)
	stb_perfect_hash_max_failures = failure;

	stb_tempfree(buffer1, as);
	stb_tempfree(buffer2, bs);
	stb_tempfree(buffer4, entries);

	return size;
}

void stb_perfect_destroy(stb_perfect *p)
{
	if (p->large_bmap) free(p->large_bmap);
	if (p->table ) free(p->table);
	p->large_bmap = NULL;
	p->table = NULL;
	p->b_mask = 0;
	p->table_mask = 0;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//                     Perfect hash clients

STB_EXTERN int stb_ischar(char s, char *set);

#ifdef STB_DEFINE

int stb_ischar(char c, char *set)
{
	static unsigned char bit[8] = {1,2,4,8,16,32,64,128};
	static stb_perfect p;
	static unsigned char (*tables)[256];
	static char ** sets = NULL;

	int z = stb_perfect_hash(&p, (int) set);
	if (z < 0) {
		int i,k,n,j,f;
		// special code that means free all existing data
		if (set == NULL) {
			stb_arr_free(sets);
			free(tables);
			tables = NULL;
			stb_perfect_destroy(&p);
			return 0;
		}
		stb_arr_push(sets, set);
		stb_perfect_destroy(&p);
		n = stb_perfect_create(&p, (unsigned int *) (char **) sets, stb_arr_len(sets));
		assert(n != 0);
		k = (n+7) >> 3;
		tables = (unsigned char (*)[256]) realloc(tables, sizeof(*tables) * k);
		memset(tables, 0, sizeof(*tables) * k);
		for (i=0; i < stb_arr_len(sets); ++i) {
			k = stb_perfect_hash(&p, (int) sets[i]);
			assert(k >= 0);
			n = k >> 3;
			f = bit[k&7];
			for (j=0; !j || sets[i][j]; ++j) {
				tables[n][(unsigned char) sets[i][j]] |= f;
			}
		}
		z = stb_perfect_hash(&p, (int) set);
	}
	return tables[z >> 3][(unsigned char) c] & bit[z & 7];
}

#endif

//////////////////////////////////////////////////////////////////////////////
//
//                     Instantiated data structures
//
// This is an attempt to implement a templated data structure.
//
// Hash table: call stb_define_hash(TYPE,N,KEY,K1,K2,HASH,VALUE)
//     TYPE     -- will define a structure type containing the hash table
//     N        -- the name, will prefix functions named:
//                        N create
//                        N destroy
//                        N get
//                        N set, N add, N update,
//                        N remove
//     KEY      -- the type of the key. 'x == y' must be valid
//       K1,K2  -- keys never used by the app, used as flags in the hashtable
//       HASH   -- a piece of code ending with 'return' that hashes key 'k'
//     VALUE    -- the type of the value. 'x = y' must be valid
//
//  Note that stb_define_hash_base can be used to define more sophisticated
//  hash tables, e.g. those that make copies of the key or use special
//  comparisons (e.g. strcmp).

#define STB_(prefix,name)     stb__##prefix##name
#define STB__(prefix,name)    prefix##name
#define STB__use(x)           x
#define STB__skip(x)

#define stb_declare_hash(PREFIX,TYPE,N,KEY,VALUE) \
   typedef struct stb__st_##TYPE TYPE;\
   PREFIX int STB__(N, init)(TYPE *h, int count);\
   PREFIX int STB__(N, memory_usage)(TYPE *h);\
   PREFIX TYPE * STB__(N, create)(void);\
   PREFIX TYPE * STB__(N, copy)(TYPE *h);\
   PREFIX void STB__(N, destroy)(TYPE *h);\
   PREFIX int STB__(N,get_flag)(TYPE *a, KEY k, VALUE *v);\
   PREFIX VALUE STB__(N,get)(TYPE *a, KEY k);\
   PREFIX int STB__(N, set)(TYPE *a, KEY k, VALUE v);\
   PREFIX int STB__(N, add)(TYPE *a, KEY k, VALUE v);\
   PREFIX int STB__(N, update)(TYPE*a,KEY k,VALUE v);\
   PREFIX int STB__(N, remove)(TYPE *a, KEY k, VALUE *v);

#define STB_nocopy(x)        (x)
#define STB_nodelete(x)      0
#define STB_nofields
#define STB_nonullvalue(x)
#define STB_nullvalue(x)     x
#define STB_safecompare(x)   x
#define STB_nosafe(x)
#define STB_noprefix

#ifdef __GNUC__
#define STB__nogcc(x)
#else
#define STB__nogcc(x)  x
#endif

#define stb_define_hash_base(PREFIX,TYPE,FIELDS,N,NC,LOAD_FACTOR,             \
                             KEY,EMPTY,DEL,COPY,DISPOSE,SAFE,                 \
                             VCOMPARE,CCOMPARE,HASH,                          \
                             VALUE,HASVNULL,VNULL)                            \
                                                                              \
typedef struct                                                                \
{                                                                             \
   KEY   k;                                                                   \
   VALUE v;                                                                   \
} STB_(N,_hashpair);                                                          \
                                                                              \
STB__nogcc( typedef struct stb__st_##TYPE TYPE;  )                            \
struct stb__st_##TYPE {                                                       \
   FIELDS                                                                     \
   STB_(N,_hashpair) *table;                                                  \
   unsigned int mask;                                                         \
   int count, limit;                                                          \
   int deleted;                                                               \
                                                                              \
   int delete_threshhold;                                                     \
   int grow_threshhold;                                                       \
   int shrink_threshhold;                                                     \
   unsigned char alloced, has_empty, has_del;                                 \
   VALUE ev; VALUE dv;                                                        \
};                                                                            \
                                                                              \
static unsigned int STB_(N, hash)(KEY k)                                      \
{                                                                             \
   HASH                                                                       \
}                                                                             \
                                                                              \
PREFIX int STB__(N, init)(TYPE *h, int count)                                        \
{                                                                             \
   int i;                                                                     \
   if (count < 4) count = 4;                                                  \
   h->limit = count;                                                          \
   h->count = 0;                                                              \
   h->mask  = count-1;                                                        \
   h->deleted = 0;                                                            \
   h->grow_threshhold = (int) (count * LOAD_FACTOR);                          \
   h->has_empty = h->has_del = 0;                                             \
   h->alloced = 0;                                                            \
   if (count <= 64)                                                           \
      h->shrink_threshhold = 0;                                               \
   else                                                                       \
      h->shrink_threshhold = (int) (count * (LOAD_FACTOR/2.25));              \
   h->delete_threshhold = (int) (count * (1-LOAD_FACTOR)/2);                  \
   h->table = (STB_(N,_hashpair)*) malloc(sizeof(h->table[0]) * count);       \
   if (h->table == NULL) return 0;                                            \
   /* ideally this gets turned into a memset32 automatically */               \
   for (i=0; i < count; ++i)                                                  \
      h->table[i].k = EMPTY;                                                  \
   return 1;                                                                  \
}                                                                             \
                                                                              \
PREFIX int STB__(N, memory_usage)(TYPE *h)                                           \
{                                                                             \
   return sizeof(*h) + h->limit * sizeof(h->table[0]);                        \
}                                                                             \
                                                                              \
PREFIX TYPE * STB__(N, create)(void)                                                 \
{                                                                             \
   TYPE *h = (TYPE *) malloc(sizeof(*h));                                     \
   if (h) {                                                                   \
      if (STB__(N, init)(h, 16))                                              \
         h->alloced = 1;                                                      \
      else { free(h); h=NULL; }                                               \
   }                                                                          \
   return h;                                                                  \
}                                                                             \
                                                                              \
PREFIX void STB__(N, destroy)(TYPE *a)                                               \
{                                                                             \
   int i;                                                                     \
   for (i=0; i < a->limit; ++i)                                               \
      if (!CCOMPARE(a->table[i].k,EMPTY) && !CCOMPARE(a->table[i].k, DEL))    \
         DISPOSE(a->table[i].k);                                              \
   free(a->table);                                                            \
   if (a->alloced)                                                            \
      free(a);                                                                \
}                                                                             \
                                                                              \
static void STB_(N, rehash)(TYPE *a, int count);                              \
                                                                              \
PREFIX int STB__(N,get_flag)(TYPE *a, KEY k, VALUE *v)                               \
{                                                                             \
   unsigned int h = STB_(N, hash)(k);                                         \
   unsigned int n = h & a->mask, s;                                           \
   if (CCOMPARE(k,EMPTY)){ if (a->has_empty) *v = a->ev; return a->has_empty;}\
   if (CCOMPARE(k,DEL)) { if (a->has_del  ) *v = a->dv; return a->has_del;   }\
   if (CCOMPARE(a->table[n].k,EMPTY)) return 0;                               \
   SAFE(if (!CCOMPARE(a->table[n].k,DEL)))                                    \
   if (VCOMPARE(a->table[n].k,k)) { *v = a->table[n].v; return 1; }            \
   s = stb_rehash(h) | 1;                                                     \
   for(;;) {                                                                  \
      n = (n + s) & a->mask;                                                  \
      if (CCOMPARE(a->table[n].k,EMPTY)) return 0;                            \
      SAFE(if (CCOMPARE(a->table[n].k,DEL)) continue;)                        \
      if (VCOMPARE(a->table[n].k,k))                                           \
         { *v = a->table[n].v; return 1; }                                    \
   }                                                                          \
}                                                                             \
                                                                              \
HASVNULL(                                                                     \
   PREFIX VALUE STB__(N,get)(TYPE *a, KEY k)                                         \
   {                                                                          \
      VALUE v;                                                                \
      if (STB__(N,get_flag)(a,k,&v)) return v;                                \
      else                           return VNULL;                            \
   }                                                                          \
)                                                                             \
                                                                              \
PREFIX int STB__(N,getkey)(TYPE *a, KEY k, KEY *kout)                                \
{                                                                             \
   unsigned int h = STB_(N, hash)(k);                                         \
   unsigned int n = h & a->mask, s;                                           \
   if (CCOMPARE(k,EMPTY)||CCOMPARE(k,DEL)) return 0;                          \
   if (CCOMPARE(a->table[n].k,EMPTY)) return 0;                               \
   SAFE(if (!CCOMPARE(a->table[n].k,DEL)))                                    \
   if (VCOMPARE(a->table[n].k,k)) { *kout = a->table[n].k; return 1; }         \
   s = stb_rehash(h) | 1;                                                     \
   for(;;) {                                                                  \
      n = (n + s) & a->mask;                                                  \
      if (CCOMPARE(a->table[n].k,EMPTY)) return 0;                            \
      SAFE(if (CCOMPARE(a->table[n].k,DEL)) continue;)                        \
      if (VCOMPARE(a->table[n].k,k))                                          \
         { *kout = a->table[n].k; return 1; }                                 \
   }                                                                          \
}                                                                             \
                                                                              \
static int STB_(N,addset)(TYPE *a, KEY k, VALUE v,                            \
                             int allow_new, int allow_old, int copy)          \
{                                                                             \
   unsigned int h = STB_(N, hash)(k);                                         \
   unsigned int n = h & a->mask;                                              \
   int b = -1;                                                                \
   if (CCOMPARE(k,EMPTY)) {                                                   \
      if (a->has_empty ? allow_old : allow_new) {                             \
          n=a->has_empty; a->ev = v; a->has_empty = 1; return !n;             \
      } else return 0;                                                        \
   }                                                                          \
   if (CCOMPARE(k,DEL)) {                                                     \
      if (a->has_del ? allow_old : allow_new) {                               \
          n=a->has_del; a->dv = v; a->has_del = 1; return !n;                 \
      } else return 0;                                                        \
   }                                                                          \
   if (!CCOMPARE(a->table[n].k, EMPTY)) {                                     \
      unsigned int s;                                                         \
      if (CCOMPARE(a->table[n].k, DEL))                                       \
         b = n;                                                               \
      else if (VCOMPARE(a->table[n].k,k)) {                                   \
         if (allow_old)                                                       \
            a->table[n].v = v;                                                \
         return !allow_new;                                                   \
      }                                                                       \
      s = stb_rehash(h) | 1;                                                  \
      for(;;) {                                                               \
         n = (n + s) & a->mask;                                               \
         if (CCOMPARE(a->table[n].k, EMPTY)) break;                           \
         if (CCOMPARE(a->table[n].k, DEL)) {                                  \
            if (b < 0) b = n;                                                 \
         } else if (VCOMPARE(a->table[n].k,k)) {                              \
            if (allow_old)                                                    \
               a->table[n].v = v;                                             \
            return !allow_new;                                                \
         }                                                                    \
      }                                                                       \
   }                                                                          \
   if (!allow_new) return 0;                                                  \
   if (b < 0) b = n; else --a->deleted;                                       \
   a->table[b].k = copy ? COPY(k) : k;                                        \
   a->table[b].v = v;                                                         \
   ++a->count;                                                                \
   if (a->count > a->grow_threshhold)                                         \
      STB_(N,rehash)(a, a->limit*2);                                          \
   return 1;                                                                  \
}                                                                             \
                                                                              \
PREFIX int STB__(N, set)(TYPE *a, KEY k, VALUE v){return STB_(N,addset)(a,k,v,1,1,1);}\
PREFIX int STB__(N, add)(TYPE *a, KEY k, VALUE v){return STB_(N,addset)(a,k,v,1,0,1);}\
PREFIX int STB__(N, update)(TYPE*a,KEY k,VALUE v){return STB_(N,addset)(a,k,v,0,1,1);}\
                                                                              \
PREFIX int STB__(N, remove)(TYPE *a, KEY k, VALUE *v)                                \
{                                                                             \
   unsigned int h = STB_(N, hash)(k);                                         \
   unsigned int n = h & a->mask, s;                                           \
   if (CCOMPARE(k,EMPTY)) { if (a->has_empty) { if(v)*v = a->ev; a->has_empty=0; return 1; } return 0; } \
   if (CCOMPARE(k,DEL))   { if (a->has_del  ) { if(v)*v = a->dv; a->has_del  =0; return 1; } return 0; } \
   if (CCOMPARE(a->table[n].k,EMPTY)) return 0;                               \
   if (SAFE(CCOMPARE(a->table[n].k,DEL) || ) !VCOMPARE(a->table[n].k,k)) {     \
      s = stb_rehash(h) | 1;                                                  \
      for(;;) {                                                               \
         n = (n + s) & a->mask;                                               \
         if (CCOMPARE(a->table[n].k,EMPTY)) return 0;                         \
         SAFE(if (CCOMPARE(a->table[n].k, DEL)) continue;)                    \
         if (VCOMPARE(a->table[n].k,k)) break;                                 \
      }                                                                       \
   }                                                                          \
   DISPOSE(a->table[n].k);                                                    \
   a->table[n].k = DEL;                                                       \
   --a->count;                                                                \
   ++a->deleted;                                                              \
   if (v != NULL)                                                             \
      *v = a->table[n].v;                                                     \
   if (a->count < a->shrink_threshhold)                                       \
      STB_(N, rehash)(a, a->limit >> 1);                                      \
   else if (a->deleted > a->delete_threshhold)                                \
      STB_(N, rehash)(a, a->limit);                                           \
   return 1;                                                                  \
}                                                                             \
                                                                              \
PREFIX TYPE * STB__(NC, copy)(TYPE *a)                                        \
{                                                                             \
   int i;                                                                     \
   TYPE *h = (TYPE *) malloc(sizeof(*h));                                     \
   if (!h) return NULL;                                                       \
   if (!STB__(N, init)(h, a->limit)) { free(h); return NULL; }                \
   h->count = a->count;                                                       \
   h->deleted = a->deleted;                                                   \
   h->alloced = 1;                                                            \
   h->ev = a->ev; h->dv = a->dv;                                              \
   h->has_empty = a->has_empty; h->has_del = a->has_del;                      \
   memcpy(h->table, a->table, h->limit * sizeof(h->table[0]));                \
   for (i=0; i < a->limit; ++i)                                               \
      if (!CCOMPARE(h->table[i].k,EMPTY) && !CCOMPARE(h->table[i].k,DEL))     \
         h->table[i].k = COPY(h->table[i].k);                                 \
   return h;                                                                  \
}                                                                             \
                                                                              \
static void STB_(N, rehash)(TYPE *a, int count)                               \
{                                                                             \
   int i;                                                                     \
   TYPE b;                                                                    \
   STB__(N, init)(&b, count);                                                 \
   for (i=0; i < a->limit; ++i)                                               \
      if (!CCOMPARE(a->table[i].k,EMPTY) && !CCOMPARE(a->table[i].k,DEL))     \
         STB_(N,addset)(&b, a->table[i].k, a->table[i].v,1,1,0);              \
   free(a->table);                                                            \
   a->table = b.table;                                                        \
   a->mask = b.mask;                                                          \
   a->count = b.count;                                                        \
   a->limit = b.limit;                                                        \
   a->deleted = b.deleted;                                                    \
   a->delete_threshhold = b.delete_threshhold;                                \
   a->grow_threshhold = b.grow_threshhold;                                    \
   a->shrink_threshhold = b.shrink_threshhold;                                \
}

#define STB_equal(a,b)  ((a) == (b))

#define stb_define_hash(TYPE,N,KEY,EMPTY,DEL,HASH,VALUE)                      \
   stb_define_hash_base(STB_noprefix, TYPE,STB_nofields,N,NC,0.85f,              \
              KEY,EMPTY,DEL,STB_nocopy,STB_nodelete,STB_nosafe,               \
              STB_equal,STB_equal,HASH,                                       \
              VALUE,STB_nonullvalue,0)

#define stb_define_hash_vnull(TYPE,N,KEY,EMPTY,DEL,HASH,VALUE,VNULL)          \
   stb_define_hash_base(STB_noprefix, TYPE,STB_nofields,N,NC,0.85f,              \
              KEY,EMPTY,DEL,STB_nocopy,STB_nodelete,STB_nosafe,               \
              STB_equal,STB_equal,HASH,                                       \
              VALUE,STB_nullvalue,VNULL)

//////////////////////////////////////////////////////////////////////////////
//
//                        stb_ptrmap
//
// An stb_ptrmap data structure is an O(1) hash table between pointers. One
// application is to let you store "extra" data associated with pointers,
// which is why it was originally called stb_extra.

stb_declare_hash(STB_EXTERN, stb_ptrmap, stb_ptrmap_, void *, void *)
stb_declare_hash(STB_EXTERN, stb_idict, stb_idict_, stb_int32, stb_int32)

STB_EXTERN void stb_ptrmap_delete(stb_ptrmap *e, void (*free_func)(void *));
STB_EXTERN stb_ptrmap *stb_ptrmap_new(void);

STB_EXTERN stb_idict * stb_idict_new_size(int size);
STB_EXTERN void stb_idict_remove_all(stb_idict *e);

#ifdef STB_DEFINE

#define STB_EMPTY ((void *) 2)
#define STB_EDEL  ((void *) 6)

stb_define_hash_base(STB_noprefix,stb_ptrmap, STB_nofields, stb_ptrmap_,stb_ptrmap_,0.85f,
		void *,STB_EMPTY,STB_EDEL,STB_nocopy,STB_nodelete,STB_nosafe,
		STB_equal,STB_equal,return stb_hashptr(k);,
		void *,STB_nullvalue,NULL)

stb_ptrmap *stb_ptrmap_new(void)
{
	return stb_ptrmap_create();
}

void stb_ptrmap_delete(stb_ptrmap *e, void (*free_func)(void *))
{
	int i;
	if (free_func)
	for (i=0; i < e->limit; ++i)
	if (e->table[i].k != STB_EMPTY && e->table[i].k != STB_EDEL) {
		if (free_func == free)
		free(e->table[i].v); // allow STB_MALLOC_WRAPPER to operate
		else
		free_func(e->table[i].v);
	}
	stb_ptrmap_destroy(e);
}

// extra fields needed for stua_dict
#define STB_IEMPTY  ((int) 1)
#define STB_IDEL    ((int) 3)
stb_define_hash_base(STB_noprefix, stb_idict, short type; short gc; STB_nofields, stb_idict_,stb_idict_,0.85f,
		stb_int32,STB_IEMPTY,STB_IDEL,STB_nocopy,STB_nodelete,STB_nosafe,
		STB_equal,STB_equal,
		return stb_rehash_improved(k);,stb_int32,STB_nonullvalue,0)

stb_idict * stb_idict_new_size(int size)
{
	stb_idict *e = (stb_idict *) malloc(sizeof(*e));
	if (e) {
		if (!stb_is_pow2(size))
		size = 1 << stb_log2_ceil(size);
		stb_idict_init(e, size);
		e->alloced = 1;
	}
	return e;
}

void stb_idict_remove_all(stb_idict *e)
{
	int n;
	for (n=0; n < e->limit; ++n)
	e->table[n].k = STB_IEMPTY;
	e->has_empty = e->has_del = 0;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//                        stb_sparse_ptr_matrix
//
// An stb_ptrmap data structure is an O(1) hash table storing an arbitrary
// block of data for a given pair of pointers.
//
// If create=0, returns

typedef struct stb__st_stb_spmatrix stb_spmatrix;

STB_EXTERN stb_spmatrix * stb_sparse_ptr_matrix_new(int val_size);
STB_EXTERN void stb_sparse_ptr_matrix_free(stb_spmatrix *z);
STB_EXTERN void * stb_sparse_ptr_matrix_get(stb_spmatrix *z, void *a, void *b,
		int create);

#ifdef STB_DEFINE
typedef struct
{
	void *a;
	void *b;
}stb__ptrpair;

static stb__ptrpair stb__ptrpair_empty = {(void *) 1, (void *) 1};
static stb__ptrpair stb__ptrpair_del = {(void *) 2, (void *) 2};

#define STB__equal_ptrpair(x,y) ((x).a == (y).a && (x).b == (y).b)

stb_define_hash_base(static, stb_spmatrix, int val_size; void *arena;, stb__spmatrix_,stb__spmatrix_, 0.85,
		stb__ptrpair, stb__ptrpair_empty, stb__ptrpair_del,
		STB_nocopy, STB_nodelete, STB_nosafe,
		STB__equal_ptrpair, STB__equal_ptrpair, return stb_rehash(stb_hashptr(k.a))+stb_hashptr(k.b);,
		void *, STB_nullvalue, 0)

stb_spmatrix *stb_sparse_ptr_matrix_new(int val_size)
{
	stb_spmatrix *m = stb__spmatrix_create();
	if (m) m->val_size = val_size;
	if (m) m->arena = stb_malloc_global(1);
	return m;
}

void stb_sparse_ptr_matrix_free(stb_spmatrix *z)
{
	if (z->arena) stb_free(z->arena);
	stb__spmatrix_destroy(z);
}

void *stb_sparse_ptr_matrix_get(stb_spmatrix *z, void *a, void *b, int create)
{
	stb__ptrpair t = {a,b};
	void *data = stb__spmatrix_get(z, t);
	if (!data && create) {
		data = stb_malloc_raw(z->arena, z->val_size);
		if (!data) return NULL;
		memset(data, 0, z->val_size);
		stb__spmatrix_add(z, t, data);
	}
	return data;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//                  SDICT: Hash Table for Strings (symbol table)
//
//           if "use_arena=1", then strings will be copied
//           into blocks and never freed until the sdict is freed;
//           otherwise they're malloc()ed and free()d on the fly.
//           (specify use_arena=1 if you never stb_sdict_remove)

stb_declare_hash(STB_EXTERN, stb_sdict, stb_sdict_, char *, void *)

STB_EXTERN stb_sdict * stb_sdict_new(int use_arena);
STB_EXTERN stb_sdict * stb_sdict_copy(stb_sdict*);
STB_EXTERN void stb_sdict_delete(stb_sdict *);
STB_EXTERN void * stb_sdict_change(stb_sdict *, char *str, void *p);
STB_EXTERN int stb_sdict_count(stb_sdict *d);

#define stb_sdict_for(d,i,q,z)                                          \
   for(i=0; i < (d)->limit ? q=(d)->table[i].k,z=(d)->table[i].v,1 : 0; ++i)    \
      if (q==NULL||q==(void *) 1);else   // reversed makes macro friendly

#ifdef STB_DEFINE

#define STB_DEL ((void *) 1)
#define STB_SDEL  ((char *) 1)

#define stb_sdict__copy(x)                                       \
   strcpy(a->arena ? stb_malloc_string(a->arena, strlen(x)+1)    \
                   : (char *) malloc(strlen(x)+1), x)

#define stb_sdict__dispose(x)  if (!a->arena) free(x)

stb_define_hash_base(STB_noprefix, stb_sdict, void*arena;, stb_sdict_,stb_sdictinternal_, 0.85f,
		char *, NULL, STB_SDEL, stb_sdict__copy, stb_sdict__dispose,
		STB_safecompare, !strcmp, STB_equal, return stb_hash(k);,
		void *, STB_nullvalue, NULL)

int stb_sdict_count(stb_sdict *a)
{
	return a->count;
}

stb_sdict * stb_sdict_new(int use_arena)
{
	stb_sdict *d = stb_sdict_create();
	if (d == NULL) return NULL;
	d->arena = use_arena ? stb_malloc_global(1) : NULL;
	return d;
}

stb_sdict* stb_sdict_copy(stb_sdict *old)
{
	stb_sdict *n;
	void *old_arena = old->arena;
	void *new_arena = old_arena ? stb_malloc_global(1) : NULL;
	old->arena = new_arena;
	n = stb_sdictinternal_copy(old);
	old->arena = old_arena;
	if (n)
	n->arena = new_arena;
	else if (new_arena)
	stb_free(new_arena);
	return n;
}

void stb_sdict_delete(stb_sdict *d)
{
	if (d->arena)
	stb_free(d->arena);
	stb_sdict_destroy(d);
}

void * stb_sdict_change(stb_sdict *d, char *str, void *p)
{
	void *q = stb_sdict_get(d, str);
	stb_sdict_set(d, str, p);
	return q;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//                     Instantiated data structures
//
// This is an attempt to implement a templated data structure.
// What you do is define a struct foo, and then include several
// pointer fields to struct foo in your struct. Then you call
// the instantiator, which creates the functions that implement
// the data structure. This requires massive undebuggable #defines,
// so we limit the cases where we do this.
//
// AA tree is an encoding of a 2-3 tree whereas RB trees encode a 2-3-4 tree;
// much simpler code due to fewer cases.

#define stb__bst_parent(x)    x
#define stb__bst_noparent(x)

#define stb_bst_fields(N)                                   \
    *STB_(N,left), *STB_(N,right);                          \
    unsigned char STB_(N,level)

#define stb_bst_fields_parent(N)                            \
    *STB_(N,left), *STB_(N,right),  *STB_(N,parent);        \
    unsigned char STB_(N,level)

#define STB__level(N,x)         ((x) ? (x)->STB_(N,level) : 0)

#define stb_bst_base(TYPE, N, TREE, M, compare, PAR)                         \
                                                                             \
static int STB_(N,_compare)(TYPE *p, TYPE *q)                                \
{                                                                            \
   compare                                                                   \
}                                                                            \
                                                                             \
static void STB_(N,setleft)(TYPE *q, TYPE *v)                                \
{                                                                            \
   q->STB_(N,left) = v;                                                      \
   PAR(if (v) v->STB_(N,parent) = q;)                                        \
}                                                                            \
                                                                             \
static void STB_(N,setright)(TYPE *q, TYPE *v)                               \
{                                                                            \
   q->STB_(N,right) = v;                                                     \
   PAR(if (v) v->STB_(N,parent) = q;)                                        \
}                                                                            \
                                                                             \
static TYPE *STB_(N,skew)(TYPE *q)                                           \
{                                                                            \
   if (q == NULL) return q;                                                  \
   if (q->STB_(N,left)                                                       \
        && q->STB_(N,left)->STB_(N,level) == q->STB_(N,level)) {             \
      TYPE *p       = q->STB_(N,left);                                       \
      STB_(N,setleft)(q, p->STB_(N,right));                                  \
      STB_(N,setright)(p, q);                                                \
      return p;                                                              \
   }                                                                         \
   return q;                                                                 \
}                                                                            \
                                                                             \
static TYPE *STB_(N,split)(TYPE *p)                                          \
{                                                                            \
   TYPE *q = p->STB_(N,right);                                               \
   if (q && q->STB_(N,right)                                                 \
         && q->STB_(N,right)->STB_(N,level) == p->STB_(N,level)) {           \
      STB_(N,setright)(p, q->STB_(N,left));                                  \
      STB_(N,setleft)(q,p);                                                  \
      ++q->STB_(N,level);                                                    \
      return q;                                                              \
   }                                                                         \
   return p;                                                                 \
}                                                                            \
                                                                             \
TYPE *STB__(N,insert)(TYPE *tree, TYPE *item)                                \
{                                                                            \
   int c;                                                                    \
   if (tree == NULL) {                                                       \
      item->STB_(N,left) = NULL;                                             \
      item->STB_(N,right) = NULL;                                            \
      item->STB_(N,level) = 1;                                               \
      PAR(item->STB_(N,parent) = NULL;)                                      \
      return item;                                                           \
   }                                                                         \
   c = STB_(N,_compare)(item,tree);                                          \
   if (c == 0) {                                                             \
      if (item != tree) {                                                    \
         STB_(N,setleft)(item, tree->STB_(N,left));                          \
         STB_(N,setright)(item, tree->STB_(N,right));                        \
         item->STB_(N,level) = tree->STB_(N,level);                          \
         PAR(item->STB_(N,parent) = NULL;)                                   \
      }                                                                      \
      return item;                                                           \
   }                                                                         \
   if (c < 0)                                                                \
      STB_(N,setleft )(tree, STB__(N,insert)(tree->STB_(N,left), item));     \
   else                                                                      \
      STB_(N,setright)(tree, STB__(N,insert)(tree->STB_(N,right), item));    \
   tree = STB_(N,skew)(tree);                                                \
   tree = STB_(N,split)(tree);                                               \
   PAR(tree->STB_(N,parent) = NULL;)                                         \
   return tree;                                                              \
}                                                                            \
                                                                             \
TYPE *STB__(N,remove)(TYPE *tree, TYPE *item)                                \
{                                                                            \
   static TYPE *delnode, *leaf, *restore;                                    \
   if (tree == NULL) return NULL;                                            \
   leaf = tree;                                                              \
   if (STB_(N,_compare)(item, tree) < 0) {                                   \
      STB_(N,setleft)(tree, STB__(N,remove)(tree->STB_(N,left), item));      \
   } else {                                                                  \
      TYPE *r;                                                               \
      delnode = tree;                                                        \
      r = STB__(N,remove)(tree->STB_(N,right), item);                        \
      /* maybe move 'leaf' up to this location */                            \
      if (restore == tree) { tree = leaf; leaf = restore = NULL;  }          \
      STB_(N,setright)(tree,r);                                              \
      assert(tree->STB_(N,right) != tree);                                   \
   }                                                                         \
   if (tree == leaf) {                                                       \
      if (delnode == item) {                                                 \
         tree = tree->STB_(N,right);                                         \
         assert(leaf->STB_(N,left) == NULL);                                 \
         /* move leaf (the right sibling) up to delnode */                   \
         STB_(N,setleft )(leaf, item->STB_(N,left ));                        \
         STB_(N,setright)(leaf, item->STB_(N,right));                        \
         leaf->STB_(N,level) = item->STB_(N,level);                          \
         if (leaf != item)                                                   \
            restore = delnode;                                               \
      }                                                                      \
      delnode = NULL;                                                        \
   } else {                                                                  \
      if (STB__level(N,tree->STB_(N,left) ) < tree->STB_(N,level)-1 ||       \
          STB__level(N,tree->STB_(N,right)) < tree->STB_(N,level)-1) {       \
         --tree->STB_(N,level);                                              \
         if (STB__level(N,tree->STB_(N,right)) > tree->STB_(N,level))        \
            tree->STB_(N,right)->STB_(N,level) = tree->STB_(N,level);        \
         tree = STB_(N,skew)(tree);                                          \
         STB_(N,setright)(tree, STB_(N,skew)(tree->STB_(N,right)));          \
         if (tree->STB_(N,right))                                            \
            STB_(N,setright)(tree->STB_(N,right),                            \
                  STB_(N,skew)(tree->STB_(N,right)->STB_(N,right)));         \
         tree = STB_(N,split)(tree);                                         \
         if (tree->STB_(N,right))                                            \
            STB_(N,setright)(tree,  STB_(N,split)(tree->STB_(N,right)));     \
      }                                                                      \
   }                                                                         \
   PAR(if (tree) tree->STB_(N,parent) = NULL;)                               \
   return tree;                                                              \
}                                                                            \
                                                                             \
TYPE *STB__(N,last)(TYPE *tree)                                              \
{                                                                            \
   if (tree)                                                                 \
      while (tree->STB_(N,right)) tree = tree->STB_(N,right);                \
   return tree;                                                              \
}                                                                            \
                                                                             \
TYPE *STB__(N,first)(TYPE *tree)                                             \
{                                                                            \
   if (tree)                                                                 \
      while (tree->STB_(N,left)) tree = tree->STB_(N,left);                  \
   return tree;                                                              \
}                                                                            \
                                                                             \
TYPE *STB__(N,next)(TYPE *tree, TYPE *item)                                  \
{                                                                            \
   TYPE *next = NULL;                                                        \
   if (item->STB_(N,right))                                                  \
      return STB__(N,first)(item->STB_(N,right));                            \
   PAR(                                                                      \
      while(item->STB_(N,parent)) {                                          \
         TYPE *up = item->STB_(N,parent);                                    \
         if (up->STB_(N,left) == item) return up;                            \
         item = up;                                                          \
      }                                                                      \
      return NULL;                                                           \
   )                                                                         \
   while (tree != item) {                                                    \
      if (STB_(N,_compare)(item, tree) < 0) {                                \
         next = tree;                                                        \
         tree = tree->STB_(N,left);                                          \
      } else {                                                               \
         tree = tree->STB_(N,right);                                         \
      }                                                                      \
   }                                                                         \
   return next;                                                              \
}                                                                            \
                                                                             \
TYPE *STB__(N,prev)(TYPE *tree, TYPE *item)                                  \
{                                                                            \
   TYPE *next = NULL;                                                        \
   if (item->STB_(N,left))                                                   \
      return STB__(N,last)(item->STB_(N,left));                              \
   PAR(                                                                      \
      while(item->STB_(N,parent)) {                                          \
         TYPE *up = item->STB_(N,parent);                                    \
         if (up->STB_(N,right) == item) return up;                           \
         item = up;                                                          \
      }                                                                      \
      return NULL;                                                           \
   )                                                                         \
   while (tree != item) {                                                    \
      if (STB_(N,_compare)(item, tree) < 0) {                                \
         tree = tree->STB_(N,left);                                          \
      } else {                                                               \
         next = tree;                                                        \
         tree = tree->STB_(N,right);                                         \
      }                                                                      \
   }                                                                         \
   return next;                                                              \
}                                                                            \
                                                                             \
STB__DEBUG(                                                                  \
   void STB__(N,_validate)(TYPE *tree, int root)                             \
   {                                                                         \
      if (tree == NULL) return;                                              \
      PAR(if(root) assert(tree->STB_(N,parent) == NULL);)                    \
      assert(STB__level(N,tree->STB_(N,left) ) == tree->STB_(N,level)-1);    \
      assert(STB__level(N,tree->STB_(N,right)) <= tree->STB_(N,level));      \
      assert(STB__level(N,tree->STB_(N,right)) >= tree->STB_(N,level)-1);    \
      if (tree->STB_(N,right)) {                                             \
        assert(STB__level(N,tree->STB_(N,right)->STB_(N,right))              \
                                               !=    tree->STB_(N,level));   \
        PAR(assert(tree->STB_(N,right)->STB_(N,parent) == tree);)            \
      }                                                                      \
      PAR(if(tree->STB_(N,left)) assert(tree->STB_(N,left)->STB_(N,parent) == tree);) \
      STB__(N,_validate)(tree->STB_(N,left) ,0);                             \
      STB__(N,_validate)(tree->STB_(N,right),0);                             \
   }                                                                         \
)                                                                            \
                                                                             \
typedef struct                                                               \
{                                                                            \
   TYPE *root;                                                               \
} TREE;                                                                      \
                                                                             \
void  STB__(M,Insert)(TREE *tree, TYPE *item)                                \
{ tree->root = STB__(N,insert)(tree->root, item); }                          \
void  STB__(M,Remove)(TREE *tree, TYPE *item)                                \
{ tree->root = STB__(N,remove)(tree->root, item); }                          \
TYPE *STB__(M,Next)(TREE *tree, TYPE *item)                                  \
{ return STB__(N,next)(tree->root, item); }                                  \
TYPE *STB__(M,Prev)(TREE *tree, TYPE *item)                                  \
{ return STB__(N,prev)(tree->root, item); }                                  \
TYPE *STB__(M,First)(TREE *tree) { return STB__(N,first)(tree->root); }      \
TYPE *STB__(M,Last) (TREE *tree) { return STB__(N,last) (tree->root); }      \
void STB__(M,Init)(TREE *tree) { tree->root = NULL; }

#define stb_bst_find(N,tree,fcompare)                                        \
{                                                                            \
   int c;                                                                    \
   while (tree != NULL) {                                                    \
      fcompare                                                               \
      if (c == 0) return tree;                                               \
      if (c < 0)  tree = tree->STB_(N,left);                                 \
      else        tree = tree->STB_(N,right);                                \
   }                                                                         \
   return NULL;                                                              \
}

#define stb_bst_raw(TYPE,N,TREE,M,vfield,VTYPE,compare,PAR)                  \
   stb_bst_base(TYPE,N,TREE,M,                                               \
         VTYPE a = p->vfield; VTYPE b = q->vfield; return (compare);, PAR )  \
                                                                             \
TYPE *STB__(N,find)(TYPE *tree, VTYPE a)                                     \
   stb_bst_find(N,tree,VTYPE b = tree->vfield; c = (compare);)               \
TYPE *STB__(M,Find)(TREE *tree, VTYPE a)                                     \
{ return STB__(N,find)(tree->root, a); }

#define stb_bst(TYPE,N,TREE,M,vfield,VTYPE,compare) \
   stb_bst_raw(TYPE,N,TREE,M,vfield,VTYPE,compare,stb__bst_noparent)
#define stb_bst_parent(TYPE,N,TREE,M,vfield,VTYPE,compare) \
   stb_bst_raw(TYPE,N,TREE,M,vfield,VTYPE,compare,stb__bst_parent)

//////////////////////////////////////////////////////////////////////////////
//
//                             Pointer Nulling
//
//  This lets you automatically NULL dangling pointers to "registered"
//  objects. Note that you have to make sure you call the appropriate
//  functions when you free or realloc blocks of memory that contain
//  pointers or pointer targets. stb.h can automatically do this for
//  stb_arr, or for all frees/reallocs if it's wrapping them.
//

#ifdef STB_NPTR

STB_EXTERN void stb_nptr_set(void *address_of_pointer, void *value_to_write);
STB_EXTERN void stb_nptr_didset(void *address_of_pointer);

STB_EXTERN void stb_nptr_didfree(void *address_being_freed, int len);
STB_EXTERN void stb_nptr_free(void *address_being_freed, int len);

STB_EXTERN void stb_nptr_didrealloc(void *new_address, void *old_address, int len);
STB_EXTERN void stb_nptr_recache(void); // recache all known pointers
// do this after pointer sets outside your control, slow

#ifdef STB_DEFINE
// for fast updating on free/realloc, we need to be able to find
// all the objects (pointers and targets) within a given block;
// this precludes hashing

// we use a three-level hierarchy of memory to minimize storage:
//   level 1: 65536 pointers to stb__memory_node (always uses 256 KB)
//   level 2: each stb__memory_node represents a 64K block of memory
//            with 256 stb__memory_leafs (worst case 64MB)
//   level 3: each stb__memory_leaf represents 256 bytes of memory
//            using a list of target locations and a list of pointers
//            (which are hopefully fairly short normally!)

// this approach won't work in 64-bit, which has a much larger address
// space. need to redesign

#define STB__NPTR_ROOT_LOG2   16
#define STB__NPTR_ROOT_NUM    (1 << STB__NPTR_ROOT_LOG2)
#define STB__NPTR_ROOT_SHIFT  (32 - STB__NPTR_ROOT_LOG2)

#define STB__NPTR_NODE_LOG2   5
#define STB__NPTR_NODE_NUM    (1 << STB__NPTR_NODE_LOG2)
#define STB__NPTR_NODE_MASK   (STB__NPTR_NODE_NUM-1)
#define STB__NPTR_NODE_SHIFT  (STB__NPTR_ROOT_SHIFT - STB__NPTR_NODE_LOG2)
#define STB__NPTR_NODE_OFFSET(x)   (((x) >> STB__NPTR_NODE_SHIFT) & STB__NPTR_NODE_MASK)

typedef struct stb__st_nptr
{
	void *ptr;   // address of actual pointer
	struct stb__st_nptr *next;// next pointer with same target
	struct stb__st_nptr **prev;// prev pointer with same target, address of 'next' field (or first)
	struct stb__st_nptr *next_in_block;
}stb__nptr;

typedef struct stb__st_nptr_target
{
	void *ptr;   // address of target
	stb__nptr *first;// address of first nptr pointing to this
	struct stb__st_nptr_target *next_in_block;
}stb__nptr_target;

typedef struct
{
	stb__nptr *pointers;
	stb__nptr_target *targets;
}stb__memory_leaf;

typedef struct
{
	stb__memory_leaf *children[STB__NPTR_NODE_NUM];
}stb__memory_node;

stb__memory_node *stb__memtab_root[STB__NPTR_ROOT_NUM];

static stb__memory_leaf *stb__nptr_find_leaf(void *mem)
{
	stb_uint32 address = (stb_uint32) mem;
	stb__memory_node *z = stb__memtab_root[address >> STB__NPTR_ROOT_SHIFT];
	if (z)
	return z->children[STB__NPTR_NODE_OFFSET(address)];
	else
	return NULL;
}

static void * stb__nptr_alloc(int size)
{
	return stb__realloc_raw(0,size);
}

static void stb__nptr_free(void *p)
{
	stb__realloc_raw(p,0);
}

static stb__memory_leaf *stb__nptr_make_leaf(void *mem)
{
	stb_uint32 address = (stb_uint32) mem;
	stb__memory_node *z = stb__memtab_root[address >> STB__NPTR_ROOT_SHIFT];
	stb__memory_leaf *f;
	if (!z) {
		int i;
		z = (stb__memory_node *) stb__nptr_alloc(sizeof(*stb__memtab_root[0]));
		stb__memtab_root[address >> STB__NPTR_ROOT_SHIFT] = z;
		for (i=0; i < 256; ++i)
		z->children[i] = 0;
	}
	f = (stb__memory_leaf *) stb__nptr_alloc(sizeof(*f));
	z->children[STB__NPTR_NODE_OFFSET(address)] = f;
	f->pointers = NULL;
	f->targets = NULL;
	return f;
}

static stb__nptr_target *stb__nptr_find_target(void *target, int force)
{
	stb__memory_leaf *p = stb__nptr_find_leaf(target);
	if (p) {
		stb__nptr_target *t = p->targets;
		while (t) {
			if (t->ptr == target)
			return t;
			t = t->next_in_block;
		}
	}
	if (force) {
		stb__nptr_target *t = (stb__nptr_target*) stb__nptr_alloc(sizeof(*t));
		if (!p) p = stb__nptr_make_leaf(target);
		t->ptr = target;
		t->first = NULL;
		t->next_in_block = p->targets;
		p->targets = t;
		return t;
	} else
	return NULL;
}

static stb__nptr *stb__nptr_find_pointer(void *ptr, int force)
{
	stb__memory_leaf *p = stb__nptr_find_leaf(ptr);
	if (p) {
		stb__nptr *t = p->pointers;
		while (t) {
			if (t->ptr == ptr)
			return t;
			t = t->next_in_block;
		}
	}
	if (force) {
		stb__nptr *t = (stb__nptr *) stb__nptr_alloc(sizeof(*t));
		if (!p) p = stb__nptr_make_leaf(ptr);
		t->ptr = ptr;
		t->next = NULL;
		t->prev = NULL;
		t->next_in_block = p->pointers;
		p->pointers = t;
		return t;
	} else
	return NULL;
}

void stb_nptr_set(void *address_of_pointer, void *value_to_write)
{
	if (*(void **)address_of_pointer != value_to_write) {
		*(void **) address_of_pointer = value_to_write;
		stb_nptr_didset(address_of_pointer);
	}
}

void stb_nptr_didset(void *address_of_pointer)
{
	// first unlink from old chain
	void *new_address;
	stb__nptr *p = stb__nptr_find_pointer(address_of_pointer, 1);// force building if doesn't exist
	if (p->prev) { // if p->prev is NULL, we just built it, or it was NULL
		*(p->prev) = p->next;
		if (p->next) p->next->prev = p->prev;
	}
	// now add to new chain
	new_address = *(void **)address_of_pointer;
	if (new_address != NULL) {
		stb__nptr_target *t = stb__nptr_find_target(new_address, 1);
		p->next = t->first;
		if (p->next) p->next->prev = &p->next;
		p->prev = &t->first;
		t->first = p;
	} else {
		p->prev = NULL;
		p->next = NULL;
	}
}

void stb__nptr_block(void *address, int len, void (*function)(stb__memory_leaf *f, int datum, void *start, void *end), int datum)
{
	void *end_address = (void *) ((char *) address + len - 1);
	stb__memory_node *n;
	stb_uint32 start = (stb_uint32) address;
	stb_uint32 end = start + len - 1;

	int b0 = start >> STB__NPTR_ROOT_SHIFT;
	int b1 = end >> STB__NPTR_ROOT_SHIFT;
	int b=b0,i,e0,e1;

	e0 = STB__NPTR_NODE_OFFSET(start);

	if (datum <= 0) {
		// first block
		n = stb__memtab_root[b0];
		if (n) {
			if (b0 != b1)
			e1 = STB__NPTR_NODE_NUM-1;
			else
			e1 = STB__NPTR_NODE_OFFSET(end);
			for (i=e0; i <= e1; ++i)
			if (n->children[i])
			function(n->children[i], datum, address, end_address);
		}
		if (b1 > b0) {
			// blocks other than the first and last block
			for (b=b0+1; b < b1; ++b) {
				n = stb__memtab_root[b];
				if (n)
				for (i=0; i <= STB__NPTR_NODE_NUM-1; ++i)
				if (n->children[i])
				function(n->children[i], datum, address, end_address);
			}
			// last block
			n = stb__memtab_root[b1];
			if (n) {
				e1 = STB__NPTR_NODE_OFFSET(end);
				for (i=0; i <= e1; ++i)
				if (n->children[i])
				function(n->children[i], datum, address, end_address);
			}
		}
	} else {
		if (b1 > b0) {
			// last block
			n = stb__memtab_root[b1];
			if (n) {
				e1 = STB__NPTR_NODE_OFFSET(end);
				for (i=e1; i >= 0; --i)
				if (n->children[i])
				function(n->children[i], datum, address, end_address);
			}
			// blocks other than the first and last block
			for (b=b1-1; b > b0; --b) {
				n = stb__memtab_root[b];
				if (n)
				for (i=STB__NPTR_NODE_NUM-1; i >= 0; --i)
				if (n->children[i])
				function(n->children[i], datum, address, end_address);
			}
		}
		// first block
		n = stb__memtab_root[b0];
		if (n) {
			if (b0 != b1)
			e1 = STB__NPTR_NODE_NUM-1;
			else
			e1 = STB__NPTR_NODE_OFFSET(end);
			for (i=e1; i >= e0; --i)
			if (n->children[i])
			function(n->children[i], datum, address, end_address);
		}
	}
}

static void stb__nptr_delete_pointers(stb__memory_leaf *f, int offset, void *start, void *end)
{
	stb__nptr **p = &f->pointers;
	while (*p) {
		stb__nptr *n = *p;
		if (n->ptr >= start && n->ptr <= end) {
			// unlink
			if (n->prev) {
				*(n->prev) = n->next;
				if (n->next) n->next->prev = n->prev;
			}
			*p = n->next_in_block;
			stb__nptr_free(n);
		} else
		p = &(n->next_in_block);
	}
}

static void stb__nptr_delete_targets(stb__memory_leaf *f, int offset, void *start, void *end)
{
	stb__nptr_target **p = &f->targets;
	while (*p) {
		stb__nptr_target *n = *p;
		if (n->ptr >= start && n->ptr <= end) {
			// null pointers
			stb__nptr *z = n->first;
			while (z) {
				stb__nptr *y = z->next;
				z->prev = NULL;
				z->next = NULL;
				*(void **) z->ptr = NULL;
				z = y;
			}
			// unlink this target
			*p = n->next_in_block;
			stb__nptr_free(n);
		} else
		p = &(n->next_in_block);
	}
}

void stb_nptr_didfree(void *address_being_freed, int len)
{
	// step one: delete all pointers in this block
	stb__nptr_block(address_being_freed, len, stb__nptr_delete_pointers, 0);
	// step two: NULL all pointers to this block; do this second to avoid NULLing deleted pointers
	stb__nptr_block(address_being_freed, len, stb__nptr_delete_targets, 0);
}

void stb_nptr_free(void *address_being_freed, int len)
{
	free(address_being_freed);
	stb_nptr_didfree(address_being_freed, len);
}

static void stb__nptr_move_targets(stb__memory_leaf *f, int offset, void *start, void *end)
{
	stb__nptr_target **t = &f->targets;
	while (*t) {
		stb__nptr_target *n = *t;
		if (n->ptr >= start && n->ptr <= end) {
			stb__nptr *z;
			stb__memory_leaf *f;
			// unlink n
			*t = n->next_in_block;
			// update n to new address
			n->ptr = (void *) ((char *) n->ptr + offset);
			f = stb__nptr_find_leaf(n->ptr);
			if (!f) f = stb__nptr_make_leaf(n->ptr);
			n->next_in_block = f->targets;
			f->targets = n;
			// now go through all pointers and make them point here
			z = n->first;
			while (z) {
				*(void**) z->ptr = n->ptr;
				z = z->next;
			}
		} else
		t = &(n->next_in_block);
	}
}

static void stb__nptr_move_pointers(stb__memory_leaf *f, int offset, void *start, void *end)
{
	stb__nptr **p = &f->pointers;
	while (*p) {
		stb__nptr *n = *p;
		if (n->ptr >= start && n->ptr <= end) {
			// unlink
			*p = n->next_in_block;
			n->ptr = (void *) ((int) n->ptr + offset);
			// move to new block
			f = stb__nptr_find_leaf(n->ptr);
			if (!f) f = stb__nptr_make_leaf(n->ptr);
			n->next_in_block = f->pointers;
			f->pointers = n;
		} else
		p = &(n->next_in_block);
	}
}

void stb_nptr_realloc(void *new_address, void *old_address, int len)
{
	if (new_address == old_address) return;

	// have to move the pointers first, because moving the targets
	//      requires writing to the pointers-to-the-targets, and if some of those moved too,
	//      we need to make sure we don't write to the old memory

	// step one: move all pointers within the block
	stb__nptr_block(old_address, len, stb__nptr_move_pointers, (char *) new_address - (char *) old_address);
	// step two: move all targets within the block
	stb__nptr_block(old_address, len, stb__nptr_move_targets, (char *) new_address - (char *) old_address);
}

void stb_nptr_move(void *new_address, void *old_address)
{
	stb_nptr_realloc(new_address, old_address, 1);
}

void stb_nptr_recache(void)
{
	int i,j;
	for (i=0; i < STB__NPTR_ROOT_NUM; ++i)
	if (stb__memtab_root[i])
	for (j=0; j < STB__NPTR_NODE_NUM; ++j)
	if (stb__memtab_root[i]->children[j]) {
		stb__nptr *p = stb__memtab_root[i]->children[j]->pointers;
		while (p) {
			stb_nptr_didset(p->ptr);
			p = p->next_in_block;
		}
	}
}

#endif // STB_DEFINE
#endif // STB_NPTR

//////////////////////////////////////////////////////////////////////////////
//
//                             File Processing
//

#ifdef _MSC_VER
#define stb_rename(x,y)   _wrename((const wchar_t *)stb__from_utf8(x), (const wchar_t *)stb__from_utf8_alt(y))
#define stb_mktemp   _mktemp
#else
#define stb_mktemp   mktemp
#define stb_rename   rename
#endif

STB_EXTERN void stb_fput_varlen64(FILE *f, stb_uint64 v);
STB_EXTERN stb_uint64 stb_fget_varlen64(FILE *f);
STB_EXTERN int stb_size_varlen64(stb_uint64 v);

#define stb_filec    (char *) stb_file
#define stb_fileu    (unsigned char *) stb_file
STB_EXTERN void * stb_file(char *filename, size_t *length);
STB_EXTERN void * stb_file_max(char *filename, size_t *length);
STB_EXTERN size_t stb_filelen(FILE *f);
STB_EXTERN int stb_filewrite(char *filename, void *data, size_t length);
STB_EXTERN int stb_filewritestr(char *filename, char *data);
STB_EXTERN char ** stb_stringfile(char *filename, int *len);
STB_EXTERN char ** stb_stringfile_trimmed(char *name, int *len, char comm);
STB_EXTERN char * stb_fgets(char *buffer, int buflen, FILE *f);
STB_EXTERN char * stb_fgets_malloc(FILE *f);
STB_EXTERN int stb_fexists(char *filename);
STB_EXTERN int stb_fcmp(char *s1, char *s2);
STB_EXTERN int stb_feq(char *s1, char *s2);
STB_EXTERN time_t stb_ftimestamp(char *filename);

STB_EXTERN int stb_fullpath(char *abs, int abs_size, char *rel);
STB_EXTERN FILE * stb_fopen(char *filename, char *mode);
STB_EXTERN int stb_fclose(FILE *f, int keep);

enum {
	stb_keep_no = 0, stb_keep_yes = 1, stb_keep_if_different = 2,
};

STB_EXTERN int stb_copyfile(char *src, char *dest);

STB_EXTERN void stb_fput_varlen64(FILE *f, stb_uint64 v);
STB_EXTERN stb_uint64 stb_fget_varlen64(FILE *f);
STB_EXTERN int stb_size_varlen64(stb_uint64 v);

STB_EXTERN void stb_fwrite32(FILE *f, stb_uint32 datum);
STB_EXTERN void stb_fput_varlen(FILE *f, int v);
STB_EXTERN void stb_fput_varlenu(FILE *f, unsigned int v);
STB_EXTERN int stb_fget_varlen(FILE *f);
STB_EXTERN stb_uint stb_fget_varlenu(FILE *f);
STB_EXTERN void stb_fput_ranged(FILE *f, int v, int b, stb_uint n);
STB_EXTERN int stb_fget_ranged(FILE *f, int b, stb_uint n);
STB_EXTERN int stb_size_varlen(int v);
STB_EXTERN int stb_size_varlenu(unsigned int v);
STB_EXTERN int stb_size_ranged(int b, stb_uint n);

STB_EXTERN int stb_fread(void *data, size_t len, size_t count, void *f);
STB_EXTERN int stb_fwrite(void *data, size_t len, size_t count, void *f);

#if 0
typedef struct
{
	FILE *base_file;
	char *buffer;
	int buffer_size;
	int buffer_off;
	int buffer_left;
}STBF;

STB_EXTERN STBF *stb_tfopen(char *filename, char *mode);
STB_EXTERN int stb_tfread(void *data, size_t len, size_t count, STBF *f);
STB_EXTERN int stb_tfwrite(void *data, size_t len, size_t count, STBF *f);
#endif

#ifdef STB_DEFINE

#if 0
STBF *stb_tfopen(char *filename, char *mode)
{
	STBF *z;
	FILE *f = fopen(filename, mode);
	if (!f) return NULL;
	z = (STBF *) malloc(sizeof(*z));
	if (!z) {fclose(f); return NULL;}
	z->base_file = f;
	if (!strcmp(mode, "rb") || !strcmp(mode, "wb")) {
		z->buffer_size = 4096;
		z->buffer_off = z->buffer_size;
		z->buffer_left = 0;
		z->buffer = malloc(z->buffer_size);
		if (!z->buffer) {free(z); fclose(f); return NULL;}
	} else {
		z->buffer = 0;
		z->buffer_size = 0;
		z->buffer_left = 0;
	}
	return z;
}

int stb_tfread(void *data, size_t len, size_t count, STBF *f)
{
	int total = len*count, done=0;
	if (!total) return 0;
	if (total <= z->buffer_left) {
		memcpy(data, z->buffer + z->buffer_off, total);
		z->buffer_off += total;
		z->buffer_left -= total;
		return count;
	} else {
		char *out = (char *) data;

		// consume all buffered data
		memcpy(data, z->buffer + z->buffer_off, z->buffer_left);
		done = z->buffer_left;
		out += z->buffer_left;
		z->buffer_left=0;

		if (total-done > (z->buffer_size >> 1)) {
			done += fread(out
				}
			}
		}
#endif

		void stb_fwrite32(FILE *f, stb_uint32 x)
		{
			fwrite(&x, 4, 1, f);
		}

#if defined(_MSC_VER) || defined(__MINGW32__)
#define stb__stat   _stat
#else
#define stb__stat   stat
#endif

		int stb_fexists(char *filename)
		{
			struct stb__stat buf;
			return stb__windows(
					_wstat((const wchar_t *)stb__from_utf8(filename), &buf),
					stat(filename,&buf)
			) == 0;
		}

		time_t stb_ftimestamp(char *filename)
		{
			struct stb__stat buf;
			if (stb__windows(
							_wstat((const wchar_t *)stb__from_utf8(filename), &buf),
							stat(filename,&buf)
					) == 0)
			{
				return buf.st_mtime;
			} else {
				return 0;
			}
		}

		size_t stb_filelen(FILE *f)
		{
			size_t len, pos;
			pos = ftell(f);
			fseek(f, 0, SEEK_END);
			len = ftell(f);
			fseek(f, pos, SEEK_SET);
			return len;
		}

		void *stb_file(char *filename, size_t *length)
		{
			FILE *f = stb__fopen(filename, "rb");
			char *buffer;
			size_t len, len2;
			if (!f) return NULL;
			len = stb_filelen(f);
			buffer = (char *) malloc(len+2); // nul + extra
			len2 = fread(buffer, 1, len, f);
			if (len2 == len) {
				if (length) *length = len;
				buffer[len] = 0;
			} else {
				free(buffer);
				buffer = NULL;
			}
			fclose(f);
			return buffer;
		}

		int stb_filewrite(char *filename, void *data, size_t length)
		{
			FILE *f = stb_fopen(filename, "wb");
			if (f) {
				unsigned char *data_ptr = (unsigned char *) data;
				size_t remaining = length;
				while (remaining > 0) {
					size_t len2 = remaining > 65536 ? 65536 : remaining;
					size_t len3 = fwrite(data_ptr, 1, len2, f);
					if (len2 != len3) {
						fprintf(stderr, "Failed while writing %s\n", filename);
						break;
					}
					remaining -= len2;
					data_ptr += len2;
				}
				stb_fclose(f, stb_keep_if_different);
			}
			return f != NULL;
		}

		int stb_filewritestr(char *filename, char *data)
		{
			return stb_filewrite(filename, data, strlen(data));
		}

		void * stb_file_max(char *filename, size_t *length)
		{
			FILE *f = stb__fopen(filename, "rb");
			char *buffer;
			size_t len, maxlen;
			if (!f) return NULL;
			maxlen = *length;
			buffer = (char *) malloc(maxlen+1);
			len = fread(buffer, 1, maxlen, f);
			buffer[len] = 0;
			fclose(f);
			*length = len;
			return buffer;
		}

		char ** stb_stringfile(char *filename, int *plen)
		{
			FILE *f = stb__fopen(filename, "rb");
			char *buffer, **list=NULL, *s;
			size_t len, count, i;

			if (!f) return NULL;
			len = stb_filelen(f);
			buffer = (char *) malloc(len+1);
			len = fread(buffer, 1, len, f);
			buffer[len] = 0;
			fclose(f);

			// two passes through: first time count lines, second time set them
			for (i=0; i < 2; ++i) {
				s = buffer;
				if (i == 1)
				list[0] = s;
				count = 1;
				while (*s) {
					if (*s == '\n' || *s == '\r') {
						// detect if both cr & lf are together
						int crlf = (s[0] + s[1]) == ('\n' + '\r');
						if (i == 1) *s = 0;
						if (crlf) ++s;
						if (s[1]) {  // it's not over yet
							if (i == 1) list[count] = s+1;
							++count;
						}
					}
					++s;
				}
				if (i == 0) {
					list = (char **) malloc(sizeof(*list) * (count+1) + len+1);
					if (!list) return NULL;
					list[count] = 0;
					// recopy the file so there's just a single allocation to free
					memcpy(&list[count+1], buffer, len+1);
					free(buffer);
					buffer = (char *) &list[count+1];
					if (plen) *plen = count;
				}
			}
			return list;
		}

		char ** stb_stringfile_trimmed(char *name, int *len, char comment)
		{
			int i,n,o=0;
			char **s = stb_stringfile(name, &n);
			if (s == NULL) return NULL;
			for (i=0; i < n; ++i) {
				char *p = stb_skipwhite(s[i]);
				if (*p && *p != comment)
				s[o++] = p;
			}
			s[o] = NULL;
			if (len) *len = o;
			return s;
		}

		char * stb_fgets(char *buffer, int buflen, FILE *f)
		{
			char *p;
			buffer[0] = 0;
			p = fgets(buffer, buflen, f);
			if (p) {
				int n = strlen(p)-1;
				if (n >= 0)
				if (p[n] == '\n')
				p[n] = 0;
			}
			return p;
		}

		char * stb_fgets_malloc(FILE *f)
		{
			// avoid reallocing for small strings
			char quick_buffer[800];
			quick_buffer[sizeof(quick_buffer)-2] = 0;
			if (!fgets(quick_buffer, sizeof(quick_buffer), f))
			return NULL;

			if (quick_buffer[sizeof(quick_buffer)-2] == 0) {
				int n = strlen(quick_buffer);
				if (n > 0 && quick_buffer[n-1] == '\n')
				quick_buffer[n-1] = 0;
				return strdup(quick_buffer);
			} else {
				char *p;
				char *a = strdup(quick_buffer);
				int len = sizeof(quick_buffer)-1;

				while (!feof(f)) {
					if (a[len-1] == '\n') break;
					a = (char *) realloc(a, len*2);
					p = &a[len];
					p[len-2] = 0;
					if (!fgets(p, len, f))
					break;
					if (p[len-2] == 0) {
						len += strlen(p);
						break;
					}
					len = len + (len-1);
				}
				if (a[len-1] == '\n')
				a[len-1] = 0;
				return a;
			}
		}

		int stb_fullpath(char *abs, int abs_size, char *rel)
		{
#ifdef _MSC_VER
			return _fullpath(abs, rel, abs_size) != NULL;
#else
			if (rel[0] == '/' || rel[0] == '~') {
				if ((int) strlen(rel) >= abs_size)
				return 0;
				strcpy(abs,rel);
				return STB_TRUE;
			} else {
				int n;
				getcwd(abs, abs_size);
				n = strlen(abs);
				if (n+(int) strlen(rel)+2 <= abs_size) {
					abs[n] = '/';
					strcpy(abs+n+1, rel);
					return STB_TRUE;
				} else {
					return STB_FALSE;
				}
			}
#endif
		}

		static int stb_fcmp_core(FILE *f, FILE *g)
		{
			char buf1[1024],buf2[1024];
			int n1,n2, res=0;

			while (1) {
				n1 = fread(buf1, 1, sizeof(buf1), f);
				n2 = fread(buf2, 1, sizeof(buf2), g);
				res = memcmp(buf1,buf2,stb_min(n1,n2));
				if (res)
				break;
				if (n1 != n2) {
					res = n1 < n2 ? -1 : 1;
					break;
				}
				if (n1 == 0)
				break;
			}

			fclose(f);
			fclose(g);
			return res;
		}

		int stb_fcmp(char *s1, char *s2)
		{
			FILE *f = stb__fopen(s1, "rb");
			FILE *g = stb__fopen(s2, "rb");

			if (f == NULL || g == NULL) {
				if (f) fclose(f);
				if (g) {
					fclose(g);
					return STB_TRUE;
				}
				return f != NULL;
			}

			return stb_fcmp_core(f,g);
		}

		int stb_feq(char *s1, char *s2)
		{
			FILE *f = stb__fopen(s1, "rb");
			FILE *g = stb__fopen(s2, "rb");

			if (f == NULL || g == NULL) {
				if (f) fclose(f);
				if (g) fclose(g);
				return f == g;
			}

			// feq is faster because it shortcuts if they're different length
			if (stb_filelen(f) != stb_filelen(g)) {
				fclose(f);
				fclose(g);
				return 0;
			}

			return !stb_fcmp_core(f,g);
		}

		static stb_ptrmap *stb__files;

		typedef struct
		{
			char *temp_name;
			char *name;
			int errors;
		}stb__file_data;

		FILE * stb_fopen(char *filename, char *mode)
		{
			FILE *f;
			char name_full[4096];
			char temp_full[sizeof(name_full) + 12];
			int p;
#ifdef _MSC_VER
			int j;
#endif
			if (mode[0] != 'w' && !strchr(mode, '+'))
			return stb__fopen(filename, mode);

			// save away the full path to the file so if the program
			// changes the cwd everything still works right! unix has
			// better ways to do this, but we have to work in windows
			name_full[0] = '\0';// stb_fullpath reads name_full[0]
			if (stb_fullpath(name_full, sizeof(name_full), filename)==0)
			return 0;

			// try to generate a temporary file in the same directory
			p = strlen(name_full)-1;
			while (p > 0 && name_full[p] != '/' && name_full[p] != '\\'
					&& name_full[p] != ':' && name_full[p] != '~')
			--p;
			++p;

			memcpy(temp_full, name_full, p);

#ifdef _MSC_VER
			// try multiple times to make a temp file... just in
			// case some other process makes the name first
			for (j=0; j < 32; ++j) {
				strcpy(temp_full+p, "stmpXXXXXX");
				if (stb_mktemp(temp_full) == NULL)
				return 0;

				f = fopen(temp_full, mode);
				if (f != NULL)
				break;
			}
#else
			{
				strcpy(temp_full+p, "stmpXXXXXX");
#ifdef __MINGW32__
				int fd = open(mktemp(temp_full), O_RDWR);
#else
				int fd = mkstemp(temp_full);
#endif
				if (fd == -1) return NULL;
				f = fdopen(fd, mode);
				if (f == NULL) {
					unlink(temp_full);
					close(fd);
					return NULL;
				}
			}
#endif
			if (f != NULL) {
				stb__file_data *d = (stb__file_data *) malloc(sizeof(*d));
				if (!d) {assert(0); /* NOTREACHED */fclose(f); return NULL;}
				if (stb__files == NULL) stb__files = stb_ptrmap_create();
				d->temp_name = strdup(temp_full);
				d->name = strdup(name_full);
				d->errors = 0;
				stb_ptrmap_add(stb__files, f, d);
				return f;
			}

			return NULL;
		}

		int stb_fclose(FILE *f, int keep)
		{
			stb__file_data *d;

			int ok = STB_FALSE;
			if (f == NULL) return 0;

			if (ferror(f))
			keep = stb_keep_no;

			fclose(f);

			if (stb__files && stb_ptrmap_remove(stb__files, f, (void **) &d)) {
				if (stb__files->count == 0) {
					stb_ptrmap_destroy(stb__files);
					stb__files = NULL;
				}
			} else
			return STB_TRUE; // not special

			if (keep == stb_keep_if_different) {
				// check if the files are identical
				if (stb_feq(d->name, d->temp_name)) {
					keep = stb_keep_no;
					ok = STB_TRUE;  // report success if no change
				}
			}

			if (keep != stb_keep_no) {
				if (stb_fexists(d->name) && remove(d->name)) {
					// failed to delete old, so don't keep new
					keep = stb_keep_no;
				} else {
					if (!stb_rename(d->temp_name, d->name))
					ok = STB_TRUE;
					else
					keep=stb_keep_no;
				}
			}

			if (keep == stb_keep_no)
			remove(d->temp_name);

			free(d->temp_name);
			free(d->name);
			free(d);

			return ok;
		}

		int stb_copyfile(char *src, char *dest)
		{
			char raw_buffer[1024];
			char *buffer;
			int buf_size = 65536;

			FILE *f, *g;

			// if file already exists at destination, do nothing
			if (stb_feq(src, dest)) return STB_TRUE;

			// open file
			f = stb__fopen(src, "rb");
			if (f == NULL) return STB_FALSE;

			// open file for writing
			g = stb__fopen(dest, "wb");
			if (g == NULL) {
				fclose(f);
				return STB_FALSE;
			}

			buffer = (char *) malloc(buf_size);
			if (buffer == NULL) {
				buffer = raw_buffer;
				buf_size = sizeof(raw_buffer);
			}

			while (!feof(f)) {
				int n = fread(buffer, 1, buf_size, f);
				if (n != 0)
				fwrite(buffer, 1, n, g);
			}

			fclose(f);
			if (buffer != raw_buffer)
			free(buffer);

			fclose(g);
			return STB_TRUE;
		}

// varlen:
//    v' = (v >> 31) + (v < 0 ? ~v : v)<<1;  // small abs(v) => small v'
// output v as big endian v'+k for v' <= k:
//   1 byte :  v' <= 0x00000080          (  -64 <= v <   64)   7 bits
//   2 bytes:  v' <= 0x00004000          (-8192 <= v < 8192)  14 bits
//   3 bytes:  v' <= 0x00200000                               21 bits
//   4 bytes:  v' <= 0x10000000                               28 bits
// the number of most significant 1-bits in the first byte
// equals the number of bytes after the first

#define stb__varlen_xform(v)     (v<0 ? (~v << 1)+1 : (v << 1))

		int stb_size_varlen(int v) {return stb_size_varlenu(stb__varlen_xform(v));}
		int stb_size_varlenu(unsigned int v)
		{
			if (v < 0x00000080) return 1;
			if (v < 0x00004000) return 2;
			if (v < 0x00200000) return 3;
			if (v < 0x10000000) return 4;
			return 5;
		}

		void stb_fput_varlen(FILE *f, int v) {stb_fput_varlenu(f, stb__varlen_xform(v));}

		void stb_fput_varlenu(FILE *f, unsigned int z)
		{
			if (z >= 0x10000000) fputc(0xF0,f);
			if (z >= 0x00200000) fputc((z < 0x10000000 ? 0xE0 : 0)+(z>>24),f);
			if (z >= 0x00004000) fputc((z < 0x00200000 ? 0xC0 : 0)+(z>>16),f);
			if (z >= 0x00000080) fputc((z < 0x00004000 ? 0x80 : 0)+(z>> 8),f);
			fputc(z,f);
		}

#define stb_fgetc(f)    ((unsigned char) fgetc(f))

		int stb_fget_varlen(FILE *f)
		{
			unsigned int z = stb_fget_varlenu(f);
			return (z & 1) ? ~(z>>1) : (z>>1);
		}

		unsigned int stb_fget_varlenu(FILE *f)
		{
			unsigned int z;
			unsigned char d;
			d = stb_fgetc(f);

			if (d >= 0x80) {
				if (d >= 0xc0) {
					if (d >= 0xe0) {
						if (d == 0xf0) z = stb_fgetc(f) << 24;
						else z = (d - 0xe0) << 24;
						z += stb_fgetc(f) << 16;
					}
					else
					z = (d - 0xc0) << 16;
					z += stb_fgetc(f) << 8;
				} else
				z = (d - 0x80) << 8;
				z += stb_fgetc(f);
			} else
			z = d;
			return z;
		}

		stb_uint64 stb_fget_varlen64(FILE *f)
		{
			stb_uint64 z;
			unsigned char d;
			d = stb_fgetc(f);

			if (d >= 0x80) {
				if (d >= 0xc0) {
					if (d >= 0xe0) {
						if (d >= 0xf0) {
							if (d >= 0xf8) {
								if (d >= 0xfc) {
									if (d >= 0xfe) {
										if (d >= 0xff)
										z = (stb_uint64) stb_fgetc(f) << 56;
										else
										z = (stb_uint64) (d - 0xfe) << 56;
										z |= (stb_uint64) stb_fgetc(f) << 48;
									} else z = (stb_uint64) (d - 0xfc) << 48;
									z |= (stb_uint64) stb_fgetc(f) << 40;
								} else z = (stb_uint64) (d - 0xf8) << 40;
								z |= (stb_uint64) stb_fgetc(f) << 32;
							} else z = (stb_uint64) (d - 0xf0) << 32;
							z |= (stb_uint) stb_fgetc(f) << 24;
						} else z = (stb_uint) (d - 0xe0) << 24;
						z |= (stb_uint) stb_fgetc(f) << 16;
					} else z = (stb_uint) (d - 0xc0) << 16;
					z |= (stb_uint) stb_fgetc(f) << 8;
				} else z = (stb_uint) (d - 0x80) << 8;
				z |= stb_fgetc(f);
			} else
			z = d;

			return (z & 1) ? ~(z >> 1) : (z >> 1);
		}

		int stb_size_varlen64(stb_uint64 v)
		{
			if (v < 0x00000080) return 1;
			if (v < 0x00004000) return 2;
			if (v < 0x00200000) return 3;
			if (v < 0x10000000) return 4;
			if (v < STB_IMM_UINT64(0x0000000800000000)) return 5;
			if (v < STB_IMM_UINT64(0x0000040000000000)) return 6;
			if (v < STB_IMM_UINT64(0x0002000000000000)) return 7;
			if (v < STB_IMM_UINT64(0x0100000000000000)) return 8;
			return 9;
		}

		void stb_fput_varlen64(FILE *f, stb_uint64 v)
		{
			stb_uint64 z = stb__varlen_xform(v);
			int first=1;
			if (z >= STB_IMM_UINT64(0x100000000000000)) {
				fputc(0xff,f);
				first=0;
			}
			if (z >= STB_IMM_UINT64(0x02000000000000)) fputc((first ? 0xFE : 0)+(char)(z>>56),f), first=0;
			if (z >= STB_IMM_UINT64(0x00040000000000)) fputc((first ? 0xFC : 0)+(char)(z>>48),f), first=0;
			if (z >= STB_IMM_UINT64(0x00000800000000)) fputc((first ? 0xF8 : 0)+(char)(z>>40),f), first=0;
			if (z >= STB_IMM_UINT64(0x00000010000000)) fputc((first ? 0xF0 : 0)+(char)(z>>32),f), first=0;
			if (z >= STB_IMM_UINT64(0x00000000200000)) fputc((first ? 0xE0 : 0)+(char)(z>>24),f), first=0;
			if (z >= STB_IMM_UINT64(0x00000000004000)) fputc((first ? 0xC0 : 0)+(char)(z>>16),f), first=0;
			if (z >= STB_IMM_UINT64(0x00000000000080)) fputc((first ? 0x80 : 0)+(char)(z>> 8),f), first=0;
			fputc((char)z,f);
		}

		void stb_fput_ranged(FILE *f, int v, int b, stb_uint n)
		{
			v -= b;
			if (n <= (1 << 31))
			assert((stb_uint) v < n);
			if (n > (1 << 24)) fputc(v >> 24, f);
			if (n > (1 << 16)) fputc(v >> 16, f);
			if (n > (1 << 8)) fputc(v >> 8, f);
			fputc(v,f);
		}

		int stb_fget_ranged(FILE *f, int b, stb_uint n)
		{
			unsigned int v=0;
			if (n > (1 << 24)) v += stb_fgetc(f) << 24;
			if (n > (1 << 16)) v += stb_fgetc(f) << 16;
			if (n > (1 << 8)) v += stb_fgetc(f) << 8;
			v += stb_fgetc(f);
			return b+v;
		}

		int stb_size_ranged(int b, stb_uint n)
		{
			if (n > (1 << 24)) return 4;
			if (n > (1 << 16)) return 3;
			if (n > (1 << 8)) return 2;
			return 1;
		}

		void stb_fput_string(FILE *f, char *s)
		{
			int len = strlen(s);
			stb_fput_varlenu(f, len);
			fwrite(s, 1, len, f);
		}

// inverse of the above algorithm
		char *stb_fget_string(FILE *f, void *p)
		{
			char *s;
			int len = stb_fget_varlenu(f);
			if (len > 4096) return NULL;
			s = p ? stb_malloc_string(p, len+1) : (char *) malloc(len+1);
			fread(s, 1, len, f);
			s[len] = 0;
			return s;
		}

		char *stb_strdup(char *str, void *pool)
		{
			int len = strlen(str);
			char *p = stb_malloc_string(pool, len+1);
			strcpy(p, str);
			return p;
		}

// strip the trailing '/' or '\\' from a directory so we can refer to it
// as a file for _stat()
		char *stb_strip_final_slash(char *t)
		{
			if (t[0]) {
				char *z = t + strlen(t) - 1;
				// *z is the last character
				if (*z == '\\' || *z == '/')
				if (z != t+2 || t[1] != ':')// but don't strip it if it's e.g. "c:/"
				*z = 0;
				if (*z == '\\')
				*z = '/';// canonicalize to make sure it matches db
			}
			return t;
		}

		char *stb_strip_final_slash_regardless(char *t)
		{
			if (t[0]) {
				char *z = t + strlen(t) - 1;
				// *z is the last character
				if (*z == '\\' || *z == '/')
				*z = 0;
				if (*z == '\\')
				*z = '/';// canonicalize to make sure it matches db
			}
			return t;
		}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//                 Options parsing
//

		STB_EXTERN char **stb_getopt_param(int *argc, char **argv, char *param);
		STB_EXTERN char **stb_getopt(int *argc, char **argv);
		STB_EXTERN void stb_getopt_free(char **opts);

#ifdef STB_DEFINE

		void stb_getopt_free(char **opts)
		{
			int i;
			char ** o2 = opts;
			for (i=0; i < stb_arr_len(o2); ++i)
			free(o2[i]);
			stb_arr_free(o2);
		}

		char **stb_getopt(int *argc, char **argv)
		{
			return stb_getopt_param(argc, argv, "");
		}

		char **stb_getopt_param(int *argc, char **argv, char *param)
		{
			char ** opts=NULL;
			int i,j=1;
			for (i=1; i < *argc; ++i) {
				if (argv[i][0] != '-') {
					argv[j++] = argv[i];
				} else {
					if (argv[i][1] == 0) { // plain - == don't parse further options
						++i;
						while (i < *argc)
						argv[j++] = argv[i++];
						break;
					} else {
						int k;
						char *q = argv[i];  // traverse options list
						for (k=1; q[k]; ++k) {
							char *s;
							if (strchr(param, q[k])) { // does it take a parameter?
								char *t = &q[k+1], z = q[k];
								int len=0;
								if (*t == 0) {
									if (i == *argc-1) { // takes a parameter, but none found
										*argc = 0;
										stb_getopt_free(opts);
										return NULL;
									}
									t = argv[++i];
								} else
								k += strlen(t);
								len = strlen(t);
								s = (char *) malloc(len+2);
								if (!s) return NULL;
								s[0] = z;
								strcpy(s+1, t);
							} else {
								// no parameter
								s = (char *) malloc(2);
								if (!s) return NULL;
								s[0] = q[k];
								s[1] = 0;
							}
							stb_arr_push(opts, s);
						}
					}
				}
			}
			stb_arr_push(opts, NULL);
			*argc = j;
			return opts;
		}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//                 Portable directory reading
//

		STB_EXTERN char **stb_readdir_files(char *dir);
		STB_EXTERN char **stb_readdir_files_mask(char *dir, char *wild);
		STB_EXTERN char **stb_readdir_subdirs(char *dir);
		STB_EXTERN char **stb_readdir_subdirs_mask(char *dir, char *wild);
		STB_EXTERN void stb_readdir_free(char **files);
		STB_EXTERN char **stb_readdir_recursive(char *dir, char *filespec);
		STB_EXTERN void stb_delete_directory_recursive(char *dir);

#ifdef STB_DEFINE

#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#include <dirent.h>
#endif

		void stb_readdir_free(char **files)
		{
			char **f2 = files;
			int i;
			for (i=0; i < stb_arr_len(f2); ++i)
			free(f2[i]);
			stb_arr_free(f2);
		}

		static int isdotdirname(char *name)
		{
			if (name[0] == '.')
			return (name[1] == '.') ? !name[2] : !name[1];
			return 0;
		}

		STB_EXTERN int stb_wildmatchi(char *expr, char *candidate);
		static char **readdir_raw(char *dir, int return_subdirs, char *mask)
		{
			char **results = NULL;
			char buffer[4096], with_slash[4096];
			size_t n;

#ifdef _MSC_VER
			stb__wchar *ws;
			struct _wfinddata_t data;
#ifdef _WIN64
			const intptr_t none = -1;
			intptr_t z;
#else
			const long none = -1;
			long z;
#endif
#else // !_MSC_VER
			const DIR *none = NULL;
			DIR *z;
#endif

			n = stb_strscpy(buffer,dir,sizeof(buffer));
			if (!n || n >= sizeof(buffer))
			return NULL;
			stb_fixpath(buffer);
			n--;

			if (n > 0 && (buffer[n-1] != '/')) {
				buffer[n++] = '/';
			}
			buffer[n] = 0;
			if (!stb_strscpy(with_slash,buffer,sizeof(with_slash)))
			return NULL;

#ifdef _MSC_VER
			if (!stb_strscpy(buffer+n,"*.*",sizeof(buffer)-n))
			return NULL;
			ws = stb__from_utf8(buffer);
			z = _wfindfirst((const wchar_t *)ws, &data);
#else
			z = opendir(dir);
#endif

			if (z != none) {
				int nonempty = STB_TRUE;
#ifndef _MSC_VER
				struct dirent *data = readdir(z);
				nonempty = (data != NULL);
#endif

				if (nonempty) {

					do {
						int is_subdir;
#ifdef _MSC_VER
						char *name = stb__to_utf8((stb__wchar *)data.name);
						if (name == NULL) {
							fprintf(stderr, "%s to convert '%S' to %s!\n", "Unable", data.name, "utf8");
							continue;
						}
						is_subdir = !!(data.attrib & _A_SUBDIR);
#else
						char *name = data->d_name;
						if (!stb_strscpy(buffer+n,name,sizeof(buffer)-n))
						break;
						// Could follow DT_LNK, but would need to check for recursive links.
//            is_subdir = !!(data->d_type & DT_DIR);
						is_subdir = !!(data->d_type);
#endif

						if (is_subdir == return_subdirs) {
							if (!is_subdir || !isdotdirname(name)) {
								if (!mask || stb_wildmatchi(mask, name)) {
									char buffer[4096],*p=buffer;
									if ( stb_snprintf(buffer, sizeof(buffer), "%s%s", with_slash, name) < 0 )
									break;
									if (buffer[0] == '.' && buffer[1] == '/')
									p = buffer+2;
									stb_arr_push(results, strdup(p));
								}
							}
						}
					}
#ifdef _MSC_VER
					while (0 == _wfindnext(z, &data));
#else
					while ((data = readdir(z)) != NULL);
#endif
				}
#ifdef _MSC_VER
				_findclose(z);
#else
				closedir(z);
#endif
			}
			return results;
		}

		char **stb_readdir_files (char *dir) {return readdir_raw(dir, 0, NULL);}
		char **stb_readdir_subdirs(char *dir) {return readdir_raw(dir, 1, NULL);}
		char **stb_readdir_files_mask(char *dir, char *wild) {return readdir_raw(dir, 0, wild);}
		char **stb_readdir_subdirs_mask(char *dir, char *wild) {return readdir_raw(dir, 1, wild);}

		int stb__rec_max=0x7fffffff;
		static char **stb_readdir_rec(char **sofar, char *dir, char *filespec)
		{
			char **files;
			char ** dirs;
			char **p;

			if (stb_arr_len(sofar) >= stb__rec_max) return sofar;

			files = stb_readdir_files_mask(dir, filespec);
			stb_arr_for(p, files) {
				stb_arr_push(sofar, strdup(*p));
				if (stb_arr_len(sofar) >= stb__rec_max) break;
			}
			stb_readdir_free(files);
			if (stb_arr_len(sofar) >= stb__rec_max) return sofar;

			dirs = stb_readdir_subdirs(dir);
			stb_arr_for(p, dirs)
			sofar = stb_readdir_rec(sofar, *p, filespec);
			stb_readdir_free(dirs);
			return sofar;
		}

		char **stb_readdir_recursive(char *dir, char *filespec)
		{
			return stb_readdir_rec(NULL, dir, filespec);
		}

		void stb_delete_directory_recursive(char *dir)
		{
			char **list = stb_readdir_subdirs(dir);
			int i;
			for (i=0; i < stb_arr_len(list); ++i)
			stb_delete_directory_recursive(list[i]);
			stb_arr_free(list);
			list = stb_readdir_files(dir);
			for (i=0; i < stb_arr_len(list); ++i)
			if (!remove(list[i])) {
				// on windows, try again after making it writeable; don't ALWAYS
				// do this first since that would be slow in the normal case
#ifdef _MSC_VER
				_chmod(list[i], _S_IWRITE);
				remove(list[i]);
#endif
			}
			stb_arr_free(list);
			stb__windows(_rmdir,rmdir)(dir);
		}

#endif

//////////////////////////////////////////////////////////////////////////////
//
//   construct trees from filenames; useful for cmirror summaries

		typedef struct stb_dirtree2 stb_dirtree2;

		struct stb_dirtree2 {
			stb_dirtree2 **subdirs;

			// make convenient for stb_summarize_tree
			int num_subdir;
			float weight;

			// actual data
			char *fullpath;
			char *relpath;
			char **files;
		};

		STB_EXTERN stb_dirtree2 *stb_dirtree2_from_files_relative(char *src,
				char **filelist, int count);
		STB_EXTERN stb_dirtree2 *stb_dirtree2_from_files(char **filelist,
				int count);
		STB_EXTERN int stb_dir_is_prefix(char *dir, int dirlen, char *file);

#ifdef STB_DEFINE

		int stb_dir_is_prefix(char *dir, int dirlen, char *file)
		{
			if (dirlen == 0) return STB_TRUE;
			if (stb_strnicmp(dir, file, dirlen)) return STB_FALSE;
			if (file[dirlen] == '/' || file[dirlen] == '\\') return STB_TRUE;
			return STB_FALSE;
		}

		stb_dirtree2 *stb_dirtree2_from_files_relative(char *src, char **filelist, int count)
		{
			char buffer1[1024];
			int i;
			int dlen = strlen(src), elen;
			stb_dirtree2 *d;
			char ** descendents = NULL;
			char ** files = NULL;
			char *s;
			if (!count) return NULL;
			// first find all the ones that belong here... note this is will take O(NM) with N files and M subdirs
			for (i=0; i < count; ++i) {
				if (stb_dir_is_prefix(src, dlen, filelist[i])) {
					stb_arr_push(descendents, filelist[i]);
				}
			}
			if (descendents == NULL)
			return NULL;
			elen = dlen;
			// skip a leading slash
			if (elen == 0 && (descendents[0][0] == '/' || descendents[0][0] == '\\'))
			++elen;
			else if (elen)
			++elen;
			// now extract all the ones that have their root here
			for (i=0; i < stb_arr_len(descendents);) {
				if (!stb_strchr2(descendents[i]+elen, '/', '\\')) {
					stb_arr_push(files, descendents[i]);
					descendents[i] = descendents[stb_arr_len(descendents)-1];
					stb_arr_pop(descendents);
				} else
				++i;
			}
			// now create a record
			d = (stb_dirtree2 *) malloc(sizeof(*d));
			d->files = files;
			d->subdirs = NULL;
			d->fullpath = strdup(src);
			s = stb_strrchr2(d->fullpath, '/', '\\');
			if (s)
			++s;
			else
			s = d->fullpath;
			d->relpath = s;
			// now create the children
			qsort(descendents, stb_arr_len(descendents), sizeof(char *), stb_qsort_stricmp(0));
			buffer1[0] = 0;
			for (i=0; i < stb_arr_len(descendents); ++i) {
				char buffer2[1024];
				char *s = descendents[i] + elen, *t;
				t = stb_strchr2(s, '/', '\\');
				assert(t);
				stb_strncpy(buffer2, descendents[i], t-descendents[i]+1);
				if (stb_stricmp(buffer1, buffer2)) {
					stb_dirtree2 *t = stb_dirtree2_from_files_relative(buffer2, descendents, stb_arr_len(descendents));
					assert(t != NULL);
					strcpy(buffer1, buffer2);
					stb_arr_push(d->subdirs, t);
				}
			}
			d->num_subdir = stb_arr_len(d->subdirs);
			d->weight = 0;
			return d;
		}

		stb_dirtree2 *stb_dirtree2_from_files(char **filelist, int count)
		{
			return stb_dirtree2_from_files_relative("", filelist, count);
		}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//                 Checksums: CRC-32, ADLER32, SHA-1
//
//    CRC-32 and ADLER32 allow streaming blocks
//    SHA-1 requires either a complete buffer, max size 2^32 - 73
//          or it can checksum directly from a file, max 2^61

#define STB_ADLER32_SEED   1
#define STB_CRC32_SEED     0    // note that we logical NOT this in the code

		STB_EXTERN stb_uint
		stb_adler32(stb_uint adler32, stb_uchar *buffer, stb_uint buflen);
		STB_EXTERN stb_uint
		stb_crc32_block(stb_uint crc32, stb_uchar *buffer, stb_uint len);
		STB_EXTERN stb_uint stb_crc32(unsigned char *buffer, stb_uint len);

		STB_EXTERN void stb_sha1(unsigned char output[20],
				unsigned char *buffer, unsigned int len);
		STB_EXTERN int stb_sha1_file(unsigned char output[20], char *file);

		STB_EXTERN void stb_sha1_readable(char display[27],
				unsigned char sha[20]);

#ifdef STB_DEFINE
		stb_uint stb_crc32_block(stb_uint crc, unsigned char *buffer, stb_uint len)
		{
			static stb_uint crc_table[256];
			stb_uint i,j,s;
			crc = ~crc;

			if (crc_table[1] == 0)
			for(i=0; i < 256; i++) {
				for (s=i, j=0; j < 8; ++j)
				s = (s >> 1) ^ (s & 1 ? 0xedb88320 : 0);
				crc_table[i] = s;
			}
			for (i=0; i < len; ++i)
			crc = (crc >> 8) ^ crc_table[buffer[i] ^ (crc & 0xff)];
			return ~crc;
		}

		stb_uint stb_crc32(unsigned char *buffer, stb_uint len)
		{
			return stb_crc32_block(0, buffer, len);
		}

		stb_uint stb_adler32(stb_uint adler32, stb_uchar *buffer, stb_uint buflen)
		{
			const unsigned long ADLER_MOD = 65521;
			unsigned long s1 = adler32 & 0xffff, s2 = adler32 >> 16;
			unsigned long blocklen, i;

			blocklen = buflen % 5552;
			while (buflen) {
				for (i=0; i + 7 < blocklen; i += 8) {
					s1 += buffer[0], s2 += s1;
					s1 += buffer[1], s2 += s1;
					s1 += buffer[2], s2 += s1;
					s1 += buffer[3], s2 += s1;
					s1 += buffer[4], s2 += s1;
					s1 += buffer[5], s2 += s1;
					s1 += buffer[6], s2 += s1;
					s1 += buffer[7], s2 += s1;

					buffer += 8;
				}

				for (; i < blocklen; ++i)
				s1 += *buffer++, s2 += s1;

				s1 %= ADLER_MOD, s2 %= ADLER_MOD;
				buflen -= blocklen;
				blocklen = 5552;
			}
			return (s2 << 16) + s1;
		}

		static void stb__sha1(stb_uchar *chunk, stb_uint h[5])
		{
			int i;
			stb_uint a,b,c,d,e;
			stb_uint w[80];

			for (i=0; i < 16; ++i)
			w[i] = stb_big32(&chunk[i*4]);
			for (i=16; i < 80; ++i) {
				stb_uint t;
				t = w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16];
				w[i] = (t + t) | (t >> 31);
			}

			a = h[0];
			b = h[1];
			c = h[2];
			d = h[3];
			e = h[4];

#define STB__SHA1(k,f)                                            \
   {                                                                 \
      stb_uint temp = (a << 5) + (a >> 27) + (f) + e + (k) + w[i];  \
      e = d;                                                       \
      d = c;                                                     \
      c = (b << 30) + (b >> 2);                               \
      b = a;                                              \
      a = temp;                                    \
   }

			i=0;
			for (; i < 20; ++i) STB__SHA1(0x5a827999, d ^ (b & (c ^ d)) );
			for (; i < 40; ++i) STB__SHA1(0x6ed9eba1, b ^ c ^ d );
			for (; i < 60; ++i) STB__SHA1(0x8f1bbcdc, (b & c) + (d & (b ^ c)) );
			for (; i < 80; ++i) STB__SHA1(0xca62c1d6, b ^ c ^ d );

#undef STB__SHA1

			h[0] += a;
			h[1] += b;
			h[2] += c;
			h[3] += d;
			h[4] += e;
		}

		void stb_sha1(stb_uchar output[20], stb_uchar *buffer, stb_uint len)
		{
			unsigned char final_block[128];
			stb_uint end_start, final_len, j;
			int i;

			stb_uint h[5];

			h[0] = 0x67452301;
			h[1] = 0xefcdab89;
			h[2] = 0x98badcfe;
			h[3] = 0x10325476;
			h[4] = 0xc3d2e1f0;

			// we need to write padding to the last one or two
			// blocks, so build those first into 'final_block'

			// we have to write one special byte, plus the 8-byte length

			// compute the block where the data runs out
			end_start = len & ~63;

			// compute the earliest we can encode the length
			if (((len+9) & ~63) == end_start) {
				// it all fits in one block, so fill a second-to-last block
				end_start -= 64;
			}

			final_len = end_start + 128;

			// now we need to copy the data in
			assert(end_start + 128 >= len+9);
			assert(end_start < len || len < 64-9);

			j = 0;
			if (end_start > len)
			j = (stb_uint) - (int) end_start;

			for (; end_start + j < len; ++j)
			final_block[j] = buffer[end_start + j];
			final_block[j++] = 0x80;
			while (j < 128-5)// 5 byte length, so write 4 extra padding bytes
			final_block[j++] = 0;
			// big-endian size
			final_block[j++] = len >> 29;
			final_block[j++] = len >> 21;
			final_block[j++] = len >> 13;
			final_block[j++] = len >> 5;
			final_block[j++] = len << 3;
			assert(j == 128 && end_start + j == final_len);

			for (j=0; j < final_len; j += 64) { // 512-bit chunks
				if (j+64 >= end_start+64)
				stb__sha1(&final_block[j - end_start], h);
				else
				stb__sha1(&buffer[j], h);
			}

			for (i=0; i < 5; ++i) {
				output[i*4 + 0] = h[i] >> 24;
				output[i*4 + 1] = h[i] >> 16;
				output[i*4 + 2] = h[i] >> 8;
				output[i*4 + 3] = h[i] >> 0;
			}
		}

#ifdef _MSC_VER
		int stb_sha1_file(stb_uchar output[20], char *file)
		{
			int i;
			stb_uint64 length=0;
			unsigned char buffer[128];

			FILE *f = stb__fopen(file, "rb");
			stb_uint h[5];

			if (f == NULL) return 0; // file not found

			h[0] = 0x67452301;
			h[1] = 0xefcdab89;
			h[2] = 0x98badcfe;
			h[3] = 0x10325476;
			h[4] = 0xc3d2e1f0;

			for(;;) {
				int n = fread(buffer, 1, 64, f);
				if (n == 64) {
					stb__sha1(buffer, h);
					length += n;
				} else {
					int block = 64;

					length += n;

					buffer[n++] = 0x80;

					// if there isn't enough room for the length, double the block
					if (n + 8 > 64)
					block = 128;

					// pad to end
					memset(buffer+n, 0, block-8-n);

					i = block - 8;
					buffer[i++] = (stb_uchar) (length >> 53);
					buffer[i++] = (stb_uchar) (length >> 45);
					buffer[i++] = (stb_uchar) (length >> 37);
					buffer[i++] = (stb_uchar) (length >> 29);
					buffer[i++] = (stb_uchar) (length >> 21);
					buffer[i++] = (stb_uchar) (length >> 13);
					buffer[i++] = (stb_uchar) (length >> 5);
					buffer[i++] = (stb_uchar) (length << 3);
					assert(i == block);
					stb__sha1(buffer, h);
					if (block == 128)
					stb__sha1(buffer+64, h);
					else
					assert(block == 64);
					break;
				}
			}
			fclose(f);

			for (i=0; i < 5; ++i) {
				output[i*4 + 0] = h[i] >> 24;
				output[i*4 + 1] = h[i] >> 16;
				output[i*4 + 2] = h[i] >> 8;
				output[i*4 + 3] = h[i] >> 0;
			}

			return 1;
		}
#endif // _MSC_VER

// client can truncate this wherever they like
		void stb_sha1_readable(char display[27], unsigned char sha[20])
		{
			char encoding[65] = "0123456789abcdefghijklmnopqrstuv"
			"wxyzABCDEFGHIJKLMNOPQRSTUVWXYZ%$";
			int num_bits = 0, acc=0;
			int i=0,o=0;
			while (o < 26) {
				int v;
				// expand the accumulator
				if (num_bits < 6) {
					assert(i != 20);
					acc += sha[i++] << num_bits;
					num_bits += 8;
				}
				v = acc & ((1 << 6) - 1);
				display[o++] = encoding[v];
				acc >>= 6;
				num_bits -= 6;
			}
			assert(num_bits == 20*8 - 26*6);
			display[o++] = encoding[acc];
		}

#endif // STB_DEFINE

///////////////////////////////////////////////////////////
//
// simplified WINDOWS registry interface... hopefully
// we'll never actually use this?

#if defined(_WIN32)

		STB_EXTERN void * stb_reg_open(char *mode, char *where); // mode: "rHKLM" or "rHKCU" or "w.."
		STB_EXTERN void stb_reg_close(void *reg);
		STB_EXTERN int stb_reg_read(void *zreg, char *str, void *data, unsigned long len);
		STB_EXTERN int stb_reg_read_string(void *zreg, char *str, char *data, int len);
		STB_EXTERN void stb_reg_write(void *zreg, char *str, void *data, unsigned long len);
		STB_EXTERN void stb_reg_write_string(void *zreg, char *str, char *data);

#if defined(STB_DEFINE) && !defined(STB_NO_REGISTRY)

#define STB_HAS_REGISTRY

#ifndef _WINDOWS_

#define HKEY void *

		STB_EXTERN __declspec(dllimport) long __stdcall RegCloseKey ( HKEY hKey );
		STB_EXTERN __declspec(dllimport) long __stdcall RegCreateKeyExA ( HKEY hKey, const char * lpSubKey,
				int Reserved, char * lpClass, int dwOptions,
				int samDesired, void *lpSecurityAttributes, HKEY * phkResult, int * lpdwDisposition );
		STB_EXTERN __declspec(dllimport) long __stdcall RegDeleteKeyA ( HKEY hKey, const char * lpSubKey );
		STB_EXTERN __declspec(dllimport) long __stdcall RegQueryValueExA ( HKEY hKey, const char * lpValueName,
				int * lpReserved, unsigned long * lpType, unsigned char * lpData, unsigned long * lpcbData );
		STB_EXTERN __declspec(dllimport) long __stdcall RegSetValueExA ( HKEY hKey, const char * lpValueName,
				int Reserved, int dwType, const unsigned char* lpData, int cbData );
		STB_EXTERN __declspec(dllimport) long __stdcall RegOpenKeyExA ( HKEY hKey, const char * lpSubKey,
				int ulOptions, int samDesired, HKEY * phkResult );

#endif // _WINDOWS_

#define STB__REG_OPTION_NON_VOLATILE  0
#define STB__REG_KEY_ALL_ACCESS       0x000f003f
#define STB__REG_KEY_READ             0x00020019

		void *stb_reg_open(char *mode, char *where)
		{
			long res;
			HKEY base;
			HKEY zreg;
			if (!stb_stricmp(mode+1, "cu") || !stb_stricmp(mode+1, "hkcu"))
			base = (HKEY) 0x80000001; // HKCU
			else if (!stb_stricmp(mode+1, "lm") || !stb_stricmp(mode+1, "hklm"))
			base = (HKEY) 0x80000002;// HKLM
			else
			return NULL;

			if (mode[0] == 'r')
			res = RegOpenKeyExA(base, where, 0, STB__REG_KEY_READ, &zreg);
			else if (mode[0] == 'w')
			res = RegCreateKeyExA(base, where, 0, NULL, STB__REG_OPTION_NON_VOLATILE, STB__REG_KEY_ALL_ACCESS, NULL, &zreg, NULL);
			else
			return NULL;

			return res ? NULL : zreg;
		}

		void stb_reg_close(void *reg)
		{
			RegCloseKey((HKEY) reg);
		}

#define STB__REG_SZ         1
#define STB__REG_BINARY     3
#define STB__REG_DWORD      4

		int stb_reg_read(void *zreg, char *str, void *data, unsigned long len)
		{
			unsigned long type;
			unsigned long alen = len;
			if (0 == RegQueryValueExA((HKEY) zreg, str, 0, &type, (unsigned char *) data, &len))
			if (type == STB__REG_BINARY || type == STB__REG_SZ || type == STB__REG_DWORD) {
				if (len < alen)
				*((char *) data + len) = 0;
				return 1;
			}
			return 0;
		}

		void stb_reg_write(void *zreg, char *str, void *data, unsigned long len)
		{
			if (zreg)
			RegSetValueExA((HKEY) zreg, str, 0, STB__REG_BINARY, (const unsigned char *) data, len);
		}

		int stb_reg_read_string(void *zreg, char *str, char *data, int len)
		{
			if (!stb_reg_read(zreg, str, data, len)) return 0;
			data[len-1] = 0; // force a 0 at the end of the string no matter what
			return 1;
		}

		void stb_reg_write_string(void *zreg, char *str, char *data)
		{
			if (zreg)
			RegSetValueExA((HKEY) zreg, str, 0, STB__REG_SZ, (const unsigned char *) data, strlen(data)+1);
		}
#endif  // STB_DEFINE
#endif  // _WIN32

//////////////////////////////////////////////////////////////////////////////
//
//     stb_cfg - This is like the registry, but the config info
//               is all stored in plain old files where we can
//               backup and restore them easily. The LOCATION of
//               the config files is gotten from... the registry!

#ifndef STB_NO_STB_STRINGS
		typedef struct stb_cfg_st stb_cfg;

		STB_EXTERN stb_cfg * stb_cfg_open(char *config, char *mode); // mode = "r", "w"
		STB_EXTERN void stb_cfg_close(stb_cfg *cfg);
		STB_EXTERN int stb_cfg_read(stb_cfg *cfg, char *key, void *value,
				int len);
		STB_EXTERN void stb_cfg_write(stb_cfg *cfg, char *key, void *value,
				int len);
		STB_EXTERN int stb_cfg_read_string(stb_cfg *cfg, char *key, char *value,
				int len);
		STB_EXTERN void stb_cfg_write_string(stb_cfg *cfg, char *key,
				char *value);
		STB_EXTERN int stb_cfg_delete(stb_cfg *cfg, char *key);
		STB_EXTERN void stb_cfg_set_directory(char *dir);

#ifdef STB_DEFINE

		typedef struct
		{
			char *key;
			void *value;
			int value_len;
		}stb__cfg_item;

		struct stb_cfg_st
		{
			stb__cfg_item *data;
			char *loaded_file;   // this needs to be freed
			FILE *f;// write the data to this file on close
		};

		static char *stb__cfg_sig = "sTbCoNfIg!\0\0";
		static char stb__cfg_dir[512];
		STB_EXTERN void stb_cfg_set_directory(char *dir)
		{
			strcpy(stb__cfg_dir, dir);
		}

		STB_EXTERN stb_cfg * stb_cfg_open(char *config, char *mode)
		{
			size_t len;
			stb_cfg *z;
			char file[512];
			if (mode[0] != 'r' && mode[0] != 'w') return NULL;

			if (!stb__cfg_dir[0]) {
#ifdef _WIN32
				strcpy(stb__cfg_dir, "c:/stb");
#else
				strcpy(stb__cfg_dir, "~/.stbconfig");
#endif

#ifdef STB_HAS_REGISTRY
				{
					void *reg = stb_reg_open("rHKLM", "Software\\SilverSpaceship\\stb");
					if (reg) {
						stb_reg_read_string(reg, "config_dir", stb__cfg_dir, sizeof(stb__cfg_dir));
						stb_reg_close(reg);
					}
				}
#endif
			}

			sprintf(file, "%s/%s.cfg", stb__cfg_dir, config);

			z = (stb_cfg *) stb_malloc(0, sizeof(*z));
			z->data = NULL;

			z->loaded_file = stb_filec(file, &len);
			if (z->loaded_file) {
				char *s = z->loaded_file;
				if (!memcmp(s, stb__cfg_sig, 12)) {
					char *s = z->loaded_file + 12;
					while (s < z->loaded_file + len) {
						stb__cfg_item a;
						int n = *(stb_int16 *) s;
						a.key = s+2;
						s = s+2 + n;
						a.value_len = *(int *) s;
						s += 4;
						a.value = s;
						s += a.value_len;
						stb_arr_push(z->data, a);
					}
					assert(s == z->loaded_file + len);
				}
			}

			if (mode[0] == 'w')
			z->f = fopen(file, "wb");
			else
			z->f = NULL;

			return z;
		}

		void stb_cfg_close(stb_cfg *z)
		{
			if (z->f) {
				int i;
				// write the file out
				fwrite(stb__cfg_sig, 12, 1, z->f);
				for (i=0; i < stb_arr_len(z->data); ++i) {
					stb_int16 n = strlen(z->data[i].key)+1;
					fwrite(&n, 2, 1, z->f);
					fwrite(z->data[i].key, n, 1, z->f);
					fwrite(&z->data[i].value_len, 4, 1, z->f);
					fwrite(z->data[i].value, z->data[i].value_len, 1, z->f);
				}
				fclose(z->f);
			}
			stb_arr_free(z->data);
			stb_free(z);
		}

		int stb_cfg_read(stb_cfg *z, char *key, void *value, int len)
		{
			int i;
			for (i=0; i < stb_arr_len(z->data); ++i) {
				if (!stb_stricmp(z->data[i].key, key)) {
					int n = stb_min(len, z->data[i].value_len);
					memcpy(value, z->data[i].value, n);
					if (n < len)
					*((char *) value + n) = 0;
					return 1;
				}
			}
			return 0;
		}

		void stb_cfg_write(stb_cfg *z, char *key, void *value, int len)
		{
			int i;
			for (i=0; i < stb_arr_len(z->data); ++i)
			if (!stb_stricmp(z->data[i].key, key))
			break;
			if (i == stb_arr_len(z->data)) {
				stb__cfg_item p;
				p.key = stb_strdup(key, z);
				p.value = NULL;
				p.value_len = 0;
				stb_arr_push(z->data, p);
			}
			z->data[i].value = stb_malloc(z, len);
			z->data[i].value_len = len;
			memcpy(z->data[i].value, value, len);
		}

		int stb_cfg_delete(stb_cfg *z, char *key)
		{
			int i;
			for (i=0; i < stb_arr_len(z->data); ++i)
			if (!stb_stricmp(z->data[i].key, key)) {
				stb_arr_fastdelete(z->data, i);
				return 1;
			}
			return 0;
		}

		int stb_cfg_read_string(stb_cfg *z, char *key, char *value, int len)
		{
			if (!stb_cfg_read(z, key, value, len)) return 0;
			value[len-1] = 0;
			return 1;
		}

		void stb_cfg_write_string(stb_cfg *z, char *key, char *value)
		{
			stb_cfg_write(z, key, value, strlen(value)+1);
		}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//     stb_dirtree  - load a description of a directory tree
//                      uses a cache and stat()s the directories for changes
//                      MUCH faster on NTFS, _wrong_ on FAT32, so should
//                      ignore the db on FAT32

#ifdef _WIN32

		typedef struct
		{
			char * path;           // full path from passed-in root
			time_t last_modified;
			int num_files;
			int flag;
		}stb_dirtree_dir;

		typedef struct
		{
			char *name;              // name relative to path
			int dir;// index into dirs[] array
			stb_int64 size;// size, max 4GB
			time_t last_modified;
			int flag;
		}stb_dirtree_file;

		typedef struct
		{
			stb_dirtree_dir *dirs;
			stb_dirtree_file *files;

			// internal use
			void * string_pool;// used to free data en masse
		}stb_dirtree;

		extern void stb_dirtree_free ( stb_dirtree *d );
		extern stb_dirtree *stb_dirtree_get ( char *dir);
		extern stb_dirtree *stb_dirtree_get_dir ( char *dir, char *cache_dir);
		extern stb_dirtree *stb_dirtree_get_with_file ( char *dir, char *cache_file);

// get a list of all the files recursively underneath 'dir'
//
// cache_file is used to store a copy of the directory tree to speed up
// later calls. It must be unique to 'dir' and the current working
// directory! Otherwise who knows what will happen (a good solution
// is to put it _in_ dir, but this API doesn't force that).
//
// Also, it might be possible to break this if you have two different processes
// do a call to stb_dirtree_get() with the same cache file at about the same
// time, but I _think_ it might just work.

// i needed to build an identical data structure representing the state of
// a mirrored copy WITHOUT bothering to rescan it (i.e. we're mirroring to
// it WITHOUT scanning it, e.g. it's over the net), so this requires access
// to all of the innards.
		extern void stb_dirtree_db_add_dir(stb_dirtree *active, char *path, time_t last);
		extern void stb_dirtree_db_add_file(stb_dirtree *active, char *name, int dir, stb_int64 size, time_t last);
		extern void stb_dirtree_db_read(stb_dirtree *target, char *filename, char *dir);
		extern void stb_dirtree_db_write(stb_dirtree *target, char *filename, char *dir);

#ifdef STB_DEFINE
		static void stb__dirtree_add_dir(char *path, time_t last, stb_dirtree *active)
		{
			stb_dirtree_dir d;
			d.last_modified = last;
			d.num_files = 0;
			d.path = stb_strdup(path, active->string_pool);
			stb_arr_push(active->dirs, d);
		}

		static void stb__dirtree_add_file(char *name, int dir, stb_int64 size, time_t last, stb_dirtree *active)
		{
			stb_dirtree_file f;
			f.dir = dir;
			f.size = size;
			f.last_modified = last;
			f.name = stb_strdup(name, active->string_pool);
			++active->dirs[dir].num_files;
			stb_arr_push(active->files, f);
		}

// version 02 supports > 4GB files
		static char stb__signature[12] = {'s', 'T', 'b', 'D', 'i', 'R', 't', 'R', 'e', 'E', '0', '2'};

		static void stb__dirtree_save_db(char *filename, stb_dirtree *data, char *root)
		{
			int i, num_dirs_final=0, num_files_final;
			char *info = root ? root : "";
			int *remap;
			FILE *f = fopen(filename, "wb");
			if (!f) return;

			fwrite(stb__signature, sizeof(stb__signature), 1, f);
			fwrite(info, strlen(info)+1, 1, f);
			// need to be slightly tricky and not write out NULLed directories, nor the root

			// build remapping table of all dirs we'll be writing out
			remap = (int *) malloc(sizeof(remap[0]) * stb_arr_len(data->dirs));
			for (i=0; i < stb_arr_len(data->dirs); ++i) {
				if (data->dirs[i].path == NULL || (root && 0==stb_stricmp(data->dirs[i].path, root))) {
					remap[i] = -1;
				} else {
					remap[i] = num_dirs_final++;
				}
			}

			fwrite(&num_dirs_final, 4, 1, f);
			for (i=0; i < stb_arr_len(data->dirs); ++i) {
				if (remap[i] >= 0) {
					fwrite(&data->dirs[i].last_modified, 4, 1, f);
					stb_fput_string(f, data->dirs[i].path);
				}
			}

			num_files_final = 0;
			for (i=0; i < stb_arr_len(data->files); ++i)
			if (remap[data->files[i].dir] >= 0 && data->files[i].name)
			++num_files_final;

			fwrite(&num_files_final, 4, 1, f);
			for (i=0; i < stb_arr_len(data->files); ++i) {
				if (remap[data->files[i].dir] >= 0 && data->files[i].name) {
					stb_fput_ranged(f, remap[data->files[i].dir], 0, num_dirs_final);
					stb_fput_varlen64(f, data->files[i].size);
					fwrite(&data->files[i].last_modified, 4, 1, f);
					stb_fput_string(f, data->files[i].name);
				}
			}

			fclose(f);
		}

// note: stomps any existing data, rather than appending
		static void stb__dirtree_load_db(char *filename, stb_dirtree *data, char *dir)
		{
			char sig[2048];
			int i,n;
			FILE *f = fopen(filename, "rb");

			if (!f) return;

			data->string_pool = stb_malloc(0,1);

			fread(sig, sizeof(stb__signature), 1, f);
			if (memcmp(stb__signature, sig, sizeof(stb__signature))) {fclose(f); return;}
			if (!fread(sig, strlen(dir)+1, 1, f)) {fclose(f); return;}
			if (stb_stricmp(sig,dir)) {fclose(f); return;}

			// we can just read them straight in, because they're guaranteed to be valid
			fread(&n, 4, 1, f);
			stb_arr_setlen(data->dirs, n);
			for(i=0; i < stb_arr_len(data->dirs); ++i) {
				fread(&data->dirs[i].last_modified, 4, 1, f);
				data->dirs[i].path = stb_fget_string(f, data->string_pool);
				if (data->dirs[i].path == NULL) goto bail;
			}
			fread(&n, 4, 1, f);
			stb_arr_setlen(data->files, n);
			for (i=0; i < stb_arr_len(data->files); ++i) {
				data->files[i].dir = stb_fget_ranged(f, 0, stb_arr_len(data->dirs));
				data->files[i].size = stb_fget_varlen64(f);
				fread(&data->files[i].last_modified, 4, 1, f);
				data->files[i].name = stb_fget_string(f, data->string_pool);
				if (data->files[i].name == NULL) goto bail;
			}

			if (0) {
				bail:
				stb_arr_free(data->dirs);
				stb_arr_free(data->files);
			}
			fclose(f);
		}

		static int stb__dircount, stb__dircount_mask, stb__showfile;
		static void stb__dirtree_scandir(char *path, time_t last_time, stb_dirtree *active)
		{
			// this is dumb depth first; theoretically it might be faster
			// to fully traverse each directory before visiting its children,
			// but it's complicated and didn't seem like a gain in the test app

			int n;

			struct _wfinddatai64_t c_file;
			long hFile;
			stb__wchar full_path[1024];
			int has_slash;
			if (stb__showfile) printf("<");

			has_slash = (path[0] && path[strlen(path)-1] == '/');

			// @TODO: do this concatenation without using swprintf to avoid this mess:
#if defined(_MSC_VER) && _MSC_VER < 1400
			if (has_slash)
			swprintf(full_path, L"%s*", stb__from_utf8(path));
			else
			swprintf(full_path, L"%s/*", stb__from_utf8(path));
#else
			if (has_slash)
			swprintf(full_path, 1024, L"%s*", stb__from_utf8(path));
			else
			swprintf(full_path, 1024, L"%s/*", stb__from_utf8(path));
#endif

			// it's possible this directory is already present: that means it was in the
			// cache, but its parent wasn't... in that case, we're done with it
			if (stb__showfile) printf("C[%d]", stb_arr_len(active->dirs));
			for (n=0; n < stb_arr_len(active->dirs); ++n)
			if (0 == stb_stricmp(active->dirs[n].path, path)) {
				if (stb__showfile) printf("D");
				return;
			}
			if (stb__showfile) printf("E");

			// otherwise, we need to add it
			stb__dirtree_add_dir(path, last_time, active);
			n = stb_arr_lastn(active->dirs);

			if (stb__showfile) printf("[");
			if( (hFile = _wfindfirsti64( full_path, &c_file )) != -1L ) {
				do {
					if (stb__showfile) printf(")");
					if (c_file.attrib & _A_SUBDIR) {
						// ignore subdirectories starting with '.', e.g. "." and ".."
						if (c_file.name[0] != '.') {
							char *new_path = (char *) full_path;
							char *temp = stb__to_utf8(c_file.name);

							if (has_slash)
							sprintf(new_path, "%s%s", path, temp);
							else
							sprintf(new_path, "%s/%s", path, temp);

							if (stb__dircount_mask) {
								++stb__dircount;
								if (!(stb__dircount & stb__dircount_mask)) {
									printf("%s\r", new_path);
								}
							}

							stb__dirtree_scandir(new_path, c_file.time_write, active);
						}
					} else {
						char *temp = stb__to_utf8(c_file.name);
						stb__dirtree_add_file(temp, n, c_file.size, c_file.time_write, active);
					}
					if (stb__showfile) printf("(");
				}while( _wfindnexti64( hFile, &c_file ) == 0 );
				if (stb__showfile) printf("]");
				_findclose( hFile );
			}
			if (stb__showfile) printf(">\n");
		}

// scan the database and see if it's all valid
		static int stb__dirtree_update_db(stb_dirtree *db, stb_dirtree *active)
		{
			int changes_detected = STB_FALSE;
			int i;
			int *remap;
			int *rescan=NULL;
			remap = (int *) malloc(sizeof(remap[0]) * stb_arr_len(db->dirs));
			memset(remap, 0, sizeof(remap[0]) * stb_arr_len(db->dirs));
			rescan = NULL;

			for (i=0; i < stb_arr_len(db->dirs); ++i) {
				struct _stat info;
				if (stb__dircount_mask) {
					++stb__dircount;
					if (!(stb__dircount & stb__dircount_mask)) {
						printf(".");
					}
				}
				if (0 == _stat(db->dirs[i].path, &info)) {
					if (info.st_mode & _S_IFDIR) {
						// it's still a directory, as expected
						int n = abs(info.st_mtime - db->dirs[i].last_modified);
						if (n > 1 && n != 3600) { // the 3600 is a hack because sometimes this jumps for no apparent reason, even when no time zone or DST issues are at play
							// it's changed! force a rescan
							// we don't want to scan it until we've stat()d its
							// subdirs, though, so we queue it
							if (stb__showfile) printf("Changed: %s - %08x:%08x\n", db->dirs[i].path, db->dirs[i].last_modified, info.st_mtime);
							stb_arr_push(rescan, i);
							// update the last_mod time
							db->dirs[i].last_modified = info.st_mtime;
							// ignore existing files in this dir
							remap[i] = -1;
							changes_detected = STB_TRUE;
						} else {
							// it hasn't changed, just copy it through unchanged
							stb__dirtree_add_dir(db->dirs[i].path, db->dirs[i].last_modified, active);
							remap[i] = stb_arr_lastn(active->dirs);
						}
					} else {
						// this path used to refer to a directory, but now it's a file!
						// assume that the parent directory is going to be forced to rescan anyway
						goto delete_entry;
					}
				} else {
					delete_entry:
					// directory no longer exists, so don't copy it
					// we don't free it because it's in the string pool now
					db->dirs[i].path = NULL;
					remap[i] = -1;
					changes_detected = STB_TRUE;
				}
			}

			// at this point, we have:
			//
			//   <rescan> holds a list of directory indices that need to be scanned due to being out of date
			//   <remap> holds the directory index in <active> for each dir in <db>, if it exists; -1 if not
			//           directories in <rescan> are not in <active> yet

			// so we can go ahead and remap all the known files right now
			for (i=0; i < stb_arr_len(db->files); ++i) {
				int dir = db->files[i].dir;
				if (remap[dir] >= 0) {
					stb__dirtree_add_file(db->files[i].name, remap[dir], db->files[i].size, db->files[i].last_modified, active);
				}
			}

			// at this point we're done with db->files, and done with remap
			free(remap);

			// now scan those directories using the standard scan
			for (i=0; i < stb_arr_len(rescan); ++i) {
				int z = rescan[i];
				stb__dirtree_scandir(db->dirs[z].path, db->dirs[z].last_modified, active);
			}
			stb_arr_free(rescan);

			return changes_detected;
		}

		static void stb__dirtree_free_raw(stb_dirtree *d)
		{
			stb_free(d->string_pool);
			stb_arr_free(d->dirs);
			stb_arr_free(d->files);
		}

		stb_dirtree *stb_dirtree_get_with_file(char *dir, char *cache_file)
		{
			stb_dirtree *output = (stb_dirtree *) malloc(sizeof(*output));
			stb_dirtree db,active;
			int prev_dir_count, cache_mismatch;

			char *stripped_dir; // store the directory name without a trailing '/' or '\\'

			// load the database of last-known state on disk
			db.string_pool = NULL;
			db.files = NULL;
			db.dirs = NULL;

			stripped_dir = stb_strip_final_slash(strdup(dir));

			if (cache_file != NULL)
			stb__dirtree_load_db(cache_file, &db, stripped_dir);
			else if (stb__showfile)
			printf("No cache file\n");

			active.files = NULL;
			active.dirs = NULL;
			active.string_pool = stb_malloc(0,1);// @TODO: share string pools between both?

			// check all the directories in the database; make note if
			// anything we scanned had changed, and rescan those things
			cache_mismatch = stb__dirtree_update_db(&db, &active);

			// check the root tree
			prev_dir_count = stb_arr_len(active.dirs);// record how many directories we've seen

			stb__dirtree_scandir(stripped_dir, 0, &active);// no last_modified time available for root

			if (stb__dircount_mask)
			printf("                                                                              \r");

			// done with the DB; write it back out if any changes, i.e. either
			//      1. any inconsistency found between cached information and actual disk
			//   or 2. if scanning the root found any new directories--which we detect because
			//         more than one directory got added to the active db during that scan
			if (cache_mismatch || stb_arr_len(active.dirs) > prev_dir_count+1)
			stb__dirtree_save_db(cache_file, &active, stripped_dir);

			free(stripped_dir);

			stb__dirtree_free_raw(&db);

			*output = active;
			return output;
		}

		stb_dirtree *stb_dirtree_get_dir(char *dir, char *cache_dir)
		{
			int i;
			stb_uint8 sha[20];
			char dir_lower[1024];
			char cache_file[1024],*s;
			if (cache_dir == NULL)
			return stb_dirtree_get_with_file(dir, NULL);
			strcpy(dir_lower, dir);
			stb_tolower(dir_lower);
			stb_sha1(sha, (unsigned char *) dir_lower, strlen(dir_lower));
			strcpy(cache_file, cache_dir);
			s = cache_file + strlen(cache_file);
			if (s[-1] != '//' && s[-1] != '\\') *s++ = '/';
			strcpy(s, "dirtree_");
			s += strlen(s);
			for (i=0; i < 8; ++i) {
				char *hex = "0123456789abcdef";
				stb_uint z = sha[i];
				*s++ = hex[z >> 4];
				*s++ = hex[z & 15];
			}
			strcpy(s, ".bin");
			return stb_dirtree_get_with_file(dir, cache_file);
		}

		stb_dirtree *stb_dirtree_get(char *dir)
		{
			char cache_dir[256];
			strcpy(cache_dir, "c:/stb");
#ifdef STB_HAS_REGISTRY
			{
				void *reg = stb_reg_open("rHKLM", "Software\\SilverSpaceship\\stb");
				if (reg) {
					stb_reg_read(reg, "dirtree", cache_dir, sizeof(cache_dir));
					stb_reg_close(reg);
				}
			}
#endif
			return stb_dirtree_get_dir(dir, cache_dir);
		}

		void stb_dirtree_free(stb_dirtree *d)
		{
			stb__dirtree_free_raw(d);
			free(d);
		}

		void stb_dirtree_db_add_dir(stb_dirtree *active, char *path, time_t last)
		{
			stb__dirtree_add_dir(path, last, active);
		}

		void stb_dirtree_db_add_file(stb_dirtree *active, char *name, int dir, stb_int64 size, time_t last)
		{
			stb__dirtree_add_file(name, dir, size, last, active);
		}

		void stb_dirtree_db_read(stb_dirtree *target, char *filename, char *dir)
		{
			char *s = stb_strip_final_slash(strdup(dir));
			target->dirs = 0;
			target->files = 0;
			target->string_pool = 0;
			stb__dirtree_load_db(filename, target, s);
			free(s);
		}

		void stb_dirtree_db_write(stb_dirtree *target, char *filename, char *dir)
		{
			stb__dirtree_save_db(filename, target, 0); // don't strip out any directories
		}

#endif // STB_DEFINE

#endif // _WIN32
#endif // STB_NO_STB_STRINGS

//////////////////////////////////////////////////////////////////////////////
//
//  STB_MALLOC_WRAPPER
//
//    you can use the wrapper functions with your own malloc wrapper,
//    or define STB_MALLOC_WRAPPER project-wide to have
//    malloc/free/realloc/strdup all get vectored to it

// this has too many very specific error messages you could google for and find in stb.h,
// so don't use it if they don't want any stb.h-identifiable strings
#if defined(STB_DEFINE) && !defined(STB_NO_STB_STRINGS)

		typedef struct
		{
			void *p;
			char *file;
			int line;
			int size;
		}stb_malloc_record;

#ifndef STB_MALLOC_HISTORY_COUNT
#define STB_MALLOC_HISTORY_COUNT 50 // 800 bytes
#endif

		stb_malloc_record *stb__allocations;
		static int stb__alloc_size, stb__alloc_limit, stb__alloc_mask;
		int stb__alloc_count;

		stb_malloc_record stb__alloc_history[STB_MALLOC_HISTORY_COUNT];
		int stb__history_pos;

		static int stb__hashfind(void *p)
		{
			stb_uint32 h = stb_hashptr(p);
			int s,n = h & stb__alloc_mask;
			if (stb__allocations[n].p == p)
			return n;
			s = stb_rehash(h)|1;
			for(;;) {
				if (stb__allocations[n].p == NULL)
				return -1;
				n = (n+s) & stb__alloc_mask;
				if (stb__allocations[n].p == p)
				return n;
			}
		}

		int stb_wrapper_allocsize(void *p)
		{
			int n = stb__hashfind(p);
			if (n < 0) return 0;
			return stb__allocations[n].size;
		}

		static int stb__historyfind(void *p)
		{
			int n = stb__history_pos;
			int i;
			for (i=0; i < STB_MALLOC_HISTORY_COUNT; ++i) {
				if (--n < 0) n = STB_MALLOC_HISTORY_COUNT-1;
				if (stb__alloc_history[n].p == p)
				return n;
			}
			return -1;
		}

		static void stb__add_alloc(void *p, int sz, char *file, int line);
		static void stb__grow_alloc(void)
		{
			int i,old_num = stb__alloc_size;
			stb_malloc_record *old = stb__allocations;
			if (stb__alloc_size == 0)
			stb__alloc_size = 64;
			else
			stb__alloc_size *= 2;

			stb__allocations = (stb_malloc_record *) stb__realloc_raw(NULL, stb__alloc_size * sizeof(stb__allocations[0]));
			if (stb__allocations == NULL)
			stb_fatal("Internal error: couldn't grow malloc wrapper table");
			memset(stb__allocations, 0, stb__alloc_size * sizeof(stb__allocations[0]));
			stb__alloc_limit = (stb__alloc_size*3)>>2;
			stb__alloc_mask = stb__alloc_size-1;

			stb__alloc_count = 0;

			for (i=0; i < old_num; ++i)
			if (old[i].p > STB_DEL) {
				stb__add_alloc(old[i].p, old[i].size, old[i].file, old[i].line);
				assert(stb__hashfind(old[i].p) >= 0);
			}
			for (i=0; i < old_num; ++i)
			if (old[i].p > STB_DEL)
			assert(stb__hashfind(old[i].p) >= 0);
			stb__realloc_raw(old, 0);
		}

		static void stb__add_alloc(void *p, int sz, char *file, int line)
		{
			stb_uint32 h;
			int n;
			if (stb__alloc_count >= stb__alloc_limit)
			stb__grow_alloc();
			h = stb_hashptr(p);
			n = h & stb__alloc_mask;
			if (stb__allocations[n].p > STB_DEL) {
				int s = stb_rehash(h)|1;
				do {
					n = (n+s) & stb__alloc_mask;
				}while (stb__allocations[n].p > STB_DEL);
			}
			assert(stb__allocations[n].p == NULL || stb__allocations[n].p == STB_DEL);
			stb__allocations[n].p = p;
			stb__allocations[n].size = sz;
			stb__allocations[n].line = line;
			stb__allocations[n].file = file;
			++stb__alloc_count;
		}

		static void stb__remove_alloc(int n, char *file, int line)
		{
			stb__alloc_history[stb__history_pos] = stb__allocations[n];
			stb__alloc_history[stb__history_pos].file = file;
			stb__alloc_history[stb__history_pos].line = line;
			if (++stb__history_pos == STB_MALLOC_HISTORY_COUNT)
			stb__history_pos = 0;
			stb__allocations[n].p = STB_DEL;
			--stb__alloc_count;
		}

		void stb_wrapper_malloc(void *p, int sz, char *file, int line)
		{
			if (!p) return;
			stb__add_alloc(p,sz,file,line);
		}

		void stb_wrapper_free(void *p, char *file, int line)
		{
			int n;

			if (p == NULL) return;

			n = stb__hashfind(p);

			if (n >= 0)
			stb__remove_alloc(n, file, line);
			else {
				// tried to free something we hadn't allocated!
				n = stb__historyfind(p);
				assert(0); /* NOTREACHED */
				if (n >= 0)
				stb_fatal("Attempted to free %d-byte block %p at %s:%d previously freed/realloced at %s:%d",
						stb__alloc_history[n].size, p,
						file, line,
						stb__alloc_history[n].file, stb__alloc_history[n].line);
				else
				stb_fatal("Attempted to free unknown block %p at %s:%d", p, file,line);
			}
		}

		void stb_wrapper_check(void *p)
		{
			int n;

			if (p == NULL) return;

			n = stb__hashfind(p);

			if (n >= 0) return;

			for (n=0; n < stb__alloc_size; ++n)
			if (stb__allocations[n].p == p)
			stb_fatal("Internal error: pointer %p was allocated, but hash search failed", p);

			// tried to free something that wasn't allocated!
			n = stb__historyfind(p);
			if (n >= 0)
			stb_fatal("Checked %d-byte block %p previously freed/realloced at %s:%d",
					stb__alloc_history[n].size, p,
					stb__alloc_history[n].file, stb__alloc_history[n].line);
			stb_fatal("Checked unknown block %p");
		}

		void stb_wrapper_realloc(void *p, void *q, int sz, char *file, int line)
		{
			int n;
			if (p == NULL) {stb_wrapper_malloc(q, sz, file, line); return;}
			if (q == NULL) return; // nothing happened

			n = stb__hashfind(p);
			if (n == -1) {
				// tried to free something we hadn't allocated!
				// this is weird, though, because we got past the realloc!
				n = stb__historyfind(p);
				assert(0); /* NOTREACHED */
				if (n >= 0)
				stb_fatal("Attempted to realloc %d-byte block %p at %s:%d previously freed/realloced at %s:%d",
						stb__alloc_history[n].size, p,
						file, line,
						stb__alloc_history[n].file, stb__alloc_history[n].line);
				else
				stb_fatal("Attempted to realloc unknown block %p at %s:%d", p, file,line);
			} else {
				if (q == p) {
					stb__allocations[n].size = sz;
					stb__allocations[n].file = file;
					stb__allocations[n].line = line;
				} else {
					stb__remove_alloc(n, file, line);
					stb__add_alloc(q,sz,file,line);
				}
			}
		}

		void stb_wrapper_listall(void (*func)(void *ptr, int sz, char *file, int line))
		{
			int i;
			for (i=0; i < stb__alloc_size; ++i)
			if (stb__allocations[i].p > STB_DEL)
			func(stb__allocations[i].p , stb__allocations[i].size,
					stb__allocations[i].file, stb__allocations[i].line);
		}

		void stb_wrapper_dump(char *filename)
		{
			int i;
			FILE *f = fopen(filename, "w");
			if (!f) return;
			for (i=0; i < stb__alloc_size; ++i)
			if (stb__allocations[i].p > STB_DEL)
			fprintf(f, "%p %7d - %4d %s\n",
					stb__allocations[i].p , stb__allocations[i].size,
					stb__allocations[i].line, stb__allocations[i].file);
		}
#endif // STB_DEFINE

//////////////////////////////////////////////////////////////////////////////
//
//                  stb_pointer_set
//
//
// For data structures that support querying by key, data structure
// classes always hand-wave away the issue of what to do if two entries
// have the same key: basically, store a linked list of all the nodes
// which have the same key (a LISP-style list).
//
// The thing is, it's not that trivial. If you have an O(log n)
// lookup data structure, but then n/4 items have the same value,
// you don't want to spend O(n) time scanning that list when
// deleting an item if you already have a pointer to the item.
// (You have to spend O(n) time enumerating all the items with
// a given key, sure, and you can't accelerate deleting a particular
// item if you only have the key, not a pointer to the item.)
//
// I'm going to call this data structure, whatever it turns out to
// be, a "pointer set", because we don't store any associated data for
// items in this data structure, we just answer the question of
// whether an item is in it or not (it's effectively one bit per pointer).
// Technically they don't have to be pointers; you could cast ints
// to (void *) if you want, but you can't store 0 or 1 because of the
// hash table.
//
// Since the fastest data structure we might want to add support for
// identical-keys to is a hash table with O(1)-ish lookup time,
// that means that the conceptual "linked list of all items with
// the same indexed value" that we build needs to have the same
// performance; that way when we index a table we think is arbitrary
// ints, but in fact half of them are 0, we don't get screwed.
//
// Therefore, it needs to be a hash table, at least when it gets
// large. On the other hand, when the data has totally arbitrary ints
// or floats, there won't be many collisions, and we'll have tons of
// 1-item bitmaps. That will be grossly inefficient as hash tables;
// trade-off; the hash table is reasonably efficient per-item when
// it's large, but not when it's small. So we need to do something
// Judy-like and use different strategies depending on the size.
//
// Like Judy, we'll use the bottom bit to encode the strategy:
//
//      bottom bits:
//          00     -   direct pointer
//          01     -   4-item bucket (16 bytes, no length, NULLs)
//          10     -   N-item array
//          11     -   hash table

		typedef struct stb_ps stb_ps;

		STB_EXTERN int stb_ps_find(stb_ps *ps, void *value);
		STB_EXTERN stb_ps * stb_ps_add(stb_ps *ps, void *value);
		STB_EXTERN stb_ps * stb_ps_remove(stb_ps *ps, void *value);
		STB_EXTERN stb_ps * stb_ps_remove_any(stb_ps *ps, void **value);
		STB_EXTERN void stb_ps_delete(stb_ps *ps);
		STB_EXTERN int stb_ps_count(stb_ps *ps);

		STB_EXTERN stb_ps * stb_ps_copy(stb_ps *ps);
		STB_EXTERN int stb_ps_subset(stb_ps *bigger, stb_ps *smaller);
		STB_EXTERN int stb_ps_eq(stb_ps *p0, stb_ps *p1);

		STB_EXTERN void ** stb_ps_getlist(stb_ps *ps, int *count);
		STB_EXTERN int stb_ps_writelist(stb_ps *ps, void **list, int size);

// enum and fastlist don't allocate storage, but you must consume the
// list before there's any chance the data structure gets screwed up;
		STB_EXTERN int stb_ps_enum(stb_ps *ps, void *data,
				int (*func)(void *value, void*data));
		STB_EXTERN void ** stb_ps_fastlist(stb_ps *ps, int *count);
//  result:
//     returns a list, *count is the length of that list,
//     but some entries of the list may be invalid;
//     test with 'stb_ps_fastlist_valid(x)'

#define stb_ps_fastlist_valid(x)   ((stb_uinta) (x) > 1)

#ifdef STB_DEFINE

		enum
		{
			STB_ps_direct = 0,
			STB_ps_bucket = 1,
			STB_ps_array = 2,
			STB_ps_hash = 3,
		};

#define STB_BUCKET_SIZE  4

		typedef struct
		{
			void *p[STB_BUCKET_SIZE];
		}stb_ps_bucket;
#define GetBucket(p)    ((stb_ps_bucket *) ((char *) (p) - STB_ps_bucket))
#define EncodeBucket(p) ((stb_ps *) ((char *) (p) + STB_ps_bucket))

		static void stb_bucket_free(stb_ps_bucket *b)
		{
			free(b);
		}

		static stb_ps_bucket *stb_bucket_create2(void *v0, void *v1)
		{
			stb_ps_bucket *b = (stb_ps_bucket*) malloc(sizeof(*b));
			b->p[0] = v0;
			b->p[1] = v1;
			b->p[2] = NULL;
			b->p[3] = NULL;
			return b;
		}

		static stb_ps_bucket * stb_bucket_create3(void **v)
		{
			stb_ps_bucket *b = (stb_ps_bucket*) malloc(sizeof(*b));
			b->p[0] = v[0];
			b->p[1] = v[1];
			b->p[2] = v[2];
			b->p[3] = NULL;
			return b;
		}

// could use stb_arr, but this will save us memory
		typedef struct
		{
			int count;
			void *p[1];
		}stb_ps_array;
#define GetArray(p)     ((stb_ps_array *) ((char *) (p) - STB_ps_array))
#define EncodeArray(p)  ((stb_ps *) ((char *) (p) + STB_ps_array))

		static int stb_ps_array_max = 13;

		typedef struct
		{
			int size, mask;
			int count, count_deletes;
			int grow_threshhold;
			int shrink_threshhold;
			int rehash_threshhold;
			int any_offset;
			void *table[1];
		}stb_ps_hash;
#define GetHash(p)      ((stb_ps_hash *) ((char *) (p) - STB_ps_hash))
#define EncodeHash(p)   ((stb_ps *) ((char *) (p) + STB_ps_hash))

#define stb_ps_empty(v)   (((stb_uint32) v) <= 1)

		static stb_ps_hash *stb_ps_makehash(int size, int old_size, void **old_data)
		{
			int i;
			stb_ps_hash *h = (stb_ps_hash *) malloc(sizeof(*h) + (size-1) * sizeof(h->table[0]));
			assert(stb_is_pow2(size));
			h->size = size;
			h->mask = size-1;
			h->shrink_threshhold = (int) (0.3f * size);
			h-> grow_threshhold = (int) (0.8f * size);
			h->rehash_threshhold = (int) (0.9f * size);
			h->count = 0;
			h->count_deletes = 0;
			h->any_offset = 0;
			memset(h->table, 0, size * sizeof(h->table[0]));
			for (i=0; i < old_size; ++i)
			if (!stb_ps_empty(old_data[i]))
			stb_ps_add(EncodeHash(h), old_data[i]);
			return h;
		}

		void stb_ps_delete(stb_ps *ps)
		{
			switch (3 & (int) ps) {
				case STB_ps_direct: break;
				case STB_ps_bucket: stb_bucket_free(GetBucket(ps)); break;
				case STB_ps_array : free(GetArray(ps)); break;
				case STB_ps_hash : free(GetHash(ps)); break;
			}
		}

		stb_ps *stb_ps_copy(stb_ps *ps)
		{
			int i;
			// not a switch: order based on expected performance/power-law distribution
			switch (3 & (int) ps) {
				case STB_ps_direct: return ps;
				case STB_ps_bucket: {
					stb_ps_bucket *n = (stb_ps_bucket *) malloc(sizeof(*n));
					*n = *GetBucket(ps);
					return EncodeBucket(n);
				}
				case STB_ps_array: {
					stb_ps_array *a = GetArray(ps);
					stb_ps_array *n = (stb_ps_array *) malloc(sizeof(*n) + stb_ps_array_max * sizeof(n->p[0]));
					n->count = a->count;
					for (i=0; i < a->count; ++i)
					n->p[i] = a->p[i];
					return EncodeArray(n);
				}
				case STB_ps_hash: {
					stb_ps_hash *h = GetHash(ps);
					stb_ps_hash *n = stb_ps_makehash(h->size, h->size, h->table);
					return EncodeHash(n);
				}
			}
			assert(0); /* NOTREACHED */
			return NULL;
		}

		int stb_ps_find(stb_ps *ps, void *value)
		{
			int i, code = 3 & (int) ps;
			assert((3 & (int) value) == STB_ps_direct);
			assert(stb_ps_fastlist_valid(value));
			// not a switch: order based on expected performance/power-law distribution
			if (code == STB_ps_direct)
			return value == ps;
			if (code == STB_ps_bucket) {
				stb_ps_bucket *b = GetBucket(ps);
				assert(STB_BUCKET_SIZE == 4);
				if (b->p[0] == value || b->p[1] == value ||
						b->p[2] == value || b->p[3] == value)
				return STB_TRUE;
				return STB_FALSE;
			}
			if (code == STB_ps_array) {
				stb_ps_array *a = GetArray(ps);
				for (i=0; i < a->count; ++i)
				if (a->p[i] == value)
				return STB_TRUE;
				return STB_FALSE;
			} else {
				stb_ps_hash *h = GetHash(ps);
				stb_uint32 hash = stb_hashptr(value);
				stb_uint32 s, n = hash & h->mask;
				void **t = h->table;
				if (t[n] == value) return STB_TRUE;
				if (t[n] == NULL) return STB_FALSE;
				s = stb_rehash(hash) | 1;
				do {
					n = (n + s) & h->mask;
					if (t[n] == value) return STB_TRUE;
				}while (t[n] != NULL);
				return STB_FALSE;
			}
		}

		stb_ps * stb_ps_add (stb_ps *ps, void *value)
		{
#ifdef STB_DEBUG
			assert(!stb_ps_find(ps,value));
#endif
			if (value == NULL) return ps; // ignore NULL adds to avoid bad breakage
			assert((3 & (int) value) == STB_ps_direct);
			assert(stb_ps_fastlist_valid(value));
			assert(value != STB_DEL);// STB_DEL is less likely

			switch (3 & (int) ps) {
				case STB_ps_direct:
				if (ps == NULL) return (stb_ps *) value;
				return EncodeBucket(stb_bucket_create2(ps,value));

				case STB_ps_bucket: {
					stb_ps_bucket *b = GetBucket(ps);
					stb_ps_array *a;
					assert(STB_BUCKET_SIZE == 4);
					if (b->p[0] == NULL) {b->p[0] = value; return ps;}
					if (b->p[1] == NULL) {b->p[1] = value; return ps;}
					if (b->p[2] == NULL) {b->p[2] = value; return ps;}
					if (b->p[3] == NULL) {b->p[3] = value; return ps;}
					a = (stb_ps_array *) malloc(sizeof(*a) + 7 * sizeof(a->p[0])); // 8 slots, must be 2^k
					memcpy(a->p, b, sizeof(*b));
					a->p[4] = value;
					a->count = 5;
					stb_bucket_free(b);
					return EncodeArray(a);
				}

				case STB_ps_array: {
					stb_ps_array *a = GetArray(ps);
					if (a->count == stb_ps_array_max) {
						// promote from array to hash
						stb_ps_hash *h = stb_ps_makehash(2 << stb_log2_ceil(a->count), a->count, a->p);
						free(a);
						return stb_ps_add(EncodeHash(h), value);
					}
					// do we need to resize the array? the array doubles in size when it
					// crosses a power-of-two
					if ((a->count & (a->count-1))==0) {
						int newsize = a->count*2;
						// clamp newsize to max if:
						//    1. it's larger than max
						//    2. newsize*1.5 is larger than max (to avoid extra resizing)
						if (newsize + a->count > stb_ps_array_max)
						newsize = stb_ps_array_max;
						a = (stb_ps_array *) realloc(a, sizeof(*a) + (newsize-1) * sizeof(a->p[0]));
					}
					a->p[a->count++] = value;
					return EncodeArray(a);
				}
				case STB_ps_hash: {
					stb_ps_hash *h = GetHash(ps);
					stb_uint32 hash = stb_hashptr(value);
					stb_uint32 n = hash & h->mask;
					void **t = h->table;
					// find first NULL or STB_DEL entry
					if (!stb_ps_empty(t[n])) {
						stb_uint32 s = stb_rehash(hash) | 1;
						do {
							n = (n + s) & h->mask;
						}while (!stb_ps_empty(t[n]));
					}
					if (t[n] == STB_DEL)
					-- h->count_deletes;
					t[n] = value;
					++ h->count;
					if (h->count == h->grow_threshhold) {
						stb_ps_hash *h2 = stb_ps_makehash(h->size*2, h->size, t);
						free(h);
						return EncodeHash(h2);
					}
					if (h->count + h->count_deletes == h->rehash_threshhold) {
						stb_ps_hash *h2 = stb_ps_makehash(h->size, h->size, t);
						free(h);
						return EncodeHash(h2);
					}
					return ps;
				}
			}
			return NULL; /* NOTREACHED */
		}

		stb_ps *stb_ps_remove(stb_ps *ps, void *value)
		{
#ifdef STB_DEBUG
			assert(stb_ps_find(ps, value));
#endif
			assert((3 & (int) value) == STB_ps_direct);
			if (value == NULL) return ps; // ignore NULL removes to avoid bad breakage
			switch (3 & (int) ps) {
				case STB_ps_direct:
				return ps == value ? NULL : ps;
				case STB_ps_bucket: {
					stb_ps_bucket *b = GetBucket(ps);
					int count=0;
					assert(STB_BUCKET_SIZE == 4);
					if (b->p[0] == value) b->p[0] = NULL; else count += (b->p[0] != NULL);
					if (b->p[1] == value) b->p[1] = NULL; else count += (b->p[1] != NULL);
					if (b->p[2] == value) b->p[2] = NULL; else count += (b->p[2] != NULL);
					if (b->p[3] == value) b->p[3] = NULL; else count += (b->p[3] != NULL);
					if (count == 1) { // shrink bucket at size 1
						value = b->p[0];
						if (value == NULL) value = b->p[1];
						if (value == NULL) value = b->p[2];
						if (value == NULL) value = b->p[3];
						assert(value != NULL);
						stb_bucket_free(b);
						return (stb_ps *) value;// return STB_ps_direct of value
					}
					return ps;
				}
				case STB_ps_array: {
					stb_ps_array *a = GetArray(ps);
					int i;
					for (i=0; i < a->count; ++i) {
						if (a->p[i] == value) {
							a->p[i] = a->p[--a->count];
							if (a->count == 3) { // shrink to bucket!
								stb_ps_bucket *b = stb_bucket_create3(a->p);
								free(a);
								return EncodeBucket(b);
							}
							return ps;
						}
					}
					return ps;
				}
				case STB_ps_hash: {
					stb_ps_hash *h = GetHash(ps);
					stb_uint32 hash = stb_hashptr(value);
					stb_uint32 s, n = hash & h->mask;
					void **t = h->table;
					if (t[n] != value) {
						s = stb_rehash(hash) | 1;
						do {
							n = (n + s) & h->mask;
						}while (t[n] != value);
					}
					t[n] = STB_DEL;
					-- h->count;
					++ h->count_deletes;
					// should we shrink down to an array?
					if (h->count < stb_ps_array_max) {
						int n = 1 << stb_log2_floor(stb_ps_array_max);
						if (h->count < n) {
							stb_ps_array *a = (stb_ps_array *) malloc(sizeof(*a) + (n-1) * sizeof(a->p[0]));
							int i,j=0;
							for (i=0; i < h->size; ++i)
							if (!stb_ps_empty(t[i]))
							a->p[j++] = t[i];
							assert(j == h->count);
							a->count = j;
							free(h);
							return EncodeArray(a);
						}
					}
					if (h->count == h->shrink_threshhold) {
						stb_ps_hash *h2 = stb_ps_makehash(h->size >> 1, h->size, t);
						free(h);
						return EncodeHash(h2);
					}
					return ps;
				}
			}
			return ps; /* NOTREACHED */
		}

		stb_ps *stb_ps_remove_any(stb_ps *ps, void **value)
		{
			assert(ps != NULL);
			switch (3 & (int) ps) {
				case STB_ps_direct:
				*value = ps;
				return NULL;
				case STB_ps_bucket: {
					stb_ps_bucket *b = GetBucket(ps);
					int count=0, slast=0, last=0;
					assert(STB_BUCKET_SIZE == 4);
					if (b->p[0]) {++count; last = 0;}
					if (b->p[1]) {++count; slast = last; last = 1;}
					if (b->p[2]) {++count; slast = last; last = 2;}
					if (b->p[3]) {++count; slast = last; last = 3;}
					*value = b->p[last];
					b->p[last] = 0;
					if (count == 2) {
						void *leftover = b->p[slast]; // second to last
						stb_bucket_free(b);
						return (stb_ps *) leftover;
					}
					return ps;
				}
				case STB_ps_array: {
					stb_ps_array *a = GetArray(ps);
					*value = a->p[a->count-1];
					if (a->count == 4)
					return stb_ps_remove(ps, *value);
					--a->count;
					return ps;
				}
				case STB_ps_hash: {
					stb_ps_hash *h = GetHash(ps);
					void **t = h->table;
					stb_uint32 n = h->any_offset;
					while (stb_ps_empty(t[n]))
					n = (n + 1) & h->mask;
					*value = t[n];
					h->any_offset = (n+1) & h->mask;
					// check if we need to skip down to the previous type
					if (h->count-1 < stb_ps_array_max || h->count-1 == h->shrink_threshhold)
					return stb_ps_remove(ps, *value);
					t[n] = STB_DEL;
					-- h->count;
					++ h->count_deletes;
					return ps;
				}
			}
			return ps; /* NOTREACHED */
		}

		void ** stb_ps_getlist(stb_ps *ps, int *count)
		{
			int i,n=0;
			void **p = NULL;
			switch (3 & (int) ps) {
				case STB_ps_direct:
				if (ps == NULL) {*count = 0; return NULL;}
				p = (void **) malloc(sizeof(*p) * 1);
				p[0] = ps;
				*count = 1;
				return p;
				case STB_ps_bucket: {
					stb_ps_bucket *b = GetBucket(ps);
					p = (void **) malloc(sizeof(*p) * STB_BUCKET_SIZE);
					for (i=0; i < STB_BUCKET_SIZE; ++i)
					if (b->p[i] != NULL)
					p[n++] = b->p[i];
					break;
				}
				case STB_ps_array: {
					stb_ps_array *a = GetArray(ps);
					p = (void **) malloc(sizeof(*p) * a->count);
					memcpy(p, a->p, sizeof(*p) * a->count);
					*count = a->count;
					return p;
				}
				case STB_ps_hash: {
					stb_ps_hash *h = GetHash(ps);
					p = (void **) malloc(sizeof(*p) * h->count);
					for (i=0; i < h->size; ++i)
					if (!stb_ps_empty(h->table[i]))
					p[n++] = h->table[i];
					break;
				}
			}
			*count = n;
			return p;
		}

		int stb_ps_writelist(stb_ps *ps, void **list, int size )
		{
			int i,n=0;
			switch (3 & (int) ps) {
				case STB_ps_direct:
				if (ps == NULL || size <= 0) return 0;
				list[0] = ps;
				return 1;
				case STB_ps_bucket: {
					stb_ps_bucket *b = GetBucket(ps);
					for (i=0; i < STB_BUCKET_SIZE; ++i)
					if (b->p[i] != NULL && n < size)
					list[n++] = b->p[i];
					return n;
				}
				case STB_ps_array: {
					stb_ps_array *a = GetArray(ps);
					n = stb_min(size, a->count);
					memcpy(list, a->p, sizeof(*list) * n);
					return n;
				}
				case STB_ps_hash: {
					stb_ps_hash *h = GetHash(ps);
					if (size <= 0) return 0;
					for (i=0; i < h->count; ++i) {
						if (!stb_ps_empty(h->table[i])) {
							list[n++] = h->table[i];
							if (n == size) break;
						}
					}
					return n;
				}
			}
			return 0; /* NOTREACHED */
		}

		int stb_ps_enum(stb_ps *ps, void *data, int (*func)(void *value, void *data))
		{
			int i;
			switch (3 & (int) ps) {
				case STB_ps_direct:
				if (ps == NULL) return STB_TRUE;
				return func(ps, data);
				case STB_ps_bucket: {
					stb_ps_bucket *b = GetBucket(ps);
					for (i=0; i < STB_BUCKET_SIZE; ++i)
					if (b->p[i] != NULL)
					if (!func(b->p[i], data))
					return STB_FALSE;
					return STB_TRUE;
				}
				case STB_ps_array: {
					stb_ps_array *a = GetArray(ps);
					for (i=0; i < a->count; ++i)
					if (!func(a->p[i], data))
					return STB_FALSE;
					return STB_TRUE;
				}
				case STB_ps_hash: {
					stb_ps_hash *h = GetHash(ps);
					for (i=0; i < h->count; ++i)
					if (!stb_ps_empty(h->table[i]))
					if (!func(h->table[i], data))
					return STB_FALSE;
					return STB_TRUE;
				}
			}
			return STB_TRUE; /* NOTREACHED */
		}

		int stb_ps_count (stb_ps *ps)
		{
			switch (3 & (int) ps) {
				case STB_ps_direct:
				return ps != NULL;
				case STB_ps_bucket: {
					stb_ps_bucket *b = GetBucket(ps);
					return (b->p[0] != NULL) + (b->p[1] != NULL) +
					(b->p[2] != NULL) + (b->p[3] != NULL);
				}
				case STB_ps_array: {
					stb_ps_array *a = GetArray(ps);
					return a->count;
				}
				case STB_ps_hash: {
					stb_ps_hash *h = GetHash(ps);
					return h->count;
				}
			}
			return 0;
		}

		void ** stb_ps_fastlist(stb_ps *ps, int *count)
		{
			static void *storage;

			switch (3 & (int) ps) {
				case STB_ps_direct:
				if (ps == NULL) {*count = 0; return NULL;}
				storage = ps;
				*count = 1;
				return &storage;
				case STB_ps_bucket: {
					stb_ps_bucket *b = GetBucket(ps);
					*count = STB_BUCKET_SIZE;
					return b->p;
				}
				case STB_ps_array: {
					stb_ps_array *a = GetArray(ps);
					*count = a->count;
					return a->p;
				}
				case STB_ps_hash: {
					stb_ps_hash *h = GetHash(ps);
					*count = h->size;
					return h->table;
				}
			}
			return NULL; /* NOTREACHED */
		}

		int stb_ps_subset(stb_ps *bigger, stb_ps *smaller)
		{
			int i, listlen;
			void **list = stb_ps_fastlist(smaller, &listlen);
			for(i=0; i < listlen; ++i)
			if (stb_ps_fastlist_valid(list[i]))
			if (!stb_ps_find(bigger, list[i]))
			return 0;
			return 1;
		}

		int stb_ps_eq(stb_ps *p0, stb_ps *p1)
		{
			if (stb_ps_count(p0) != stb_ps_count(p1))
			return 0;
			return stb_ps_subset(p0, p1);
		}

#undef GetBucket
#undef GetArray
#undef GetHash

#undef EncodeBucket
#undef EncodeArray
#undef EncodeHash

#endif

//////////////////////////////////////////////////////////////////////////////
//
//               Random Numbers via Meresenne Twister or LCG
//

		STB_EXTERN unsigned long stb_srandLCG(unsigned long seed);
		STB_EXTERN unsigned long stb_randLCG(void);
		STB_EXTERN double stb_frandLCG(void);

		STB_EXTERN void stb_srand(unsigned long seed);
		STB_EXTERN unsigned long stb_rand(void);
		STB_EXTERN double stb_frand(void);
		STB_EXTERN void stb_shuffle(void *p, size_t n, size_t sz,
				unsigned long seed);
		STB_EXTERN void stb_reverse(void *p, size_t n, size_t sz);

		STB_EXTERN unsigned long stb_randLCG_explicit(unsigned long seed);

#define stb_rand_define(x,y)                                         \
                                                                     \
   unsigned long x(void)                                             \
   {                                                                 \
      static unsigned long stb__rand = y;                            \
      stb__rand = stb__rand * 2147001325 + 715136305; /* BCPL */     \
      return 0x31415926 ^ ((stb__rand >> 16) + (stb__rand << 16));   \
   }

#ifdef STB_DEFINE
		unsigned long stb_randLCG_explicit(unsigned long seed)
		{
			return seed * 2147001325 + 715136305;
		}

		static unsigned long stb__rand_seed=0;

		unsigned long stb_srandLCG(unsigned long seed)
		{
			unsigned long previous = stb__rand_seed;
			stb__rand_seed = seed;
			return previous;
		}

		unsigned long stb_randLCG(void)
		{
			stb__rand_seed = stb__rand_seed * 2147001325 + 715136305; // BCPL generator
			// shuffle non-random bits to the middle, and xor to decorrelate with seed
			return 0x31415926 ^ ((stb__rand_seed >> 16) + (stb__rand_seed << 16));
		}

		double stb_frandLCG(void)
		{
			return stb_randLCG() / ((double) (1 << 16) * (1 << 16));
		}

		void stb_shuffle(void *p, size_t n, size_t sz, unsigned long seed)
		{
			char *a;
			unsigned long old_seed;
			int i;
			if (seed)
			old_seed = stb_srandLCG(seed);
			a = (char *) p + (n-1) * sz;

			for (i=n; i > 1; --i) {
				int j = stb_randLCG() % i;
				stb_swap(a, (char *) p + j * sz, sz);
				a -= sz;
			}
			if (seed)
			stb_srandLCG(old_seed);
		}

		void stb_reverse(void *p, size_t n, size_t sz)
		{
			int i,j = n-1;
			for (i=0; i < j; ++i,--j) {
				stb_swap((char *) p + i * sz, (char *) p + j * sz, sz);
			}
		}

// public domain Mersenne Twister by Michael Brundage
#define STB__MT_LEN       624

		int stb__mt_index = STB__MT_LEN*sizeof(unsigned long)+1;
		unsigned long stb__mt_buffer[STB__MT_LEN];

		void stb_srand(unsigned long seed)
		{
			int i;
			unsigned long old = stb_srandLCG(seed);
			for (i = 0; i < STB__MT_LEN; i++)
			stb__mt_buffer[i] = stb_randLCG();
			stb_srandLCG(old);
			stb__mt_index = STB__MT_LEN*sizeof(unsigned long);
		}

#define STB__MT_IA           397
#define STB__MT_IB           (STB__MT_LEN - STB__MT_IA)
#define STB__UPPER_MASK      0x80000000
#define STB__LOWER_MASK      0x7FFFFFFF
#define STB__MATRIX_A        0x9908B0DF
#define STB__TWIST(b,i,j)    ((b)[i] & STB__UPPER_MASK) | ((b)[j] & STB__LOWER_MASK)
#define STB__MAGIC(s)        (((s)&1)*STB__MATRIX_A)

		unsigned long stb_rand()
		{
			unsigned long * b = stb__mt_buffer;
			int idx = stb__mt_index;
			unsigned long s,r;
			int i;

			if (idx >= STB__MT_LEN*sizeof(unsigned long)) {
				if (idx > STB__MT_LEN*sizeof(unsigned long))
				stb_srand(0);
				idx = 0;
				i = 0;
				for (; i < STB__MT_IB; i++) {
					s = STB__TWIST(b, i, i+1);
					b[i] = b[i + STB__MT_IA] ^ (s >> 1) ^ STB__MAGIC(s);
				}
				for (; i < STB__MT_LEN-1; i++) {
					s = STB__TWIST(b, i, i+1);
					b[i] = b[i - STB__MT_IB] ^ (s >> 1) ^ STB__MAGIC(s);
				}

				s = STB__TWIST(b, STB__MT_LEN-1, 0);
				b[STB__MT_LEN-1] = b[STB__MT_IA-1] ^ (s >> 1) ^ STB__MAGIC(s);
			}
			stb__mt_index = idx + sizeof(unsigned long);

			r = *(unsigned long *)((unsigned char *)b + idx);

			r ^= (r >> 11);
			r ^= (r << 7) & 0x9D2C5680;
			r ^= (r << 15) & 0xEFC60000;
			r ^= (r >> 18);

			return r;
		}

		double stb_frand(void)
		{
			return stb_rand() / ((double) (1 << 16) * (1 << 16));
		}

#endif

//////////////////////////////////////////////////////////////////////////////
//
//                        stb_dupe
//
// stb_dupe is a duplicate-finding system for very, very large data
// structures--large enough that sorting is too slow, but not so large
// that we can't keep all the data in memory. using it works as follows:
//
//     1. create an stb_dupe:
//          provide a hash function
//          provide an equality function
//          provide an estimate for the size
//          optionally provide a comparison function
//
//     2. traverse your data, 'adding' pointers to the stb_dupe
//
//     3. finish and ask for duplicates
//
//        the stb_dupe will discard its intermediate data and build
//        a collection of sorted lists of duplicates, with non-duplicate
//        entries omitted entirely
//
//
//  Implementation strategy:
//
//     while collecting the N items, we keep a hash table of approximate
//     size sqrt(N). (if you tell use the N up front, the hash table is
//     just that size exactly)
//
//     each entry in the hash table is just an stb__arr of pointers (no need
//     to use stb_ps, because we don't need to delete from these)
//
//     for step 3, for each entry in the hash table, we apply stb_dupe to it
//     recursively. once the size gets small enough (or doesn't decrease
//     significantly), we switch to either using qsort() on the comparison
//     function, or else we just do the icky N^2 gather

		typedef struct stb_dupe stb_dupe;

		typedef int (*stb_compare_func)(void *a, void *b);
		typedef int (*stb_hash_func)(void *a, unsigned int seed);

		STB_EXTERN void stb_dupe_free(stb_dupe *sd);
		STB_EXTERN stb_dupe *stb_dupe_create(stb_hash_func hash,
				stb_compare_func eq, int size, stb_compare_func ineq);
		STB_EXTERN void stb_dupe_add(stb_dupe *sd, void *item);
		STB_EXTERN void stb_dupe_finish(stb_dupe *sd);
		STB_EXTERN int stb_dupe_numsets(stb_dupe *sd);
		STB_EXTERN void **stb_dupe_set(stb_dupe *sd, int num);
		STB_EXTERN int stb_dupe_set_count(stb_dupe *sd, int num);

		struct stb_dupe {
			void ***hash_table;
			int hash_size;
			int size_log2;
			int population;

			int hash_shift;
			stb_hash_func hash;

			stb_compare_func eq;
			stb_compare_func ineq;

			void ***dupes;
		};

#ifdef STB_DEFINE

		int stb_dupe_numsets(stb_dupe *sd)
		{
			assert(sd->hash_table == NULL);
			return stb_arr_len(sd->dupes);
		}

		void **stb_dupe_set(stb_dupe *sd, int num)
		{
			assert(sd->hash_table == NULL);
			return sd->dupes[num];
		}

		int stb_dupe_set_count(stb_dupe *sd, int num)
		{
			assert(sd->hash_table == NULL);
			return stb_arr_len(sd->dupes[num]);
		}

		stb_dupe *stb_dupe_create(stb_hash_func hash, stb_compare_func eq, int size,
				stb_compare_func ineq)
		{
			int i, hsize;
			stb_dupe *sd = (stb_dupe *) malloc(sizeof(*sd));

			sd->size_log2 = 4;
			hsize = 1 << sd->size_log2;
			while (hsize * hsize < size) {
				++sd->size_log2;
				hsize *= 2;
			}

			sd->hash = hash;
			sd->eq = eq;
			sd->ineq = ineq;
			sd->hash_shift = 0;

			sd->population = 0;
			sd->hash_size = hsize;
			sd->hash_table = (void ***) malloc(sizeof(*sd->hash_table) * hsize);
			for (i=0; i < hsize; ++i)
			sd->hash_table[i] = NULL;

			sd->dupes = NULL;

			return sd;
		}

		void stb_dupe_add(stb_dupe *sd, void *item)
		{
			stb_uint32 hash = sd->hash(item, sd->hash_shift);
			int z = hash & (sd->hash_size-1);
			stb_arr_push(sd->hash_table[z], item);
			++sd->population;
		}

		void stb_dupe_free(stb_dupe *sd)
		{
			int i;
			for (i=0; i < stb_arr_len(sd->dupes); ++i)
			if (sd->dupes[i])
			stb_arr_free(sd->dupes[i]);
			stb_arr_free(sd->dupes);
			free(sd);
		}

		static stb_compare_func stb__compare;

		static int stb__dupe_compare(const void *a, const void *b)
		{
			void *p = *(void **) a;
			void *q = *(void **) b;

			return stb__compare(p,q);
		}

		void stb_dupe_finish(stb_dupe *sd)
		{
			int i,j,k;
			assert(sd->dupes == NULL);
			for (i=0; i < sd->hash_size; ++i) {
				void ** list = sd->hash_table[i];
				if (list != NULL) {
					int n = stb_arr_len(list);
					// @TODO: measure to find good numbers instead of just making them up!
					int thresh = (sd->ineq ? 200 : 20);
					// if n is large enough to be worth it, and n is smaller than
					// before (so we can guarantee we'll use a smaller hash table);
					// and there are enough hash bits left, assuming full 32-bit hash
					if (n > thresh && n < (sd->population >> 3) && sd->hash_shift + sd->size_log2*2 < 32) {

						// recursively process this row using stb_dupe, O(N log log N)

						stb_dupe *d = stb_dupe_create(sd->hash, sd->eq, n, sd->ineq);
						d->hash_shift = stb_randLCG_explicit(sd->hash_shift);
						for (j=0; j < n; ++j)
						stb_dupe_add(d, list[j]);
						stb_arr_free(sd->hash_table[i]);
						stb_dupe_finish(d);
						for (j=0; j < stb_arr_len(d->dupes); ++j) {
							stb_arr_push(sd->dupes, d->dupes[j]);
							d->dupes[j] = NULL; // take over ownership
						}
						stb_dupe_free(d);

					} else if (sd->ineq) {

						// process this row using qsort(), O(N log N)
						stb__compare = sd->ineq;
						qsort(list, n, sizeof(list[0]), stb__dupe_compare);

						// find equal subsequences of the list
						for (j=0; j < n-1; ) {
							// find a subsequence from j..k
							for (k=j; k < n; ++k)
							// only use ineq so eq can be left undefined
							if (sd->ineq(list[j], list[k]))
							break;
							// k is the first one not in the subsequence
							if (k-j > 1) {
								void **mylist = NULL;
								stb_arr_setlen(mylist, k-j);
								memcpy(mylist, list+j, sizeof(list[j]) * (k-j));
								stb_arr_push(sd->dupes, mylist);
							}
							j = k;
						}
						stb_arr_free(sd->hash_table[i]);
					} else {

						// process this row using eq(), O(N^2)
						for (j=0; j < n; ++j) {
							if (list[j] != NULL) {
								void **output = NULL;
								for (k=j+1; k < n; ++k) {
									if (sd->eq(list[j], list[k])) {
										if (output == NULL)
										stb_arr_push(output, list[j]);
										stb_arr_push(output, list[k]);
										list[k] = NULL;
									}
								}
								list[j] = NULL;
								if (output)
								stb_arr_push(sd->dupes, output);
							}
						}
						stb_arr_free(sd->hash_table[i]);
					}
				}
			}
			free(sd->hash_table);
			sd->hash_table = NULL;
		}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//                       templatized Sort routine
//
// This is an attempt to implement a templated sorting algorithm.
// To use it, you have to explicitly instantiate it as a _function_,
// then you call that function. This allows the comparison to be inlined,
// giving the sort similar performance to C++ sorts.
//
// It implements quicksort with three-way-median partitioning (generally
// well-behaved), with a final insertion sort pass.
//
// When you define the compare expression, you should assume you have
// elements of your array pointed to by 'a' and 'b', and perform the comparison
// on those. OR you can use one or more statements; first say '0;', then
// write whatever code you want, and compute the result into a variable 'c'.

#define stb_declare_sort(FUNCNAME, TYPE)    \
                       void FUNCNAME(TYPE *p, int n)
#define stb_define_sort(FUNCNAME,TYPE,COMPARE) \
                       stb__define_sort(       void, FUNCNAME,TYPE,COMPARE)
#define stb_define_sort_static(FUNCNAME,TYPE,COMPARE) \
                       stb__define_sort(static void, FUNCNAME,TYPE,COMPARE)

#define stb__define_sort(MODE, FUNCNAME, TYPE, COMPARE)                       \
                                                                              \
static void STB_(FUNCNAME,_ins_sort)(TYPE *p, int n)                          \
{                                                                             \
   int i,j;                                                                   \
   for (i=1; i < n; ++i) {                                                    \
      TYPE t = p[i], *a = &t;                                                 \
      j = i;                                                                  \
      while (j > 0) {                                                         \
         TYPE *b = &p[j-1];                                                   \
         int c = COMPARE;                                                     \
         if (!c) break;                                                       \
         p[j] = p[j-1];                                                       \
         --j;                                                                 \
      }                                                                       \
      if (i != j)                                                             \
         p[j] = t;                                                            \
   }                                                                          \
}                                                                             \
                                                                              \
static void STB_(FUNCNAME,_quicksort)(TYPE *p, int n)                         \
{                                                                             \
   /* threshhold for transitioning to insertion sort */                       \
   while (n > 12) {                                                           \
      TYPE *a,*b,t;                                                           \
      int c01,c12,c,m,i,j;                                                    \
                                                                              \
      /* compute median of three */                                           \
      m = n >> 1;                                                             \
      a = &p[0];                                                              \
      b = &p[m];                                                              \
      c = COMPARE;                                                            \
      c01 = c;                                                                \
      a = &p[m];                                                              \
      b = &p[n-1];                                                            \
      c = COMPARE;                                                            \
      c12 = c;                                                                \
      /* if 0 >= mid >= end, or 0 < mid < end, then use mid */                \
      if (c01 != c12) {                                                       \
         /* otherwise, we'll need to swap something else to middle */         \
         int z;                                                               \
         a = &p[0];                                                           \
         b = &p[n-1];                                                         \
         c = COMPARE;                                                         \
         /* 0>mid && mid<n:  0>n => n; 0<n => 0 */                            \
         /* 0<mid && mid>n:  0>n => 0; 0<n => n */                            \
         z = (c == c12) ? 0 : n-1;                                            \
         t = p[z];                                                            \
         p[z] = p[m];                                                         \
         p[m] = t;                                                            \
      }                                                                       \
      /* now p[m] is the median-of-three */                                   \
      /* swap it to the beginning so it won't move around */                  \
      t = p[0];                                                               \
      p[0] = p[m];                                                            \
      p[m] = t;                                                               \
                                                                              \
      /* partition loop */                                                    \
      i=1;                                                                    \
      j=n-1;                                                                  \
      for(;;) {                                                               \
         /* handling of equality is crucial here */                           \
         /* for sentinels & efficiency with duplicates */                     \
         b = &p[0];                                                           \
         for (;;++i) {                                                        \
            a=&p[i];                                                          \
            c = COMPARE;                                                      \
            if (!c) break;                                                    \
         }                                                                    \
         a = &p[0];                                                           \
         for (;;--j) {                                                        \
            b=&p[j];                                                          \
            c = COMPARE;                                                      \
            if (!c) break;                                                    \
         }                                                                    \
         /* make sure we haven't crossed */                                   \
         if (i >= j) break;                                                   \
         t = p[i];                                                            \
         p[i] = p[j];                                                         \
         p[j] = t;                                                            \
                                                                              \
         ++i;                                                                 \
         --j;                                                                 \
      }                                                                       \
      /* recurse on smaller side, iterate on larger */                        \
      if (j < (n-i)) {                                                        \
         STB_(FUNCNAME,_quicksort)(p,j);                                       \
         p = p+i;                                                             \
         n = n-i;                                                             \
      } else {                                                                \
         STB_(FUNCNAME,_quicksort)(p+i, n-i);                                  \
         n = j;                                                               \
      }                                                                       \
   }                                                                          \
}                                                                             \
                                                                              \
MODE FUNCNAME(TYPE *p, int n)                                                 \
{                                                                             \
   STB_(FUNCNAME, _quicksort)(p, n);                                           \
   STB_(FUNCNAME, _ins_sort)(p, n);                                            \
}                                                                             \


//////////////////////////////////////////////////////////////////////////////
//
//      stb_bitset   an array of booleans indexed by integers
//

		typedef stb_uint32 stb_bitset;

		STB_EXTERN stb_bitset *stb_bitset_new(int value, int len);

#define stb_bitset_clearall(arr,len)     (memset(arr,   0, 4 * (len)))
#define stb_bitset_setall(arr,len)       (memset(arr, 255, 4 * (len)))

#define stb_bitset_setbit(arr,n)         ((arr)[(n) >> 5] |=  (1 << (n & 31)))
#define stb_bitset_clearbit(arr,n)       ((arr)[(n) >> 5] &= ~(1 << (n & 31)))
#define stb_bitset_testbit(arr,n)        ((arr)[(n) >> 5] &   (1 << (n & 31)))

		STB_EXTERN stb_bitset *stb_bitset_union(stb_bitset *p0, stb_bitset *p1,
				int len);

		STB_EXTERN int *stb_bitset_getlist(stb_bitset *out, int start, int end);

		STB_EXTERN int stb_bitset_eq(stb_bitset *p0, stb_bitset *p1, int len);
		STB_EXTERN int stb_bitset_disjoint(stb_bitset *p0, stb_bitset *p1,
				int len);
		STB_EXTERN int stb_bitset_disjoint_0(stb_bitset *p0, stb_bitset *p1,
				int len);
		STB_EXTERN int stb_bitset_subset(stb_bitset *bigger,
				stb_bitset *smaller, int len);
		STB_EXTERN int stb_bitset_unioneq_changed(stb_bitset *p0,
				stb_bitset *p1, int len);

#ifdef STB_DEFINE
		int stb_bitset_eq(stb_bitset *p0, stb_bitset *p1, int len)
		{
			int i;
			for (i=0; i < len; ++i)
			if (p0[i] != p1[i]) return 0;
			return 1;
		}

		int stb_bitset_disjoint(stb_bitset *p0, stb_bitset *p1, int len)
		{
			int i;
			for (i=0; i < len; ++i)
			if (p0[i] & p1[i]) return 0;
			return 1;
		}

		int stb_bitset_disjoint_0(stb_bitset *p0, stb_bitset *p1, int len)
		{
			int i;
			for (i=0; i < len; ++i)
			if ((p0[i] | p1[i]) != 0xffffffff) return 0;
			return 1;
		}

		int stb_bitset_subset(stb_bitset *bigger, stb_bitset *smaller, int len)
		{
			int i;
			for (i=0; i < len; ++i)
			if ((bigger[i] & smaller[i]) != smaller[i]) return 0;
			return 1;
		}

		stb_bitset *stb_bitset_union(stb_bitset *p0, stb_bitset *p1, int len)
		{
			int i;
			stb_bitset *d = (stb_bitset *) malloc(sizeof(*d) * len);
			for (i=0; i < len; ++i) d[i] = p0[i] | p1[i];
			return d;
		}

		int stb_bitset_unioneq_changed(stb_bitset *p0, stb_bitset *p1, int len)
		{
			int i, changed=0;
			for (i=0; i < len; ++i) {
				stb_bitset d = p0[i] | p1[i];
				if (d != p0[i]) {
					p0[i] = d;
					changed = 1;
				}
			}
			return changed;
		}

		stb_bitset *stb_bitset_new(int value, int len)
		{
			int i;
			stb_bitset *d = (stb_bitset *) malloc(sizeof(*d) * len);
			if (value) value = 0xffffffff;
			for (i=0; i < len; ++i) d[i] = value;
			return d;
		}

		int *stb_bitset_getlist(stb_bitset *out, int start, int end)
		{
			int *list = NULL;
			int i;
			for (i=start; i < end; ++i)
			if (stb_bitset_testbit(out, i))
			stb_arr_push(list, i);
			return list;
		}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//      stb_wordwrap    quality word-wrapping for fixed-width fonts
//

		STB_EXTERN int stb_wordwrap(int *pairs, int pair_max, int count,
				char *str);
		STB_EXTERN int *stb_wordwrapalloc(int count, char *str);

#ifdef STB_DEFINE

		int stb_wordwrap(int *pairs, int pair_max, int count, char *str)
		{
			int n=0,i=0, start=0,nonwhite=0;
			if (pairs == NULL) pair_max = 0x7ffffff0;
			else pair_max *= 2;
			// parse
			for(;;) {
				int s=i; // first whitespace char; last nonwhite+1
				int w;// word start
				// accept whitespace
				while (isspace(str[i])) {
					if (str[i] == '\n' || str[i] == '\r') {
						if (str[i] + str[i+1] == '\n' + '\r') ++i;
						if (n >= pair_max) return -1;
						if (pairs) pairs[n] = start, pairs[n+1] = s-start;
						n += 2;
						nonwhite=0;
						start = i+1;
						s = start;
					}
					++i;
				}
				if (i >= start+count) {
					// we've gone off the end using whitespace
					if (nonwhite) {
						if (n >= pair_max) return -1;
						if (pairs) pairs[n] = start, pairs[n+1] = s-start;
						n += 2;
						start = s = i;
						nonwhite=0;
					} else {
						// output all the whitespace
						while (i >= start+count) {
							if (n >= pair_max) return -1;
							if (pairs) pairs[n] = start, pairs[n+1] = count;
							n += 2;
							start += count;
						}
						s = start;
					}
				}

				if (str[i] == 0) break;
				// now scan out a word and see if it fits
				w = i;
				while (str[i] && !isspace(str[i])) {
					++i;
				}
				// wrapped?
				if (i > start + count) {
					// huge?
					if (i-s <= count) {
						if (n >= pair_max) return -1;
						if (pairs) pairs[n] = start, pairs[n+1] = s-start;
						n += 2;
						start = w;
					} else {
						// This word is longer than one line. If we wrap it onto N lines
						// there are leftover chars. do those chars fit on the cur line?
						// But if we have leading whitespace, we force it to start here.
						if ((w-start) + ((i-w) % count) <= count || !nonwhite) {
							// output a full line
							if (n >= pair_max) return -1;
							if (pairs) pairs[n] = start, pairs[n+1] = count;
							n += 2;
							start += count;
							w = start;
						} else {
							// output a partial line, trimming trailing whitespace
							if (s != start) {
								if (n >= pair_max) return -1;
								if (pairs) pairs[n] = start, pairs[n+1] = s-start;
								n += 2;
								start = w;
							}
						}
						// now output full lines as needed
						while (start + count <= i) {
							if (n >= pair_max) return -1;
							if (pairs) pairs[n] = start, pairs[n+1] = count;
							n += 2;
							start += count;
						}
					}
				}
				nonwhite=1;
			}
			if (start < i) {
				if (n >= pair_max) return -1;
				if (pairs) pairs[n] = start, pairs[n+1] = i-start;
				n += 2;
			}
			return n>>1;
		}

		int *stb_wordwrapalloc(int count, char *str)
		{
			int n = stb_wordwrap(NULL,0,count,str);
			int *z = NULL;
			stb_arr_setlen(z, n*2);
			stb_wordwrap(z, n, count, str);
			return z;
		}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//         stb_match:    wildcards and regexping
//

		STB_EXTERN int stb_wildmatch(char *expr, char *candidate);
		STB_EXTERN int stb_wildmatchi(char *expr, char *candidate);
		STB_EXTERN int stb_wildfind(char *expr, char *candidate);
		STB_EXTERN int stb_wildfindi(char *expr, char *candidate);

		STB_EXTERN int stb_regex(char *regex, char *candidate);

		typedef struct stb_matcher stb_matcher;

		STB_EXTERN stb_matcher *stb_regex_matcher(char *regex);
		STB_EXTERN int stb_matcher_match(stb_matcher *m, char *str);
		STB_EXTERN int stb_matcher_find(stb_matcher *m, char *str);
		STB_EXTERN void stb_matcher_free(stb_matcher *f);

		STB_EXTERN stb_matcher *stb_lex_matcher(void);
		STB_EXTERN int stb_lex_item(stb_matcher *m, char *str, int result);
		STB_EXTERN int stb_lex_item_wild(stb_matcher *matcher, char *regex,
				int result);
		STB_EXTERN int stb_lex(stb_matcher *m, char *str, int *len);

#ifdef STB_DEFINE

		static int stb__match_qstring(char *candidate, char *qstring, int qlen, int insensitive)
		{
			int i;
			if (insensitive) {
				for (i=0; i < qlen; ++i)
				if (qstring[i] == '?') {
					if (!candidate[i]) return 0;
				} else
				if (tolower(qstring[i]) != tolower(candidate[i]))
				return 0;
			} else {
				for (i=0; i < qlen; ++i)
				if (qstring[i] == '?') {
					if (!candidate[i]) return 0;
				} else
				if (qstring[i] != candidate[i])
				return 0;
			}
			return 1;
		}

		static int stb__find_qstring(char *candidate, char *qstring, int qlen, int insensitive)
		{
			char c;

			int offset=0;
			while (*qstring == '?') {
				++qstring;
				--qlen;
				++candidate;
				if (qlen == 0) return 0;
				if (*candidate == 0) return -1;
			}

			c = *qstring++;
			--qlen;
			if (insensitive) c = tolower(c);

			while (candidate[offset]) {
				if (c == (insensitive ? tolower(candidate[offset]) : candidate[offset]))
				if (stb__match_qstring(candidate+offset+1, qstring, qlen, insensitive))
				return offset;
				++offset;
			}

			return -1;
		}

		int stb__wildmatch_raw2(char *expr, char *candidate, int search, int insensitive)
		{
			int where=0;
			int start = -1;

			if (!search) {
				// parse to first '*'
				if (*expr != '*')
				start = 0;
				while (*expr != '*') {
					if (!*expr)
					return *candidate == 0 ? 0 : -1;
					if (*expr == '?') {
						if (!*candidate) return -1;
					} else {
						if (insensitive) {
							if (tolower(*candidate) != tolower(*expr))
							return -1;
						} else
						if (*candidate != *expr)
						return -1;
					}
					++candidate, ++expr, ++where;
				}
			} else {
				// 0-length search string
				if (!*expr)
				return 0;
			}

			assert(search || *expr == '*');
			if (!search)
			++expr;

			// implicit '*' at this point

			while (*expr) {
				int o=0;
				// combine redundant * characters
				while (expr[0] == '*') ++expr;

				// ok, at this point, expr[-1] == '*',
				// and expr[0] != '*'

				if (!expr[0]) return start >= 0 ? start : 0;

				// now find next '*'
				o = 0;
				while (expr[o] != '*') {
					if (expr[o] == 0)
					break;
					++o;
				}
				// if no '*', scan to end, then match at end
				if (expr[o] == 0 && !search) {
					int z;
					for (z=0; z < o; ++z)
					if (candidate[z] == 0)
					return -1;
					while (candidate[z])
					++z;
					// ok, now check if they match
					if (stb__match_qstring(candidate+z-o, expr, o, insensitive))
					return start >= 0 ? start : 0;
					return -1;
				} else {
					// if yes '*', then do stb__find_qmatch on the intervening chars
					int n = stb__find_qstring(candidate, expr, o, insensitive);
					if (n < 0)
					return -1;
					if (start < 0)
					start = where + n;
					expr += o;
					candidate += n+o;
				}

				if (*expr == 0) {
					assert(search);
					return start;
				}

				assert(*expr == '*');
				++expr;
			}

			return start >= 0 ? start : 0;
		}

		int stb__wildmatch_raw(char *expr, char *candidate, int search, int insensitive)
		{
			char buffer[256];
			// handle multiple search strings
			char *s = strchr(expr, ';');
			char *last = expr;
			while (s) {
				int z;
				// need to allow for non-writeable strings... assume they're small
				if (s - last < 256) {
					stb_strncpy(buffer, last, s-last+1);
					z = stb__wildmatch_raw2(buffer, candidate, search, insensitive);
				} else {
					*s = 0;
					z = stb__wildmatch_raw2(last, candidate, search, insensitive);
					*s = ';';
				}
				if (z >= 0) return z;
				last = s+1;
				s = strchr(last, ';');
			}
			return stb__wildmatch_raw2(last, candidate, search, insensitive);
		}

		int stb_wildmatch(char *expr, char *candidate)
		{
			return stb__wildmatch_raw(expr, candidate, 0,0) >= 0;
		}

		int stb_wildmatchi(char *expr, char *candidate)
		{
			return stb__wildmatch_raw(expr, candidate, 0,1) >= 0;
		}

		int stb_wildfind(char *expr, char *candidate)
		{
			return stb__wildmatch_raw(expr, candidate, 1,0);
		}

		int stb_wildfindi(char *expr, char *candidate)
		{
			return stb__wildmatch_raw(expr, candidate, 1,1);
		}

		typedef struct
		{
			stb_int16 transition[256];
		}stb_dfa;

// an NFA node represents a state you're in; it then has
// an arbitrary number of edges dangling off of it
// note this isn't utf8-y
		typedef struct
		{
			stb_int16 match; // character/set to match
			stb_uint16 node;// output node to go to
		}stb_nfa_edge;

		typedef struct
		{
			stb_int16 goal;   // does reaching this win the prize?
			stb_uint8 active;// is this in the active list
			stb_nfa_edge *out;
			stb_uint16 *eps;// list of epsilon closures
		}stb_nfa_node;

#define STB__DFA_UNDEF  -1
#define STB__DFA_GOAL   -2
#define STB__DFA_END    -3
#define STB__DFA_MGOAL  -4
#define STB__DFA_VALID  0

#define STB__NFA_STOP_GOAL -1

// compiled regexp
		struct stb_matcher
		{
			stb_uint16 start_node;
			stb_int16 dfa_start;
			stb_uint32 *charset;
			int num_charset;
			int match_start;
			stb_nfa_node *nodes;
			int does_lex;

			// dfa matcher
			stb_dfa * dfa;
			stb_uint32 * dfa_mapping;
			stb_int16 * dfa_result;
			int num_words_per_dfa;
		};

		static int stb__add_node(stb_matcher *matcher)
		{
			stb_nfa_node z;
			z.active = 0;
			z.eps = 0;
			z.goal = 0;
			z.out = 0;
			stb_arr_push(matcher->nodes, z);
			return stb_arr_len(matcher->nodes)-1;
		}

		static void stb__add_epsilon(stb_matcher *matcher, int from, int to)
		{
			assert(from != to);
			if (matcher->nodes[from].eps == NULL)
			stb_arr_malloc((void **) &matcher->nodes[from].eps, matcher);
			stb_arr_push(matcher->nodes[from].eps, to);
		}

		static void stb__add_edge(stb_matcher *matcher, int from, int to, int type)
		{
			stb_nfa_edge z = {type, to};
			if (matcher->nodes[from].out == NULL)
			stb_arr_malloc((void **) &matcher->nodes[from].out, matcher);
			stb_arr_push(matcher->nodes[from].out, z);
		}

		static char *stb__reg_parse_alt(stb_matcher *m, int s, char *r, stb_uint16 *e);
		static char *stb__reg_parse(stb_matcher *matcher, int start, char *regex, stb_uint16 *end)
		{
			int n;
			int last_start = -1;
			stb_uint16 last_end = start;

			while (*regex) {
				switch (*regex) {
					case '(':
					last_start = last_end;
					regex = stb__reg_parse_alt(matcher, last_end, regex+1, &last_end);
					if (regex == NULL || *regex != ')')
					return NULL;
					++regex;
					break;

					case '|':
					case ')':
					*end = last_end;
					return regex;

					case '?':
					if (last_start < 0) return NULL;
					stb__add_epsilon(matcher, last_start, last_end);
					++regex;
					break;

					case '*':
					if (last_start < 0) return NULL;
					stb__add_epsilon(matcher, last_start, last_end);

					// fall through

					case '+':
					if (last_start < 0) return NULL;
					stb__add_epsilon(matcher, last_end, last_start);
					// prevent links back to last_end from chaining to last_start
					n = stb__add_node(matcher);
					stb__add_epsilon(matcher, last_end, n);
					last_end = n;
					++regex;
					break;

					case '{':// not supported!
					// @TODO: given {n,m}, clone last_start to last_end m times,
					// and include epsilons from start to first m-n blocks
					return NULL;

					case '\\':
					++regex;
					if (!*regex) return NULL;

					// fallthrough
					default:// match exactly this character
					n = stb__add_node(matcher);
					stb__add_edge(matcher, last_end, n, *regex);
					last_start = last_end;
					last_end = n;
					++regex;
					break;

					case '$':
					n = stb__add_node(matcher);
					stb__add_edge(matcher, last_end, n, '\n');
					last_start = last_end;
					last_end = n;
					++regex;
					break;

					case '.':
					n = stb__add_node(matcher);
					stb__add_edge(matcher, last_end, n, -1);
					last_start = last_end;
					last_end = n;
					++regex;
					break;

					case '[': {
						stb_uint8 flags[256];
						int invert = 0,z;
						++regex;
						if (matcher->num_charset == 0) {
							matcher->charset = (stb_uint *) stb_malloc(matcher, sizeof(*matcher->charset) * 256);
							memset(matcher->charset, 0, sizeof(*matcher->charset) * 256);
						}

						memset(flags,0,sizeof(flags));

						// leading ^ is special
						if (*regex == '^')
						++regex, invert = 1;

						// leading ] is special
						if (*regex == ']') {
							flags[']'] = 1;
							++regex;
						}
						while (*regex != ']') {
							stb_uint a;
							if (!*regex) return NULL;
							a = *regex++;
							if (regex[0] == '-' && regex[1] != ']') {
								stb_uint i,b = regex[1];
								regex += 2;
								if (b == 0) return NULL;
								if (a > b) return NULL;
								for (i=a; i <= b; ++i)
								flags[i] = 1;
							} else
							flags[a] = 1;
						}
						++regex;
						if (invert) {
							int i;
							for (i=0; i < 256; ++i)
							flags[i] = 1-flags[i];
						}

						// now check if any existing charset matches
						for (z=0; z < matcher->num_charset; ++z) {
							int i, k[2] = {0, 1 << z};
							for (i=0; i < 256; ++i) {
								unsigned int f = k[flags[i]];
								if ((matcher->charset[i] & k[1]) != f)
								break;
							}
							if (i == 256) break;
						}

						if (z == matcher->num_charset) {
							int i;
							++matcher->num_charset;
							if (matcher->num_charset > 32) {
								assert(0); /* NOTREACHED */
								return NULL; // too many charsets, oops
							}
							for (i=0; i < 256; ++i)
							if (flags[i])
							matcher->charset[i] |= (1 << z);
						}

						n = stb__add_node(matcher);
						stb__add_edge(matcher, last_end, n, -2 - z);
						last_start = last_end;
						last_end = n;
						break;
					}
				}
			}
			*end = last_end;
			return regex;
		}

		static char *stb__reg_parse_alt(stb_matcher *matcher, int start, char *regex, stb_uint16 *end)
		{
			stb_uint16 last_end = start;
			stb_uint16 main_end;

			int head, tail;

			head = stb__add_node(matcher);
			stb__add_epsilon(matcher, start, head);

			regex = stb__reg_parse(matcher, head, regex, &last_end);
			if (regex == NULL) return NULL;
			if (*regex == 0 || *regex == ')') {
				*end = last_end;
				return regex;
			}

			main_end = last_end;
			tail = stb__add_node(matcher);

			stb__add_epsilon(matcher, last_end, tail);

			// start alternatives from the same starting node; use epsilon
			// transitions to combine their endings
			while(*regex && *regex != ')') {
				assert(*regex == '|');
				head = stb__add_node(matcher);
				stb__add_epsilon(matcher, start, head);
				regex = stb__reg_parse(matcher, head, regex+1, &last_end);
				if (regex == NULL)
				return NULL;
				stb__add_epsilon(matcher, last_end, tail);
			}

			*end = tail;
			return regex;
		}

		static char *stb__wild_parse(stb_matcher *matcher, int start, char *str, stb_uint16 *end)
		{
			int n;
			stb_uint16 last_end;

			last_end = stb__add_node(matcher);
			stb__add_epsilon(matcher, start, last_end);

			while (*str) {
				switch (*str) {
					// fallthrough
					default:// match exactly this character
					n = stb__add_node(matcher);
					if (toupper(*str) == tolower(*str)) {
						stb__add_edge(matcher, last_end, n, *str);
					} else {
						stb__add_edge(matcher, last_end, n, tolower(*str));
						stb__add_edge(matcher, last_end, n, toupper(*str));
					}
					last_end = n;
					++str;
					break;

					case '?':
					n = stb__add_node(matcher);
					stb__add_edge(matcher, last_end, n, -1);
					last_end = n;
					++str;
					break;

					case '*':
					n = stb__add_node(matcher);
					stb__add_edge(matcher, last_end, n, -1);
					stb__add_epsilon(matcher, last_end, n);
					stb__add_epsilon(matcher, n, last_end);
					last_end = n;
					++str;
					break;
				}
			}

			// now require end of string to match
			n = stb__add_node(matcher);
			stb__add_edge(matcher, last_end, n, 0);
			last_end = n;

			*end = last_end;
			return str;
		}

		static int stb__opt(stb_matcher *m, int n)
		{
			for(;;) {
				stb_nfa_node *p = &m->nodes[n];
				if (p->goal) return n;
				if (stb_arr_len(p->out)) return n;
				if (stb_arr_len(p->eps) != 1) return n;
				n = p->eps[0];
			}
		}

		static void stb__optimize(stb_matcher *m)
		{
			// if the target of any edge is a node with exactly
			// one out-epsilon, shorten it
			int i,j;
			for (i=0; i < stb_arr_len(m->nodes); ++i) {
				stb_nfa_node *p = &m->nodes[i];
				for (j=0; j < stb_arr_len(p->out); ++j)
				p->out[j].node = stb__opt(m,p->out[j].node);
				for (j=0; j < stb_arr_len(p->eps); ++j)
				p->eps[j] = stb__opt(m,p->eps[j] );
			}
			m->start_node = stb__opt(m,m->start_node);
		}

		void stb_matcher_free(stb_matcher *f)
		{
			stb_free(f);
		}

		static stb_matcher *stb__alloc_matcher(void)
		{
			stb_matcher *matcher = (stb_matcher *) stb_malloc(0,sizeof(*matcher));

			matcher->start_node = 0;
			stb_arr_malloc((void **) &matcher->nodes, matcher);
			matcher->num_charset = 0;
			matcher->match_start = 0;
			matcher->does_lex = 0;

			matcher->dfa_start = STB__DFA_UNDEF;
			stb_arr_malloc((void **) &matcher->dfa, matcher);
			stb_arr_malloc((void **) &matcher->dfa_mapping, matcher);
			stb_arr_malloc((void **) &matcher->dfa_result, matcher);

			stb__add_node(matcher);

			return matcher;
		}

		static void stb__lex_reset(stb_matcher *matcher)
		{
			// flush cached dfa data
			stb_arr_setlen(matcher->dfa, 0);
			stb_arr_setlen(matcher->dfa_mapping, 0);
			stb_arr_setlen(matcher->dfa_result, 0);
			matcher->dfa_start = STB__DFA_UNDEF;
		}

		stb_matcher *stb_regex_matcher(char *regex)
		{
			char *z;
			stb_uint16 end;
			stb_matcher *matcher = stb__alloc_matcher();
			if (*regex == '^') {
				matcher->match_start = 1;
				++regex;
			}

			z = stb__reg_parse_alt(matcher, matcher->start_node, regex, &end);

			if (!z || *z) {
				stb_free(matcher);
				return NULL;
			}

			((matcher->nodes)[(int) end]).goal = STB__NFA_STOP_GOAL;

			return matcher;
		}

		stb_matcher *stb_lex_matcher(void)
		{
			stb_matcher *matcher = stb__alloc_matcher();

			matcher->match_start = 1;
			matcher->does_lex = 1;

			return matcher;
		}

		int stb_lex_item(stb_matcher *matcher, char *regex, int result)
		{
			char *z;
			stb_uint16 end;

			z = stb__reg_parse_alt(matcher, matcher->start_node, regex, &end);

			if (z == NULL)
			return 0;

			stb__lex_reset(matcher);

			matcher->nodes[(int) end].goal = result;
			return 1;
		}

		int stb_lex_item_wild(stb_matcher *matcher, char *regex, int result)
		{
			char *z;
			stb_uint16 end;

			z = stb__wild_parse(matcher, matcher->start_node, regex, &end);

			if (z == NULL)
			return 0;

			stb__lex_reset(matcher);

			matcher->nodes[(int) end].goal = result;
			return 1;
		}

		static void stb__clear(stb_matcher *m, stb_uint16 *list)
		{
			int i;
			for (i=0; i < stb_arr_len(list); ++i)
			m->nodes[(int) list[i]].active = 0;
		}

		static int stb__clear_goalcheck(stb_matcher *m, stb_uint16 *list)
		{
			int i, t=0;
			for (i=0; i < stb_arr_len(list); ++i) {
				t += m->nodes[(int) list[i]].goal;
				m->nodes[(int) list[i]].active = 0;
			}
			return t;
		}

		static stb_uint16 * stb__add_if_inactive(stb_matcher *m, stb_uint16 *list, int n)
		{
			if (!m->nodes[n].active) {
				stb_arr_push(list, n);
				m->nodes[n].active = 1;
			}
			return list;
		}

		static stb_uint16 * stb__eps_closure(stb_matcher *m, stb_uint16 *list)
		{
			int i,n = stb_arr_len(list);

			for(i=0; i < n; ++i) {
				stb_uint16 *e = m->nodes[(int) list[i]].eps;
				if (e) {
					int j,k = stb_arr_len(e);
					for (j=0; j < k; ++j)
					list = stb__add_if_inactive(m, list, e[j]);
					n = stb_arr_len(list);
				}
			}

			return list;
		}

		int stb_matcher_match(stb_matcher *m, char *str)
		{
			int result = 0;
			int i,j,y,z;
			stb_uint16 *previous = NULL;
			stb_uint16 *current = NULL;
			stb_uint16 *temp;

			stb_arr_setsize(previous, 4);
			stb_arr_setsize(current, 4);

			previous = stb__add_if_inactive(m, previous, m->start_node);
			previous = stb__eps_closure(m,previous);
			stb__clear(m, previous);

			while (*str && stb_arr_len(previous)) {
				y = stb_arr_len(previous);
				for (i=0; i < y; ++i) {
					stb_nfa_node *n = &m->nodes[(int) previous[i]];
					z = stb_arr_len(n->out);
					for (j=0; j < z; ++j) {
						if (n->out[j].match >= 0) {
							if (n->out[j].match == *str)
							current = stb__add_if_inactive(m, current, n->out[j].node);
						} else if (n->out[j].match == -1) {
							if (*str != '\n')
							current = stb__add_if_inactive(m, current, n->out[j].node);
						} else if (n->out[j].match < -1) {
							int z = -n->out[j].match - 2;
							if (m->charset[(stb_uint8) *str] & (1 << z))
							current = stb__add_if_inactive(m, current, n->out[j].node);
						}
					}
				}
				stb_arr_setlen(previous, 0);

				temp = previous;
				previous = current;
				current = temp;

				previous = stb__eps_closure(m,previous);
				stb__clear(m, previous);

				++str;
			}

			// transition to pick up a '$' at the end
			y = stb_arr_len(previous);
			for (i=0; i < y; ++i)
			m->nodes[(int) previous[i]].active = 1;

			for (i=0; i < y; ++i) {
				stb_nfa_node *n = &m->nodes[(int) previous[i]];
				z = stb_arr_len(n->out);
				for (j=0; j < z; ++j) {
					if (n->out[j].match == '\n')
					current = stb__add_if_inactive(m, current, n->out[j].node);
				}
			}

			previous = stb__eps_closure(m,previous);
			stb__clear(m, previous);

			y = stb_arr_len(previous);
			for (i=0; i < y; ++i)
			if (m->nodes[(int) previous[i]].goal)
			result = 1;

			stb_arr_free(previous);
			stb_arr_free(current);

			return result && *str == 0;
		}

		stb_int16 stb__get_dfa_node(stb_matcher *m, stb_uint16 *list)
		{
			stb_uint16 node;
			stb_uint32 data[8], *state, *newstate;
			int i,j,n;

			state = (stb_uint32 *) stb_temp(data, m->num_words_per_dfa * 4);
			memset(state, 0, m->num_words_per_dfa*4);

			n = stb_arr_len(list);
			for (i=0; i < n; ++i) {
				int x = list[i];
				state[x >> 5] |= 1 << (x & 31);
			}

			// @TODO use a hash table
			n = stb_arr_len(m->dfa_mapping);
			i=j=0;
			for(; j < n; ++i, j += m->num_words_per_dfa) {
				// @TODO special case for <= 32
				if (!memcmp(state, m->dfa_mapping + j, m->num_words_per_dfa*4)) {
					node = i;
					goto done;
				}
			}

			assert(stb_arr_len(m->dfa) == i);
			node = i;

			newstate = stb_arr_addn(m->dfa_mapping, m->num_words_per_dfa);
			memcpy(newstate, state, m->num_words_per_dfa*4);

			// set all transitions to 'unknown'
			stb_arr_add(m->dfa);
			memset(m->dfa[i].transition, -1, sizeof(m->dfa[i].transition));

			if (m->does_lex) {
				int result = -1;
				n = stb_arr_len(list);
				for (i=0; i < n; ++i) {
					if (m->nodes[(int) list[i]].goal > result)
					result = m->nodes[(int) list[i]].goal;
				}

				stb_arr_push(m->dfa_result, result);
			}

			done:
			stb_tempfree(data, state);
			return node;
		}

		static int stb__matcher_dfa(stb_matcher *m, char *str_c, int *len)
		{
			stb_uint8 *str = (stb_uint8 *) str_c;
			stb_int16 node,prevnode;
			stb_dfa *trans;
			int match_length = 0;
			stb_int16 match_result=0;

			if (m->dfa_start == STB__DFA_UNDEF) {
				stb_uint16 *list;

				m->num_words_per_dfa = (stb_arr_len(m->nodes)+31) >> 5;
				stb__optimize(m);

				list = stb__add_if_inactive(m, NULL, m->start_node);
				list = stb__eps_closure(m,list);
				if (m->does_lex) {
					m->dfa_start = stb__get_dfa_node(m,list);
					stb__clear(m, list);
					// DON'T allow start state to be a goal state!
					// this allows people to specify regexes that can match 0
					// characters without them actually matching (also we don't
					// check _before_ advancing anyway
					if (m->dfa_start <= STB__DFA_MGOAL)
					m->dfa_start = -(m->dfa_start - STB__DFA_MGOAL);
				} else {
					if (stb__clear_goalcheck(m, list))
					m->dfa_start = STB__DFA_GOAL;
					else
					m->dfa_start = stb__get_dfa_node(m,list);
				}
				stb_arr_free(list);
			}

			prevnode = STB__DFA_UNDEF;
			node = m->dfa_start;
			trans = m->dfa;

			if (m->dfa_start == STB__DFA_GOAL)
			return 1;

			for(;;) {
				assert(node >= STB__DFA_VALID);

				// fast inner DFA loop; especially if STB__DFA_VALID is 0

				do {
					prevnode = node;
					node = trans[node].transition[*str++];
				}while (node >= STB__DFA_VALID);

				assert(node >= STB__DFA_MGOAL - stb_arr_len(m->dfa));
				assert(node < stb_arr_len(m->dfa));

				// special case for lex: need _longest_ match, so notice goal
				// state without stopping
				if (node <= STB__DFA_MGOAL) {
					match_length = str - (stb_uint8 *) str_c;
					node = -(node - STB__DFA_MGOAL);
					match_result = node;
					continue;
				}

				// slow NFA->DFA conversion

				// or we hit the goal or the end of the string, but those
				// can only happen once per search...

				if (node == STB__DFA_UNDEF) {
					// build a list  -- @TODO special case <= 32 states
					// heck, use a more compact data structure for <= 16 and <= 8 ?!

					// @TODO keep states/newstates around instead of reallocating them
					stb_uint16 *states = NULL;
					stb_uint16 *newstates = NULL;
					int i,j,y,z;
					stb_uint32 *flags = &m->dfa_mapping[prevnode * m->num_words_per_dfa];
					assert(prevnode != STB__DFA_UNDEF);
					stb_arr_setsize(states, 4);
					stb_arr_setsize(newstates,4);
					for (j=0; j < m->num_words_per_dfa; ++j) {
						for (i=0; i < 32; ++i) {
							if (*flags & (1 << i))
							stb_arr_push(states, j*32+i);
						}
						++flags;
					}
					// states is now the states we were in in the previous node;
					// so now we can compute what node it transitions to on str[-1]

					y = stb_arr_len(states);
					for (i=0; i < y; ++i) {
						stb_nfa_node *n = &m->nodes[(int) states[i]];
						z = stb_arr_len(n->out);
						for (j=0; j < z; ++j) {
							if (n->out[j].match >= 0) {
								if (n->out[j].match == str[-1] || (str[-1] == 0 && n->out[j].match == '\n'))
								newstates = stb__add_if_inactive(m, newstates, n->out[j].node);
							} else if (n->out[j].match == -1) {
								if (str[-1] != '\n' && str[-1])
								newstates = stb__add_if_inactive(m, newstates, n->out[j].node);
							} else if (n->out[j].match < -1) {
								int z = -n->out[j].match - 2;
								if (m->charset[str[-1]] & (1 << z))
								newstates = stb__add_if_inactive(m, newstates, n->out[j].node);
							}
						}
					}
					// AND add in the start state!
					if (!m->match_start || (str[-1] == '\n' && !m->does_lex))
					newstates = stb__add_if_inactive(m, newstates, m->start_node);
					// AND epsilon close it
					newstates = stb__eps_closure(m, newstates);
					// if it's a goal state, then that's all there is to it
					if (stb__clear_goalcheck(m, newstates)) {
						if (m->does_lex) {
							match_length = str - (stb_uint8 *) str_c;
							node = stb__get_dfa_node(m,newstates);
							match_result = node;
							node = -node + STB__DFA_MGOAL;
							trans = m->dfa; // could have gotten realloc()ed
						} else
						node = STB__DFA_GOAL;
					} else if (str[-1] == 0 || stb_arr_len(newstates) == 0) {
						node = STB__DFA_END;
					} else {
						node = stb__get_dfa_node(m,newstates);
						trans = m->dfa; // could have gotten realloc()ed
					}
					trans[prevnode].transition[str[-1]] = node;
					if (node <= STB__DFA_MGOAL)
					node = -(node - STB__DFA_MGOAL);
					stb_arr_free(newstates);
					stb_arr_free(states);
				}

				if (node == STB__DFA_GOAL) {
					return 1;
				}
				if (node == STB__DFA_END) {
					if (m->does_lex) {
						if (match_result) {
							if (len) *len = match_length;
							return m->dfa_result[(int) match_result];
						}
					}
					return 0;
				}

				assert(node != STB__DFA_UNDEF);
			}
		}

		int stb_matcher_find(stb_matcher *m, char *str)
		{
			assert(m->does_lex == 0);
			return stb__matcher_dfa(m, str, NULL);
		}

		int stb_lex(stb_matcher *m, char *str, int *len)
		{
			assert(m->does_lex);
			return stb__matcher_dfa(m, str, len);
		}

		int stb_regex(char *regex, char *str)
		{
			static stb_perfect p;
			static stb_matcher ** matchers;
			static char ** regexps;
			static char ** regexp_cache;
			static unsigned short *mapping;
			int z = stb_perfect_hash(&p, (int) regex);
			if (z >= 0) {
				if (strcmp(regex, regexp_cache[(int) mapping[z]])) {
					int i = mapping[z];
					stb_matcher_free(matchers[i]);
					free(regexp_cache[i]);
					regexps[i] = regex;
					regexp_cache[i] = strdup(regex);
					matchers[i] = stb_regex_matcher(regex);
				}
			} else {
				int i,n;
				if (regex == NULL) {
					for (i=0; i < stb_arr_len(matchers); ++i) {
						stb_matcher_free(matchers[i]);
						free(regexp_cache[i]);
					}
					stb_arr_free(matchers);
					stb_arr_free(regexps);
					stb_arr_free(regexp_cache);
					stb_perfect_destroy(&p);
					free(mapping); mapping = NULL;
					return -1;
				}
				stb_arr_push(regexps, regex);
				stb_arr_push(regexp_cache, strdup(regex));
				stb_arr_push(matchers, stb_regex_matcher(regex));
				stb_perfect_destroy(&p);
				n = stb_perfect_create(&p, (unsigned int *) (char **) regexps, stb_arr_len(regexps));
				mapping = (unsigned short *) realloc(mapping, n * sizeof(*mapping));
				for (i=0; i < stb_arr_len(regexps); ++i)
				mapping[stb_perfect_hash(&p, (int) regexps[i])] = i;
				z = stb_perfect_hash(&p, (int) regex);
			}
			return stb_matcher_find(matchers[(int) mapping[z]], str);
		}

#endif // STB_DEFINE

//////////////////////////////////////////////////////////////////////////////
//
//  File abstraction... tired of not having this... we can write
//  compressors to be layers over these that auto-close their children.

		typedef struct stbfile {
			int (*getbyte)(struct stbfile *);  // -1 on EOF
			unsigned int (*getdata)(struct stbfile *, void *block,
					unsigned int len);

			int (*putbyte)(struct stbfile *, int byte);
			unsigned int (*putdata)(struct stbfile *, void *block,
					unsigned int len);

			unsigned int (*size)(struct stbfile *);

			unsigned int (*tell)(struct stbfile *);
			void (*backpatch)(struct stbfile *, unsigned int tell, void *block,
					unsigned int len);

			void (*close)(struct stbfile *);

			FILE *f;  // file to fread/fwrite
			unsigned char *buffer; // input/output buffer
			unsigned char *indata, *inend; // input buffer
//   union {
			int various;
			void *ptr;
//   };
		} stbfile;

		STB_EXTERN unsigned int stb_getc(stbfile *f); // read
		STB_EXTERN int stb_putc(stbfile *f, int ch); // write
		STB_EXTERN unsigned int stb_getdata(stbfile *f, void *buffer,
				unsigned int len); // read
		STB_EXTERN unsigned int stb_putdata(stbfile *f, void *buffer,
				unsigned int len); // write
		STB_EXTERN unsigned int stb_tell(stbfile *f); // read
		STB_EXTERN unsigned int stb_size(stbfile *f); // read/write
		STB_EXTERN void stb_backpatch(stbfile *f, unsigned int tell,
				void *buffer, unsigned int len); // write

#ifdef STB_DEFINE

		unsigned int stb_getc(stbfile *f) {return f->getbyte(f);}
		int stb_putc(stbfile *f, int ch) {return f->putbyte(f, ch);}

		unsigned int stb_getdata(stbfile *f, void *buffer, unsigned int len)
		{
			return f->getdata(f, buffer, len);
		}
		unsigned int stb_putdata(stbfile *f, void *buffer, unsigned int len)
		{
			return f->putdata(f, buffer, len);
		}
		void stb_close(stbfile *f)
		{
			f->close(f);
			free(f);
		}
		unsigned int stb_tell(stbfile *f) {return f->tell(f);}
		unsigned int stb_size(stbfile *f) {return f->size(f);}
		void stb_backpatch(stbfile *f, unsigned int tell, void *buffer, unsigned int len)
		{
			f->backpatch(f,tell,buffer,len);
		}

// FILE * implementation
		static int stb__fgetbyte(stbfile *f) {return fgetc(f->f);}
		static int stb__fputbyte(stbfile *f, int ch) {return fputc(ch, f->f)==0;}
		static unsigned int stb__fgetdata(stbfile *f, void *buffer, unsigned int len) {return fread(buffer,1,len,f->f);}
		static unsigned int stb__fputdata(stbfile *f, void *buffer, unsigned int len) {return fwrite(buffer,1,len,f->f);}
		static unsigned int stb__fsize(stbfile *f) {return stb_filelen(f->f);}
		static unsigned int stb__ftell(stbfile *f) {return ftell(f->f);}
		static void stb__fbackpatch(stbfile *f, unsigned int where, void *buffer, unsigned int len)
		{
			fseek(f->f, where, SEEK_SET);
			fwrite(buffer, 1, len, f->f);
			fseek(f->f, 0, SEEK_END);
		}
		static void stb__fclose(stbfile *f) {fclose(f->f);}

		stbfile *stb_openf(FILE *f)
		{
			stbfile m = {stb__fgetbyte, stb__fgetdata,
				stb__fputbyte, stb__fputdata,
				stb__fsize, stb__ftell, stb__fbackpatch, stb__fclose,
				0,0,0,};
			stbfile *z = (stbfile *) malloc(sizeof(*z));
			if (z) {
				*z = m;
				z->f = f;
			}
			return z;
		}

		static int stb__nogetbyte(stbfile *f) {assert(0); return -1;}
		static unsigned int stb__nogetdata(stbfile *f, void *buffer, unsigned int len) {assert(0); return 0;}
		static int stb__noputbyte(stbfile *f, int ch) {assert(0); return 0;}
		static unsigned int stb__noputdata(stbfile *f, void *buffer, unsigned int len) {assert(0); return 0;}
		static void stb__nobackpatch(stbfile *f, unsigned int where, void *buffer, unsigned int len) {assert(0);}

		static int stb__bgetbyte(stbfile *s)
		{
			if (s->indata < s->inend)
			return *s->indata++;
			else
			return -1;
		}

		static unsigned int stb__bgetdata(stbfile *s, void *buffer, unsigned int len)
		{
			if (s->indata + len > s->inend)
			len = s->inend - s->indata;
			memcpy(buffer, s->indata, len);
			s->indata += len;
			return len;
		}
		static unsigned int stb__bsize(stbfile *s) {return s->inend - s->buffer;}
		static unsigned int stb__btell(stbfile *s) {return s->indata - s->buffer;}

		static void stb__bclose(stbfile *s)
		{
			if (s->various)
			free(s->buffer);
		}

		stbfile *stb_open_inbuffer(void *buffer, unsigned int len)
		{
			stbfile m = {stb__bgetbyte, stb__bgetdata,
				stb__noputbyte, stb__noputdata,
				stb__bsize, stb__btell, stb__nobackpatch, stb__bclose};
			stbfile *z = (stbfile *) malloc(sizeof(*z));
			if (z) {
				*z = m;
				z->buffer = (unsigned char *) buffer;
				z->indata = z->buffer;
				z->inend = z->indata + len;
			}
			return z;
		}

		stbfile *stb_open_inbuffer_free(void *buffer, unsigned int len)
		{
			stbfile *z = stb_open_inbuffer(buffer, len);
			if (z)
			z->various = 1; // free
			return z;
		}

#ifndef STB_VERSION
// if we've been cut-and-pasted elsewhere, you get a limited
// version of stb_open, without the 'k' flag and utf8 support
		static void stb__fclose2(stbfile *f)
		{
			fclose(f->f);
		}

		stbfile *stb_open(char *filename, char *mode)
		{
			FILE *f = fopen(filename, mode);
			stbfile *s;
			if (f == NULL) return NULL;
			s = stb_openf(f);
			if (s)
			s->close = stb__fclose2;
			return s;
		}
#else
// the full version depends on some code in stb.h; this
// also includes the memory buffer output format implemented with stb_arr
		static void stb__fclose2(stbfile *f)
		{
			stb_fclose(f->f, f->various);
		}

		stbfile *stb_open(char *filename, char *mode)
		{
			FILE *f = stb_fopen(filename, mode[0] == 'k' ? mode+1 : mode);
			stbfile *s;
			if (f == NULL) return NULL;
			s = stb_openf(f);
			if (s) {
				s->close = stb__fclose2;
				s->various = mode[0] == 'k' ? stb_keep_if_different : stb_keep_yes;
			}
			return s;
		}

		static int stb__aputbyte(stbfile *f, int ch)
		{
			stb_arr_push(f->buffer, ch);
			return 1;
		}
		static unsigned int stb__aputdata(stbfile *f, void *data, unsigned int len)
		{
			memcpy(stb_arr_addn(f->buffer, (int) len), data, len);
			return len;
		}
		static unsigned int stb__asize(stbfile *f) {return stb_arr_len(f->buffer);}
		static void stb__abackpatch(stbfile *f, unsigned int where, void *data, unsigned int len)
		{
			memcpy(f->buffer+where, data, len);
		}
		static void stb__aclose(stbfile *f)
		{
			*(unsigned char **) f->ptr = f->buffer;
		}

		stbfile *stb_open_outbuffer(unsigned char **update_on_close)
		{
			stbfile m = {stb__nogetbyte, stb__nogetdata,
				stb__aputbyte, stb__aputdata,
				stb__asize, stb__asize, stb__abackpatch, stb__aclose};
			stbfile *z = (stbfile *) malloc(sizeof(*z));
			if (z) {
				z->ptr = update_on_close;
				*z = m;
			}
			return z;
		}
#endif
#endif

#undef STB_EXTERN
#endif // STB_INCLUDE_STB_H
