
#ifndef STB_LIB_H__
#define STB_LIB_H__


#include <assert.h>

//////////////////////////////////////////////////////////////////////////////
//
//                         bit operations
//

#define stb_big32(c)    (((c)[0]<<24) + (c)[1]*65536 + (c)[2]*256 + (c)[3])
#define stb_little32(c) (((c)[3]<<24) + (c)[2]*65536 + (c)[1]*256 + (c)[0])
#define stb_big16(c)    ((c)[0]*256 + (c)[1])
#define stb_little16(c) ((c)[1]*256 + (c)[0])

extern unsigned char stb_getbit8(unsigned char v, int index);
extern unsigned char stb_setbit8(unsigned char v, int index);

extern int stb_bitcount(unsigned int a);
extern unsigned int stb_bitreverse8(unsigned char n);
extern unsigned int stb_bitreverse(unsigned int n);

extern int stb_is_pow2(unsigned int n);
extern int stb_log2_ceil(unsigned int n);
extern int stb_log2_floor(unsigned int n);

extern int stb_lowbit8(unsigned int n);
extern int stb_highbit8(unsigned int n);

unsigned char stb_getbit8(unsigned char v, int index) {
	return (v >> (index)) & 0x01;
}

unsigned char stb_setbit8(unsigned char v, int index) {
	return (0x01 << (index)) | v;
}

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
#endif //STB_LIB_H__
