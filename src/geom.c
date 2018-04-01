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
#include <float.h>
#include <assert.h>
#include "geom.h"
#include "matrix.h"

#define EPSILON		1e-6f

static struct hinterv *interval_union(struct hinterv *a, struct hinterv *b);
static struct hinterv *interval_isect(struct hinterv *a, struct hinterv *b);
static struct hinterv *interval_sub(struct hinterv *a, struct hinterv *b);

/* TODO custom hit allocator */
struct hinterv *alloc_hit(void)
{
	struct hinterv *hit = calloc(sizeof *hit, 1);
	if(!hit) {
		perror("failed to allocate ray hit node");
		abort();
	}
	return hit;
}

struct hinterv *alloc_hits(int n)
{
	int i;
	struct hinterv *list = 0;

	for(i=0; i<n; i++) {
		struct hinterv *hit = alloc_hit();
		hit->next = list;
		list = hit;
	}
	return list;
}


void free_hit(struct hinterv *hit)
{
	free(hit);
}

void free_hit_list(struct hinterv *hit)
{
	while(hit) {
		struct hinterv *tmp = hit;
		hit = hit->next;
		free_hit(tmp);
	}
}

struct hinterv *ray_intersect(csg_ray *ray, csg_object *o)
{
	switch(o->ob.type) {
	case OB_SPHERE:
		return ray_sphere(ray, o);
	case OB_CYLINDER:
		return ray_cylinder(ray, o);
	case OB_PLANE:
		return ray_plane(ray, o);
	case OB_BOX:
		return ray_box(ray, o);
	case OB_UNION:
		return ray_csg_un(ray, o);
	case OB_INTERSECTION:
		return ray_csg_isect(ray, o);
	case OB_SUBTRACTION:
		return ray_csg_sub(ray, o);
	default:
		break;
	}
	return 0;
}

struct hinterv *ray_sphere(csg_ray *ray, csg_object *o)
{
	int i;
	float a, b, c, d, sqrt_d, t[2], sq_rad, tmp;
	struct hinterv *hit;
	csg_ray locray = *ray;

	if(o->sph.rad == 0.0f) {
		return 0;
	}
	sq_rad = o->sph.rad * o->sph.rad;

	xform_ray(&locray, o->ob.inv_xform);

	a = locray.dx * locray.dx + locray.dy * locray.dy + locray.dz * locray.dz;
	b = 2.0f * (locray.dx * locray.x + locray.dy * locray.y + locray.dz * locray.z);
	c = (locray.x * locray.x + locray.y * locray.y + locray.z * locray.z) - sq_rad;

	d = b * b - 4.0f * a * c;
	if(d < EPSILON) return 0;

	sqrt_d = sqrt(d);
	t[0] = (-b + sqrt_d) / (2.0f * a);
	t[1] = (-b - sqrt_d) / (2.0f * a);

	if(t[0] < EPSILON && t[1] < EPSILON) {
		return 0;
	}

	if(t[1] < t[0]) {
		tmp = t[0];
		t[0] = t[1];
		t[1] = tmp;
	}

	hit = alloc_hits(1);
	hit->o = o;
	for(i=0; i<2; i++) {
		float c[3] = {0, 0, 0};
		float x, y, z;

		mat4_xform3(c, o->ob.xform, c);

		x = ray->x + ray->dx * t[i];
		y = ray->y + ray->dy * t[i];
		z = ray->z + ray->dz * t[i];

		hit->end[i].t = t[i];
		hit->end[i].x = x;
		hit->end[i].y = y;
		hit->end[i].z = z;
		hit->end[i].nx = (x - c[0]) / o->sph.rad;
		hit->end[i].ny = (y - c[1]) / o->sph.rad;
		hit->end[i].nz = (z - c[2]) / o->sph.rad;
		hit->end[i].o = o;
	}
	return hit;
}

static int ray_cylcap(csg_ray *ray, float y, float rad, float *tres)
{
	float ndotr, ndotv, vy, t;
	float ny = y > 0.0f ? 1.0f : -1.0f;
	float x, z, lensq;

	ndotr = ny * ray->dy;
	if(fabs(ndotr) < EPSILON) return 0;

	vy = y - ray->y;

	ndotv = ny * vy;

	t = ndotv / ndotr;

	x = ray->x + ray->dx * t;
	z = ray->z + ray->dz * t;
	lensq = x * x + z * z;

	if(lensq <= rad * rad) {
		*tres = t;
		return 1;
	}
	return 0;
}

