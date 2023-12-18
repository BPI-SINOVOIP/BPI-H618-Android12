//from pppd.h, can't include it directly in the fuzzer because C -> C++ issues
#define	EAP_HEADERLEN	4

/* EAP message codes. */
#define	EAP_REQUEST	1
#define	EAP_RESPONSE	2
#define	EAP_SUCCESS	3
#define	EAP_FAILURE	4

/* EAP types */
#define	EAPT_IDENTITY		1
#define	EAPT_NOTIFICATION	2
#define	EAPT_NAK		3	/* (response only) */
#define	EAPT_MD5CHAP		4
#define	EAPT_OTP		5	/* One-Time Password; RFC 1938 */
#define	EAPT_TOKEN		6	/* Generic Token Card */
/* 7 and 8 are unassigned. */
#define	EAPT_RSA		9	/* RSA Public Key Authentication */
#define	EAPT_DSS		10	/* DSS Unilateral */
#define	EAPT_KEA		11	/* KEA */
#define	EAPT_KEA_VALIDATE	12	/* KEA-VALIDATE	*/
#define	EAPT_TLS		13	/* EAP-TLS */
#define	EAPT_DEFENDER		14	/* Defender Token (AXENT) */
#define	EAPT_W2K		15	/* Windows 2000 EAP */
#define	EAPT_ARCOT		16	/* Arcot Systems */
#define	EAPT_CISCOWIRELESS	17	/* Cisco Wireless */
#define	EAPT_NOKIACARD		18	/* Nokia IP smart card */
#define	EAPT_SRP		19	/* Secure Remote Password */
/* 20 is deprecated */

/* EAP SRP-SHA1 Subtypes */
#define	EAPSRP_CHALLENGE	1	/* Request 1 - Challenge */
#define	EAPSRP_CKEY		1	/* Response 1 - Client Key */
#define	EAPSRP_SKEY		2	/* Request 2 - Server Key */
#define	EAPSRP_CVALIDATOR	2	/* Response 2 - Client Validator */
#define	EAPSRP_SVALIDATOR	3	/* Request 3 - Server Validator */
#define	EAPSRP_ACK		3	/* Response 3 - final ack */
#define	EAPSRP_LWRECHALLENGE	4	/* Req/resp 4 - Lightweight rechal */

#define	SRPVAL_EBIT	0x00000001	/* Use shared key for ECP */

#define	SRP_PSEUDO_ID	"pseudo_"
#define	SRP_PSEUDO_LEN	7

#define MD5_SIGNATURE_SIZE	16
#define MIN_CHALLENGE_LENGTH	16
#define MAX_CHALLENGE_LENGTH	24

void init();
void proxy_packet(uint8_t *data, int len);

//override output so we don't write to a broken fd
void output (int unit, unsigned char *p, int len)
{

}
