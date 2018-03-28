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
#include <string.h>
#include <math.h>

#define M(r, c) ((r << 2) + c)

static const float idmat[] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

void mat4_identity(float *m)
{
	memcpy(m, idmat, sizeof idmat);
}

void mat4_copy(float *dest, float *src)
{
	memcpy(dest, src, 16 * sizeof(float));
}

void mat4_mul(float *dest, float *a, float *b)
{
	int i, j;
	float tmp[16];

	if(dest == a) {
		memcpy(tmp, a, sizeof tmp);
		a = tmp;
	} else if(dest == b) {
		memcpy(tmp, b, sizeof tmp);
		b = tmp;
	}

	for(i=0; i<4; i++) {
		for(j=0; j<4; j++) {
			dest[M(i, j)] = a[M(i, 0)] * b[M(0, j)] + a[M(i, 1)] * b[M(1, j)] +
				a[M(i, 2)] * b[M(2, j)] + a[M(i, 3)] * b[M(3, j)];
		}
	}
}


void mat4_xform3(float *vdest, float *m, float *v)
{
	float x = m[0] * v[0] + m[4] * v[1] + m[8] * v[2] + m[12];
	float y = m[1] * v[0] + m[5] * v[1] + m[9] * v[2] + m[13];
	vdest[2] = m[2] * v[0] + m[6] * v[1] + m[10] * v[2] + m[14];
	vdest[0] = x;
	vdest[1] = y;
}

void mat4_xform4(float *vdest, float *m, float *v)
{
	float x = m[0] * v[0] + m[4] * v[1] + m[8] * v[2] + m[12] * v[3];
	float y = m[1] * v[0] + m[5] * v[1] + m[9] * v[2] + m[13] * v[3];
	float z = m[2] * v[0] + m[6] * v[1] + m[10] * v[2] + m[14] * v[3];
	vdest[3] = m[3] * v[0] + m[7] * v[1] + m[11] * v[2] + m[15] * v[3];
	vdest[0] = x;
	vdest[1] = y;
	vdest[2] = z;
}


float *mat4_row(float *m, int row)
{
	return m + (row << 2);
}

float mat4_elem(float *m, int row, int col)
{
	return m[M(row, col)];
}


void mat4_upper3x3(float *m)
{
	m[3] = m[7] = m[11] = m[12] = m[13] = m[14] = 0.0f;
	m[15] = 1.0f;
}

#define swap(a, b)	\
	do { \
		float tmp = a; \
		a = b; \
		b = tmp; \
	} while(0)

void mat4_transpose(float *m)
{
	swap(m[1], m[4]);
	swap(m[2], m[8]);
	swap(m[3], m[12]);
	swap(m[6], m[9]);
	swap(m[7], m[13]);
	swap(m[11], m[14]);
}

float mat4_determinant(float *m)
{
	float d11 =	(m[M(1, 1)] * (m[M(2, 2)] * m[M(3, 3)] - m[M(2, 3)] * m[M(3, 2)])) -
				(m[M(2, 1)] * (m[M(1, 2)] * m[M(3, 3)] - m[M(1, 3)] * m[M(3, 2)])) +
				(m[M(3, 1)] * (m[M(1, 2)] * m[M(2, 3)] - m[M(1, 3)] * m[M(2, 2)]));

	float d12 =	(m[M(0, 1)] * (m[M(2, 2)] * m[M(3, 3)] - m[M(2, 3)] * m[M(3, 2)])) -
				(m[M(2, 1)] * (m[M(0, 2)] * m[M(3, 3)] - m[M(0, 3)] * m[M(3, 2)])) +
				(m[M(3, 1)] * (m[M(0, 2)] * m[M(2, 3)] - m[M(0, 3)] * m[M(2, 2)]));

	float d13 =	(m[M(0, 1)] * (m[M(1, 2)] * m[M(3, 3)] - m[M(1, 3)] * m[M(3, 2)])) -
				(m[M(1, 1)] * (m[M(0, 2)] * m[M(3, 3)] - m[M(0, 3)] * m[M(3, 2)])) +
				(m[M(3, 1)] * (m[M(0, 2)] * m[M(1, 3)] - m[M(0, 3)] * m[M(1, 2)]));

	float d14 =	(m[M(0, 1)] * (m[M(1, 2)] * m[M(2, 3)] - m[M(1, 3)] * m[M(2, 2)])) -
				(m[M(1, 1)] * (m[M(0, 2)] * m[M(2, 3)] - m[M(0, 3)] * m[M(2, 2)])) +
				(m[M(2, 1)] * (m[M(0, 2)] * m[M(1, 3)] - m[M(0, 3)] * m[M(1, 2)]));

	return m[M(0, 0)] * d11 - m[M(1, 0)] * d12 + m[M(2, 0)] * d13 - m[M(3, 0)] * d14;
}

