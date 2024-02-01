#include "sha256.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

inline static uint32_t rdbe32(uint8_t *w) {
	return ((uint32_t) w[0] << 24) |
	       ((uint32_t) w[1] << 16) |
	       ((uint32_t) w[2] <<  8) |
	       ((uint32_t) w[3] <<  0);
}

inline static uint32_t ror32(uint32_t a, int b) {
	return (a >> b) | (a << (32 - b));
}

static void sha256_block(sha256_state_t *s) {
	const uint32_t k[64] = {
		0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
		0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
		0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
		0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
		0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
		0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
		0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
		0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
		0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
		0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
		0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
		0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
		0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
		0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
		0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
		0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
	};
	uint32_t w[64];
	uint32_t wv[8];
	int i;
	
	/* Copy buf contents to w. */
	for (i = 0; i < 64; i += 4)
		w[i / 4] = rdbe32(s->buf + i);
	
	/* Extend block to fill w. */
	for (i = 16; i < 64; ++i)
		w[i] = w[i - 16] + (ror32(w[i - 15], 7) ^
		       ror32(w[i - 15], 18) ^ (w[i - 15] >> 3)) + w[i - 7] +
		       (ror32(w[i - 2], 17) ^ ror32(w[i - 2], 19) ^
		       (w[i - 2] >> 10));
	
	/* Initialize working variables to current hash value. */
	memcpy(wv, s->h, sizeof(uint32_t) * 8);
	
	/* Compression function main loop. */
	for (i = 0; i < 64; ++i) {
		uint32_t t1 = wv[7] + (ror32(wv[4], 6) ^ ror32(wv[4], 11) ^
		              ror32(wv[4], 25)) + ((wv[4] & wv[5]) ^
		              (~wv[4] & wv[6])) + k[i] + w[i];
		uint32_t t2 = (ror32(wv[0], 2) ^ ror32(wv[0], 13) ^
		              ror32(wv[0], 22)) + ((wv[0] & wv[1]) ^
		              (wv[0] & wv[2]) ^ (wv[1] & wv[2]));
		wv[7] = wv[6];
		wv[6] = wv[5];
		wv[5] = wv[4];
		wv[4] = wv[3] + t1;
		wv[3] = wv[2];
		wv[2] = wv[1];
		wv[1] = wv[0];
		wv[0] = t1 + t2;
	}
	
	/* Add the compressed block to the current hash value. */
	for (i = 0; i < 8; ++i)
		s->h[i] += wv[i];
}

void sha256_init(sha256_state_t *s) {
	s->len = 0;
	s->h[0] = 0x6a09e667;
	s->h[1] = 0xbb67ae85;
	s->h[2] = 0x3c6ef372;
	s->h[3] = 0xa54ff53a;
	s->h[4] = 0x510e527f;
	s->h[5] = 0x9b05688c;
	s->h[6] = 0x1f83d9ab;
	s->h[7] = 0x5be0cd19;
}

void sha256_update(sha256_state_t *s, const uint8_t *msg, size_t len) {
	while (len) {
		int off = s->len % 64;
		int elen = (len > 64 - off) ? 64 - off : len;
		memcpy(s->buf + off, msg, elen);
		s->len += elen;
		msg += elen;
		len -= elen;
		if (s->len % 64 == 0)
			sha256_block(s);
	}
}

void sha256_final(uint8_t h[32], sha256_state_t *s) {
	uint64_t lenb = s->len * 8;
	int i;
	s->buf[s->len++ % 64] = 0x80;
	if (s->len % 64 > 56)
		while (s->len % 64)
			s->buf[s->len++ % 64] = 0;
	if (s->len % 64 == 0)
		sha256_block(s);
	while (s->len % 64 != 56)
		s->buf[s->len++ % 64] = 0;
	s->buf[56] = lenb >> 56;
	s->buf[57] = lenb >> 48;
	s->buf[58] = lenb >> 40;
	s->buf[59] = lenb >> 32;
	s->buf[60] = lenb >> 24;
	s->buf[61] = lenb >> 16;
	s->buf[62] = lenb >>  8;
	s->buf[63] = lenb >>  0;
	sha256_block(s);
	for (i = 0; i < 32; ++i)
		h[i] = s->h[i / 4] >> (24 - i % 4 * 8);
}

void sha256(uint8_t h[32], uint8_t *msg, size_t len) {
	sha256_state_t s;
	sha256_init(&s);
	sha256_update(&s, msg, len);
	sha256_final(h, &s);
}

void sha256_hmac(uint8_t h[32], uint8_t *key, size_t key_len,
                 uint8_t *msg, size_t msg_len) {
	uint8_t dkey_i[64] = { 0 }, dkey_o[64];
	if (key_len > 64)
		sha256((uint8_t *) dkey_i, key, key_len);
	else memcpy((void *) dkey_i, (void *) key, key_len);
	
	int i;
	for (i = 0; i < 64; ++i) {
		dkey_o[i] = dkey_i[i] ^ 0x5C;
		dkey_i[i] ^= 0x36;
	}
	
	sha256_state_t s;
	sha256_init(&s);
	sha256_update(&s, (uint8_t *) dkey_i, 64);
	sha256_update(&s, (uint8_t *) msg, msg_len);
	sha256_final(h, &s);
	sha256_init(&s);
	sha256_update(&s, (uint8_t *) dkey_o, 64);
	sha256_update(&s, h, 32);
	sha256_final(h, &s);
}
