#include <assert.h>

//////////////////////////////////////////////////////////////////////////////
//
//                         bit operations
//

#define stb_big32(c)    (((c)[0]<<24) + (c)[1]*65536 + (c)[2]*256 + (c)[3])
#define stb_little32(c) (((c)[3]<<24) + (c)[2]*65536 + (c)[1]*256 + (c)[0])
#define stb_big16(c)    ((c)[0]*256 + (c)[1])
#define stb_little16(c) ((c)[1]*256 + (c)[0])

extern int stb_bitcount(unsigned int a);
extern unsigned int stb_bitreverse8(unsigned char n);
extern unsigned int stb_bitreverse(unsigned int n);

extern int stb_is_pow2(unsigned int n);
extern int stb_log2_ceil(unsigned int n);
extern int stb_log2_floor(unsigned int n);

extern int stb_lowbit8(unsigned int n);
extern int stb_highbit8(unsigned int n);

int stb_bitcount(unsigned int a) {
	a = (a & 0x55555555) + ((a >> 1) & 0x55555555); // max 2
	a = (a & 0x33333333) + ((a >> 2) & 0x33333333); // max 4
	a = (a + (a >> 4)) & 0x0f0f0f0f; // max 8 per 4, now 8 bits
	a = (a + (a >> 8)); // max 16 per 8 bits
	a = (a + (a >> 16)); // max 32 per 8 bits
	return a & 0xff;
}

unsigned int stb_bitreverse8(unsigned char n) {
	n = ((n & 0xAA) >> 1) + ((n & 0x55) << 1);
	n = ((n & 0xCC) >> 2) + ((n & 0x33) << 2);
	return (unsigned char) ((n >> 4) + (n << 4));
}

unsigned int stb_bitreverse(unsigned int n) {
	n = ((n & 0xAAAAAAAA) >> 1) | ((n & 0x55555555) << 1);
	n = ((n & 0xCCCCCCCC) >> 2) | ((n & 0x33333333) << 2);
	n = ((n & 0xF0F0F0F0) >> 4) | ((n & 0x0F0F0F0F) << 4);
	n = ((n & 0xFF00FF00) >> 8) | ((n & 0x00FF00FF) << 8);
	return (n >> 16) | (n << 16);
}

