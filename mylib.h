#ifndef MYLIB_H
#define MYLIB_H

#if !defined(__cplusplus)
#include <stdbool.h>
#else
extern "C" /* Use C linkage for kernel_main. */
#endif // ! defined(__cplusplus)
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

// #define io_sti interrupt_enable
// #define io_cli interrupt_disable

void wait(int32_t count);
void waitMicro(uint32_t us);
void dump(uint8_t *a, uint32_t size);
char *strcpy(char *dst, const char *src);
int strcmp(const char *str1, const char *str2);
int strncmp(const char *str1, const char *str2, size_t len);
void *memSet(void *str, int c, size_t n);
void *memCopy(void *dest, const void *src, size_t n);
int memCompare(const void *str1, const void *str2, size_t n);
size_t strlen(const char* str);
int printf(const char *format, ...);
int vsprintf(char *str, const char *format, va_list listPointer);
int sprintf(char *str, const char *format, ...);
int strHex(char *str, uint32_t ad, int len, int fill);
int strNum(char *str, uint32_t ui, int len, int fill, int sflag);
void log_error(int error_id);
int log_note(char *str, const char *format, ...);

#define STR_GUARD 1024

// Severity (change this before building if you want different values)
//#define LOG_ERROR	1
//#define LOG_WARNING	2x
//#define LOG_NOTICE	3
//#define LOG_DEBUG	4

void LogWrite (const char *pSource,		// short name of module
	       unsigned	   Severity,		// see above
	       const char *pMessage, ...);	// uses printf format options

//
// Debug support
//
#ifndef NDEBUG
// display "assertion failed" message and halt
void uspi_assertion_failed (const char *pExpr, const char *pFile, unsigned nLine);
// display hex dump (pSource can be 0)
void DebugHexdump (const void *pBuffer, unsigned nBufLen, const char *pSource /* = 0 */);
#endif  // NDEBUG

#endif  // MYLIB_H