struct hinterv *ray_cylinder(csg_ray *ray, csg_object *o)
{
	int i, out[2] = {0}, t_is_cap[2] = {0};
	float a, b, c, d, sqrt_d, t[2], sq_rad, tmp, y[2], hh, cap_t;
	struct hinterv *hit;
	csg_ray locray = *ray;
	float dirmat[16];

	if(o->cyl.rad == 0.0f || o->cyl.height == 0.0f) {
		return 0;
	}
	sq_rad = o->cyl.rad * o->cyl.rad;
	hh = o->cyl.height / 2.0f;

	xform_ray(&locray, o->ob.inv_xform);

	a = locray.dx * locray.dx + locray.dz * locray.dz;
	b = 2.0f * (locray.dx * locray.x + locray.dz * locray.z);
	c = locray.x * locray.x + locray.z * locray.z - sq_rad;

	d = b * b - 4.0f * a * c;
	if(d < EPSILON) return 0;

	sqrt_d = sqrt(d);
	t[0] = (-b + sqrt_d) / (2.0f * a);
	t[1] = (-b - sqrt_d) / (2.0f * a);

	if(t[0] < EPSILON && t[1] < EPSILON) {
		return 0;
	}
	if(t[1] < t[0]) {
		tmp = t[0];
		t[0] = t[1];
		t[1] = tmp;
	}

	y[0] = locray.y + locray.dy * t[0];
	y[1] = locray.y + locray.dy * t[1];

	if(y[0] < -hh || y[0] > hh) {
		out[0] = 1;
	}
	if(y[1] < -hh || y[1] > hh) {
		out[1] = 1;
	}

	if(out[0]) {
		t[0] = t[1];
	}
	if(out[1]) {
		t[1] = t[0];
	}

	if(ray_cylcap(&locray, hh, o->cyl.rad, &cap_t)) {
		if(cap_t < t[0]) {
			t[0] = cap_t;
			t_is_cap[0] = 1;
			out[0] = 0;
		}
		if(cap_t > t[1]) {
			t[1] = cap_t;
			t_is_cap[1] = 1;
			out[1] = 0;
		}
	}
	if(ray_cylcap(&locray, -hh, o->cyl.rad, &cap_t)) {
		if(cap_t < t[0]) {
			t[0] = cap_t;
			t_is_cap[0] = -1;
			out[0] = 0;
		}
		if(cap_t > t[1]) {
			t[1] = cap_t;
			t_is_cap[1] = -1;
			out[1] = 0;
		}
	}

	if(out[0] && out[1]) {
		return 0;
	}

	mat4_copy(dirmat, o->ob.xform);
	mat4_upper3x3(dirmat);

	hit = alloc_hits(1);
	hit->o = o;
	for(i=0; i<2; i++) {
		float x, y, z;
		float lnorm[3];

		x = ray->x + ray->dx * t[i];
		y = ray->y + ray->dy * t[i];
		z = ray->z + ray->dz * t[i];

		if(t_is_cap[i]) {
			lnorm[0] = lnorm[2] = 0.0f;
			lnorm[1] = t_is_cap[i] > 0 ? 1.0f : -1.0f;
		} else {
			lnorm[0] = (locray.x + locray.dx * t[i]) / o->cyl.rad;
			lnorm[1] = 0;
			lnorm[2] = (locray.z + locray.dz * t[i]) / o->cyl.rad;
		}
		mat4_xform3(&hit->end[i].nx, dirmat, lnorm);

		hit->end[i].t = t[i];
		hit->end[i].x = x;
		hit->end[i].y = y;
		hit->end[i].z = z;
		hit->end[i].o = o;
	}
	return hit;
}

struct hinterv *ray_plane(csg_ray *ray, csg_object *o)
{
	float vx, vy, vz, ndotv, ndotr, t;
	struct hinterv *hit;
	csg_ray locray = *ray;

	xform_ray(&locray, o->ob.inv_xform);

	ndotr = o->plane.nx * locray.dx + o->plane.ny * locray.dy + o->plane.nz * locray.dz;
	if(fabs(ndotr) < EPSILON) return 0;

	vx = o->plane.nx * o->plane.d - locray.x;
	vy = o->plane.ny * o->plane.d - locray.y;
	vz = o->plane.nz * o->plane.d - locray.z;