int stb_is_pow2(unsigned int n) {
	return (n & (n - 1)) == 0;
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
int stb_log2_floor(unsigned int n) {
	static signed char log2_4[16] = { -1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3,
			3, 3 };

	// 2 compares if n < 16, 3 compares otherwise
	if (n < (1U << 14))
		if (n < (1U << 4))
			return 0 + log2_4[n];
		else if (n < (1U << 9))
			return 5 + log2_4[n >> 5];
		else
			return 10 + log2_4[n >> 10];
	else if (n < (1U << 24))
		if (n < (1U << 19))
			return 15 + log2_4[n >> 15];
		else
			return 20 + log2_4[n >> 20];
	else if (n < (1U << 29))
		return 25 + log2_4[n >> 25];
	else
		return 30 + log2_4[n >> 30];
}
#endif

// define ceil from floor
int stb_log2_ceil(unsigned int n) {
	if (stb_is_pow2(n))
		return stb_log2_floor(n);
	else
		return 1 + stb_log2_floor(n);
}

int stb_highbit8(unsigned int n) {
	return stb_log2_ceil(n & 255);
}

int stb_lowbit8(unsigned int n) {
	static signed char lowbit4[16] = { -1, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2,
			0, 1, 0 };
	int k = lowbit4[n & 15];
	if (k >= 0)
		return k;
	k = lowbit4[(n >> 4) & 15];
	if (k >= 0)
		return k + 4;
	return k;
}

//////////////////////////////////////////////////////////////////////////////
//
//                           String Processing
//

#define stb_prefixi(s,t)  (0==stb_strnicmp((s),(t),strlen(t)))

enum stb_splitpath_flag {
	STB_PATH = 1, STB_FILE = 2, STB_EXT = 4, STB_PATH_FILE = STB_PATH
			+ STB_FILE, STB_FILE_EXT = STB_FILE + STB_EXT, STB_EXT_NO_PERIOD = 8,
};

extern char * stb_skipwhite(char *s);
extern char * stb_trimwhite(char *s);
extern char * stb_skipnewline(char *s);
extern char * stb_strncpy(char *s, char *t, int n);
extern char * stb_substr(char *t, int n);
extern char * stb_duplower(char *s);
extern void stb_tolower(char *s);
extern char * stb_strchr2(char *s, char p1, char p2);
extern char * stb_strrchr2(char *s, char p1, char p2);
extern char * stb_strtok(char *output, char *src, char *delimit);
extern char * stb_strtok_keep(char *output, char *src, char *delimit);
extern char * stb_strtok_invert(char *output, char *src, char *allowed);
extern char * stb_dupreplace(char *s, char *find, char *replace);
extern void stb_replaceinplace(char *s, char *find, char *replace);
extern char * stb_splitpath(char *output, char *src, int flag);
extern char * stb_splitpathdup(char *src, int flag);
extern char * stb_replacedir(char *output, char *src, char *dir);
extern char * stb_replaceext(char *output, char *src, char *ext);
extern void stb_fixpath(char *path);
extern char * stb_shorten_path_readable(char *path, int max_len);
extern int stb_suffix(char *s, char *t);
extern int stb_prefix(char *s, char *t);
extern char * stb_strichr(char *s, char t);
extern int stb_prefix_count(char *s, char *t);
extern char * stb_plural(int n);  // "s" or ""
extern size_t stb_strscpy(char *d, const char *s, size_t n);

extern char **stb_tokens(char *src, char *delimit, int *count);
extern char **stb_tokens_nested(char *src, char *delimit, int *count,
		char *nest_in, char *nest_out);
extern char **stb_tokens_nested_empty(char *src, char *delimit, int *count,
		char *nest_in, char *nest_out);
extern char **stb_tokens_allowempty(char *src, char *delimit, int *count);
extern char **stb_tokens_stripwhite(char *src, char *delimit, int *count);
extern char **stb_tokens_withdelim(char *src, char *delimit, int *count);
extern char **stb_tokens_quoted(char *src, char *delimit, int *count);
// with 'quoted', allow delimiters to appear inside quotation marks, and don't
// strip whitespace inside them (and we delete the quotation marks unless they
// appear back to back, in which case they're considered escaped)

size_t stb_strscpy(char *d, const char *s, size_t n) {
	size_t len = strlen(s);
	if (len >= n) {
		if (n)
			d[0] = 0;
		return 0;
	}
	strcpy(d, s);
	return len + 1;
}

char *stb_plural(int n) {
	return n == 1 ? "" : "s";
}

int stb_prefix(char *s, char *t) {
	while (*t)
		if (*s++ != *t++)
			return 0;
	return 1;
}

int stb_prefix_count(char *s, char *t) {
	int c = 0;
	while (*t) {
		if (*s++ != *t++)
			break;
		++c;
	}
	return c;
}

int stb_suffix(char *s, char *t) {
	size_t n = strlen(s);
	size_t m = strlen(t);
	if (m <= n)
		return 0 == strcmp(s + n - m, t);
	else
		return 0;
}

// originally I was using this table so that I could create known sentinel
// values--e.g. change whitetable[0] to be true if I was scanning for whitespace,
// and false if I was scanning for nonwhite. I don't appear to be using that
// functionality anymore (I do for tokentable, though), so just replace it
// with isspace()
char *stb_skipwhite(char *s) {
	while (isspace((unsigned char) *s))
		++s;
	return s;
}

char *stb_skipnewline(char *s) {
	if (s[0] == '\r' || s[0] == '\n') {
		if (s[0] + s[1] == '\r' + '\n')
			++s;
		++s;
	}
	return s;
}

char *stb_trimwhite(char *s) {
	int i, n;
	s = stb_skipwhite(s);
	n = (int) strlen(s);
	for (i = n - 1; i >= 0; --i)
		if (!isspace(s[i]))
			break;
	s[i + 1] = 0;
	return s;
}

char *stb_strncpy(char *s, char *t, int n) {
	strncpy(s, t, n);
	s[n - 1] = 0;
	return s;
}

char *stb_substr(char *t, int n) {
	char *a;
	int z = (int) strlen(t);
	if (z < n)
		n = z;
	a = (char *) malloc(n + 1);
	strncpy(a, t, n);
	a[n] = 0;
	return a;
}

char *stb_duplower(char *s) {
	char *p = strdup(s), *q = p;
	while (*q) {
		*q = tolower(*q);
		++q;
	}
	return p;
}

void stb_tolower(char *s) {
	while (*s) {
		*s = tolower(*s);
		++s;
	}
}

char *stb_strchr2(char *s, char x, char y) {
	for (; *s; ++s)
		if (*s == x || *s == y)
			return s;
	return NULL;
}

char *stb_strrchr2(char *s, char x, char y) {
	char *r = NULL;
	for (; *s; ++s)
		if (*s == x || *s == y)
			r = s;
	return r;
}

char *stb_strichr(char *s, char t) {
	if (tolower(t) == toupper(t))
		return strchr(s, t);
	return stb_strchr2(s, (char) tolower(t), (char) toupper(t));
}

static char *stb_strtok_raw(char *output, char *src, char *delimit, int keep,
		int invert) {
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
		return *src ? src + 1 : src;
}

char *stb_strtok(char *output, char *src, char *delimit) {
	return stb_strtok_raw(output, src, delimit, 0, 0);
}

char *stb_strtok_keep(char *output, char *src, char *delimit) {
	return stb_strtok_raw(output, src, delimit, 1, 0);
}

char *stb_strtok_invert(char *output, char *src, char *delimit) {
	return stb_strtok_raw(output, src, delimit, 1, 1);
}

static char **stb_tokens_raw(char *src_, char *delimit, int *count,
		int stripwhite, int allow_empty, char *start, char *end) {
	int nested = 0;
	unsigned char *src = (unsigned char *) src_;
	static char stb_tokentable[256]; // rely on static initializion to 0
	static char stable[256], etable[256];
	char *out;
	char **result;
	int num = 0;
	unsigned char *s;

	s = (unsigned char *) delimit;
	while (*s)
		stb_tokentable[*s++] = 1;
	if (start) {
		s = (unsigned char *) start;
		while (*s)
			stable[*s++] = 1;
		s = (unsigned char *) end;
		if (s)
			while (*s)
				stable[*s++] = 1;
		s = (unsigned char *) end;
		if (s)
			while (*s)
				etable[*s++] = 1;
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
			if (!*s)
				break;
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
								if (s[1] == '"')
									s += 2; // "" -> "
								else {
									++s;
									break;
								} // terminating "
							} else
								++s;
						}
					}
				}
			}
		} else
			while (nested || !stb_tokentable[*s]) {
				if (stable[*s]) {
					if (!*s)
						break;
					if (end ? etable[*s] : nested)
						--nested;
					else
						++nested;
				}
				++s;
			}
		if (allow_empty) {
			if (*s)
				++s;
		}
	}
	// now num has the actual count... malloc our output structure
	// need space for all the strings: strings won't be any longer than
	// original input, since for every '\0' there's at least one delimiter
	result = (char **) malloc(sizeof(*result) * (num + 1) + (s - src + 1));
	if (result == NULL)
		return result;
	out = (char *) (result + (num + 1));
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
			while (isspace(*s))
				++s;
		}
		if (!*s)
			break;
		// we're past any leading delimiters and whitespace
		result[num] = out;
		++num;
		// copy non-delimiters
		stb_tokentable[0] = 1;
		last_nonwhite = out - 1;
		if (stripwhite == 2) {
			while (!stb_tokentable[*s]) {
				if (*s != '"') {
					if (!isspace(*s))
						last_nonwhite = out;
					*out++ = *s++;
				} else {
					++s;
					if (*s == '"') {
						if (!isspace(*s))
							last_nonwhite = out;
						*out++ = *s++; // "" -> ", not start string
					} else {
						// begin a quoted string
						while (*s) {
							if (s[0] == '"') {
								if (s[1] == '"') {
									*out++ = *s;
									s += 2;
								} else {
									++s;
									break;
								} // terminating "
							} else
								*out++ = *s++;
						}
						last_nonwhite = out - 1; // all in quotes counts as non-white
					}
				}
			}
		} else {
			while (nested || !stb_tokentable[*s]) {
				if (!isspace(*s))
					last_nonwhite = out;
				if (stable[*s]) {
					if (!*s)
						break;
					if (end ? etable[*s] : nested)
						--nested;
					else
						++nested;
				}
				*out++ = *s++;
			}
		}

		if (stripwhite) // rewind to last non-whitespace char
			out = last_nonwhite + 1;
		*out++ = '\0';

		if (*s)
			++s; // skip delimiter
	}
	s = (unsigned char *) delimit;
	while (*s)
		stb_tokentable[*s++] = 0;
	if (start) {
		s = (unsigned char *) start;
		while (*s)
			stable[*s++] = 1;
		s = (unsigned char *) end;
		if (s)
			while (*s)
				stable[*s++] = 1;
		s = (unsigned char *) end;
		if (s)
			while (*s)
				etable[*s++] = 1;
	}
	if (count != NULL)
		*count = num;
	result[num] = 0;
	return result;
}

