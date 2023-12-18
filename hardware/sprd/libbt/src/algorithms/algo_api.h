#ifndef __BT_ALGO_API_H__
#define  __BT_ALGO_API_H__

#include <stdint.h>

void algo_p256_generate_public_key(uint8_t *own_private_key, uint8_t *own_public_key_x, uint8_t *own_public_key_y);
void algo_p192_generate_public_key(uint8_t *own_private_key, uint8_t *own_public_key_x, uint8_t *own_public_key_y);


void algo_p256_generate_private_key(uint8_t *private_key);
void algo_p192_generate_private_key(uint8_t *private_key);

void algo_p256_generate_dhkey(uint8_t *own_private_key, uint8_t *peer_public_key_x, uint8_t *peer_public_key_y, uint8_t *ecdh_key);
void algo_p192_generate_dhkey(uint8_t *own_private_key, uint8_t *peer_public_key_x, uint8_t *peer_public_key_y, uint8_t *ecdh_key);


void alogo_init(void);

#endif
