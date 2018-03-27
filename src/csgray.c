#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "csgimpl.h"
#include "matrix.h"
#include "geom.h"

static void calc_primary_ray(struct ray *ray, int x, int y, int w, int h, float aspect);
static int ray_trace(struct ray *ray, float *col);
static void shade(float *col, struct ray *ray, struct hit *hit);
static void background(float *col, struct ray *ray);
static int find_intersection(struct ray *ray, struct hit *best);

static float ambient[3];
static struct camera cam;
static csg_object *oblist;
static csg_object *plights;

int csg_init(void)
{
	oblist = 0;
	plights = 0;

	csg_ambient(0, 0, 0);
	csg_view(0, 0, 5, 0, 0, 0);
	csg_fov(50);

	return 0;
}

void csg_destroy(void)
{
	while(oblist) {
		csg_object *o = oblist;
		oblist = oblist->ob.next;
		csg_free_object(o);
	}
	oblist = 0;
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

void csg_add_object(csg_object *o)
{
	o->ob.next = oblist;
	oblist = o;

	if(o->ob.type == OB_NULL && (o->ob.emr > 0.0f || o->ob.emg > 0.0f || o->ob.emb > 0.0f)) {
		o->ob.plt_next = plights;
		plights = o;
	}
}

int csg_remove_object(csg_object *o)
{
	csg_object dummy, *n;

	dummy.ob.next = oblist;
	n = &dummy;

	while(n->ob.next) {
		if(n->ob.next == o) {
			n->ob.next = o->ob.next;
			return 1;
		}
		n = n->ob.next;
	}
	return 0;
}

void csg_free_object(csg_object *o)
{
	if(o->ob.destroy) {
		o->ob.destroy(o);
	}
	free(o);
}

static union csg_object *alloc_object(int type)
{
	csg_object *o;

	if(!(o = calloc(sizeof *o, 1))) {
		return 0;
	}
	o->ob.type = type;
	mat4_identity(o->ob.xform);
	mat4_identity(o->ob.inv_xform);

	csg_emission(o, 0, 0, 0);
	csg_color(o, 1, 1, 1);
	csg_roughness(o, 1);
	csg_opacity(o, 1);

	return o;
}

csg_object *csg_null(float x, float y, float z)
{
	csg_object *o;

	if(!(o = alloc_object(OB_NULL))) {
		return 0;
	}

	mat4_translation(o->ob.xform, x, y, z);
	return o;
}

csg_object *csg_sphere(float x, float y, float z, float r)
{
	csg_object *o;

	if(!(o = alloc_object(OB_SPHERE))) {
		return 0;
	}

	o->sph.rad = r;
	mat4_translation(o->ob.xform, x, y, z);
	mat4_copy(o->ob.inv_xform, o->ob.xform);
	mat4_inverse(o->ob.inv_xform);
	return o;
}

csg_object *csg_cylinder(float x0, float y0, float z0, float x1, float y1, float z1, float r)
{
	csg_object *o;
	float dx, dy, dz;
	int major;

	if(!(o = alloc_object(OB_CYLINDER))) {
		return 0;
	}

	dx = x1 - x0;
	dy = y1 - y0;
	dz = z1 - z0;

	if(fabs(dx) > fabs(dy) && fabs(dx) > fabs(dz)) {
		major = 0;
	} else if(fabs(dy) > fabs(dz)) {
		major = 1;
	} else {
		major = 2;
	}

	o->cyl.rad = r;
	mat4_lookat(o->ob.xform, x0, y0, z0, x1, y1, z1, 0, major == 2 ? 1 : 0, major == 2 ? 0 : 1);
	mat4_copy(o->ob.inv_xform, o->ob.xform);
	mat4_inverse(o->ob.inv_xform);
	return o;
}

csg_object *csg_plane(float x, float y, float z, float nx, float ny, float nz)
{
	csg_object *o;
	float len;

	if(!(o = alloc_object(OB_PLANE))) {
		return 0;
	}

	len = sqrt(nx * nx + ny * ny + nz * nz);
	if(len != 0.0f) {
		float s = 1.0f / len;
		nx *= s;
		ny *= s;
		nz *= s;
	}

	o->plane.nx = nx;
	o->plane.ny = ny;
	o->plane.nz = nz;
	o->plane.d = x * nx + y * ny + z * nz;
	return o;
}

csg_object *csg_box(float x, float y, float z, float xsz, float ysz, float zsz)
{
	return 0;
}

csg_object *csg_union(csg_object *a, csg_object *b)
{
	csg_object *o;

	if(!(o = alloc_object(OB_UNION))) {
		return 0;
	}
	o->un.a = a;
	o->un.b = b;
	return o;
}

csg_object *csg_intersection(csg_object *a, csg_object *b)
{
	csg_object *o;

	if(!(o = alloc_object(OB_INTERSECTION))) {
		return 0;
	}
	o->isect.a = a;
	o->isect.b = b;
	return o;
}

csg_object *csg_subtraction(csg_object *a, csg_object *b)
{
	csg_object *o;

	if(!(o = alloc_object(OB_SUBTRACTION))) {
		return 0;
	}
	o->sub.a = a;
	o->sub.b = b;
	return o;
}

void csg_ambient(float r, float g, float b)
{
	ambient[0] = r;
	ambient[1] = g;
	ambient[2] = b;
}

void csg_emission(csg_object *o, float r, float g, float b)
{
	o->ob.emr = r;
	o->ob.emg = g;
	o->ob.emb = b;
}

void csg_color(csg_object *o, float r, float g, float b)
{
	o->ob.r = r;
	o->ob.g = g;
	o->ob.b = b;
}

void csg_roughness(csg_object *o, float r)
{
	o->ob.roughness = r;
}

void csg_opacity(csg_object *o, float p)
{
	o->ob.opacity = p;
}

void csg_metallic(csg_object *o, int m)
{
	o->ob.metallic = m;
}


void csg_render_pixel(int x, int y, int width, int height, float aspect, float *color)
{
	struct ray ray;

	calc_primary_ray(&ray, x, y, width, height, aspect);
	ray_trace(&ray, color);
}

void csg_render_image(float *pixels, int width, int height)
{
	int i, j;
	float aspect = (float)width / (float)height;

	for(i=0; i<height; i++) {
		for(j=0; j<width; j++) {
			csg_render_pixel(j, i, width, height, aspect, pixels);
			pixels += 3;
		}
	}
}

static void calc_primary_ray(struct ray *ray, int x, int y, int w, int h, float aspect)
{
	/* TODO */
	ray->dx = aspect * ((float)x / (float)w * 2.0f - 1.0f);
	ray->dy = 1.0f - (float)y / (float)h * 2.0f;
	ray->dz = -1.0f / tan(cam.fov * 0.5f);

	ray->x = cam.x;
	ray->y = cam.y;
	ray->z = cam.z;
}

static int ray_trace(struct ray *ray, float *col)
{
	struct hit hit;

	if(!find_intersection(ray, &hit)) {
		background(col, ray);
		return 0;
	}

	shade(col, ray, &hit);
	return 1;
}

#define NULLXPOS(o)	((o)->ob.xform[12])
#define NULLYPOS(o)	((o)->ob.xform[13])
#define NULLZPOS(o)	((o)->ob.xform[14])

static void shade(float *col, struct ray *ray, struct hit *hit)
{
	float ndotl, ndoth, len, falloff, spec;
	csg_object *o, *lt = plights;
	float dcol[3], scol[3] = {0};
	float ldir[3], lcol[3], hdir[3];
	struct ray sray;
	struct hit tmphit;

	o = hit->o;
	dcol[0] = ambient[0];
	dcol[1] = ambient[1];
	dcol[2] = ambient[2];

	while(lt) {
		ldir[0] = NULLXPOS(lt) - hit->x;
		ldir[1] = NULLYPOS(lt) - hit->y;
		ldir[2] = NULLZPOS(lt) - hit->z;

		sray.x = hit->x;
		sray.y = hit->y;
		sray.z = hit->z;
		sray.dx = ldir[0];
		sray.dy = ldir[1];
		sray.dz = ldir[2];

		if(!find_intersection(&sray, &tmphit) || tmphit.t > 1.0f) {
			if((len = sqrt(ldir[0] * ldir[0] + ldir[1] * ldir[1] + ldir[2] * ldir[2])) != 0.0f) {
				float s = 1.0f / len;
				ldir[0] *= s;
				ldir[1] *= s;
				ldir[2] *= s;
			}
			falloff = 1.0f / (len * len);

			lcol[0] = lt->ob.emr * falloff;
			lcol[1] = lt->ob.emg * falloff;
			lcol[2] = lt->ob.emb * falloff;

			if((ndotl = hit->nx * ldir[0] + hit->ny * ldir[1] + hit->nz * ldir[2]) < 0.0f) {
				ndotl = 0.0f;
			}

			dcol[0] += o->ob.r * lcol[0] * ndotl;
			dcol[1] += o->ob.g * lcol[1] * ndotl;
			dcol[2] += o->ob.b * lcol[2] * ndotl;

			if(o->ob.roughness < 1.0f) {
				float gloss = 1.0f - o->ob.roughness;

				hdir[0] = ldir[0] - ray->dx;
				hdir[1] = ldir[1] - ray->dy;
				hdir[2] = ldir[2] - ray->dz;
				if((len = sqrt(hdir[0] * hdir[0] + hdir[1] * hdir[1] + hdir[2] * hdir[2])) != 0.0f) {
					float s = 1.0f / len;
					hdir[0] *= s;
					hdir[1] *= s;
					hdir[2] *= s;
				}

				if((ndoth = hit->nx * hdir[0] + hit->ny * hdir[1] + hit->nz * hdir[2]) < 0.0f) {
					ndoth = 0.0f;
				}
				spec = gloss * pow(ndoth, 100.0f * gloss);

				if(o->ob.metallic) {
					lcol[0] *= o->ob.r;
					lcol[1] *= o->ob.g;
					lcol[2] *= o->ob.b;
				}
				scol[0] += lcol[0] * spec;
				scol[1] += lcol[1] * spec;
				scol[2] += lcol[2] * spec;
			}
		}

		lt = lt->ob.plt_next;
	}

	col[0] = dcol[0] + scol[0];
	col[1] = dcol[1] + scol[1];
	col[2] = dcol[2] + scol[2];
}

static void background(float *col, struct ray *ray)
{
	col[0] = col[1] = col[2] = 0.0f;
}

static int find_intersection(struct ray *ray, struct hit *best)
{
	int idx = 0;
	csg_object *o;
	struct hinterv *hit, *it;

	best->t = FLT_MAX;
	best->o = 0;

	o = oblist;
	while(o) {
		if((hit = ray_intersect(ray, o))) {
			it = hit;
			while(it) {
				if(it->end[0].t > 1e-6) {
					idx = 0;
					break;
				}
				if(it->end[1].t > 1e-6) {
					idx = 1;
					break;
				}
				it = it->next;
			}

			if(it && it->end[idx].t < best->t) {
				*best = it->end[idx];
			}
		}
		free_hit_list(hit);
		o = o->ob.next;
	}

	return best->o != 0;
}
