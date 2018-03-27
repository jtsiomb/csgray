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

	float r, g, b;
	float emr, emg, emb;
	float roughness;
	float opacity;

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
	float rad;
};

struct plane {
	struct object ob;
	float nx, ny, nz;
	float d;
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
	struct csgop un, isect, sub;
};

struct camera {
	float x, y, z;
	float tx, ty, tz;
	float fov;
};

#endif	/* CSGIMPL_H_ */