char **stb_tokens(char *src, char *delimit, int *count) {
	return stb_tokens_raw(src, delimit, count, 0, 0, 0, 0);
}

char **stb_tokens_nested(char *src, char *delimit, int *count, char *nest_in,
		char *nest_out) {
	return stb_tokens_raw(src, delimit, count, 0, 0, nest_in, nest_out);
}

char **stb_tokens_nested_empty(char *src, char *delimit, int *count,
		char *nest_in, char *nest_out) {
	return stb_tokens_raw(src, delimit, count, 0, 1, nest_in, nest_out);
}

char **stb_tokens_allowempty(char *src, char *delimit, int *count) {
	return stb_tokens_raw(src, delimit, count, 0, 1, 0, 0);
}

char **stb_tokens_stripwhite(char *src, char *delimit, int *count) {
	return stb_tokens_raw(src, delimit, count, 1, 1, 0, 0);
}

char **stb_tokens_quoted(char *src, char *delimit, int *count) {
	return stb_tokens_raw(src, delimit, count, 2, 1, 0, 0);
}

char *stb_dupreplace(char *src, char *find, char *replace) {
	size_t len_find = strlen(find);
	size_t len_replace = strlen(replace);
	int count = 0;

	char *s, *p, *q;

	s = strstr(src, find);
	if (s == NULL)
		return strdup(src);
	do {
		++count;
		s = strstr(s + len_find, find);
	} while (s != NULL);

	p = (char *) malloc(strlen(src) + count * (len_replace - len_find) + 1);
	if (p == NULL)
		return p;
	q = p;
	s = src;
	for (;;) {
		char *t = strstr(s, find);
		if (t == NULL) {
			strcpy(q, s);
			assert(strlen(p) == strlen(src) + count * (len_replace - len_find));
			return p;
		}
		memcpy(q, s, t - s);
		q += t - s;
		memcpy(q, replace, len_replace);
		q += len_replace;
		s = t + len_find;
	}
}

