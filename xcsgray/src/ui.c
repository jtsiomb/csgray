#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include "ui.h"

#ifdef WIN32
#include <malloc.h>
#else
#include <alloca.h>
#endif

extern int win_width, win_height;

void glprintf(int x, int y, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	glvprintf(x, y, fmt, ap);
	va_end(ap);
}

void glvprintf(int x, int y, const char *fmt, va_list ap)
{
	int len;
	char *buf;
	va_list tmp;

	va_copy(tmp, ap);
	if((len = vsnprintf(0, 0, fmt, tmp)) <= 0) {
		len = 512;	/* this shouldn't happen on C99-conformant implementations */
	}
	va_end(tmp);

	buf = alloca(len + 1);
	vsnprintf(buf, len + 1, fmt, ap);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, win_width, 0, win_height, -1, 1);

	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	glRasterPos2i(x, y);
	while(*buf) {
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *buf++);
	}

	glPopAttrib();
	glPopMatrix();
}
