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
#ifndef GEOM_H_
#define GEOM_H_

#include "csgray.h"
#include "csgimpl.h"

struct ray {
	float x, y, z;
	float dx, dy, dz;
};

struct hit {
	float t;
	float x, y, z;
	float nx, ny, nz;
	csg_object *o;
};

struct hinterv {
	struct hit end[2];
	csg_object *o;

	struct hinterv *next;
};


struct hinterv *alloc_hits(int n);
void free_hit(struct hinterv *hv);
void free_hit_list(struct hinterv *hv);

struct hinterv *ray_intersect(struct ray *ray, csg_object *o);

struct hinterv *ray_sphere(struct ray *ray, csg_object *o);
struct hinterv *ray_cylinder(struct ray *ray, csg_object *o);
struct hinterv *ray_plane(struct ray *ray, csg_object *o);
struct hinterv *ray_box(struct ray *ray, csg_object *o);
struct hinterv *ray_csg_un(struct ray *ray, csg_object *o);
struct hinterv *ray_csg_isect(struct ray *ray, csg_object *o);
struct hinterv *ray_csg_sub(struct ray *ray, csg_object *o);

void xform_ray(struct ray *ray, float *mat);

#endif	/* GEOM_H_ */
