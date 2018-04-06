#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mathutil.h"

float frand(void)
{
	return (float)rand() / RAND_MAX;
}

void sphrand(float rad, float *res)
{
	float u = frand();
	float v = frand();

	float theta = 2.0 * M_PI * u;
	float phi = acos(2.0 * v - 1.0);

	res[0] = rad * cos(theta) * sin(phi);
	res[1] = rad * sin(theta) * sin(phi);
	res[2] = rad * cos(phi);
}

void cylrand(float rad, float h, float *res)
{
	float theta = 2.0 * M_PI * frand();
	float r = sqrt(frand()) * rad;

	res[0] = cos(theta) * r;
	res[1] = (frand() - 0.5) * h;
	res[2] = sin(theta) * r;
}
