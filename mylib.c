#include "mylib.h"
#include "uart.h"
#include "mmio.h"

inline void io_sti()
{
	asm volatile("cpsie i");
}

inline void io_cli()
{
	asm volatile("cpsid i");
}

uint64_t get_system_timer(void)
{
	uint32_t chi, clo;
	
	chi = mmio_read(STIMER_CHI);
	clo = mmio_read(STIMER_CLO);

	if(chi !=  mmio_read(STIMER_CHI)){
		chi = mmio_read(STIMER_CHI);
		clo = mmio_read(STIMER_CLO);
	}

	return (((unsigned long long) chi) << 32) | clo;

}

void wait(int32_t count)
{
  asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
               : "=r"(count): [count]"0"(count) : "cc");
}

void waitMicro(uint32_t tt)
{
	uint64_t t0;
	t0 = get_system_timer();
	
	while(get_system_timer() < t0 + tt);

	return;
	
}

char *strcpy(char *dst, const char *src)
{
    size_t i;
    
    for (i = 0; src[i] != '\0'; i++)
        dst[i] = src[i];
    dst[i] = '\0';
    
    return dst;
}

int strcmp(const char *str1, const char *str2)
{
	char *s1 = (char *) str1;
	char *s2 = (char *) str2;
	while(*s1 == *s2) {
		if (*s1 == 0) {
			return 0;
		}
		s1++;
		s2++;
	}
	if (*s1<*s2) {
		return -1;
	} else {
		return 1;
	}
}

int strncmp(const char *str1, const char *str2, size_t len)
{
	char *s1 = (char *) str1;
	char *s2 = (char *) str2;
	while(len--) {
		if (*s1 != *s2) {
			return *s1-*s2;
		}
		s1++;
		s2++;
	}
	return 0;
}

void *memSet(void *str, int c, size_t n)
{
	char *dst = (char *) str;
	while(n--) {
		*dst++ = (char) (c & 0xFF);
	}
	return str;
}


void *memCopy(void *dest, const void *src, size_t n)
{
	char *d = (char *) dest;
	char *s = (char *) src;
	while(n--) {
		*d++ = *s++;
	}
	return dest;
}

int memCompare(const void *str1, const void *str2, size_t n)
{
	const char *s1 = str1;
	const char *s2 = str2;
	while(n--) {
		if (*s1<*s2) {
			return -1;
		} else if (*s1>*s2) {
			return 1;
		}
		s1++;
		s2++;
	}
	return 0;
}

void dump(unsigned char *a, uint32_t size) {
	uint32_t j, i;
	char s[17];
	
	for(j=0; j<(size+15)/16; j++) {
		printf("0x%08x:", (uint32_t) a);
		for(i = 0; i<16; i++) {
			if (i%4==0) {
				printf(" ");
			}
			printf("%02x ", *(a+i));
			s[i] = *(a+i);
			s[i] = (32 <= s[i] && s[i] < 126) ? s[i] : '.';
		}
		s[i]=0;
		printf(" | %s\n", s);
		a += 16;
	}
}

/* uint32_t u32_div(uint32_t n, uint32_t d) */
/* { */
/* 	signed int q; */
/* 	uint32_t bit; */

/* 	q = 0; */
/* 	if (d==0) { */
/* 		q=0xFFFFFFFF; */
/* 	} else { */
/* 		bit = 1; */
/* 		while(((d & 0x80000000) == 0) && (d<n)) { */
/* 			d <<=1; */
/* 			bit <<=1; */
/* 		} */
		
/* 		while(bit>0) { */
/* 			if (n>=d) { */
/* 				n -= d; */
/* 				q += bit; */
/* 			} */
/* 			d   >>= 1; */
/* 			bit >>= 1; */
/* 		} */
/* 	} */

/* 	return q; */
/* } */

size_t strlen(const char* str)
{
	size_t ret = 0;
	while ( str[ret] != 0 ) {
		ret++;
	}
	return ret;
}



int putchar(char a)
{
	// todo: add stdio support
	uart_putc(a);
	return (int) a;
}

int getchar()
{
	// todo: add stdio support
	return uart_getc();
}

void puts(const char* str)
{
	size_t size, i;
	size = strlen(str);
	for (i = 0; i < size; i++ )
		putchar(str[i]);
}

int printf(const char *format, ...)
{
	va_list ap;
	char s[1024];
	int i;

	va_start(ap, format);
	i = vsprintf(s, format, ap);
	puts(s);
	va_end(ap);
	return i;
}

