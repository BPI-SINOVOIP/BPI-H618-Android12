#ifndef _CHARSET_CONVERT_
#define _CHARSET_CONVERT_
#include <iconv.h>  //need static lib libiconv

static int charset_convert(const char* tocode, const char* fromcode,
              char* * inbuf, size_t *inbytesleft,
              char* * outbuf, size_t *outbytesleft)
{
    iconv_t cd;

    cd = iconv_open(tocode, fromcode);
    if (cd == (iconv_t)(-1)) {
        printf("failed to iconv open from %s to %s\n", fromcode, tocode);
        return -1;
    }

    memset(*outbuf, 0, *outbytesleft);

    if (iconv(cd, inbuf, inbytesleft, outbuf, outbytesleft) == (size_t)(-1)) {
        iconv_close(cd);
        printf("failed to iconv from %s to %s\n", fromcode, tocode);
        return -1;
    }

    iconv_close(cd);
    return 0;
}

static int gbk2utf8(char *inbuf, size_t *inlen, char *outbuf, size_t *outlen)
{
    return charset_convert("UTF-8", "GBK", &inbuf, inlen, &outbuf, outlen);
}

static int utf16utf8(char *inbuf, size_t *inlen, char *outbuf, size_t *outlen)
{
    return charset_convert("UTF-8", "UTF-16BE", &inbuf, inlen, &outbuf, outlen);
}


static int figure_name_len(const char *label, int len)
{
    const char *end = label + len - 1;

    while (end >= label && (*end == ' ' || *end == 0))
        --end;
    if (end >= label)
        return end - label + 1;
    return 0;
}

#endif
