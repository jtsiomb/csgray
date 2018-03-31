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

struct hinterv {
	csg_hit end[2];
	csg_object *o;

	struct hinterv *next;
};


struct hinterv *alloc_hits(int n);
void free_hit(struct hinterv *hv);
void free_hit_list(struct hinterv *hv);

struct hinterv *ray_intersect(csg_ray *ray, csg_object *o);

struct hinterv *ray_sphere(csg_ray *ray, csg_object *o);
struct hinterv *ray_cylinder(csg_ray *ray, csg_object *o);
struct hinterv *ray_plane(csg_ray *ray, csg_object *o);
struct hinterv *ray_box(csg_ray *ray, csg_object *o);
struct hinterv *ray_csg_un(csg_ray *ray, csg_object *o);
struct hinterv *ray_csg_isect(csg_ray *ray, csg_object *o);
struct hinterv *ray_csg_sub(csg_ray *ray, csg_object *o);

void xform_ray(csg_ray *ray, float *mat);

#endif	/* GEOM_H_ */