int vsprintf(char *str, const char *format, va_list listPointer)
{
	char c;
	int32_t i, si;
	uint32_t ui;
	int pf;
	char *s;
	int len, fill, sign_flag;
	
	i=0;
	pf = 0;
	len = 0;
	fill = 0;
	while((c=*format++)!=0 && i<STR_GUARD) {
		if (pf==0) {
			// after regular character
			if (c=='%') {
				pf=1;
				len = 0;
				fill = 0;
			} else {
				str[i++]=c;
			}
		} else if (pf>0) {
			// previous character was '%'
			if (c=='x' || c=='X') {
				ui = va_arg(listPointer, unsigned);
				i += strHex(str+i, (unsigned) ui, len, fill);
				pf=0;
			} else if (c=='u' || c=='U' || c=='i' || c=='I') {
				ui = va_arg(listPointer, unsigned);
				i += strNum(str+i, (unsigned) ui, len, fill, 0);
				pf=0;
			} else if (c=='d' || c=='D') {
				si = va_arg(listPointer, int);
				if (si<0) {
					ui = -si;
					sign_flag = 1;
				} else {
					ui = si;
					sign_flag = 0;
				}
				i += strNum(str+i, (unsigned) ui, len, fill, sign_flag);
				pf=0;
			} else if (c=='s' || c=='S') {
				s = va_arg(listPointer, char *);
				strcpy(str+i, s);
                i += strlen(s);
				pf=0;
			} else if ('0'<=c && c<='9') {
				if (pf==1 && c=='0') {
					fill = 1;
				} else {
					len = len*10+(c-'0');
				}
				pf=2;
			} else {
				// this shouldn't happen
				str[i++]=c;
				pf=0;
			}
		}
	}
	str[i]=0;
	if (i>STR_GUARD) {
		log_error(1);
	}
	return (i>0) ? i : -1;
}

int sprintf(char *str, const char *format, ...)
{
	va_list ap;
	int i;

	va_start(ap, format);
	i = vsprintf(str, format, ap);
	va_end(ap);
	return i;
}

int strHex(char *str, uint32_t ad, int len, int fill)
{
	int i, j=0;
	int c;
	char s[1024];
	int st;

	st = 0;
	for(i=0; i<8; i++) {
		if ((c = ((ad>>28) & 0x0F))>0) {
			c = (c<10) ? (c+'0') : (c+'a'-10);
			s[j++] = (char) c;
			st = 1;
		} else if (st || i==7) {
			s[j++] = '0';
		}
		ad = ad << 4;
	}
	s[j]=0;
	for(i=0; i<len-j; i++) {
		str[i] = (fill) ? '0' : ' ';
	}
	strcpy(str+i, s);
	
	return j+i;
}

int strNum(char *str, uint32_t ui, int len, int fill, int sflag)
{
	unsigned int cmp = 1;
	int i, j;
	int d;
	int l;
	char c;
	char s[1024];

	cmp=1;
	l=1;
	//	printf("ui:%d, cmp:%d, l:%d\n", ui, cmp, l);
	while(cmp*10<=ui && l<10) {
		//2^32 = 4.29 * 10^9 -> Max digits =10 for 32 bit
		cmp *=10;
		l++;
		//		printf("ui:%d, cmp:%d, l:%d\n", ui, cmp, l);
	}

	j = 0;
	if (sflag) {
		s[j++]='-';
		l++; // add one space for '0'
	}
	//	printf("ui:%d, cmp:%d, j:%d, d:%d, c:%d\n", ui, cmp, j, d, c);
	while(j<l) {
		//		d = u32_div(ui, cmp);
		d = ui/cmp;
		c = (char) (d+'0');
		s[j++] = c;
		ui = ui - d*cmp;
		//cmp = u32_div(cmp, 10);
		cmp = cmp/10;
		//		Printf("ui:%d, cmp:%d, j:%d, d:%d, c:%d\n", ui, cmp, j, d, c);
	}
	s[j]=0;
	for(i=0; i<len-l; i++) {
		str[i] = (fill) ? '0' : ' ';
	}
	strcpy(str+i, s);
	
	return j+i;
}

int log_note(char *str, const char *format, ...)
{
	va_list ap;
	int i;
	int len;

	va_start(ap, format);
	len = strlen(str);
	i = vsprintf(str+len, format, ap);
	if (len+i>STR_GUARD) {
		log_error(2);
	}
	va_end(ap);
	return i;
}

void log_error(int error_id) {
	char s[] = "Error at ";
	int i;

    io_cli();
	i = 0;
	while(s[i]!=0) {
		uart_putc(s[i++]);
	}
	uart_putc('0'+error_id); // error_id must be between 0 and 9;
	while(1) {}
}


/* Following are for future uspi support */
void LogWrite (const char *pSource,		// short name of module
	       unsigned	   Severity,		// see above
	       const char *pMessage, ...)	// uses printf format options
{
	switch(Severity) {
	case 1:
		printf("Error: ");
		break;
	case 2:
		printf("Warning: ");
		break;
	case 3:
		printf("Notice: ");
		break;
	case 4:
	default:
		printf("DEBUG: ");
	}
	printf("from %s, %s\n", pSource, pMessage);
}

void uspi_assertion_failed (const char *pExpr, const char *pFile, unsigned nLine)
{
	printf("Assertion (%s) from %s failed %d\n", pExpr, pFile, nLine);
	while(1) {}
}

void DebugHexdump (const void *pBuffer, unsigned nBufLen, const char *pSource /* = 0 */)
{
	printf("Dump from %s\n", pSource);
	dump((unsigned char *) pBuffer, nBufLen);
}