	ndotv = o->plane.nx * vx + o->plane.ny * vy + o->plane.nz * vz;

	t = ndotv / ndotr;
	if(t < EPSILON) {
		return 0;
	}

	hit = alloc_hits(1);
	hit->o = hit->end[0].o = hit->end[1].o = o;
	hit->end[0].t = t;
	hit->end[0].x = ray->x + ray->dx * t;
	hit->end[0].y = ray->y + ray->dy * t;
	hit->end[0].z = ray->z + ray->dz * t;

	hit->end[0].nx = hit->end[1].nx = o->plane.nx;
	hit->end[0].ny = hit->end[1].ny = o->plane.ny;
	hit->end[0].nz = hit->end[1].nz = o->plane.nz;

	hit->end[1].t = 10000.0f;
	hit->end[1].x = ray->x + ray->dx * 10000.0f;
	hit->end[1].y = ray->y + ray->dy * 10000.0f;
	hit->end[1].z = ray->z + ray->dz * 10000.0f;
	return hit;
}

#define BEXT(x)	((x) * 0.49999)

struct hinterv *ray_box(csg_ray *ray, csg_object *o)
{
	int i, sign[3];
	float param[2][3];
	float inv_dir[3];
	float tmin, tmax, tymin, tymax, tzmin, tzmax;
	struct hinterv *hit;
	csg_ray locray = *ray;
	float dirmat[16];

	xform_ray(&locray, o->ob.inv_xform);

	for(i=0; i<3; i++) {
		float sz = *(&o->box.xsz + i);
		param[0][i] = -0.5 * sz;
		param[1][i] = 0.5 * sz;

		inv_dir[i] = 1.0f / *(&locray.dx + i);
		sign[i] = inv_dir[i] < 0;
	}

	tmin = (param[sign[0]][0] - locray.x) * inv_dir[0];
	tmax = (param[1 - sign[0]][0] - locray.x) * inv_dir[0];
	tymin = (param[sign[1]][1] - locray.y) * inv_dir[1];
	tymax = (param[1 - sign[1]][1] - locray.y) * inv_dir[1];

	if(tmin > tymax || tymin > tmax) {
		return 0;
	}
	if(tymin > tmin) {
		tmin = tymin;
	}
	if(tymax < tmax) {
		tmax = tymax;
	}

	tzmin = (param[sign[2]][2] - locray.z) * inv_dir[2];
	tzmax = (param[1 - sign[2]][2] - locray.z) * inv_dir[2];

	if(tmin > tzmax || tzmin > tmax) {
		return 0;
	}
	if(tzmin > tmin) {
		tmin = tzmin;
	}
	if(tzmax < tmax) {
		tmax = tzmax;
	}

	mat4_copy(dirmat, o->ob.xform);
	mat4_upper3x3(dirmat);

	hit = alloc_hits(1);
	hit->o = o;
	for(i=0; i<2; i++) {
		float n[3] = {0};
		float t = i == 0 ? tmin : tmax;

		float x = (locray.x + locray.dx * t) / o->box.xsz;
		float y = (locray.y + locray.dy * t) / o->box.ysz;
		float z = (locray.z + locray.dz * t) / o->box.zsz;

		if(fabs(x) > fabs(y) && fabs(x) > fabs(z)) {
			n[0] = x > 0.0f ? 1.0f : -1.0f;
		} else if(fabs(y) > fabs(z)) {
			n[1] = y > 0.0f ? 1.0f : -1.0f;
		} else {
			n[2] = z > 0.0f ? 1.0f : -1.0f;
		}

		hit->end[i].o = o;
		hit->end[i].t = t;
		hit->end[i].x = ray->x + ray->dx * t;
		hit->end[i].y = ray->y + ray->dy * t;
		hit->end[i].z = ray->z + ray->dz * t;
		mat4_xform3(&hit->end[i].nx, dirmat, n);
	}
	return hit;
}

struct hinterv *ray_csg_un(csg_ray *ray, csg_object *o)
{
	struct hinterv *hita, *hitb, *res;

	hita = ray_intersect(ray, o->csg.a);
	hitb = ray_intersect(ray, o->csg.b);

	if(!hita) return hitb;
	if(!hitb) return hita;

	res = interval_union(hita, hitb);
	free_hit_list(hita);
	free_hit_list(hitb);
	return res;
}

