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
