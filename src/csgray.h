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
#ifndef CSGRAY_H_
#define CSGRAY_H_

typedef union csg_object csg_object;

int csg_init(void);
void csg_destroy(void);

void csg_view(float x, float y, float z, float tx, float ty, float tz);
void csg_fov(float fov);

int csg_load(const char *fname);
int csg_save(const char *fname);

void csg_add_object(csg_object *o);
int csg_remove_object(csg_object *o);
void csg_free_object(csg_object *o);

csg_object *csg_null(float x, float y, float z);
csg_object *csg_sphere(float x, float y, float z, float r);
csg_object *csg_cylinder(float x0, float y0, float z0, float x1, float y1, float z1, float r);
csg_object *csg_plane(float x, float y, float z, float nx, float ny, float nz);
csg_object *csg_box(float x, float y, float z, float xsz, float ysz, float zsz);

csg_object *csg_union(csg_object *a, csg_object *b);
csg_object *csg_intersection(csg_object *a, csg_object *b);
csg_object *csg_subtraction(csg_object *a, csg_object *b);

void csg_ambient(float r, float g, float b);

void csg_emission(csg_object *o, float r, float g, float b);
void csg_color(csg_object *o, float r, float g, float b);
void csg_roughness(csg_object *o, float r);
void csg_opacity(csg_object *o, float p);
void csg_metallic(csg_object *o, int m);

void csg_reset_xform(csg_object *o);
void csg_translate(csg_object *o, float x, float y, float z);
void csg_rotate(csg_object *o, float angle, float x, float y, float z);
void csg_scale(csg_object *o, float x, float y, float z);
void csg_lookat(csg_object *o, float x, float y, float z, float tx, float ty, float tz, float ux, float uy, float uz);

void csg_render_pixel(int x, int y, int width, int height, float aspect, float *color);
void csg_render_image(float *pixels, int width, int height);

#endif	/* CSGRAY_H_ */
