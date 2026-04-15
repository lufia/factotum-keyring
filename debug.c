#include <stdio.h>
#include <stdarg.h>

int debug;

void
debugf(char *fmt, ...)
{
	va_list arg;

	if(debug <= 0)
		return;
	va_start(arg, fmt);
	vfprintf(stderr, fmt, arg);
	va_end(arg);
}
