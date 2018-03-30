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
#ifndef CSGIMPL_H_
#define CSGIMPL_H_

#include "csgray.h"

enum {
	OB_NULL,
	OB_SPHERE,
	OB_CYLINDER,
	OB_PLANE,
	OB_BOX,
	OB_UNION,
	OB_INTERSECTION,
	OB_SUBTRACTION
};

struct object {
	int type;

	char *name;

	float r, g, b;
	float emr, emg, emb;
	float roughness;
	float opacity;
	int metallic;

	float xform[16], inv_xform[16];

	csg_object *next;
	csg_object *plt_next;

	void (*destroy)(csg_object*);
};

struct sphere {
	struct object ob;
	float rad;
};

struct cylinder {
	struct object ob;
	float rad, height;
};

struct plane {
	struct object ob;
	float nx, ny, nz;
	float d;
};

struct box {
	struct object ob;
	float xsz, ysz, zsz;
};

struct csgop {
	struct object ob;
	csg_object *a, *b;
};

union csg_object {
	struct object ob;
	struct sphere sph;
	struct cylinder cyl;
	struct plane plane;
	struct box box;
	struct csgop un, isect, sub;
};

struct camera {
	float x, y, z;
	float tx, ty, tz;
	float ux, uy, uz;
	float fov;

	float xform[16];
};

int csg_dbg_pixel;
int csg_dbg_pixel_x, csg_dbg_pixel_y;

#endif	/* CSGIMPL_H_ */