void stb_replaceinplace(char *src, char *find, char *replace) {
	size_t len_find = strlen(find);
	size_t len_replace = strlen(replace);
	int delta;

	char *s, *p, *q;

	delta = len_replace - len_find;
	assert(delta <= 0);
	if (delta > 0)
		return;

	p = strstr(src, find);
	if (p == NULL)
		return;

	s = q = p;
	while (*s) {
		memcpy(q, replace, len_replace);
		p += len_find;
		q += len_replace;
		s = strstr(p, find);
		if (s == NULL)
			s = p + strlen(p);
		memmove(q, p, s - p);
		q += s - p;
		p = s;
	}
	*q = 0;
}

void stb_fixpath(char *path) {
	for (; *path; ++path)
		if (*path == '\\')
			*path = '/';
}

void stb__add_section(char *buffer, char *data, int curlen, int newlen) {
	if (newlen < curlen) {
		int z1 = newlen >> 1, z2 = newlen - z1;
		memcpy(buffer, data, z1 - 1);
		buffer[z1 - 1] = '.';
		buffer[z1 - 0] = '.';
		memcpy(buffer + z1 + 1, data + curlen - z2 + 1, z2 - 1);
	} else
		memcpy(buffer, data, curlen);
}

char * stb_shorten_path_readable(char *path, int len) {
	static char buffer[1024];
	int n = strlen(path), n1, n2, r1, r2;
	char *s;
	if (n <= len)
		return path;
	if (len > 1024)
		return path;
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
	if (n1 < len >> 1) {
		r1 = n1;
		r2 = len - r1;
	} else if (n2 < len >> 1) {
		r2 = n2;
		r1 = len - r2;
	} else {
		r1 = n1 * len / n;
		r2 = n2 * len / n;
		if (r1 < len >> 2)
			r1 = len >> 2, r2 = len - r1;
		if (r2 < len >> 2)
			r2 = len >> 2, r1 = len - r2;
	}
	assert(r1 <= n1 && r2 <= n2);
	if (n1)
		stb__add_section(buffer, path, n1, r1);
	stb__add_section(buffer + r1, s, n2, r2);
	buffer[len] = 0;
	return buffer;
}

