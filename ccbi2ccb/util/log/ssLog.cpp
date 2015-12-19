#include "ssLog.h"
#include <stdarg.h>
#include <stdio.h>

void SSLog(const char * pszFormat, ...)
{
	printf("ScatterStarStudio: ");
	char szBuf[kMaxLogLen + 1] = {0};
	va_list ap;
	va_start(ap, pszFormat);
	vsnprintf_s(szBuf, kMaxLogLen, pszFormat, ap);
	va_end(ap);
	printf("%s", szBuf);
	printf("\n");
}