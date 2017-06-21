#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

#define ADDRESSSIZE		4


uint32_t inet_aton(const char *cp)
{
	uint32_t val;
	uint8_t base, n;
	char c;
	uint32_t parts[4];
	uint32_t *pp = parts;

	c = *cp;
	for (;;) {
		/*
		 * Collect number up to ``.''.
		 * Values are specified as for C:
		 * 0x=hex, 0=octal, isdigit=decimal.
		 */
		if (!isdigit(c))
			return false;
		val = 0; base = 10;
		if (c == '0') {
			c = *++cp;
			if (c == 'x' || c == 'X')
				base = 16, c = *++cp;
			else
				base = 8;
		}
		for (;;) {
			if (isascii(c) && isdigit(c)) {
				val = (val * base) + (c - '0');
				c = *++cp;
			} else if (base == 16 && isascii(c) && isxdigit(c)) {
				val = (val << 4) |
					(c + 10 - (islower(c) ? 'a' : 'A'));
				c = *++cp;
			} else
				break;
		}
		if (c == '.') {
			/*
			 * Internet format:
			 *	a.b.c.d
			 *	a.b.c	(with c treated as 16 bits)
			 *	a.b	(with b treated as 24 bits)
			 */
			if (pp >= parts + 3)
				return false;
			*pp++ = val;
			c = *++cp;
		} else
			break;
	}
	/*
	 * Check for trailing characters.
	 */
	if (c != '\0' && (!isascii(c) || !isspace(c)))
		return false;
	/*
	 * Concoct the address according to
	 * the number of parts specified.
	 */
	n = pp - parts + 1;
	switch (n) {

	case 0:
		return false;		/* initial nondigit */

	case 1:				/* a -- 32 bits */
		break;

	case 2:				/* a.b -- 8.24 bits */
		if ((val > 0xffffff) || (parts[0] > 0xff))
			return false;
		val |= parts[0] << 24;
		break;

	case 3:				/* a.b.c -- 8.8.16 bits */
		if ((val > 0xffff) || (parts[0] > 0xff) || (parts[1] > 0xff))
			return false;
		val |= (parts[0] << 24) | (parts[1] << 16);
		break;

	case 4:				/* a.b.c.d -- 8.8.8.8 bits */
		if ((val > 0xff) || (parts[0] > 0xff) || (parts[1] > 0xff) || (parts[2] > 0xff))
			return false;
		val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
		break;
	}
	return (val);
}

uint32_t inet_pton_ipv4(const char *src, unsigned char *dst)
{
	static const char digits[] = "0123456789";
	int saw_digit, octets, ch;
	unsigned char tmp[ADDRESSSIZE], *tp;

	saw_digit = 0;
	octets = 0;
	tp = tmp;
	*tp = 0;
	while((ch = *src++) != '\0') {
		const char *pch;

		if((pch = strchr(digits, ch)) != NULL) {
			unsigned int val = *tp * 10 + (unsigned int)(pch - digits);

			if(saw_digit && *tp == 0)
			return false;
			if(val > 255)
			return false;
			*tp = (unsigned char)val;
			if(! saw_digit) {
				if(++octets > 4)
				return false;
				saw_digit = 1;
			}
		}
		else if(ch == '.' && saw_digit) {
			if(octets == 4)
			return false;
			*++tp = 0;
			saw_digit = 0;
		}
		else
		return false;
	}
	if(octets < 4)
	return false;
	memcpy(dst, tmp, ADDRESSSIZE);
	return true;
}

static unsigned int integer_to_ascii(char* destination,unsigned int source) {
   unsigned int temp=source;
   unsigned int length=0;

   if(source>=100){
	   *destination++=temp/100+'0';
	   temp=temp%100;
	   ++length;
	}

   if(source>=10){
	   *destination++=temp/10+'0';
	   temp=temp%10;
	   ++length;
	}

   *destination++=temp+'0';
   return length+1;
}

char *inet_ntoa(uint32_t ip_address){
   static char buffer[20];
   unsigned int length;
   unsigned char *ip=(unsigned char*)&ip_address;

   length=integer_to_ascii(buffer,ip[3]);
   buffer[length]='.';
   ++length;

   length+=integer_to_ascii(buffer + length,ip[2]);
   buffer[length]='.';
   ++length;

   length+=integer_to_ascii(buffer + length,ip[1]);
   buffer[length]='.';
   ++length;

   length+=integer_to_ascii(buffer + length,ip[0]);
   buffer[length]=0;

   return buffer;
}
