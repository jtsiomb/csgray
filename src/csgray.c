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
#include <string.h>
#include <math.h>
#include <float.h>
#include <treestore.h>
#include "csgimpl.h"
#include "matrix.h"
#include "geom.h"

static void calc_primary_ray(struct ray *ray, int x, int y, int w, int h, float aspect);
static int ray_trace(struct ray *ray, float *col);
static void shade(float *col, struct ray *ray, struct hit *hit);
static void background(float *col, struct ray *ray);
static int find_intersection(struct ray *ray, struct hit *best);
static csg_object *load_object(struct ts_node *node);

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
	float dir[3];
	float len;

	cam.x = x;
	cam.y = y;
	cam.z = z;
	cam.tx = tx;
	cam.ty = ty;
	cam.tz = tz;

	dir[0] = tx - x;
	dir[1] = ty - y;
	dir[2] = tz - z;
	len = sqrt(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);

	if(1.0f - fabs(ty - y) / len < 1e-6f) {
		cam.ux = cam.uy = 0.0f;
		cam.uz = -1.0f;
	} else {
		cam.ux = cam.uz = 0.0f;
		cam.uy = 1.0f;
	}

	mat4_lookat(cam.xform, x, y, z, tx, ty, tz, cam.ux, cam.uy, cam.uz);
}

void csg_fov(float fov)
{
	cam.fov = M_PI * fov / 180.0f;
}


int csg_load(const char *fname)
{
	struct ts_node *root = 0, *c;
	csg_object *o;

	if(!(root = ts_load(fname))) {
		fprintf(stderr, "failed to open %s\n", fname);
		return -1;
	}
	if(strcmp(root->name, "csgray_scene") != 0) {
		fprintf(stderr, "invalid scene file: %s\n", fname);
		goto err;
	}

	c = root->child_list;
	while(c) {
		if(strcmp(c->name, "viewer") == 0) {
			static float def_pos[] = {0, 0, 5};
			static float def_targ[] = {0, 0, 0};

			float *p = ts_get_attr_vec(c, "position", def_pos);
			float *t = ts_get_attr_vec(c, "target", def_targ);

			csg_view(p[0], p[1], p[2], t[0], t[1], t[2]);
			csg_fov(ts_get_attr_num(c, "fov", 50.0f));

		} else if((o = load_object(c))) {
			csg_add_object(o);
		}
		c = c->next;
	}

	ts_free_tree(root);
	return 0;

err:
	if(root) {
		ts_free_tree(root);
	}
	return -1;
}

int csg_save(const char *fname)
{
	return -1;	/* TODO */
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
	if(o) {
		free(o->ob.name);
		if(o->ob.destroy) {
			o->ob.destroy(o);
		}
		free(o);
	}
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
	mat4_translation(o->ob.inv_xform, -x, -y, -z);
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
	mat4_translation(o->ob.inv_xform, -x, -y, -z);
	return o;
}

csg_object *csg_cylinder(float x0, float y0, float z0, float x1, float y1, float z1, float r)
{
	csg_object *o;
	float x, y, z, dx, dy, dz;
	int major;

	if(!(o = alloc_object(OB_CYLINDER))) {
		return 0;
	}
	o->cyl.rad = r;

	dx = x1 - x0;
	dy = y1 - y0;
	dz = z1 - z0;

	o->cyl.height = sqrt(dx * dx + dy * dy + dz * dz);

	if(fabs(dx) > fabs(dy) && fabs(dx) > fabs(dz)) {
		major = 0;
	} else if(fabs(dy) > fabs(dz)) {
		major = 1;
	} else {
		major = 2;
	}

	x = (x0 + x1) / 2.0f;
	y = (y0 + y1) / 2.0f;
	z = (z0 + z1) / 2.0f;

	mat4_lookat(o->ob.xform, x, y, z, dx, dz, -dy, 0, major == 2 ? 0 : 1, major == 2 ? 1 : 0);
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
	o->plane.d = 0.0f;

	mat4_translation(o->ob.xform, x, y, z);
	mat4_translation(o->ob.inv_xform, -x, -y, -z);
	return o;
}