struct hinterv *ray_csg_isect(csg_ray *ray, csg_object *o)
{
	struct hinterv *hita, *hitb, *res;

	hita = ray_intersect(ray, o->csg.a);
	hitb = ray_intersect(ray, o->csg.b);

	if(!hita || !hitb) {
		free_hit_list(hita);
		free_hit_list(hitb);
		return 0;
	}

	res = interval_isect(hita, hitb);
	free_hit_list(hita);
	free_hit_list(hitb);
	return res;
}

struct hinterv *ray_csg_sub(csg_ray *ray, csg_object *o)
{
	struct hinterv *hita, *hitb, *res;

	if(!(hita = ray_intersect(ray, o->csg.a))) {
		return 0;
	}

	hitb = ray_intersect(ray, o->csg.b);
	if(!hitb) return hita;

	res = interval_sub(hita, hitb);
	free_hit_list(hita);
	free_hit_list(hitb);
	return res;
}


void xform_ray(csg_ray *ray, float *mat)
{
	float m3x3[16];

	mat4_copy(m3x3, mat);
	mat4_upper3x3(m3x3);

	mat4_xform3(&ray->x, mat, &ray->x);
	mat4_xform3(&ray->dx, m3x3, &ray->dx);
}

static void flip_hit(csg_hit *hit)
{
	hit->nx = -hit->nx;
	hit->ny = -hit->ny;
	hit->nz = -hit->nz;
}

static struct hinterv *interval_union(struct hinterv *a, struct hinterv *b)
{
	struct hinterv *res, *res2;

	if(a->end[0].t > b->end[1].t || a->end[1].t < b->end[0].t) {
		/* disjoint */
		res = alloc_hits(2);
		res2 = res->next;

		if(a->end[0].t < b->end[0].t) {
			*res = *a;
			*res2 = *b;
		} else {
			*res = *b;
			*res2 = *a;
		}
		res->next = res2;
		res2->next = 0;
		return res;
	}

	res = alloc_hits(1);
	res->end[0] = a->end[0].t <= b->end[0].t ? a->end[0] : b->end[0];
	res->end[1] = a->end[1].t >= b->end[1].t ? a->end[1] : b->end[1];
	return res;
}

static struct hinterv *interval_isect(struct hinterv *a, struct hinterv *b)
{
	struct hinterv *res;

	if(a->end[0].t > b->end[1].t || a->end[1].t < b->end[0].t) {
		/* disjoint */
		return 0;
	}

	res = alloc_hits(1);

	if(a->end[0].t <= b->end[0].t && a->end[1].t >= b->end[1].t) {
		/* B in A */
		*res = *b;
		res->next = 0;
		return res;
	}
	if(a->end[0].t > b->end[0].t && a->end[1].t < b->end[1].t) {
		/* A in B */
		*res = *a;
		res->next = 0;
		return res;
	}

	/* partial overlap */
	if(a->end[0].t < b->end[0].t) {
		res->end[0] = b->end[0];
		res->end[1] = a->end[1];
	} else {
		res->end[0] = a->end[0];
		res->end[1] = b->end[1];
	}
	return res;
}

static struct hinterv *interval_sub(struct hinterv *a, struct hinterv *b)
{
	struct hinterv *res;

	if(a->end[0].t >= b->end[0].t && a->end[1].t <= b->end[1].t) {
		/* A in B */
		return 0;
	}

	if(a->end[0].t < b->end[0].t && a->end[1].t > b->end[1].t) {
		/* B in A */
		res = alloc_hits(2);
		res->o = a->o;
		res->end[0] = a->end[0];
		res->end[1] = b->end[0];
		res->next->o = a->o;
		res->next->end[0] = b->end[1];
		res->next->end[1] = a->end[1];
		return res;
	}

	res = alloc_hits(1);

	if(a->end[0].t > b->end[1].t || a->end[1].t < b->end[0].t) {
		/* disjoint */
		*res = *a;
		res->next = 0;
		return res;
	}

	/* partial overlap */
	res->o = a->o;
	if(a->end[0].t <= b->end[0].t) {
		res->end[0] = a->end[0];
		res->end[1] = b->end[0];
	} else {
		res->end[0] = b->end[1];
		flip_hit(res->end + 0);
		res->end[1] = a->end[1];
	}
	return res;
}