void mat4_adjoint(float *res, float *m)
{
	int i, j;
	float cof[16];

	cof[M(0, 0)] =	(m[M(1, 1)] * (m[M(2, 2)] * m[M(3, 3)] - m[M(2, 3)] * m[M(3, 2)])) -
					(m[M(2, 1)] * (m[M(1, 2)] * m[M(3, 3)] - m[M(1, 3)] * m[M(3, 2)])) +
					(m[M(3, 1)] * (m[M(1, 2)] * m[M(2, 3)] - m[M(1, 3)] * m[M(2, 2)]));
	cof[M(0, 1)] =	(m[M(0, 1)] * (m[M(2, 2)] * m[M(3, 3)] - m[M(2, 3)] * m[M(3, 2)])) -
					(m[M(2, 1)] * (m[M(0, 2)] * m[M(3, 3)] - m[M(0, 3)] * m[M(3, 2)])) +
					(m[M(3, 1)] * (m[M(0, 2)] * m[M(2, 3)] - m[M(0, 3)] * m[M(2, 2)]));
	cof[M(0, 2)] =	(m[M(0, 1)] * (m[M(1, 2)] * m[M(3, 3)] - m[M(1, 3)] * m[M(3, 2)])) -
					(m[M(1, 1)] * (m[M(0, 2)] * m[M(3, 3)] - m[M(0, 3)] * m[M(3, 2)])) +
					(m[M(3, 1)] * (m[M(0, 2)] * m[M(1, 3)] - m[M(0, 3)] * m[M(1, 2)]));
	cof[M(0, 3)] =	(m[M(0, 1)] * (m[M(1, 2)] * m[M(2, 3)] - m[M(1, 3)] * m[M(2, 2)])) -
					(m[M(1, 1)] * (m[M(0, 2)] * m[M(2, 3)] - m[M(0, 3)] * m[M(2, 2)])) +
					(m[M(2, 1)] * (m[M(0, 2)] * m[M(1, 3)] - m[M(0, 3)] * m[M(1, 2)]));

	cof[M(1, 0)] =	(m[M(1, 0)] * (m[M(2, 2)] * m[M(3, 3)] - m[M(2, 3)] * m[M(3, 2)])) -
					(m[M(2, 0)] * (m[M(1, 2)] * m[M(3, 3)] - m[M(1, 3)] * m[M(3, 2)])) +
					(m[M(3, 0)] * (m[M(1, 2)] * m[M(2, 3)] - m[M(1, 3)] * m[M(2, 2)]));
	cof[M(1, 1)] =	(m[M(0, 0)] * (m[M(2, 2)] * m[M(3, 3)] - m[M(2, 3)] * m[M(3, 2)])) -
					(m[M(2, 0)] * (m[M(0, 2)] * m[M(3, 3)] - m[M(0, 3)] * m[M(3, 2)])) +
					(m[M(3, 0)] * (m[M(0, 2)] * m[M(2, 3)] - m[M(0, 3)] * m[M(2, 2)]));
	cof[M(1, 2)] =	(m[M(0, 0)] * (m[M(1, 2)] * m[M(3, 3)] - m[M(1, 3)] * m[M(3, 2)])) -
					(m[M(1, 0)] * (m[M(0, 2)] * m[M(3, 3)] - m[M(0, 3)] * m[M(3, 2)])) +
					(m[M(3, 0)] * (m[M(0, 2)] * m[M(1, 3)] - m[M(0, 3)] * m[M(1, 2)]));
	cof[M(1, 3)] =	(m[M(0, 0)] * (m[M(1, 2)] * m[M(2, 3)] - m[M(1, 3)] * m[M(2, 2)])) -
					(m[M(1, 0)] * (m[M(0, 2)] * m[M(2, 3)] - m[M(0, 3)] * m[M(2, 2)])) +
					(m[M(2, 0)] * (m[M(0, 2)] * m[M(1, 3)] - m[M(0, 3)] * m[M(1, 2)]));

	cof[M(2, 0)] =	(m[M(1, 0)] * (m[M(2, 1)] * m[M(3, 3)] - m[M(2, 3)] * m[M(3, 1)])) -
					(m[M(2, 0)] * (m[M(1, 1)] * m[M(3, 3)] - m[M(1, 3)] * m[M(3, 1)])) +
					(m[M(3, 0)] * (m[M(1, 1)] * m[M(2, 3)] - m[M(1, 3)] * m[M(2, 1)]));
	cof[M(2, 1)] =	(m[M(0, 0)] * (m[M(2, 1)] * m[M(3, 3)] - m[M(2, 3)] * m[M(3, 1)])) -
					(m[M(2, 0)] * (m[M(0, 1)] * m[M(3, 3)] - m[M(0, 3)] * m[M(3, 1)])) +
					(m[M(3, 0)] * (m[M(0, 1)] * m[M(2, 3)] - m[M(0, 3)] * m[M(2, 1)]));
	cof[M(2, 2)] =	(m[M(0, 0)] * (m[M(1, 1)] * m[M(3, 3)] - m[M(1, 3)] * m[M(3, 1)])) -
					(m[M(1, 0)] * (m[M(0, 1)] * m[M(3, 3)] - m[M(0, 3)] * m[M(3, 1)])) +
					(m[M(3, 0)] * (m[M(0, 1)] * m[M(1, 3)] - m[M(0, 3)] * m[M(1, 1)]));
	cof[M(2, 3)] =	(m[M(0, 0)] * (m[M(1, 1)] * m[M(2, 3)] - m[M(1, 3)] * m[M(2, 1)])) -
					(m[M(1, 0)] * (m[M(0, 1)] * m[M(2, 3)] - m[M(0, 3)] * m[M(2, 1)])) +
					(m[M(2, 0)] * (m[M(0, 1)] * m[M(1, 3)] - m[M(0, 3)] * m[M(1, 1)]));

	cof[M(3, 0)] =	(m[M(1, 0)] * (m[M(2, 1)] * m[M(3, 2)] - m[M(2, 2)] * m[M(3, 1)])) -
					(m[M(2, 0)] * (m[M(1, 1)] * m[M(3, 2)] - m[M(1, 2)] * m[M(3, 1)])) +
					(m[M(3, 0)] * (m[M(1, 1)] * m[M(2, 2)] - m[M(1, 2)] * m[M(2, 1)]));
	cof[M(3, 1)] =	(m[M(0, 0)] * (m[M(2, 1)] * m[M(3, 2)] - m[M(2, 2)] * m[M(3, 1)])) -
					(m[M(2, 0)] * (m[M(0, 1)] * m[M(3, 2)] - m[M(0, 2)] * m[M(3, 1)])) +
					(m[M(3, 0)] * (m[M(0, 1)] * m[M(2, 2)] - m[M(0, 2)] * m[M(2, 1)]));
	cof[M(3, 2)] =	(m[M(0, 0)] * (m[M(1, 1)] * m[M(3, 2)] - m[M(1, 2)] * m[M(3, 1)])) -
					(m[M(1, 0)] * (m[M(0, 1)] * m[M(3, 2)] - m[M(0, 2)] * m[M(3, 1)])) +
					(m[M(3, 0)] * (m[M(0, 1)] * m[M(1, 2)] - m[M(0, 2)] * m[M(1, 1)]));
	cof[M(3, 3)] =	(m[M(0, 0)] * (m[M(1, 1)] * m[M(2, 2)] - m[M(1, 2)] * m[M(2, 1)])) -
					(m[M(1, 0)] * (m[M(0, 1)] * m[M(2, 2)] - m[M(0, 2)] * m[M(2, 1)])) +
					(m[M(2, 0)] * (m[M(0, 1)] * m[M(1, 2)] - m[M(0, 2)] * m[M(1, 1)]));

	for(i=0; i<4; i++) {
		for(j=0; j<4; j++) {
			float val = j % 2 ? -cof[M(j, i)] : cof[M(j, i)];
			res[M(j, i)] = (i % 2) ? -val : val;
		}
	}
}