csg_object *csg_box(float x, float y, float z, float xsz, float ysz, float zsz)
{
	csg_object *o;

	if(!(o = alloc_object(OB_BOX))) {
		return 0;
	}

	o->box.xsz = xsz;
	o->box.ysz = ysz;
	o->box.zsz = zsz;

	mat4_translation(o->ob.xform, x, y, z);
	mat4_translation(o->ob.inv_xform, -x, -y, -z);
	return o;
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

void csg_name(csg_object *o, const char *name)
{
	free(o->ob.name);
	o->ob.name = 0;

	if(name) {
		if(!(o->ob.name = malloc(strlen(name) + 1))) {
			fprintf(stderr, "failed to allocate object name string\n");
			abort();
		}
		strcpy(o->ob.name, name);
	}
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

void csg_reset_xform(csg_object *o)
{
	mat4_identity(o->ob.xform);
	mat4_identity(o->ob.inv_xform);
}

void csg_translate(csg_object *o, float x, float y, float z)
{
	mat4_translate(o->ob.xform, x, y, z);
	mat4_pre_translate(o->ob.inv_xform, -x, -y, -z);
}

void csg_rotate(csg_object *o, float angle, float x, float y, float z)
{
	angle = M_PI * angle / 180.0f;
	mat4_rotate(o->ob.xform, angle, x, y, z);
	mat4_pre_rotate(o->ob.inv_xform, -angle, x, y, z);
}

void csg_scale(csg_object *o, float x, float y, float z)
{
	mat4_scale(o->ob.xform, x, y, z);
	mat4_pre_scale(o->ob.inv_xform, 1.0f / x, 1.0f / y, 1.0f / z);
}

void csg_lookat(csg_object *o, float x, float y, float z, float tx, float ty, float tz, float ux, float uy, float uz)
{
	mat4_lookat(o->ob.xform, x, y, z, tx, ty, tz, ux, uy, uz);
	mat4_inv_lookat(o->ob.inv_xform, x, y, z, tx, ty, tz, ux, uy, uz);
}

void csg_render_pixel(int x, int y, int width, int height, float aspect, float *color)
{
	struct ray ray;

	if(csg_dbg_pixel_x > 0 && csg_dbg_pixel_x == x && csg_dbg_pixel_y == y) {
		csg_dbg_pixel = 1;
		csg_dbg_pixel_x = 0;
	} else {
		csg_dbg_pixel = 0;
	}

	calc_primary_ray(&ray, x, y, width, height, aspect);
	ray_trace(&ray, color);
}

void csg_render_image(float *pixels, int width, int height)
{
	int i, j;
	float aspect = (float)width / (float)height;

#pragma omp parallel for private(j) schedule(dynamic, 32)
	for(i=0; i<height; i++) {
		float *pptr = pixels + i * width * 3;
		for(j=0; j<width; j++) {
			csg_render_pixel(j, i, width, height, aspect, pptr);
			pptr += 3;
		}
	}
}

static void calc_primary_ray(struct ray *ray, int x, int y, int w, int h, float aspect)
{
	ray->dx = aspect * ((float)x / (float)w * 2.0f - 1.0f);
	ray->dy = 1.0f - (float)y / (float)h * 2.0f;
	ray->dz = -1.0f / tan(cam.fov * 0.5f);

	ray->x = 0;
	ray->y = 0;
	ray->z = 0;

	xform_ray(ray, cam.xform);
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
static int dbg_in_shadow_ray;

static void shade(float *col, struct ray *ray, struct hit *hit)
{
	float ndotl, ndoth, len, falloff, spec;
	csg_object *o, *lt = plights;
	float dcol[3], scol[3] = {0};
	float ldir[3], lcol[3], hdir[3];
	struct ray sray;
	struct hit tmphit;

	dbg_in_shadow_ray = 1;

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

		if(!find_intersection(&sray, &tmphit) || tmphit.t < 0.00001 || tmphit.t > 1.0f) {
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
				spec = gloss * pow(ndoth, 128.0f * gloss);

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

	dbg_in_shadow_ray = 0;

	col[0] = dcol[0] + scol[0];
	col[1] = dcol[1] + scol[1];
	col[2] = dcol[2] + scol[2];

	len = sqrt(hit->nx * hit->nx + hit->ny * hit->ny + hit->nz * hit->nz);
	hit->nx /= len;
	hit->ny /= len;
	hit->nz /= len;

	col[0] = hit->nx * 0.5 + 0.5;
	col[1] = hit->ny * 0.5 + 0.5;
	col[2] = hit->nz * 0.5 + 0.5;
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

static csg_object *load_object(struct ts_node *node)
{
	float *avec;
	struct ts_node *c;
	csg_object *sub, *o = 0, *olist = 0, *otail = 0;
	int num_subobj = 0, is_csgop = 0;

	if(strcmp(node->name, "null") == 0) {
		if(!(o = csg_null(0, 0, 0))) {
			goto err;
		}

	} else if(strcmp(node->name, "sphere") == 0) {
		float rad = ts_get_attr_num(node, "radius", 1.0f);
		if(!(o = csg_sphere(0, 0, 0, rad))) {
			goto err;
		}

	} else if(strcmp(node->name, "cylinder") == 0) {
		float rad = ts_get_attr_num(node, "radius", 1.0f);
		float height = ts_get_attr_num(node, "height", 1.0f);
		if(!(o = csg_cylinder(0, -height/2.0f, 0, 0, height/2.0f, 0, rad))) {
			goto err;
		}

	} else if(strcmp(node->name, "plane") == 0) {
		static float def_norm[] = {0, 1, 0};
		float *norm = ts_get_attr_vec(node, "normal", def_norm);
		if(!(o = csg_plane(0, 0, 0, norm[0], norm[1], norm[2]))) {
			goto err;
		}

	} else if(strcmp(node->name, "box") == 0) {
		static float def_sz[] = {1, 1, 1};
		float *sz = ts_get_attr_vec(node, "size", def_sz);
		if(!(o = csg_box(0, 0, 0, sz[0], sz[1], sz[2]))) {
			goto err;
		}

	} else if(strcmp(node->name, "union") == 0) {
		if(!(o = csg_union(0, 0))) {
			goto err;
		}
		is_csgop = 1;

	} else if(strcmp(node->name, "intersect") == 0) {
		if(!(o = csg_intersection(0, 0))) {
			goto err;
		}
		is_csgop = 1;

	} else if(strcmp(node->name, "subtract") == 0) {
		if(!(o = csg_subtraction(0, 0))) {
			goto err;
		}
		is_csgop = 1;

	} else {
		goto err;
	}

	if(is_csgop) {
		c = node->child_list;
		while(c) {
			if((sub = load_object(c))) {
				if(olist) {
					otail->ob.next = sub;
					otail = sub;
				} else {
					olist = otail = sub;
				}
				++num_subobj;
			}
			c = c->next;
		}

		if(num_subobj != 2) {
			goto err;
		}
		o->un.a = olist;
		o->un.b = olist->ob.next;
		olist->ob.next = 0;
	}

	if((avec = ts_get_attr_vec(node, "position", 0))) {
		csg_translate(o, avec[0], avec[1], avec[2]);
	}
	if((avec = ts_get_attr_vec(node, "rotaxis", 0))) {
		csg_rotate(o, ts_get_attr_num(node, "rotangle", 0.0f), avec[0], avec[1], avec[2]);
	}
	if((avec = ts_get_attr_vec(node, "scaling", 0))) {
		csg_scale(o, avec[0], avec[1], avec[2]);
	}
	if((avec = ts_get_attr_vec(node, "target", 0))) {
		/* don't move this before position */
		float def_up[] = {0, 1, 0};
		float *up = ts_get_attr_vec(node, "up", def_up);
		float x = o->ob.xform[12];
		float y = o->ob.xform[13];
		float z = o->ob.xform[14];
		csg_lookat(o, x, y, z, avec[0], avec[1], avec[2], up[0], up[1], up[2]);
	}

	csg_name(o, ts_get_attr_str(node, "name", 0));

	if((avec = ts_get_attr_vec(node, "color", 0))) {
		csg_color(o, avec[0], avec[1], avec[2]);
	}
	if((avec = ts_get_attr_vec(node, "emission", 0))) {
		csg_emission(o, avec[0], avec[1], avec[2]);
	}

	csg_roughness(o, ts_get_attr_num(node, "roughness", o->ob.roughness));
	csg_opacity(o, ts_get_attr_num(node, "opacity", o->ob.opacity));
	csg_metallic(o, ts_get_attr_int(node, "metallic", o->ob.metallic));

	return o;

err:
	csg_free_object(o);
	while(olist) {
		o = olist;
		olist = olist->ob.next;
		csg_free_object(o);
	}
	return 0;
}