static char *stb__splitpath_raw(char *buffer, char *path, int flag) {
	int len = 0, x, y, n = (int) strlen(path), f1, f2;
	char *s = stb_strrchr2(path, '/', '\\');
	char *t = strrchr(path, '.');
	if (s && t && t < s)
		t = NULL;
	if (s)
		++s;

	if (flag == STB_EXT_NO_PERIOD)
		flag |= STB_EXT;

	if (!(flag & (STB_PATH | STB_FILE | STB_EXT)))
		return NULL;

	f1 = s == NULL ? 0 : s - path; // start of filename
	f2 = t == NULL ? n : t - path; // just past end of filename

	if (flag & STB_PATH) {
		x = 0;
		if (f1 == 0 && flag == STB_PATH)
			len = 2;
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
		buffer = (char *) malloc(y - x + len + 1);
		if (!buffer)
			return NULL;
	}

	if (len) {
		strcpy(buffer, "./");
		return buffer;
	}
	strncpy(buffer, path + x, y - x);
	buffer[y - x] = 0;
	return buffer;
}

char *stb_splitpath(char *output, char *src, int flag) {
	return stb__splitpath_raw(output, src, flag);
}

char *stb_splitpathdup(char *src, int flag) {
	return stb__splitpath_raw(NULL, src, flag);
}

char *stb_replacedir(char *output, char *src, char *dir) {
	char buffer[4096];
	stb_splitpath(buffer, src, STB_FILE | STB_EXT);
	if (dir)
		sprintf(output, "%s/%s", dir, buffer);
	else
		strcpy(output, buffer);
	return output;
}

char *stb_replaceext(char *output, char *src, char *ext) {
	char buffer[4096];
	stb_splitpath(buffer, src, STB_PATH | STB_FILE);
	if (ext)
		sprintf(output, "%s.%s", buffer, ext[0] == '.' ? ext + 1 : ext);
	else
		strcpy(output, buffer);
	return output;
}

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

extern unsigned int stb_hash(char *str);
extern unsigned int stb_hashptr(void *p);
extern unsigned int stb_hashlen(char *str, int len);
extern unsigned int stb_rehash_improved(unsigned int v);
extern unsigned int stb_hash_fast(void *p, int len);
extern unsigned int stb_hash2(char *str, unsigned int *hash2_ptr);
extern unsigned int stb_hash_number(unsigned int hash);

#define stb_rehash(x)  ((x) + ((x) >> 6) + ((x) >> 19))

unsigned int stb_hash(char *str) {
	unsigned int hash = 0;
	while (*str)
		hash = (hash << 7) + (hash >> 25) + *str++;
	return hash + (hash >> 16);
}

unsigned int stb_hashlen(char *str, int len) {
	unsigned int hash = 0;
	while (len-- > 0 && *str)
		hash = (hash << 7) + (hash >> 25) + *str++;
	return hash + (hash >> 16);
}