int mat4_inverse(float *m)
{
	int i, j;
	float adj[16];
	float det;

	mat4_adjoint(adj, m);
	det = mat4_determinant(m);
	if(det == 0.0f) {
		return -1;
	}

	for(i=0; i<4; i++) {
		for(j=0; j<4; j++) {
			m[M(j, i)] = adj[M(j, i)] / det;
		}
	}
	return 0;
}


void mat4_translation(float *m, float x, float y, float z)
{
	mat4_identity(m);
	m[12] = x;
	m[13] = y;
	m[14] = z;
}

void mat4_rotation_x(float *m, float angle)
{
	float sa, ca;

	mat4_identity(m);
	sa = sin(angle);
	ca = cos(angle);
	m[5] = ca;
	m[6] = sa;
	m[9] = -sa;
	m[10] = ca;
}

void mat4_rotation_y(float *m, float angle)
{
	float sa, ca;

	mat4_identity(m);
	sa = sin(angle);
	ca = cos(angle);
	m[0] = ca;
	m[2] = -sa;
	m[8] = sa;
	m[10] = ca;
}

void mat4_rotation_z(float *m, float angle)
{
	float sa, ca;

	mat4_identity(m);
	sa = sin(angle);
	ca = cos(angle);
	m[0] = ca;
	m[1] = sa;
	m[4] = -sa;
	m[5] = ca;
}

