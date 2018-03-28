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
#include <stdio.h>
#include <GL/freeglut.h>
#include <windows.h>
#include "mainloop.h"

static int redraw_pending;

void main_loop(struct resman *rman)
{
	for(;;) {
		if(!redraw_pending) {
			int num_handles;
			unsigned int idx;
			void **handles = resman_get_wait_handles(rman, &num_handles);

			idx = MsgWaitForMultipleObjects(num_handles, handles, 0, INFINITE, QS_ALLEVENTS);
			if(idx == WAIT_FAILED) {
				unsigned int err = GetLastError();
				fprintf(stderr, "failed to wait for events: %u\n", err);
			}

			if((int)idx < num_handles) {
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
