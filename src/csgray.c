#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "csgray.h"
#include "matrix.h"

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

	float xform[16];

	struct object *next;
	struct object *clist, *ctail;
	struct object *parent;
};

struct sphere {
	struct object ob;
	float rad;
};

struct plane {
	struct object ob;
	float nx, ny, nz;
	float d;
};

union csg_object {
	struct object ob;
	struct sphere sph;
	struct plane plane;
};

struct camera {
	float x, y, z;
	float tx, ty, tz;
	float fov;
};

static struct camera cam;
static csg_object *root;

int csg_init(void)
{
	if(!(root = csg_null(0, 0, 0))) {
		return -1;
	}

	csg_view(0, 0, 5, 0, 0, 0);
	csg_fov(50);

	return 0;
}

void csg_destroy(void)
{
	csg_free_object(root);
	root = 0;
}

void csg_view(float x, float y, float z, float tx, float ty, float tz)
{
	cam.x = x;
	cam.y = y;
	cam.z = z;
	cam.tx = tx;
	cam.ty = ty;
	cam.tz = tz;
}

void csg_fov(float fov)
{
	cam.fov = M_PI * fov / 180.0f;
}


int csg_load(const char *fname)
{
	return 0;	/* TODO */
}

int csg_save(const char *fname)
{
	return 0;	/* TODO */
}

#define OBPTR(o) ((struct object*)(o))

void csg_add_object(csg_object *parent, csg_object *child)
{
	if(!parent) {
		parent = root;
	}

	if(parent->ob.clist) {
		parent->ob.ctail->next = OBPTR(child);
		parent->ob.ctail = OBPTR(child);
	} else {
		parent->ob.clist = parent->ob.ctail = OBPTR(child);
	}
	child->ob.parent = OBPTR(parent);
}

void csg_remove_object(csg_object *parent, csg_object *child)
{
	csg_object *c;

	if(!parent) {
		parent = root;
	}

	c = (csg_object*)parent->ob.clist;
	while(c->ob.next) {
		if(c->ob.next == OBPTR(child)) {
			c->ob.next = child->ob.next;
			child->ob.next = 0;
			child->ob.parent = 0;
			return;
		}
		c = (csg_object*)c->ob.next;
	}
}

void csg_free_object(csg_object *o)
{
	csg_object *c = (csg_object*)o->ob.clist;
	while(c) {
		csg_object *tmp = c;
		c = (csg_object*)c->ob.next;
		csg_free_object(tmp);
	}
	free(o);
}

static union csg_object *alloc_object(void)
{
	csg_object *o;

	if(!(o = calloc(sizeof *o, 1))) {
		return 0;
	}
	o->ob.type = OB_NULL;
	mat4_identity(o->ob.xform);

	csg_emission(o, 0, 0, 0);
	csg_color(o, 1, 1, 1);
	csg_roughness(o, 1);
	csg_opacity(o, 1);

	return o;
}

csg_object *csg_null(float x, float y, float z)
{
	return alloc_object();
}

csg_object *csg_sphere(float x, float y, float z, float r)
{
	csg_object *o;

	if(!(o = alloc_object())) {
		return 0;
	}

	o->sph.rad = r;
	mat4_translation(o->ob.xform, x, y, z);
	return o;
}

csg_object *csg_cylinder(float x0, float y0, float z0, float x1, float y1, float z1, float r)
{
	return 0;
}

csg_object *csg_plane(float x, float y, float z, float nx, float ny, float nz)
{
	return 0;
}

csg_object *csg_box(float x, float y, float z, float xsz, float ysz, float zsz)
{
	return 0;
}


csg_object *csg_union(csg_object *a, csg_object *b)
{
	return 0;
}

csg_object *csg_intersection(csg_object *a, csg_object *b)
{
	return 0;
}

csg_object *csg_subtraction(csg_object *a, csg_object *b)
{
	return 0;
}


void csg_emission(csg_object *o, float r, float g, float b)
{
}

void csg_color(csg_object *o, float r, float g, float b)
{
}

void csg_roughness(csg_object *o, float r)
{
}

void csg_opacity(csg_object *o, float p)
{
}


void csg_render_pixel(int x, int y, float *color)
{
}

void csg_render_image(float *pixels, int width, int height)
{
}
