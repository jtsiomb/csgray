#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
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

struct hinterv *ray_intersect(struct ray *ray, csg_object *o)
{
	switch(o->ob.type) {
	case OB_SPHERE:
		return ray_sphere(ray, o);
	case OB_CYLINDER:
		return ray_cylinder(ray, o);
	case OB_PLANE:
		return ray_plane(ray, o);
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

struct hinterv *ray_sphere(struct ray *ray, csg_object *o)
{
	int i;
	float a, b, c, d, sqrt_d, t[2], sq_rad, tmp;
	struct hinterv *hit;
	struct ray locray = *ray;

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

struct hinterv *ray_cylinder(struct ray *ray, csg_object *o)
{
	struct ray locray = *ray;

	xform_ray(&locray, o->ob.inv_xform);
	return 0;	/* TODO */
}

struct hinterv *ray_plane(struct ray *ray, csg_object *o)
{
	float vx, vy, vz, ndotv, ndotr, t;
	struct hinterv *hit;
	struct ray locray = *ray;

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

	hit->end[1].t = FLT_MAX;
	hit->end[1].x = ray->x + ray->dx * 10000.0f;
	hit->end[1].y = ray->y + ray->dy * 10000.0f;
	hit->end[1].z = ray->z + ray->dz * 10000.0f;
	return hit;
}

struct hinterv *ray_csg_un(struct ray *ray, csg_object *o)
{
	struct hinterv *hita, *hitb, *res;

	hita = ray_intersect(ray, o->un.a);
	hitb = ray_intersect(ray, o->un.b);

	if(!hita) return hitb;
	if(!hitb) return hita;

	res = interval_union(hita, hitb);
	free_hit_list(hita);
	free_hit_list(hitb);
	return res;
}

struct hinterv *ray_csg_isect(struct ray *ray, csg_object *o)
{
	struct hinterv *hita, *hitb, *res;

	hita = ray_intersect(ray, o->isect.a);
	hitb = ray_intersect(ray, o->isect.b);

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

struct hinterv *ray_csg_sub(struct ray *ray, csg_object *o)
{
	struct hinterv *hita, *hitb, *res;

	hita = ray_intersect(ray, o->un.a);
	hitb = ray_intersect(ray, o->un.b);

	if(!hita) return 0;
	if(!hitb) return hita;

	res = interval_sub(hita, hitb);
	free_hit_list(hita);
	free_hit_list(hitb);
	return res;
}


void xform_ray(struct ray *ray, float *mat)
{
	float m3x3[16];

	mat4_copy(m3x3, mat);
	mat4_upper3x3(m3x3);

	mat4_xform3(&ray->x, mat, &ray->x);
	mat4_xform3(&ray->dx, m3x3, &ray->dx);
}

static void flip_hit(struct hit *hit)
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
		*res = *a;
		res->next = 0;
		return res;
	}
	if(a->end[0].t > b->end[0].t && a->end[1].t < b->end[1].t) {
		/* A in B */
		*res = *b;
		res->next = 0;
		return res;
	}

	/* partial overlap */
	if(a->end[0].t < b->end[1].t) {
		res->end[0] = b->end[1];
		res->end[1] = a->end[0];
	} else {
		res->end[0] = a->end[1];
		res->end[1] = a->end[0];
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
		res->end[0] = a->end[0];
		res->end[1] = b->end[0];
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
	if(a->end[0].t <= b->end[0].t) {
		res->end[0] = a->end[0];
		res->end[1] = b->end[0];
	} else {
		res->end[0] = b->end[1];
		res->end[1] = a->end[1];
	}

	flip_hit(res->end + 0);
	flip_hit(res->end + 1);
	return res;
}
