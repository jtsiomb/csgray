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

	struct hit *next;
};

struct hit *alloc_hit(void);
void free_hit(struct hit *hit);
void free_hit_list(struct hit *hit);

struct hit *ray_intersect(struct ray *ray, csg_object *o);

struct hit *ray_sphere(struct ray *ray, csg_object *o);
struct hit *ray_cylinder(struct ray *ray, csg_object *o);
struct hit *ray_plane(struct ray *ray, csg_object *o);
struct hit *ray_csg_un(struct ray *ray, csg_object *o);
struct hit *ray_csg_isect(struct ray *ray, csg_object *o);
struct hit *ray_csg_sub(struct ray *ray, csg_object *o);

void xform_ray(struct ray *ray, float *mat);

#endif	/* GEOM_H_ */
