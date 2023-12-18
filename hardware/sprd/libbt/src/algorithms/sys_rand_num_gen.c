/*****************************************************************************
 *
 * MODULE NAME:    sys_rand_num_gen.c
 * PROJECT CODE:    BlueStream
 * DESCRIPTION:    32-bit pseudo random number generation
 * MAINTAINER:     Ivan Griffin
 * CREATION DATE:  17 February 2000
 *
 * SOURCE CONTROL: $Id: sys_rand_num_gen.c,v 1.16 2013/06/05 13:03:33 garyf Exp $
 *
 * LICENSE:
 *     This source code is copyright (c) 2000-2004 Ceva Inc.
 *     All rights reserved.
 *
 * REVISION HISTORY:
 *     17.Feb.2000       IG      Initial Version
 *
 * ISSUES:
 *     This represents a trade-off between spatial dispersion and
 *     speed/processing power
 *
 *     For more interesting random number algorithms, see:
 *
 *     "The Art of Computer Programming: Seminumerical Algorithms",
 *     Donald E. Knuth, 3rd Edition, Section 3.6
 *
 *     "Towards a Universal Random Number Generator" by George Marsaglia
 *     and Arif Zaman, Florida State University Report: FSU-SCRI-87-50 (1987)
 *
 *     and "A Review of Pseuo-random Number Generators" by F. James.
 *****************************************************************************/

#include "sys_rand_num_gen.h"

/*
 * simple Random Algorithm from ISO C.
 * extended to permutate lower bits more
 */

/*
 * Reported problem with random number generation system:
 * 1. Initial seed too predictable - IUT often issues
 *    same t_rand challenge to a given peer device during security
 *    procedures.
 *    - Measures 2,3,4,5 designed to address this.
 *
 * A second issue detected during analysis phase:
 * 1. All PRTH devices will use same pseudo-random sequence
 *    - All inquiry results come at same time;
 *    - If crack one PRTH device from security point of view,
 *      can crack all PRTH devices.
 *    - Measure 1 designed to address this.
 *
 *
 * Two Issues with random number generation system:
 * 1. Obscuring the initial seed:
 *    - Measures 1, 2, 3, and 4 are designed to
 *      to obscure the initial seed.
 *
 * 2. Obscuring the algorithm:
 *    - Measures 4 and 5 are designed to obscure
 *      the operation of the algorithm.
 *
 * 1. Default seed with bits[15:0] of local device address (shifted up 16)
 * 2. Override default seed from sys_hal_config.c if a specific platform
 *    happens to have a more random seed.
 * 3. default seed | bits[15:0] of native clock are used as initial seed.
 *
 * 4. Each time the sceduler runs, the seed is updated to:
 *    rand_output ^ (native clock & 0xFF)
 *    So gather a pseudo-random number and XOR it with the bottom
 *    8 bits of the native clock. Use this value as the new seed.
 *
 *    Some platforms force the scheduler to run once per frame, these
 *    will be unaffected by this behaviour. Some platforms do not,
 *    these should be further strengthened by this measure. This
 *    takes place in bt_mini_sched.c
 *
 * 5. Finally, call SYSrand_Get_Rand() from lmp_power_control.c
 *    for a different amount of times depending on the RSSI.
 *    Effectively make the rand algorithm more difficult to predict.
 *    Note: this only takes effect if both local and peer device
 *    support power control.
 */

u_int32 srand_seed;

void SYSrand_Seed_Rand(u_int32 seed)
{
    srand_seed = seed;
}

u_int32 SYSrand_Get_Rand(void)
{
#if(BUILD_TYPE==UNIT_TEST_BUILD)
    SYSrand_Get_Rand_R(&srand_seed); /* run the Rand_R to ensure coverage, but */
    return 2;                        /* return a fixed value for the purposes of the tests */
#else
    return (SYSrand_Get_Rand_R(&srand_seed));
#endif
}

u_int32 SYSrand_Get_Rand_R(u_int32 *seed) /* reentrant */
{
    u_int32 result;
    u_int32 next_seed = *seed;

    next_seed = (next_seed * 1103515245) + 12345; /* permutate seed */
    result = next_seed & 0xfffc0000; /* use only top 14 bits */

    next_seed = (next_seed * 1103515245) + 12345; /* permutate seed */
    result = result + ((next_seed & 0xffe00000) >> 14); /* top 11 bits */

    next_seed = (next_seed * 1103515245) + 12345; /* permutate seed */
    result = result + ((next_seed & 0xfe000000) >> (25)); /* use only top 7 bits */

    *seed = next_seed;

    return (result & SYS_RAND_MAX);
}

u_int8* SYSrand_Get_Rand_128_Ex(u_int8* buf)
{
    u_int32 result;
    int i;

    for (i = 0; i < 4; i++)
    {
        result = SYSrand_Get_Rand();
        *(buf + (4 * i) + 0) = result & 0xff;
        *(buf + (4 * i) + 1) = (result & 0xff00) >> 8;
        *(buf + (4 * i) + 2) = (result & 0xff0000) >> 16;
        *(buf + (4 * i) + 3) = (result & 0xff000000) >> 24;
    }

    return buf;
}

u_int8* SYSrand_Get_Rand_192_Ex(u_int8* buf)
{
    u_int32 result;
    int i;

    for (i = 0; i < 6; i++)
    {
        result = SYSrand_Get_Rand();
        *(buf + (4 * i) + 0) = result & 0xff;
        *(buf + (4 * i) + 1) = (result & 0xff00) >> 8;
        *(buf + (4 * i) + 2) = (result & 0xff0000) >> 16;
        *(buf + (4 * i) + 3) = (result & 0xff000000) >> 24;
    }
    return buf;
}

u_int8* SYSrand_Get_Rand_256_Ex(u_int8* buf)
{
    u_int32 result;
    int i;

    for (i = 0; i < 8; i++)
    {
        result = SYSrand_Get_Rand();
        *(buf + (4 * i) + 0) = result & 0xff;
        *(buf + (4 * i) + 1) = (result & 0xff00) >> 8;
        *(buf + (4 * i) + 2) = (result & 0xff0000) >> 16;
        *(buf + (4 * i) + 3) = (result & 0xff000000) >> 24;
    }
    return buf;
}
