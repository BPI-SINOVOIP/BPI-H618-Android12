#module ckvold "1.0-007"

/* C K V O L D  --  C-Kermit file system support for VAX/VMS V4.  */

/* Edit history
 * 001 22-Jan-91 wb  Initial version with memcpy, memmove
 * 002 24-Apr-91 wb  Added memset
 * 003 01-Jul-91 wb  Added strstr
 * 004 26-Nov-91 wb  Added system
 * 006 10-Nov-96 lh  All needed w/ VAXCRTL 4-009 (VMS 4.4)
 *                   Only strstr needed w/ VAXCRTL 4-013 (VMS 4.6)
 * 007 05-Sep-99 wb  Added fmod
 */


char *strstr(a, b)
const char *a;
const char *b;
{
	int len;

	if (!a || !b) return((char *)0);
	if (!*b) return(a);
	len = strlen(b);
	while (*a) {
		if (!strncmp(a, b, len)) return(a);
		a++;
	}
	return((char *)0);
}

#ifndef VMS_V46
void *memcpy(d, s, n)
void *d, *s;
register int n;
{
    register char *ss = (char *)s, *dd = (char *)d;
    while (n-- > 0)
	*dd++ = *ss++;
    return d;
}

void *memmove(d, s, n)
void *d;
const void *s;
register int n;
{
    register char *dd = (char *)d, *ss = (char *)s;
    if (dd < ss || dd - ss >= n) {
	while (n-- > 0)
	    *dd++ = *ss++;
    } else if (n > 0) {
	dd += n;
	ss += n;
	while (n-- > 0)
	    *--dd = *--ss;
    }
    return d;
}

void *memset(d, c, n)
void *d;
register int c;
register int n;
{
    register char *dd = (char *)d;
    while (n-- > 0)
	*dd++ = c;
    return d;
}

int system(s)
char *s;
{
	while (*s == ' ' || *s == '$') s++;
	zsyscmd(s);
	return(1);
}

double fmod(x, y)
double x, y;
{
	int a;
	if (x == 0 || y == 0) {
		a = x;
	} else if ((x > 0 && y > 0) || (x < 0 && y < 0)) {
		a = x - y*floor(x/y);
	} else {
		a = x - y*ceil(x/y);
	}
	return a;
}
#endif /* ! VMS_V46 */
