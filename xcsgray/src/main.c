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
#include <stdlib.h>
#include <math.h>
#include <GL/freeglut.h>
#include <resman.h>
#include "csgray.h"
#include "mainloop.h"

static int init(void);
static void cleanup(void);
static void display(void);
static void reshape(int x, int y);
static void keydown(unsigned char key, int x, int y);
static void mouse(int bn, int st, int x, int y);
static void motion(int x, int y);

static int load_func(const char *fname, int id, void *cls);
static int done_func(int id, void *cls);

static int win_width = 800, win_height = 600;
static int tex_width, tex_height;

static float *framebuf;

static const char *fname = "scene";

static float cam_theta, cam_phi, cam_dist = 5;
static float cam_pos[3];

static int prev_x, prev_y;
static int bnstate[8];

static struct resman *resman;


int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitWindowSize(800, 600);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_SRGB);
	glutCreateWindow("xcsgray");

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keydown);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);

	if(init() == -1) {
		return 1;
	}
	atexit(cleanup);

	main_loop(resman);
	return 0;
}

static int init(void)
{
	glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	if(!(resman = resman_create())) {
		fprintf(stderr, "failed to create resource manager\n");
		return -1;
	}
	resman_set_load_func(resman, load_func, 0);
	resman_set_done_func(resman, done_func, 0);

	if(resman_add(resman, fname, 0) == -1) {
		return -1;
	}
	return 0;
}

static void cleanup(void)
{
	resman_free(resman);
}

static void display(void)
{
	float pos[3];
	float theta, phi;

	if(tex_width != win_width || tex_height != win_height) {
		tex_width = win_width;
		tex_height = win_height;
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, tex_width, tex_height, 0, GL_RGB, GL_FLOAT, 0);

		framebuf = realloc(framebuf, win_width * win_height * 3 * sizeof *framebuf);
		if(!framebuf) {
			fprintf(stderr, "failed to allocate framebuffer\n");
			abort();
		}
	}

	theta = M_PI * cam_theta / 180.0f;
	phi = M_PI * cam_phi / 180.0f;
	pos[0] = -sin(theta) * cos(phi) * cam_dist + cam_pos[0];
	pos[1] = sin(phi) * cam_dist + cam_pos[1];
	pos[2] = cos(theta) * cos(phi) * cam_dist + cam_pos[2];

	csg_view(pos[0], pos[1], pos[2], cam_pos[0], cam_pos[1], cam_pos[2]);

	csg_render_image(framebuf, win_width, win_height);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_width, tex_height, GL_RGB, GL_FLOAT, framebuf);

	resman_poll(resman);

	glBegin(GL_QUADS);
	glTexCoord2f(0, 1);
	glVertex2f(-1, -1);
	glTexCoord2f(1, 1);
	glVertex2f(1, -1);
	glTexCoord2f(1, 0);
	glVertex2f(1, 1);
	glTexCoord2f(0, 0);
	glVertex2f(-1, 1);
	glEnd();

	glutSwapBuffers();
}

static void reshape(int x, int y)
{
	glViewport(0, 0, x, y);
	win_width = x;
	win_height = y;
}

static void keydown(unsigned char key, int x, int y)
{
	switch(key) {
	case 27:
		exit(0);
	}
}

static void mouse(int bn, int st, int x, int y)
{
	bnstate[bn - GLUT_LEFT] = (st == GLUT_DOWN) ? 1 : 0;
	prev_x = x;
	prev_y = y;
}

static void motion(int x, int y)
{
	int dx = x - prev_x;
	int dy = y - prev_y;
	prev_x = x;
	prev_y = y;

	if(!dx && !dy) return;

	if(bnstate[0]) {
		cam_theta += dx * 0.5;
		cam_phi += dy * 0.5;

		if(cam_phi < -90) cam_phi = -90;
		if(cam_phi > 90) cam_phi = 90;
		post_redisplay();
	}
	/*
	if(bnstate[1]) {
		float right[3], up[3];
		float panx = -dx * 0.01;
		float pany = -dy * 0.01;

		float theta = M_PI * cam_theta / 180.0f;
		float phi = M_PI * cam_phi / 180.0f;

		float cmat[16];
		mat4_identity(cmat);
		mat4_pre_rotate_x(phi);
		mat4_pre_rotate_y(theta);

		right[0] = panx;
		right[1] = right[2] = 0.0f;
		mat4_xform3(right, cmat, right);

		up[1] = pany;
		up[0] = up[2] = 0.0f;
		mat4_xform3(up, cmat, up);

		cam_pos[0] += right[0] - up[0];
		cam_pos[1] += right[1] - up[1];
		cam_pos[2] += right[2] - up[2];

		post_redisplay();
	}
	*/
	if(bnstate[2]) {
		cam_dist += dy * 0.1;

		if(cam_dist < 0) cam_dist = 0;
		post_redisplay();
	}
}

static int load_func(const char *fname, int id, void *cls)
{
	printf("loading %s\n", fname);

	csg_destroy();
	csg_init();

	return csg_load(fname);
}

static int done_func(int id, void *cls)
{
	printf("done!\n");
	post_redisplay();
	return 0;
}