/*
csgray - simple CSG raytracer
Copyright (C) 2018  John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <errno.h>
#include <unistd.h>
#include <sys/select.h>
#include <GL/freeglut.h>
#include <GL/glx.h>
#include <X11/Xlib.h>
#include "mainloop.h"

static int redraw_pending;

void main_loop(struct resman *rman)
{
	Display *dpy = glXGetCurrentDisplay();
	int xfd = ConnectionNumber(dpy);

	for(;;) {
		if(!redraw_pending) {
			int num_rman_fds, res;
			int *rman_fds = resman_get_wait_fds(rman, &num_rman_fds);

			fd_set rdset;
			int maxfd = xfd;

			FD_ZERO(&rdset);
			FD_SET(xfd, &rdset);

			for(int i=0; i<num_rman_fds; i++) {
				FD_SET(rman_fds[i], &rdset);
				if(rman_fds[i] > maxfd) maxfd = rman_fds[i];
			}

			while((res = select(maxfd + 1, &rdset, 0, 0, 0)) == -1 && errno == EINTR);

			if((res > 0 && !FD_ISSET(xfd, &rdset)) || res > 1) {
				glutPostRedisplay();
			}
		}

		redraw_pending = 0;
		glutMainLoopEvent();
	}
}

void post_redisplay(void)
{
	redraw_pending = 1;
	glutPostRedisplay();
}