unsigned int stb_hashptr(void *p) {
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

unsigned int stb_rehash_improved(unsigned int v) {
	return stb_hashptr((void *) (size_t) v);
}

unsigned int stb_hash2(char *str, unsigned int *hash2_ptr) {
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

unsigned int stb_hash_fast(void *p, int len) {
	unsigned char *q = (unsigned char *) p;
	unsigned int hash = len;

	if (len <= 0 || q == NULL)
		return 0;

	/* Main loop */
	if (((int) q & 1) == 0) {
		for (; len > 3; len -= 4) {
			unsigned int val;
			hash += stb__get16(q);
			val = (stb__get16(q+2) << 11);
			hash = (hash << 16) ^ hash ^ val;
			q += 4;
			hash += hash >> 11;
		}
	} else {
		for (; len > 3; len -= 4) {
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
	case 3:
		hash += stb__get16_slow(q);
		hash ^= hash << 16;
		hash ^= q[2] << 18;
		hash += hash >> 11;
		break;
	case 2:
		hash += stb__get16_slow(q);
		hash ^= hash << 11;
		hash += hash >> 17;
		break;
	case 1:
		hash += q[0];
		hash ^= hash << 10;
		hash += hash >> 1;
		break;
	case 0:
		break;
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

unsigned int stb_hash_number(unsigned int hash) {
	hash ^= hash << 3;
	hash += hash >> 5;
	hash ^= hash << 4;
	hash += hash >> 17;
	hash ^= hash << 25;
	hash += hash >> 6;
	return hash;
}

//////////////////////////////////////////////////////////////////////////////
//
//                 Checksums: CRC-32, ADLER32, SHA-1
//
//    CRC-32 and ADLER32 allow streaming blocks
//    SHA-1 requires either a complete buffer, max size 2^32 - 73
//          or it can checksum directly from a file, max 2^61

#define STB_ADLER32_SEED   1
#define STB_CRC32_SEED     0    // note that we logical NOT this in the code

typedef short stb_int16;
typedef unsigned int stb_uint;
typedef unsigned char stb_uchar;
typedef unsigned short stb_uint16;

extern stb_uint
stb_adler32(stb_uint adler32, stb_uchar *buffer, stb_uint buflen);
extern stb_uint
stb_crc32_block(stb_uint crc32, stb_uchar *buffer, stb_uint len);
extern stb_uint stb_crc32(unsigned char *buffer, stb_uint len);

extern void stb_sha1(unsigned char output[20], unsigned char *buffer,
		unsigned int len);
extern int stb_sha1_file(unsigned char output[20], char *file);

extern void stb_sha1_readable(char display[27], unsigned char sha[20]);

stb_uint stb_crc32_block(stb_uint crc, unsigned char *buffer, stb_uint len) {
	static stb_uint crc_table[256];
	stb_uint i, j, s;
	crc = ~crc;

	if (crc_table[1] == 0)
		for (i = 0; i < 256; i++) {
			for (s = i, j = 0; j < 8; ++j)
				s = (s >> 1) ^ (s & 1 ? 0xedb88320 : 0);
			crc_table[i] = s;
		}
	for (i = 0; i < len; ++i)
		crc = (crc >> 8) ^ crc_table[buffer[i] ^ (crc & 0xff)];
	return ~crc;
}

stb_uint stb_crc32(unsigned char *buffer, stb_uint len) {
	return stb_crc32_block(0, buffer, len);
}

stb_uint stb_adler32(stb_uint adler32, stb_uchar *buffer, stb_uint buflen) {
	const unsigned long ADLER_MOD = 65521;
	unsigned long s1 = adler32 & 0xffff, s2 = adler32 >> 16;
	unsigned long blocklen, i;

	blocklen = buflen % 5552;
	while (buflen) {
		for (i = 0; i + 7 < blocklen; i += 8) {
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

static void stb__sha1(stb_uchar *chunk, stb_uint h[5]) {
	int i;
	stb_uint a, b, c, d, e;
	stb_uint w[80];

	for (i = 0; i < 16; ++i)
		w[i] = stb_big32(&chunk[i * 4]);
	for (i = 16; i < 80; ++i) {
		stb_uint t;
		t = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16];
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

	i = 0;
	for (; i < 20; ++i)
		STB__SHA1(0x5a827999, d ^ (b & (c ^ d)));
	for (; i < 40; ++i)
		STB__SHA1(0x6ed9eba1, b ^ c ^ d);
	for (; i < 60; ++i)
		STB__SHA1(0x8f1bbcdc, (b & c) + (d & (b ^ c)));
	for (; i < 80; ++i)
		STB__SHA1(0xca62c1d6, b ^ c ^ d);

#undef STB__SHA1

	h[0] += a;
	h[1] += b;
	h[2] += c;
	h[3] += d;
	h[4] += e;
}

void stb_sha1(stb_uchar output[20], stb_uchar *buffer, stb_uint len) {
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
	if (((len + 9) & ~63) == end_start) {
		// it all fits in one block, so fill a second-to-last block
		end_start -= 64;
	}

	final_len = end_start + 128;

	// now we need to copy the data in
	assert(end_start + 128 >= len + 9);
	assert(end_start < len || len < 64 - 9);

	j = 0;
	if (end_start > len)
		j = (stb_uint) -(int) end_start;

	for (; end_start + j < len; ++j)
		final_block[j] = buffer[end_start + j];
	final_block[j++] = 0x80;
	while (j < 128 - 5) // 5 byte length, so write 4 extra padding bytes
		final_block[j++] = 0;
	// big-endian size
	final_block[j++] = len >> 29;
	final_block[j++] = len >> 21;
	final_block[j++] = len >> 13;
	final_block[j++] = len >> 5;
	final_block[j++] = len << 3;
	assert(j == 128 && end_start + j == final_len);

	for (j = 0; j < final_len; j += 64) { // 512-bit chunks
		if (j + 64 >= end_start + 64)
			stb__sha1(&final_block[j - end_start], h);
		else
			stb__sha1(&buffer[j], h);
	}

	for (i = 0; i < 5; ++i) {
		output[i * 4 + 0] = h[i] >> 24;
		output[i * 4 + 1] = h[i] >> 16;
		output[i * 4 + 2] = h[i] >> 8;
		output[i * 4 + 3] = h[i] >> 0;
	}
}

// client can truncate this wherever they like
void stb_sha1_readable(char display[27], unsigned char sha[20]) {
	char encoding[65] = "0123456789abcdefghijklmnopqrstuv"
			"wxyzABCDEFGHIJKLMNOPQRSTUVWXYZ%$";
	int num_bits = 0, acc = 0;
	int i = 0, o = 0;
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
	assert(num_bits == 20 * 8 - 26 * 6);
	display[o++] = encoding[acc];
}

//////////////////////////////////////////////////////////////////////////////
//
//      stb_bitset   an array of booleans indexed by integers
//

typedef unsigned int stb_uint32;
typedef stb_uint32 stb_bitset;

extern stb_bitset *stb_bitset_new(int value, int len);

#define stb_bitset_clearall(arr,len)     (memset(arr,   0, 4 * (len)))
#define stb_bitset_setall(arr,len)       (memset(arr, 255, 4 * (len)))

#define stb_bitset_setbit(arr,n)         ((arr)[(n) >> 5] |=  (1 << (n & 31)))
#define stb_bitset_clearbit(arr,n)       ((arr)[(n) >> 5] &= ~(1 << (n & 31)))
#define stb_bitset_testbit(arr,n)        ((arr)[(n) >> 5] &   (1 << (n & 31)))

extern stb_bitset *stb_bitset_union(stb_bitset *p0, stb_bitset *p1, int len);
extern int stb_bitset_eq(stb_bitset *p0, stb_bitset *p1, int len);
extern int stb_bitset_disjoint(stb_bitset *p0, stb_bitset *p1, int len);
extern int stb_bitset_disjoint_0(stb_bitset *p0, stb_bitset *p1, int len);
extern int stb_bitset_subset(stb_bitset *bigger, stb_bitset *smaller, int len);
extern int stb_bitset_unioneq_changed(stb_bitset *p0, stb_bitset *p1, int len);

int stb_bitset_eq(stb_bitset *p0, stb_bitset *p1, int len) {
	int i;
	for (i = 0; i < len; ++i)
		if (p0[i] != p1[i])
			return 0;
	return 1;
}

int stb_bitset_disjoint(stb_bitset *p0, stb_bitset *p1, int len) {
	int i;
	for (i = 0; i < len; ++i)
		if (p0[i] & p1[i])
			return 0;
	return 1;
}

int stb_bitset_disjoint_0(stb_bitset *p0, stb_bitset *p1, int len) {
	int i;
	for (i = 0; i < len; ++i)
		if ((p0[i] | p1[i]) != 0xffffffff)
			return 0;
	return 1;
}

int stb_bitset_subset(stb_bitset *bigger, stb_bitset *smaller, int len) {
	int i;
	for (i = 0; i < len; ++i)
		if ((bigger[i] & smaller[i]) != smaller[i])
			return 0;
	return 1;
}

stb_bitset *stb_bitset_union(stb_bitset *p0, stb_bitset *p1, int len) {
	int i;
	stb_bitset *d = (stb_bitset *) malloc(sizeof(*d) * len);
	for (i = 0; i < len; ++i)
		d[i] = p0[i] | p1[i];
	return d;
}

int stb_bitset_unioneq_changed(stb_bitset *p0, stb_bitset *p1, int len) {
	int i, changed = 0;
	for (i = 0; i < len; ++i) {
		stb_bitset d = p0[i] | p1[i];
		if (d != p0[i]) {
			p0[i] = d;
			changed = 1;
		}
	}
	return changed;
}

stb_bitset *stb_bitset_new(int value, int len) {
	int i;
	stb_bitset *d = (stb_bitset *) malloc(sizeof(*d) * len);
	if (value)
		value = 0xffffffff;
	for (i = 0; i < len; ++i)
		d[i] = value;
	return d;
}


//////////////////////////////////////////////////////////////////////////////
//
//    Now you can do the following things with this array:
//
//         sb_free(TYPE *a)           free the array
//         sb_count(TYPE *a)          the number of elements in the array
//         sb_push(TYPE *a, TYPE v)   adds v on the end of the array, a la push_back
//         sb_add(TYPE *a, int n)     adds n uninitialized elements at end of array & returns pointer to first added
//         sb_last(TYPE *a)           returns an lvalue of the last item in the array
//         a[n]                       access the nth (counting from 0) element of the array
//


#ifndef STB_STRETCHY_BUFFER_H_INCLUDED
#define STB_STRETCHY_BUFFER_H_INCLUDED

#define stb_sb_free(a)         ((a) ? free(stb__sbraw(a)),0 : 0)
#define stb_sb_push(a,v)       (stb__sbmaybegrow(a,1), (a)[stb__sbn(a)++] = (v))
#define stb_sb_count(a)        ((a) ? stb__sbn(a) : 0)
#define stb_sb_add(a,n)        (stb__sbmaybegrow(a,n), stb__sbn(a)+=(n), &(a)[stb__sbn(a)-(n)])
#define stb_sb_last(a)         ((a)[stb__sbn(a)-1])

#define stb__sbraw(a) ((int *) (a) - 2)
#define stb__sbm(a)   stb__sbraw(a)[0]
#define stb__sbn(a)   stb__sbraw(a)[1]

#define stb__sbneedgrow(a,n)  ((a)==0 || stb__sbn(a)+(n) >= stb__sbm(a))
#define stb__sbmaybegrow(a,n) (stb__sbneedgrow(a,(n)) ? stb__sbgrow(a,n) : 0)
#define stb__sbgrow(a,n)      ((a) = stb__sbgrowf((a), (n), sizeof(*(a))))

#include <stdlib.h>

static void * stb__sbgrowf(void *arr, int increment, int itemsize) {
	int dbl_cur = arr ? 2 * stb__sbm(arr) : 0;
	int min_needed = stb_sb_count(arr) + increment;
	int m = dbl_cur > min_needed ? dbl_cur : min_needed;
	int *p = (int *) realloc(arr ? stb__sbraw(arr) : 0,
			itemsize * m + sizeof(int) * 2);
	if (p) {
		if (!arr)
			p[1] = 0;
		p[0] = m;
		return p + 2;
	} else {
		return (void *) (2 * sizeof(int)); // try to force a NULL pointer exception later
	}
}
#endif




