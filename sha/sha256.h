/*
	MIT License

	Copyright (c) 2024 Netman

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/

#include <stdint.h>
#include <stddef.h>

typedef struct {
	uint32_t h[8];
	uint8_t buf[64];
	uint64_t len;
} sha256_state_t;

extern void sha256_init(sha256_state_t *s);
extern void sha256_update(sha256_state_t *s, const uint8_t *msg, size_t len);
extern void sha256_final(uint8_t h[32], sha256_state_t *s);
extern void sha256(uint8_t h[32], uint8_t *msg, size_t len);
extern void sha256_hmac(uint8_t h[32], uint8_t *key, size_t key_len, uint8_t *msg, size_t msg_len);