void mat4_rotation(float *m, float angle, float x, float y, float z)
{
	float sa = sin(angle);
	float ca = cos(angle);
	float invca = 1.0f - ca;
	float xsq = x * x;
	float ysq = y * y;
	float zsq = z * z;

	mat4_identity(m);
	m[0] = xsq + (1.0f - xsq) * ca;
	m[4] = x * y * invca - z * sa;
	m[8] = x * z * invca + y * sa;

	m[1] = x * y * invca + z * sa;
	m[5] = ysq + (1.0f - ysq) * ca;
	m[9] = y * z * invca - x * sa;

	m[2] = x * z * invca - y * sa;
	m[6] = y * z * invca + x * sa;
	m[10] = zsq + (1.0f - zsq) * ca;
}

void mat4_scaling(float *m, float sx, float sy, float sz)
{
	mat4_identity(m);
	m[0] = sx;
	m[5] = sy;
	m[10] = sz;
}


void mat4_translate(float *m, float x, float y, float z)
{
	float tmp[16];
	mat4_translation(tmp, x, y, z);
	mat4_mul(m, m, tmp);
}

void mat4_rotate_x(float *m, float angle)
{
	float tmp[16];
	mat4_rotation_x(tmp, angle);
	mat4_mul(m, m, tmp);
}

void mat4_rotate_y(float *m, float angle)
{
	float tmp[16];
	mat4_rotation_y(tmp, angle);
	mat4_mul(m, m, tmp);
}

void mat4_rotate_z(float *m, float angle)
{
	float tmp[16];
	mat4_rotation_z(tmp, angle);
	mat4_mul(m, m, tmp);
}

void mat4_rotate(float *m, float angle, float x, float y, float z)
{
	float tmp[16];
	mat4_rotation(tmp, angle, x, y, z);
	mat4_mul(m, m, tmp);
}

void mat4_scale(float *m, float sx, float sy, float sz)
{
	float tmp[16];
	mat4_scaling(tmp, sx, sy, sz);
	mat4_mul(m, m, tmp);
}


void mat4_pre_translate(float *m, float x, float y, float z)
{
	float tmp[16];
	mat4_translation(tmp, x, y, z);
	mat4_mul(m, tmp, m);
}

void mat4_pre_rotate_x(float *m, float angle)
{
	float tmp[16];
	mat4_rotation_x(tmp, angle);
	mat4_mul(m, m, tmp);
}

void mat4_pre_rotate_y(float *m, float angle)
{
	float tmp[16];
	mat4_rotation_y(tmp, angle);
	mat4_mul(m, m, tmp);
}

void mat4_pre_rotate_z(float *m, float angle)
{
	float tmp[16];
	mat4_rotation_z(tmp, angle);
	mat4_mul(m, m, tmp);
}

void mat4_pre_rotate(float *m, float angle, float x, float y, float z)
{
	float tmp[16];
	mat4_rotation(tmp, angle, x, y, z);
	mat4_mul(m, m, tmp);
}

void mat4_pre_scale(float *m, float sx, float sy, float sz)
{
	float tmp[16];
	mat4_scaling(tmp, sx, sy, sz);
	mat4_mul(m, m, tmp);
}

