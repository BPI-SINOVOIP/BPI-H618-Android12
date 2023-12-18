#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "algo_api.h"
#include "algo_utils.h"
#include "p_256_ecc_pp.h"
#include "lmp_ecc.h"

 static const uint8_t max_p192_secret_key[24] = { 0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
                                   0xcc,0xef,0x7c,0x1b,0x0a,0x35,0xe4,0xd8,0xda,0x69,0x14,0x18};

 static const uint8_t max_p256_secret_key[32] = {0x0c, 0xcc, 0xcc, 0xcc,
                                         0xc0, 0x00, 0x00, 0x00,
                                         0x0c, 0xcc, 0xcc, 0xcc,
                                         0xcc, 0xcc, 0xcc, 0xcc,
                                         0xc9, 0x71, 0xeb, 0x94,
                                         0xaf, 0x5a, 0x6e, 0xf9,
                                         0x4b, 0x97, 0xb1, 0xa5,
                                         0x7e, 0x31, 0x92, 0xa8};


void alogo_init(void)
{
    p_256_init_curve(KEY_LENGTH_DWORDS_P256);
}

static void algo_generate_private_key(uint8_t *private_key, size_t size)
{
    int urandom_fd, ret;

    urandom_fd = open("/dev/urandom", O_RDONLY);
    if(urandom_fd < 0) {
        return;
    }

    ret = read(urandom_fd, private_key, size);
    if (ret < 0 || ret != (int)size) {
        close(urandom_fd);
        return;
    }

    close(urandom_fd);
}

void algo_p256_generate_private_key(uint8_t *private_key)
{
    uint8_t key[32];
    uint32_t i = 0;

    do {
        algo_generate_private_key(key, sizeof(key));

        for (i = 0; i < sizeof(key); i++) {
            if (key[i] < max_p256_secret_key[i]) {
                break;
            } else {
                key[i] = max_p256_secret_key[i];
            }
        }
    } while (i == sizeof(key));

    memcpy(private_key, key, sizeof(key));
}

void algo_p192_generate_private_key(uint8_t *private_key)
{
    uint8_t key[24];
    uint32_t i = 0;

    do {
        algo_generate_private_key(key, sizeof(key));

        for (i = 0; i < sizeof(key); i++) {
            if (key[i] < max_p192_secret_key[i]) {
                break;
            } else {
                key[i] = max_p192_secret_key[i];
            }
        }

    } while (i == sizeof(key));

    memcpy(private_key, key, sizeof(key));
}

void algo_p256_generate_public_key(uint8_t *own_private_key, uint8_t *own_public_key_x, uint8_t *own_public_key_y)
{
    Point own_public_key;
    uint8_t private_key[32];

    memcpy(private_key, own_private_key, sizeof(private_key));
    big2nd(private_key, sizeof(private_key));
    memset(&own_public_key, 0, sizeof(Point));

    ECC_PointMult(&own_public_key, &(curve_p256.G), (DWORD*) private_key, KEY_LENGTH_DWORDS_P256);

    memcpy(own_public_key_x, own_public_key.x, sizeof(own_public_key.x));
    memcpy(own_public_key_y, own_public_key.y, sizeof(own_public_key.y));
    big2nd(own_public_key_x, sizeof(own_public_key.x));
    big2nd(own_public_key_y, sizeof(own_public_key.y));
}



void algo_p256_generate_dhkey(uint8_t *own_private_key, uint8_t *peer_public_key_x, uint8_t *peer_public_key_y, uint8_t *ecdh_key)
{
    Point peer_public_key, dhkey;
    uint8_t private_key[32];

    memcpy(private_key, own_private_key, sizeof(private_key));
    big2nd(private_key, sizeof(private_key));
    memset(&peer_public_key, 0, sizeof(Point));
    memcpy(peer_public_key.x, peer_public_key_x, sizeof(peer_public_key.x));
    big2nd((uint8_t*)peer_public_key.x, sizeof(peer_public_key.x));
    memcpy(peer_public_key.y, peer_public_key_y, sizeof(peer_public_key.y));
    big2nd((uint8_t*)peer_public_key.y, sizeof(peer_public_key.y));

    ECC_PointMult(&dhkey, &peer_public_key, (DWORD*) private_key, KEY_LENGTH_DWORDS_P256);

    memcpy(ecdh_key, dhkey.x, sizeof(dhkey.x));
    big2nd(ecdh_key, sizeof(peer_public_key.x));
}


void algo_p192_generate_public_key(uint8_t *own_private_key, uint8_t *own_public_key_x, uint8_t *own_public_key_y)
{
    LMecc_Generate_ECC_Key(own_private_key, BasePoint_x,BasePoint_y, 0, 1);
    LMecc_Get_Public_Key(own_public_key_x, own_public_key_y);
}



void algo_p192_generate_dhkey(uint8_t *own_private_key, uint8_t *peer_public_key_x, uint8_t *peer_public_key_y, uint8_t *ecdh_key)
{
    LMecc_Generate_ECC_Key(own_private_key, peer_public_key_x, peer_public_key_y, 0, 1);
    LMecc_Get_Public_Key(ecdh_key, NULL);
}
