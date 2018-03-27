#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "geom.h"
#include "matrix.h"

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
	float a, b, c, d, sqrt_d, t0, t1, t, sq_rad;
	struct hit *hit;
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
	if(d < 1e-6) return 0;

	sqrt_d = sqrt(d);
	t0 = (-b + sqrt_d) / (2.0f * a);
	t1 = (-b - sqrt_d) / (2.0f * a);

	if(t0 < 1e-6) t0 = t1;
	if(t1 < 1e-6) t1 = t0;
	t = t0 < t1 ? t0 : t1;
	if(t < 1e-6) return 0;

	hit = alloc_hit();
	hit->t = t;
	hit->x = ray->x + ray->dx * t;
	hit->y = ray->y + ray->dy * t;
	hit->z = ray->z + ray->dz * t;
	hit->nx = hit->x / o->sph.rad;
	hit->ny = hit->y / o->sph.rad;
	hit->nz = hit->z / o->sph.rad;
	hit->o = o;
	return hit;
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
	if(fabs(ndotv) < 1e-6) return 0;

	ndotr = o->plane.nx * locray.dx + o->plane.ny * locray.dy + o->plane.nz * locray.dz;
	t = ndotr / ndotv;

	if(t > 1e-6) {
		hit = alloc_hit();
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
	hitb = ray_intersect(ray, o->un.a);

	if(!hita) return hitb;
	if(!hitb) return hita;

	if(hita->t < hitb->t) {
		free_hit(hitb);
		return hita;
	}
	free_hit(hita);
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
