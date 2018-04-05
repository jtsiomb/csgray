#ifndef UI_H_
#define UI_H_

#include <stdarg.h>

void glprintf(int x, int y, const char *fmt, ...);
void glvprintf(int x, int y, const char *fmt, va_list ap);

#endif	/* UI_H_ */
