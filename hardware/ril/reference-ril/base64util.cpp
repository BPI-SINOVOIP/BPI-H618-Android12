/*
**
** Copyright 2020, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include "base64util.h"
#include <string.h>

extern "C" {

static const char *base64Table="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const char base64TableMap[] = {
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,  62, 255, 255, 255,  63,
     52,  53,  54,  55,  56,  57,  58,  59,  60,  61, 255, 255, 255, 255, 255, 255,
    255,   0,   1,   2,   3,   4,  5,    6,   7,   8,   9,  10,  11,  12,  13,  14,
     15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25, 255, 255, 255, 255, 255,
    255,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,
     41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};

/** encode base64, returns the number of bytes decoded */
char *base64_encode(const unsigned char *bindata, char *base64, int binlength)
{
    int i, n = 0, state = 0, pos = 0;
    for (i = 0; i < binlength; i++) {
        switch (pos) {
            case 0:
                state = (bindata[i] & 0xFC) >> 2;
                base64[n++] = base64Table[state];
                state = (bindata[i] & 0x3) << 4;
                pos = 1;
                break;
            case 1:
                state |= ((bindata[i] & 0xF0) >> 4);
                base64[n++] = base64Table[state];
                state = (bindata[i] & 0xF) << 2;
                pos = 2;
                break;
            case 2:
                state |= ((bindata[i] & 0xC0) >> 6);
                base64[n++] = base64Table[state];
                state = bindata[i] & 0x3F;
                base64[n++] = base64Table[state];
                pos = 0;
                break;
        }
    }

    if (pos == 1) {
        base64[n++] = base64Table[state];
        base64[n++] = '=';
        base64[n++] = '=';
    } else if (pos == 2) {
        base64[n++] = base64Table[state];
        base64[n++] = '=';
    }
    return (n > 0) ? base64 : 0;
}

/** decode base64, returns the encoded output */
int base64_decode(const char *base64, unsigned char *bindata)
{
    int pos = 0, i, n = 0, cch;
    char t;
    int base64Len = strlen(base64);
    for (i = 0; i < base64Len; i++) {
        cch = base64[i];
        t = base64TableMap[cch];
        if (cch == '=')
            break;
        switch (pos) {
            case 0:
                bindata[n] = (t << 2) & 0xff;
                pos = 1;
                break;
            case 1:
                bindata[n] = (bindata[n] | ((t & 0x30) >> 4)) & 0xff;
                n++;
                bindata[n] = ((t & 0xf) << 4) & 0xff;
                pos = 2;
                break;
            case 2:
                bindata[n] = (bindata[n] | ((t & 0x3c) >> 2)) & 0xff;
                n++;
                bindata[n] = ((t & 0x3) << 6) & 0xff;
                pos = 3;
                break;
            case 3:
                bindata[n] = (bindata[n] | t) & 0xff;
                n++;
                pos = 0;
                break;
            }
    }
    return n;
}

}