static void normalize(float *v)
{
	float len = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	if(len != 0.0f) {
		float s = 1.0f / len;
		v[0] *= s;
		v[1] *= s;
		v[2] *= s;
	}
}

static void cross(float *res, float *a, float *b)
{
	res[0] = a[1] * b[2] - a[2] * b[1];
	res[1] = a[2] * b[0] - a[0] * b[2];
	res[2] = a[0] * b[1] - a[1] * b[0];
}

void mat4_lookat(float *m, float x, float y, float z, float tx, float ty, float tz, float ux, float uy, float uz)
{
	int i;
	float dir[3], right[3], up[3];

	dir[0] = tx - x;
	dir[1] = ty - y;
	dir[2] = tz - z;
	normalize(dir);

	up[0] = ux;
	up[1] = uy;
	up[2] = uz;
	cross(right, dir, up);
	normalize(right);

	cross(up, right, dir);
	normalize(up);

	mat4_identity(m);

	for(i=0; i<3; i++) {
		m[i] = right[i];
		m[i + 4] = up[i];
		m[i + 8] = -dir[i];
	}

	mat4_translate(m, x, y, z);
}

void mat4_inv_lookat(float *m, float x, float y, float z, float tx, float ty, float tz, float ux, float uy, float uz)
{
	int i;
	float dir[3], right[3], up[3];

	dir[0] = tx - x;
	dir[1] = ty - y;
	dir[2] = tz - z;
	normalize(dir);

	up[0] = ux;
	up[1] = uy;
	up[2] = uz;
	cross(right, dir, up);
	normalize(right);

	cross(up, right, dir);
	normalize(up);

	mat4_identity(m);

	for(i=0; i<3; i++) {
		m[i << 2] = right[i];
		m[(i << 2) + 1] = up[i];
		m[(i << 2) + 2] = -dir[i];
	}

	mat4_pre_translate(m, -x, -y, -z);
}

void mat4_ortho(float *m, float left, float right, float bottom, float top, float znear, float zfar)
{
	float dx = right - left;
	float dy = top - bottom;
	float dz = zfar - znear;

	mat4_identity(m);
	m[0] = 2.0f / dx;
	m[5] = 2.0f / dy;
	m[10] = -2.0f / dz;
	m[12] = -(right + left) / dx;
	m[13] = -(top + bottom) / dy;
	m[14] = -(zfar + znear) / dz;
}

void mat4_frustum(float *m, float left, float right, float bottom, float top, float znear, float zfar)
{
	float dx = right - left;
	float dy = top - bottom;
	float dz = zfar - znear;

	memset(m, 0, 16 * sizeof *m);
	m[0] = 2.0f * znear / dx;
	m[5] = 2.0f * znear / dy;
	m[8] = (right + left) / dx;
	m[9] = (top + bottom) / dy;
	m[10] = -(zfar + znear) / dz;
	m[14] = -2.0f * zfar * znear / dz;
	m[11] = -1.0f;
}

void mat4_perspective(float *m, float fov, float aspect, float znear, float zfar)
{
	float s = 1.0f / tan(fov / 2.0f);
	float range = znear - zfar;

	memset(m, 0, 16 * sizeof *m);
	m[0] = s / aspect;
	m[5] = s;
	m[10] = (znear + zfar) / range;
	m[14] = 2.0f * znear * zfar / range;
	m[11] = -1.0f;
}


void mat4_mirror(float *m, float a, float b, float c, float d)
{
	m[0] = 1.0f - 2.0f * a * a;
	m[5] = 1.0f - 2.0f * b * b;
	m[10] = 1.0f - 2.0f * c * c;
	m[15] = 1.0f;

	m[1] = m[4] = -2.0f * a * b;
	m[2] = m[8] = -2.0f * a * c;
	m[6] = m[9] = -2.0f * b * c;

	m[12] = -2.0f * a * d;
	m[13] = -2.0f * b * d;
	m[14] = -2.0f * c * d;

	m[3] = m[7] = m[11] = 0.0f;
}


void mat4_print(float *m, FILE *fp)
{
	int i;

	for(i=0; i<4; i++) {
		fprintf(fp, "[ %4.4g %4.4g %4.4g %4.4g ]\n", m[0], m[1], m[2], m[3]);
		m += 4;
	}
	fputc('\n', fp);
}

