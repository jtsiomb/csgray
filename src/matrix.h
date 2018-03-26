#ifndef MATRIX_H_
#define MATRIX_H_

#include <stdio.h>

void mat4_identity(float *m);
void mat4_copy(float *dest, float *src);
void mat4_mul(float *dest, float *a, float *b);

void mat4_xform3(float *vdest, float *m, float *v);
void mat4_xform4(float *vdest, float *m, float *v);

float *mat4_row(float *m, int row);
float mat4_elem(float *m, int row, int col);

void mat4_upper3x3(float *m);

void mat4_transpose(float *m);
int mat4_inverse(float *m);

void mat4_translation(float *m, float x, float y, float z);
void mat4_rotation_x(float *m, float angle);
void mat4_rotation_y(float *m, float angle);
void mat4_rotation_z(float *m, float angle);
void mat4_rotation(float *m, float angle, float x, float y, float z);
void mat4_scaling(float *m, float sx, float sy, float sz);

void mat4_translate(float *m, float x, float y, float z);
void mat4_rotate_x(float *m, float angle);
void mat4_rotate_y(float *m, float angle);
void mat4_rotate_z(float *m, float angle);
void mat4_rotate(float *m, float angle, float x, float y, float z);
void mat4_scale(float *m, float sx, float sy, float sz);

void mat4_pre_translate(float *m, float x, float y, float z);
void mat4_pre_rotate_x(float *m, float angle);
void mat4_pre_rotate_y(float *m, float angle);
void mat4_pre_rotate_z(float *m, float angle);
void mat4_pre_rotate(float *m, float angle, float x, float y, float z);
void mat4_pre_scale(float *m, float sx, float sy, float sz);

void mat4_lookat(float *m, float x, float y, float z, float tx, float ty, float tz, float ux, float uy, float uz);
void mat4_inv_lookat(float *m, float x, float y, float z, float tx, float ty, float tz, float ux, float uy, float uz);
void mat4_ortho(float *m, float left, float right, float bottom, float top, float znear, float zfar);
void mat4_frustum(float *m, float *left, float right, float bottom, float top, float znear, float zfar);
void mat4_perspective(float *m, float fov, float aspect, float znear, float zfar);

void mat4_mirror(float *m, float a, float b, float c, float d);

void mat4_print(float *m, FILE *fp);

#endif	/* MATRIX_H_ */
