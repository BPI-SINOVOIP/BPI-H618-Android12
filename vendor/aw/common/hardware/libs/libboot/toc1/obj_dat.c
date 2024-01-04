#include"openssl/asn1.h"
#include"openssl/lhash.h"
#include"openssl/objects.h"
#include"openssl/err.h"



//#define OBJerr(f,r)  ERR_PUT_error(ERR_LIB_OBJ,(f),(r),__FILE__,__LINE__)

static _LHASH *added=NULL;

void reset_OBJ_nid2ln_reset(void)
{
	added=NULL;
}



int OBJ_obj2name(char *dst_buf, int buf_len, const ASN1_OBJECT *a)
{
	if(buf_len < a->length)
	{
		printf("OBJ_obj2name err: not enough buffer to store name\n");

		return -1;
	}
	memcpy(dst_buf, a->data, a->length);

	return a->length;
}
