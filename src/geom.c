#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "geom.h"
#include "matrix.h"

#define EPSILON		1e-6f

/* TODO custom hit allocator */
struct hit *alloc_hit(void)
{
	struct hit *hit = calloc(sizeof *hit, 1);
	if(!hit) {
		perror("failed to allocate ray hit node");
		abort();
	}
	return hit;
}

struct hit *alloc_hits(int n)
{
	int i;
	struct hit *list = 0;

	for(i=0; i<n; i++) {
		struct hit *hit = alloc_hit();
		hit->next = list;
		list = hit;
	}
	return list;
}


void free_hit(struct hit *hit)
{
	free(hit);
}

void free_hit_list(struct hit *hit)
{
	while(hit) {
		struct hit *tmp = hit;
		hit = hit->next;
		free_hit(tmp);
	}
}

struct hit *ray_intersect(struct ray *ray, csg_object *o)
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

struct hit *ray_sphere(struct ray *ray, csg_object *o)
{
	int i;
	float a, b, c, d, sqrt_d, t[2], sq_rad, tmp;
	struct hit *hit, *hitlist;
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

	if(t[0] < EPSILON) t[0] = EPSILON;
	if(t[1] < EPSILON) t[1] = EPSILON;

	hitlist = hit = alloc_hits(2);
	for(i=0; i<2; i++) {
		float c[3] = {0, 0, 0};
		mat4_xform3(c, o->ob.xform, c);

		hit->t = t[i];
		hit->x = ray->x + ray->dx * t[i];
		hit->y = ray->y + ray->dy * t[i];
		hit->z = ray->z + ray->dz * t[i];
		hit->nx = (hit->x - c[0]) / o->sph.rad;
		hit->ny = (hit->y - c[1]) / o->sph.rad;
		hit->nz = (hit->z - c[2]) / o->sph.rad;
		hit->o = o;

		hit = hit->next;
	}
	return hitlist;
}

struct hit *ray_cylinder(struct ray *ray, csg_object *o)
{
	struct ray locray = *ray;

	xform_ray(&locray, o->ob.inv_xform);
	return 0;	/* TODO */
}

struct hit *ray_plane(struct ray *ray, csg_object *o)
{
	float vx, vy, vz, ndotv, ndotr, t;
	struct hit *hit = 0;
	struct ray locray = *ray;

	xform_ray(&locray, o->ob.inv_xform);

	vx = o->plane.nx * o->plane.d - locray.x;
	vy = o->plane.ny * o->plane.d - locray.y;
	vz = o->plane.nz * o->plane.d - locray.z;

	ndotv = o->plane.nx * vx + o->plane.ny * vy + o->plane.nz * vz;
	if(fabs(ndotv) < EPSILON) return 0;

	ndotr = o->plane.nx * locray.dx + o->plane.ny * locray.dy + o->plane.nz * locray.dz;
	t = ndotr / ndotv;

	if(t > EPSILON) {
		hit = alloc_hits(1);
		hit->t = t;
		hit->x = ray->x + ray->dx * t;
		hit->y = ray->y + ray->dy * t;
		hit->z = ray->z + ray->dz * t;
		hit->nx = o->plane.nx;
		hit->ny = o->plane.ny;
		hit->nz = o->plane.nz;
		hit->o = o;
	}
	return hit;
}

struct hit *ray_csg_un(struct ray *ray, csg_object *o)
{
	struct hit *hita, *hitb;

	hita = ray_intersect(ray, o->un.a);
	hitb = ray_intersect(ray, o->un.b);

	if(!hita) return hitb;
	if(!hitb) return hita;

	if(hita->t < hitb->t) {
		free_hit_list(hitb);
		return hita;
	}
	free_hit_list(hita);
	return hitb;
}

struct hit *ray_csg_isect(struct ray *ray, csg_object *o)
{
	return 0;
}

struct hit *ray_csg_sub(struct ray *ray, csg_object *o)
{
	return 0;
}


void xform_ray(struct ray *ray, float *mat)
{
	float m3x3[16];

	mat4_copy(m3x3, mat);
	mat4_upper3x3(m3x3);

	mat4_xform3(&ray->x, mat, &ray->x);
	mat4_xform3(&ray->dx, m3x3, &ray->dx);
}
