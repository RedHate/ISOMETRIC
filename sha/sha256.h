
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


